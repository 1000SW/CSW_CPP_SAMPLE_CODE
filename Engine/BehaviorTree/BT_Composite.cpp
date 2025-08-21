#include "BT_Composite.h"
#include "BT_Decorator.h"
#include "BehaviorTree.h"
#include "GameInstance.h"

CBT_Composite::CBT_Composite()
{
    m_strClassName = "CBT_Composite";
}

CBT_Composite::CBT_Composite(const CBT_Composite& Prototype)
	: CBT_Node(Prototype)
	, m_eCompositeType{Prototype.m_eCompositeType}
{
	/*for (auto& pChild : Prototype.m_ChildNodes)
		m_ChildNodes.push_back(pChild->Clone(nullptr));*/
}

HRESULT CBT_Composite::Initialize(void* pArg)
{
	__super::Initialize(pArg);

	for (auto& pChild : m_ChildNodes)
	{
		pChild->Set_ParentNode(this);
		pChild->Set_ParentNodeIndex(m_iNodeIndex);
		pChild->Initialize(pArg);
	}

	if (m_strNodeName == "")
	{
		switch (m_eCompositeType)
		{
		case Engine::CBT_Composite::TYPE_SEQUENCE:
			m_strNodeName = "Sequence";
			break;
		case Engine::CBT_Composite::TYPE_SELECTOR:
			m_strNodeName = "Selector";
			break;
		case Engine::CBT_Composite::TYPE_END:
			break;
		default:
			break;
		}
	}

	return S_OK;
}

HRESULT CBT_Composite::Initialize_Prototype(COMPOSITE_TYPE eType, const string& strNodeName)
{
	m_eCompositeType = eType;

    m_strNodeName = strNodeName;
    switch (m_eCompositeType)
    {
    case Engine::CBT_Composite::TYPE_SEQUENCE:
        m_strNodeName += " Sequence";
        break;
    case Engine::CBT_Composite::TYPE_SELECTOR:
        m_strNodeName += " Selector";
        break;
    case Engine::CBT_Composite::TYPE_END:
        break;
    default:
        break;
    }

	return S_OK;
}

HRESULT CBT_Composite::Initialize_Prototype(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
	__super::Initialize_Prototype(pJsonValue, pTree);

	return S_OK;
}

HRESULT CBT_Composite::Add_Node(CBT_Node* pNode)
{
	if (pNode == nullptr)
		return E_FAIL;
	
	m_ChildNodes.push_back(pNode);
	
	return S_OK;
}

HRESULT CBT_Composite::Insert_Node(_int iTargetIndex, CBT_Node* pNode)
{
    if (pNode == nullptr)
        return E_FAIL;

    if (iTargetIndex < 0 || iTargetIndex > (m_ChildNodes.size()))
        return E_FAIL;

    m_ChildNodes.insert(m_ChildNodes.begin() + iTargetIndex, pNode);

    m_pBehaviorTree->Reorder_Nodes();

    return S_OK;
}

HRESULT CBT_Composite::Delete_Child(_uint iIndex)
{
	if (iIndex < 0 || iIndex >= m_ChildNodes.size())
		return E_FAIL;

	Safe_Release(m_ChildNodes[iIndex]);
	m_ChildNodes.erase(m_ChildNodes.begin() + iIndex);

    m_pBehaviorTree->Reorder_Nodes();

	return S_OK;
}

void CBT_Composite::Request_Abort()
{
    m_bAbortRequested = true;

    // 자신부터 시작해서 재귀적으로 자식들까지 모든 서비스들을 비활성화시킴
    for (auto& pService : m_Services)
        pService->FinishService();
    m_ChildNodes[m_iExecuteIndex]->Request_Abort();
}

BT_RESULT_TYPE CBT_Composite::Receive_Excute(CGameObject* pOwner)
{
    if (m_bAbortRequested)
    {
        m_bAbortRequested = false;
        return m_eResultType = BT_RESULT_FAIL;
    }

	if (BT_RESULT_NONE == m_eResultType)
	{
        if (false == Checking_Decorator())
            return m_eResultType = BT_RESULT_FAIL;

        for (auto& pService : m_Services)
        {
            pService->PreExecuteService();
        }
	}

    m_eResultType = Run(pOwner);

    /*BT_RESULT_TYPE finalResultType = BT_RESULT_NONE;
	for (auto& pDecorator : m_Decorators)
	{
        BT_RESULT_TYPE decoratorResult = pDecorator->Late_Run(pOwner);

        if (BT_RESULT_FAIL == decoratorResult)
            finalResultType = BT_RESULT_TYPE::BT_RESULT_FAIL;
        else if (BT_RESULT_SUCCESS == decoratorResult && BT_RESULT_FAIL != finalResultType)
            finalResultType = BT_RESULT_SUCCESS;
	}*/
    
    if (BT_RESULT_FAIL == m_eResultType || BT_RESULT_SUCCESS == m_eResultType)
    {
        for (auto& pService : m_Services)
            pService->FinishService();
    }

	return m_eResultType;
}

