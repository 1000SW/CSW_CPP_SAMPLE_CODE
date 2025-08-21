#include "BehaviorTree.h"
#include "Blackboard.h"
#include "BT_Node.h"
#include "BT_Decorator.h"
#include "GameObject.h"
#include "GameInstance.h"
#include "Json_Manager.h"

CBehaviorTree::CBehaviorTree(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CComponent(pDevice, pContext)
{
}

CBehaviorTree::CBehaviorTree(const CBehaviorTree& Prototype)
    : CComponent(Prototype)
    , m_fnCompositeFactoryMethod(Prototype.m_fnCompositeFactoryMethod)
    , m_fnTaskFactoryMethod(Prototype.m_fnTaskFactoryMethod)
    , m_fnDecoratorFactoryMethod(Prototype.m_fnDecoratorFactoryMethod)
    , m_fnServiceFactoryMethod(Prototype.m_fnServiceFactoryMethod)
    , m_iUniqueNodeIndex(Prototype.m_iUniqueNodeIndex)
{
    m_pBlackBoard = CBlackboard::Create(m_pDevice, m_pContext);
    /*if (nullptr == m_pBlackBoard)
        return E_FAIL;*/

        /*m_pBlackBoard = Prototype.m_pBlackBoard->Clone(nullptr);
        if (Prototype.m_pRootNode)
            m_pRootNode = Prototype.m_pRootNode->Clone(nullptr);*/
}

void CBehaviorTree::Set_LatentTask(IBT_Task* pNode)
{
    m_pLatentTask = pNode;
}

HRESULT CBehaviorTree::Add_BlackboardKey(const _string& strValueKey, pair<const VariantTypes, TYPES> Value)
{
	return m_pBlackBoard->Add_Variable(strValueKey, Value);
}

HRESULT CBehaviorTree::Set_BlackboardKey(const _string& strValueKey, const VariantTypes& Value)
{
	return m_pBlackBoard->Set_Variable(strValueKey, Value);
}

HRESULT CBehaviorTree::Save_JsonFile(Document& jsonDoc, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
	wstring wstrObjTag = m_pOwner->Get_ObjTag();
	if (wstrObjTag.empty())
		return E_FAIL;
	
	if (nullptr == m_pRootNode)
		return E_FAIL;
	
	rapidjson::Value value = m_pRootNode->Save_JsonFile(Alloc);
	if (value.IsNull())
		return E_FAIL;
	
	rapidjson::Value ObjValue(rapidjson::kObjectType);
	ObjValue.AddMember("m_pBehaviorTreeCom", value, Alloc);
	
	rapidjson::Value StrValue(rapidjson::kStringType);
    StrValue.SetString(StringRef(CMyUtils::Convert_String(wstrObjTag).c_str()), Alloc);
	jsonDoc.AddMember(StrValue, ObjValue, Alloc);
	
	return S_OK;
}

HRESULT CBehaviorTree::Load_JsonFile(const string& strFilePath, const string& ObjKey)
{
    if (E_FAIL == m_pGameInstance->Load_JsonFile(ObjKey, strFilePath))
        return E_FAIL;

    rapidjson::Document& dom = m_pGameInstance->Get_Document(ObjKey);
    if (false == dom.HasMember(ObjKey.c_str()))
        return E_FAIL;

	rapidjson::Value* pValue = &dom[ObjKey.c_str()];
	
	if (pValue)
	{
        // 기존 루트 노드를 없애버리고 새롭게 트리 갱신
        if (m_pRootNode)
        {
            Reset();
            Safe_Release(m_pRootNode);
        }

		rapidjson::Value& Value = (*pValue)["m_pBehaviorTreeCom"];

        m_pRootNode = m_fnCompositeFactoryMethod(Value["m_strClassName"].GetString(), &Value, this);
        if (nullptr == m_pRootNode)
            return E_FAIL;

        m_pRootNode->Load_JsonFile(&Value, this);
	}

    m_pGameInstance->Erase_Document(ObjKey);

    Reorder_Nodes();

    for (auto& pNode : m_Nodes)
        pNode->Late_Initialize(nullptr);
	
	return S_OK;
}

HRESULT CBehaviorTree::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CBehaviorTree::Initialize_Prototype(IBT_Task::fnTaskFactoryMethod fnTaskFactoryMethod, CBT_Decorator::fnDecoratorFactoryMethod fnDecoratorFactoryMethod, CBT_Composite::fnCompositeFactoryMethod fnCompositeFactoryMethod, IBT_Service::fnServiceFactoryMethod fnServiceFactoryMethod)
{
	m_fnCompositeFactoryMethod = fnCompositeFactoryMethod;
	m_fnDecoratorFactoryMethod = fnDecoratorFactoryMethod;
	m_fnTaskFactoryMethod = fnTaskFactoryMethod;
	m_fnServiceFactoryMethod = fnServiceFactoryMethod;

	return S_OK;
}

HRESULT CBehaviorTree::Initialize(void* pArg)
{
	return S_OK;
}

HRESULT CBehaviorTree::Late_Initialize(void* pArg)
{
	return S_OK;
}

void CBehaviorTree::Update(_float fTimeDelta)
{
    if (false == m_bActive || nullptr == m_pRootNode)
        return;

	if (m_bResetFlag)
		Reset();

    for (auto& pService : m_ActivatingServices)
        pService->PreTickService(fTimeDelta);

    // 오로지 LatentTask가 존재하지 않았을 경우에만 트리 평가 수행
    // LatentTask가 존재했다면 해당 Task가 TickTask였다면 업데이트 로직 수행
    if (nullptr == m_pLatentTask)
    {
        BT_RESULT_TYPE eResultType = m_pRootNode->Receive_Excute(m_pOwner);

	    if (eResultType != BT_RESULT_TYPE::BT_RESULT_CONTINUE)
		    m_bResetFlag = true;
    }
    else if (m_pLatentTask->Get_TickingTask())
        m_pLatentTask->TickTask(fTimeDelta);
}

void CBehaviorTree::Reorder_Nodes()
{
	m_Nodes.clear();

    m_iUniqueNodeIndex = 0;

    if (m_pRootNode)
    	m_Nodes = m_pRootNode->Reorder_Nodes(this);
}

void CBehaviorTree::Remove_Service(IBT_Service* pService)
{
    auto it = find(m_ActivatingServices.begin(), m_ActivatingServices.end(), pService);

    if (it == m_ActivatingServices.end())
        return;
    
    /* Swap And Pop */
    if (it != (m_ActivatingServices.end() - 1)) // 'it'가 마지막 요소가 아닌지 확인
    {
        *it = m_ActivatingServices.back();
    }

    // 벡터의 맨 끝 요소를 제거 (이전 단계에서 제거하려던 요소가 맨 끝으로 옮겨졌거나,
    //    원래부터 맨 끝 요소였다면 그 요소가 제거.)
    m_ActivatingServices.pop_back();
}

void CBehaviorTree::Reset()
{
	m_pLatentTask = nullptr;

    m_ActivatingServices.clear();

    if (m_pRootNode)
    	m_pRootNode->Reset();

	m_bResetFlag = false;
}

CBehaviorTree* CBehaviorTree::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CBehaviorTree* pInstance = new CBehaviorTree(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed To Created : CBehaviorTree");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CBehaviorTree* CBehaviorTree::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, IBT_Task::fnTaskFactoryMethod fnTaskFactoryMethod, CBT_Decorator::fnDecoratorFactoryMethod fnDecoratorFactoryMethod, CBT_Composite::fnCompositeFactoryMethod fnCompositeFactoryMethod, IBT_Service::fnServiceFactoryMethod fnServiceFactoryMethod)
{
	CBehaviorTree* pInstance = new CBehaviorTree(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype(fnTaskFactoryMethod, fnDecoratorFactoryMethod, fnCompositeFactoryMethod, fnServiceFactoryMethod)))
	{
		MSG_BOX("Failed To Created : CBehaviorTree");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CBehaviorTree::Clone(void* pArg)
{
	CComponent* pInstance = new CBehaviorTree(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed To Cloned : CBehaviorTree");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBehaviorTree::Free()
{
	__super::Free();

	Safe_Release(m_pRootNode);
	Safe_Release(m_pBlackBoard);
	Safe_Release(m_pGameInstance);
}