BT_RESULT_TYPE CBT_Composite::Run(CGameObject* pOwner)
{
	for (int i= m_iExecuteIndex; i<m_ChildNodes.size(); i++)
	{
		m_eResultType = m_ChildNodes[i]->Receive_Excute(pOwner);

        if (BT_RESULT_TYPE::BT_RESULT_CONTINUE == m_eResultType)
        {
            m_iExecuteIndex = i;
            return m_eResultType;
        }

		switch (m_eCompositeType)
		{
		case Engine::CBT_Composite::TYPE_SEQUENCE:
		{
			if (BT_RESULT_TYPE::BT_RESULT_FAIL == m_eResultType)
				return m_eResultType;
			break;
		}
		case Engine::CBT_Composite::TYPE_SELECTOR:
		{
			if (BT_RESULT_TYPE::BT_RESULT_SUCCESS == m_eResultType)
				return m_eResultType;
			break;
		}
		default:
			break;
		}
	}

	switch (m_eCompositeType)
	{
	case Engine::CBT_Composite::TYPE_SEQUENCE:
		return m_eResultType = BT_RESULT_TYPE::BT_RESULT_SUCCESS;
		break;
	case Engine::CBT_Composite::TYPE_SELECTOR:
		return m_eResultType = BT_RESULT_TYPE::BT_RESULT_FAIL;
		break;
	default:
		break;
	}

	return BT_RESULT_TYPE::BT_RESULT_FAIL;
}

void CBT_Composite::Reset()
{
	m_eResultType = BT_RESULT_NONE;
	m_iExecuteIndex = 0;
	for (auto* pNode : m_ChildNodes)
		pNode->Reset();
}

void CBT_Composite::Set_Nodes(vector<class CBT_Node*> pNodes)
{
	m_ChildNodes = pNodes;
}

vector<CBT_Node*> CBT_Composite::Reorder_Nodes(CBehaviorTree* pTree)
{
    // 데코레이터 수집
     vector<CBT_Node*> ret = CBT_Node::Reorder_Nodes(pTree);

	for (auto& pNode : m_ChildNodes)
	{
        pNode->Set_ParentNode(this);
        pNode->Set_ParentNodeIndex(m_iNodeIndex);

		auto Collection = pNode->Reorder_Nodes(pTree);
		ret.insert(ret.end(), Collection.begin(), Collection.end());
	}

	return ret;
}

rapidjson::Value CBT_Composite::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
	rapidjson::Value ObjValue =  __super::Save_JsonFile(Alloc);

	ObjValue.AddMember("m_eCompositeType", m_eCompositeType, Alloc);
	ObjValue.AddMember("m_bApplyDecoratorScope", m_bApplyDecoratorScope, Alloc);

	rapidjson::Value ArrayValue(kArrayType);

	for (auto& pChild : m_ChildNodes)
		ArrayValue.PushBack(pChild->Save_JsonFile(Alloc), Alloc);

	ObjValue.AddMember("m_ChildNodes", ArrayValue, Alloc);

	return ObjValue;
}

HRESULT CBT_Composite::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Load_JsonFile(pJsonValue, pTree);

    if (pJsonValue)
    {
        using namespace rapidjson;

        m_eCompositeType = (COMPOSITE_TYPE)(*pJsonValue)["m_eCompositeType"].GetInt();
        if ((*pJsonValue).HasMember("m_bApplyDecoratorScope"))
            m_bApplyDecoratorScope = (*pJsonValue)["m_bApplyDecoratorScope"].GetBool();

        Value& Childs = (*pJsonValue)["m_ChildNodes"];
        for (Value::ValueIterator itr = Childs.Begin(); itr != Childs.End(); itr++)
        {
            Value& ObjValue = itr->GetObj();

            /* Node 타입이었다면 */
            CBT_Node* pNode = m_pBehaviorTree->Get_TaskFactoryMethod()(ObjValue["m_strClassName"].GetString(), &ObjValue, m_pBehaviorTree);
            if (pNode)
                Add_Node(pNode);
            /* Composite 타입이었다면*/
            else
            {
                pNode = m_pBehaviorTree->Get_CompositeFactoryMethod()(ObjValue["m_strClassName"].GetString(), &ObjValue, m_pBehaviorTree);
                Add_Node(pNode);
            }
            /* 둘 중 뭐가 됐든 생성되었다면 재귀적으로 Json 파일을 로드하는 함수를 실행*/
            if (pNode)
            {
                pNode->Load_JsonFile(&ObjValue, pTree);
            }
        }
    }

    return S_OK;
}

CBT_Composite* CBT_Composite::Create(COMPOSITE_TYPE eType, const string& strNodeName)
{
	CBT_Composite* pInstance = new CBT_Composite();

	if (FAILED(pInstance->Initialize_Prototype(eType, strNodeName)))
	{
		MSG_BOX("Failed To Created : CBT_Composite");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CBT_Composite* CBT_Composite::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
	CBT_Composite* pInstance = new CBT_Composite();

	if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
	{
		MSG_BOX("Failed To Created : CBT_Composite");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CBT_Node* CBT_Composite::Clone(void* pArg)
{
	CBT_Composite* pInstance = new CBT_Composite(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed To Cloned : CBT_Composite");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBT_Composite::Free()
{
	__super::Free();

	for (auto& pChild : m_ChildNodes)
		Safe_Release(pChild);

	m_ChildNodes.clear();
}
