#include "BT_Decorator.h"
#include "Blackboard.h"
#include "BehaviorTree.h"
#include "GameObject.h"
#include "BT_Node.h"
#include "GameInstance.h"
#include "ImGui/imgui.h"

using namespace rapidjson;

CBT_Decorator::CBT_Decorator()
{
}

CBT_Decorator::CBT_Decorator(const CBT_Decorator& Prototype)
	: CBT_Node(Prototype)
	, m_eAbortType{ Prototype.m_eAbortType }
	, m_bInverseCondition{ Prototype.m_bInverseCondition }
    , m_bFlowControlDecorator {Prototype.m_bFlowControlDecorator }
{
}

HRESULT CBT_Decorator::Initialize(void* pArg)
{
	if (pArg)
	{
		BT_NODE_DESC* pDesc = static_cast<BT_NODE_DESC*>(pArg);

		m_pBehaviorTree = pDesc->pTree;
		m_pBlackboard = pDesc->pBloackBoard;
		m_iNodeIndex = pDesc->iNodeIndex++;
	}

	return S_OK;
}

HRESULT CBT_Decorator::Initialize_Prototype(class CBehaviorTree* pTree)
{
	m_pBehaviorTree = pTree;
	return S_OK;
}

BT_RESULT_TYPE CBT_Decorator::Receive_Excute(CGameObject* pOwner)
{
	return Run(pOwner);
}

BT_RESULT_TYPE CBT_Decorator::Run(CGameObject* pOwner)
{
	return BT_RESULT_SUCCESS;
}

BT_RESULT_TYPE CBT_Decorator::Late_Run(CGameObject* pOwner)
{
	return BT_RESULT_TYPE::BT_RESULT_NONE;
}

_bool CBT_Decorator::Perform_ConditionCheck(CGameObject* pOwner)
{
	return _bool();
}

rapidjson::Value CBT_Decorator::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    Value ObjValue = __super::Save_JsonFile(Alloc);

    return ObjValue;
}

void CBT_Decorator::Free()
{
    __super::Free();
}

CDecorator_Blackboard::CDecorator_Blackboard()
{
    m_strNodeName = "CDecorator_Blackboard";
    m_strClassName = "CDecorator_Blackboard";

    m_bFlowControlDecorator = true;
}

CDecorator_Blackboard::CDecorator_Blackboard(const CDecorator_Blackboard& Prototype)
	: CBT_Decorator::CBT_Decorator(Prototype)
	, m_eQueryType {Prototype.m_eQueryType}
	, m_pairBlackboardValue{ Prototype.m_pairBlackboardValue }
	, m_Value(Prototype.m_Value)
{
}

HRESULT CDecorator_Blackboard::Initialize_Prototype(pair<string, TYPES> pairBloackboardKey, VariantTypes Value, KeyQueryType eQueryType, Observer_Abort eAbortType)
{
	m_Value = Value;
	m_pairBlackboardValue = pairBloackboardKey;
	m_eQueryType = eQueryType;
	m_eAbortType = eAbortType;

	if (m_strNodeName == "")
	{
		m_strNodeName = m_pairBlackboardValue.first;
	}

	return S_OK;
}

HRESULT CDecorator_Blackboard::Initialize_Prototype(rapidjson::Value* pValue, CBehaviorTree* pTree)
{
    m_strClassName = "CDecorator_Blackboard";

	return S_OK;
}

HRESULT CDecorator_Blackboard::Initialize(void* pArg)
{
	__super::Initialize(pArg);

	return S_OK;
}

BT_RESULT_TYPE CDecorator_Blackboard::Run(CGameObject* pOwner)
{
	return static_cast<BT_RESULT_TYPE>(Perform_ConditionCheck(pOwner));
}

_bool CDecorator_Blackboard::Perform_ConditionCheck(CGameObject* pOwner)
{
	TYPES eType = m_pairBlackboardValue.second;

	switch (eType)
	{
	case Engine::TYPE_INT:
		return Excute_Compare<_int>(m_eQueryType);
	case Engine::TYPE_UINT:
		return Excute_Compare<_uint>(m_eQueryType);
	case Engine::TYPE_FLOAT:
		return Excute_Compare<_float>(m_eQueryType);
	case Engine::TYPE_DOUBLE:
		return Excute_Compare<_double>(m_eQueryType);
	case Engine::TYPE_BOOL:
		return Excute_Compare<_bool>(m_eQueryType);
	case Engine::TYPE_VOID_PTR:
		return Is_Set<void*>(m_eQueryType);
	case Engine::TYPE_GAMEOBJECT_PTR:
		return Is_Set<CGameObject*>(m_eQueryType);
    case Engine::TYPE_STRING:
        return Excute_Compare<string>(m_eQueryType);
	case Engine::TYPE_END:
		break;
	default:
		break;
	}

	return false;
}

rapidjson::Value CDecorator_Blackboard::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
	Value ObjValue = __super::Save_JsonFile(Alloc);

	ObjValue.AddMember("m_eQueryType", m_eQueryType, Alloc);
	ObjValue.AddMember("m_eAbortType", m_eAbortType, Alloc);
	
	switch (m_pairBlackboardValue.second)
	{
	case TYPE_INT:
		ObjValue.AddMember("m_Value", get<_int>(m_Value), Alloc);
		break;
	case TYPE_FLOAT:
		ObjValue.AddMember("m_Value", get<_float>(m_Value), Alloc);
		break;
	case TYPE_BOOL:
		ObjValue.AddMember("m_Value", get<_bool>(m_Value), Alloc);
		break;
	case TYPE_GAMEOBJECT_PTR:
		ObjValue.AddMember("m_Value", kNullType, Alloc);
		break;
	case TYPE_VOID_PTR:
		ObjValue.AddMember("m_Value", kNullType, Alloc);
		break;
    case TYPE_STRING:
        ObjValue.AddMember("m_Value", StringRef(get<string>(m_Value).c_str()), Alloc);
        break;
	default:
		MSG_BOX("CDecorator_Blackboard : Save Error Type Value!");
		break;
	}

	rapidjson::Value PairValue(rapidjson::kObjectType);
	rapidjson::Value strValue;
	strValue.SetString(m_pairBlackboardValue.first.c_str(), Alloc);
	PairValue.AddMember("VariableKey", strValue, Alloc);
	PairValue.AddMember("VariableType", m_pairBlackboardValue.second, Alloc);
	ObjValue.AddMember("m_pairBlackboardValue", PairValue, Alloc);

	return ObjValue;
}

HRESULT CDecorator_Blackboard::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    if (E_FAIL == __super::Load_JsonFile(pJsonValue, pTree))
        return E_FAIL;

    if (pJsonValue)
    {
        m_eQueryType = (KeyQueryType)(*pJsonValue)["m_eQueryType"].GetInt();
        m_eAbortType = (Observer_Abort)(*pJsonValue)["m_eAbortType"].GetInt();
        rapidjson::Value& pairValue = (*pJsonValue)["m_pairBlackboardValue"].GetObj();
        if (pairValue.HasMember("VariableKey") && pairValue.HasMember("VariableType"))
        {
            m_pairBlackboardValue = { pairValue["VariableKey"].GetString(), (TYPES)pairValue["VariableType"].GetInt() };
        }
      
        switch (m_pairBlackboardValue.second)
        {
        case TYPE_INT:
            m_Value = (*pJsonValue)["m_Value"].GetInt();
            break;
        case TYPE_FLOAT:
            m_Value = (*pJsonValue)["m_Value"].GetFloat();
            break;
        case TYPE_BOOL:
            m_Value = (*pJsonValue)["m_Value"].GetBool();
            break;
        case TYPE_GAMEOBJECT_PTR:
            m_Value = (CGameObject*)nullptr;
            break;
        case TYPE_VOID_PTR:
            m_Value = (void*)nullptr;
            break;
        case TYPE_STRING:
            m_Value = (string)(*pJsonValue)["m_Value"].GetString();
            break;
        default:
            MSG_BOX("CDecorator_Blackboard : Load Error Type Value!");
            break;
        }
    }

    if (m_eAbortType != ABORT_NONE && m_eAbortType != ABORT_END)
        m_pBlackboard->Bind_Delegate(m_pairBlackboardValue.first,
            bind(&CDecorator_Blackboard::On_Notify, this));

    return S_OK;
}

void CDecorator_Blackboard::On_Notify()
{
    _bool bCondition = Perform_ConditionCheck(m_pBehaviorTree->Get_Owner());
    _bool bLowerPriorityCondition = bCondition;

    CBT_Node* pLatentTask = m_pBehaviorTree->Get_LatentTask();
    if (nullptr == pLatentTask)
        return;

    _uint iContinueNodeIndex = pLatentTask->Get_NodeIndex();

    // LatentTask가 데코레이터보다 더 왼쪽에 존재한다면 바로 나감(아직 실행 조건을 만족 못함)
    _bool bChildNodeFlag = (iContinueNodeIndex >= m_iNodeIndex);
    if (false == bChildNodeFlag)
        return;

    // 현재 LatentTask가 데코레이터와 서브트리인지 아닌지 체킹
    vector<_uint> LatentTaskSubtreeIndexes = pLatentTask->Get_SubTreeindices();
    _bool bSubtree = false;
    for (auto& iIdx : LatentTaskSubtreeIndexes)
    {
        if (m_pParentNode->Get_NodeIndex() == iIdx)
        {
            bSubtree = true;
            break;
        }
    }

    /* Lower/Both Abort 조건 판단 */
    if (false == bSubtree && true == bLowerPriorityCondition)
    {
        vector<CBT_Node*> AllParentNodes = GetAllAncestors();

        CBT_Composite* pDecoratorScopeCompositeNode = nullptr;
        for (auto& pParentNode : AllParentNodes)
        {
            CBT_Composite* pComposite = dynamic_cast<CBT_Composite*>(pParentNode);
            if (pComposite && true == pComposite->Get_ApplyDecoratorScope())
            {
                pDecoratorScopeCompositeNode = pComposite;
                break;
            }
        }

        if (pDecoratorScopeCompositeNode)
        {
            /* Composite 노드의 Decorator Scope 영역에 LatentTask가 존재하는지 판단 및 플래그 체킹 */
            _bool bIsInsideDecoratorScope = { false };
            for (auto& iIdx : LatentTaskSubtreeIndexes)
            {
                if (pDecoratorScopeCompositeNode->Get_NodeIndex() == iIdx)
                {
                    bIsInsideDecoratorScope = true;
                    break;
                }
            }

            if (false == bIsInsideDecoratorScope)
                bLowerPriorityCondition = false;
        }
    }

    _bool bTreeResetFlag = { false };
    _bool bNodeAbortFlag = { false };

    switch (m_eAbortType)
    {
    case Engine::CBT_Decorator::ABORT_SELF:
        bNodeAbortFlag = (bSubtree && !bCondition);
        break;
    case Engine::CBT_Decorator::ABORT_LOWER_PRIORITY:
        bTreeResetFlag = !bSubtree && bLowerPriorityCondition;
        break;
    case Engine::CBT_Decorator::ABORT_BOTH:
        bNodeAbortFlag = ((bSubtree && !bCondition));
        bTreeResetFlag = (!bSubtree && bLowerPriorityCondition);
        break;
    default:
        break;
    }

    if (bTreeResetFlag)
    {
        if (IBT_Task* pTask = m_pBehaviorTree->Get_LatentTask())
            pTask->FinishLatentAbort();
        m_pBehaviorTree->Set_ResetFlag(bTreeResetFlag);
    }
    else if (bNodeAbortFlag)
    {
        if (m_pParentNode)
        {
            if (IBT_Task* pTask = m_pBehaviorTree->Get_LatentTask())
            {
                pTask->FinishLatentAbort();
            }
            static_cast<CBT_Composite*>(m_pParentNode)->Request_Abort();
        }
    }
}

_int CDecorator_Blackboard::Convert_To_Enum(const string& strEnumValueName)
{
    //enum KeyQueryType
    //{
    //    TYPE_IS_SET,
    //    TYPE_IS_NOT_SET,
    //    TYPE_EQUAL,
    //    TYPE_NOTEQUAL,
    //    TYPE_LESSEQUAL,
    //    TYPE_LESS, // 5
    //    TYPE_GREATEREQUAL,
    //    TYPE_GREATER,
    //    TYPE_END
    //};

    //enum Observer_Abort
    //{
    //    ABORT_NONE,
    //    ABORT_SELF,
    //    ABORT_LOWER_PRIORITY,
    //    ABORT_BOTH,
    //    ABORT_END
    //};

    return _int();
}

string CDecorator_Blackboard::Convert_To_String(int iEnumValue)
{
    return string();
}

CDecorator_Blackboard* CDecorator_Blackboard::Create(pair<_string, TYPES> pairBloackboardKey, VariantTypes Value, KeyQueryType eQueryType, Observer_Abort eAbortType)
{
	CDecorator_Blackboard* pInstance = new CDecorator_Blackboard;

	if (FAILED(pInstance->Initialize_Prototype(pairBloackboardKey, Value, eQueryType, eAbortType)))
	{
		MSG_BOX("Failed To Created : CDecorator_Blackboard");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CDecorator_Blackboard* CDecorator_Blackboard::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
	CDecorator_Blackboard* pInstance = new CDecorator_Blackboard;

	if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
	{
		MSG_BOX("Failed To Created : CDecorator_Blackboard");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CBT_Decorator* CDecorator_Blackboard::Clone(void* pArg)
{
	CDecorator_Blackboard* pInstance = new CDecorator_Blackboard(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed To Cloned : CDecorator_Blackboard");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CDecorator_Blackboard::Free()
{
    if (m_pBehaviorTree)
        m_pBehaviorTree->Get_Blackboard()->Remove_Delegate(m_pairBlackboardValue.first);

	__super::Free();
}

CDecorator_SetBlackboardRandomValue::CDecorator_SetBlackboardRandomValue()
{
}

CDecorator_SetBlackboardRandomValue::CDecorator_SetBlackboardRandomValue(const CDecorator_SetBlackboardRandomValue& Prototype)
	: CBT_Decorator(Prototype)
	, m_strBlackboardKey {Prototype.m_strBlackboardKey }
{
}

HRESULT CDecorator_SetBlackboardRandomValue::Initialize_Prototype(_string strBlackboardKey)
{
	m_strBlackboardKey = strBlackboardKey;
	return S_OK;
}

HRESULT CDecorator_SetBlackboardRandomValue::Initialize_Prototype(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
	m_strNodeName = "Deco_SetBlackbaordFloatKeyRandomValue";
	m_pBehaviorTree = pTree;

	if (pJsonValue)
	{
		//m_strBlackboardKey = m_pGameInstance->Convert_String2WString((*pJsonValue)["m_strBlackboardKey"].GetString());
	}

	return S_OK;
}

HRESULT CDecorator_SetBlackboardRandomValue::Initialize(void* pArg)
{
	__super::Initialize(pArg);

	m_strNodeName = "Deco_SetBlackbaordFloatKeyRandomValue";

	return S_OK;
}

BT_RESULT_TYPE CDecorator_SetBlackboardRandomValue::Run(CGameObject* pOwner)
{
	//if (FAILED(m_pBlackboard->Set_Variable(m_strBlackboardKey, m_pGameInstance->Random_Normalize())))
	//	return  BT_RESULT_TYPE::BT_RESULT_FAIL;
	return BT_RESULT_TYPE::BT_RESULT_SUCCESS;
}

rapidjson::Value CDecorator_SetBlackboardRandomValue::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
	Value ObjValue(kObjectType);

	ObjValue.AddMember("m_strNodeName", StringRef(m_strNodeName.c_str()), Alloc);

	rapidjson::Value strValue;
	strValue.SetString(m_strBlackboardKey.c_str(), Alloc);
	ObjValue.AddMember("m_strBlackboardKey", strValue, Alloc);

	return ObjValue;
}

HRESULT CDecorator_SetBlackboardRandomValue::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{


    return S_OK;
}

CDecorator_SetBlackboardRandomValue* CDecorator_SetBlackboardRandomValue::Create(_string strBlackboardKey)
{
	CDecorator_SetBlackboardRandomValue* pInstance = new CDecorator_SetBlackboardRandomValue;

	if (FAILED(pInstance->Initialize_Prototype(strBlackboardKey)))
	{
		MSG_BOX("Failed To Created : CDecorator_SetBlackboardRandomValue");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CDecorator_SetBlackboardRandomValue* CDecorator_SetBlackboardRandomValue::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
	CDecorator_SetBlackboardRandomValue* pInstance = new CDecorator_SetBlackboardRandomValue;

	if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
	{
		MSG_BOX("Failed To Created : CDecorator_SetBlackboardRandomValue");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CBT_Decorator* CDecorator_SetBlackboardRandomValue::Clone(void* pArg)
{
	CDecorator_SetBlackboardRandomValue* pInstance = new CDecorator_SetBlackboardRandomValue(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed To Cloned : CDecorator_SetBlackboardRandomValue");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CDecorator_SetBlackboardRandomValue::Free()
{
	__super::Free();
}

CDecorator_CloseToTarget::CDecorator_CloseToTarget()
{
}

CDecorator_CloseToTarget::CDecorator_CloseToTarget(const CDecorator_CloseToTarget& Prototype)
	: CBT_Decorator(Prototype)
	, m_fAcceptableRadius { Prototype.m_fAcceptableRadius}
	, m_pairBlackboardValue { Prototype.m_pairBlackboardValue}

{
}

HRESULT CDecorator_CloseToTarget::Initialize_Prototype(const pair<_string, TYPES> pairBlackboardKey, _float fAcceptableRadius)
{
	m_pairBlackboardValue = pairBlackboardKey;
	m_fAcceptableRadius = fAcceptableRadius;

	return S_OK;
}

HRESULT CDecorator_CloseToTarget::Initialize_Prototype(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
	m_strNodeName = "Decorator_CloseToTarget";
	m_pBehaviorTree = pTree;

	if (pJsonValue)
	{
		rapidjson::Value& pairValue = (*pJsonValue)["m_pairBlackboardValue"];
		//m_pairBlackboardValue = { m_pGameInstance->Convert_String2WString(pairValue["first"].GetString()), (TYPES)pairValue["second"].GetInt() };

		m_fAcceptableRadius = (*pJsonValue)["m_fAcceptableRadius"].GetFloat();
		m_bInverseCondition = (*pJsonValue)["m_bInverseCondition"].GetBool();
	}

	return S_OK;
}

HRESULT CDecorator_CloseToTarget::Initialize(void* pArg)
{
	__super::Initialize(pArg);

	m_strNodeName = "Decorator_CloseToTarget";

	return S_OK;
}

BT_RESULT_TYPE CDecorator_CloseToTarget::Late_Run(CGameObject* pOwner)
{
	switch (m_pairBlackboardValue.second)
	{
	case TYPE_GAMEOBJECT_PTR:
		if (nullptr == m_pObj)
			m_pObj = m_pBlackboard->Get_Variable<CGameObject*>(m_pairBlackboardValue.first);
		else
			//m_vGoalLocation = m_pObj->Get_TransformCom()->Get_Position();
		break;
	case TYPE_VECTOR:
		m_vGoalLocation = m_pBlackboard->Get_Variable<_vector>(m_pairBlackboardValue.first);
		break;
	default:
		break;
	}

	//_vector vOwnerPos = pOwner->Get_TransformCom()->Get_Position();
	//_float fDistance = XMVectorGetX(XMVector4Length(m_vGoalLocation - vOwnerPos));

	/*if (m_fAcceptableRadius >= fDistance)
	{
		if (!m_bInverseCondition)
			return BT_RESULT_SUCCESS;
		else
			return BT_RESULT_FAIL;
	}*/

	return BT_RESULT_TYPE::BT_RESULT_CONTINUE;
}

rapidjson::Value CDecorator_CloseToTarget::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    Value ObjValue = __super::Save_JsonFile(Alloc);

	ObjValue.AddMember("m_fAcceptableRadius", m_fAcceptableRadius, Alloc);

	rapidjson::Value PairValue(rapidjson::kObjectType);
	rapidjson::Value strValue;
	//strValue.SetString(m_pGameInstance->Convert_WString2MultiByte(m_pairBlackboardValue.first), Alloc);
	PairValue.AddMember("first", strValue, Alloc);
	PairValue.AddMember("second", m_pairBlackboardValue.second, Alloc);
	ObjValue.AddMember("m_pairBlackboardValue", PairValue, Alloc);
	ObjValue.AddMember("m_bInverseCondition", m_bInverseCondition, Alloc);

	return ObjValue;
}

CDecorator_CloseToTarget* CDecorator_CloseToTarget::Create(const pair<_string, TYPES> pairBlackboardKey, _float fAcceptableRadius)
{
	CDecorator_CloseToTarget* pInstance = new CDecorator_CloseToTarget;

	if (FAILED(pInstance->Initialize_Prototype(pairBlackboardKey, fAcceptableRadius)))
	{
		MSG_BOX("Failed To Created : CDecorator_CloseToTarget");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CDecorator_CloseToTarget* CDecorator_CloseToTarget::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
	CDecorator_CloseToTarget* pInstance = new CDecorator_CloseToTarget;

	if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
	{
		MSG_BOX("Failed To Created : CDecorator_CloseToTarget");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CBT_Decorator* CDecorator_CloseToTarget::Clone(void* pArg)
{
	CDecorator_CloseToTarget* pInstance = new CDecorator_CloseToTarget(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed To Cloned : CDecorator_CloseToTarget");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CDecorator_CloseToTarget::Free()
{
	__super::Free();
}

CDecorator_TimeLimit::CDecorator_TimeLimit()
{
}

CDecorator_TimeLimit::CDecorator_TimeLimit(const CDecorator_TimeLimit& Prototype)
	: CBT_Decorator(Prototype)
	, m_fTimeLimit { Prototype.m_fTimeLimit }
{
}

HRESULT CDecorator_TimeLimit::Initialize_Prototype(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
	m_strNodeName = "CDecorator_TimeLimit";

	m_pBehaviorTree = pTree;

	if (pJsonValue)
	{
		m_fTimeLimit = (*pJsonValue)["m_fTimeLimit"].GetFloat();
	}

	return S_OK;
}

HRESULT CDecorator_TimeLimit::Initialize(void* pArg)
{
	__super::Initialize(pArg);

	m_strNodeName = "CDecorator_TimeLimit";

	return S_OK;
}

BT_RESULT_TYPE CDecorator_TimeLimit::Run(CGameObject* pOwner)
{
	m_fAccTime = 0.f;

	return BT_RESULT_SUCCESS;
}

BT_RESULT_TYPE CDecorator_TimeLimit::Late_Run(CGameObject* pOwner)
{
	m_fAccTime += m_pGameInstance->Get_TimeDelta();
	if (m_fAccTime >= m_fTimeLimit)
		return BT_RESULT_FAIL;

	return BT_RESULT_TYPE::BT_RESULT_NONE;
}

rapidjson::Value CDecorator_TimeLimit::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
	Value ObjValue(kObjectType);

	ObjValue.AddMember("m_fTimeLimit", m_fTimeLimit, Alloc);

	return ObjValue;
}

CDecorator_TimeLimit* CDecorator_TimeLimit::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
	CDecorator_TimeLimit* pInstance = new CDecorator_TimeLimit;

	if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
	{
		MSG_BOX("Failed To Created : CDecorator_TimeLimit");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CBT_Decorator* CDecorator_TimeLimit::Clone(void* pArg)
{
	CDecorator_TimeLimit* pInstance = new CDecorator_TimeLimit(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed To Cloned : CDecorator_TimeLimit");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CDecorator_TimeLimit::Free()
{
	__super::Free();
}


CDecorator_FarFromTarget::CDecorator_FarFromTarget()
{
}

CDecorator_FarFromTarget::CDecorator_FarFromTarget(const CDecorator_FarFromTarget& Prototype)
	: CBT_Decorator(Prototype)
	, m_fAcceptableRadius{ Prototype.m_fAcceptableRadius }
	, m_pairBlackboardValue{ Prototype.m_pairBlackboardValue }

{
}

HRESULT CDecorator_FarFromTarget::Initialize_Prototype(const pair<_string, TYPES> pairBlackboardKey, _float fAcceptableRadius)
{
	m_pairBlackboardValue = pairBlackboardKey;
	m_fAcceptableRadius = fAcceptableRadius;

	return S_OK;
}

HRESULT CDecorator_FarFromTarget::Initialize_Prototype(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
	

	return S_OK;
}

HRESULT CDecorator_FarFromTarget::Initialize(void* pArg)
{
	__super::Initialize(pArg);

	m_strNodeName = "Decorator_FarFromTarget";

	return S_OK;
}

BT_RESULT_TYPE CDecorator_FarFromTarget::Late_Run(CGameObject* pOwner)
{
	switch (m_pairBlackboardValue.second)
	{
	case TYPE_GAMEOBJECT_PTR:
		if (nullptr == m_pObj)
			m_pObj = m_pBlackboard->Get_Variable<CGameObject*>(m_pairBlackboardValue.first);
		else
			m_vGoalLocation = m_pObj->Get_TransformCom()->Get_Position();
		break;
	case TYPE_VECTOR:
		m_vGoalLocation = m_pBlackboard->Get_Variable<_vector>(m_pairBlackboardValue.first);
		break;
	default:
		break;
	}

	_vector vOwnerPos = pOwner->Get_TransformCom()->Get_Position();
	_float fDistance = XMVectorGetX(XMVector4Length(m_vGoalLocation - vOwnerPos));

	if (m_fAcceptableRadius <= fDistance)
	{
		if (!m_bInverseCondition)
			return BT_RESULT_SUCCESS;
		else
			return BT_RESULT_FAIL;
	}

	return BT_RESULT_TYPE::BT_RESULT_CONTINUE;
}

rapidjson::Value CDecorator_FarFromTarget::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    Value ObjValue = __super::Save_JsonFile(Alloc);

	ObjValue.AddMember("m_fAcceptableRadius", m_fAcceptableRadius, Alloc);

	rapidjson::Value PairValue(rapidjson::kObjectType);
	rapidjson::Value strValue;
	strValue.SetString(StringRef(m_pairBlackboardValue.first.c_str()), Alloc);
	PairValue.AddMember("first", strValue, Alloc);
	PairValue.AddMember("second", m_pairBlackboardValue.second, Alloc);
	ObjValue.AddMember("m_pairBlackboardValue", PairValue, Alloc);
	ObjValue.AddMember("m_bInverseCondition", m_bInverseCondition, Alloc);

	return ObjValue;
}

HRESULT CDecorator_FarFromTarget::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Load_JsonFile(pJsonValue, pTree);

    if (pJsonValue)
    {
        rapidjson::Value& pairValue = (*pJsonValue)["m_pairBlackboardValue"];
        m_pairBlackboardValue = { pairValue["first"].GetString(), (TYPES)pairValue["second"].GetInt() };

        m_fAcceptableRadius = (*pJsonValue)["m_fAcceptableRadius"].GetFloat();
        m_bInverseCondition = (*pJsonValue)["m_bInverseCondition"].GetBool();
    }

    return S_OK;
}

CDecorator_FarFromTarget* CDecorator_FarFromTarget::Create(const pair<_string, TYPES> pairBlackboardKey, _float fAcceptableRadius)
{
	CDecorator_FarFromTarget* pInstance = new CDecorator_FarFromTarget;

	if (FAILED(pInstance->Initialize_Prototype(pairBlackboardKey, fAcceptableRadius)))
	{
		MSG_BOX("Failed To Created : CDecorator_FarFromTarget");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CDecorator_FarFromTarget* CDecorator_FarFromTarget::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
	CDecorator_FarFromTarget* pInstance = new CDecorator_FarFromTarget;

	if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
	{
		MSG_BOX("Failed To Created : CDecorator_FarFromTarget");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CBT_Decorator* CDecorator_FarFromTarget::Clone(void* pArg)
{
	CDecorator_FarFromTarget* pInstance = new CDecorator_FarFromTarget(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed To Cloned : CDecorator_FarFromTarget");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CDecorator_FarFromTarget::Free()
{
	__super::Free();
}


CDecorator_CoolDown::CDecorator_CoolDown()
{
    m_strClassName = "CDecorator_CoolDown";

    m_bFlowControlDecorator = true;
}

CDecorator_CoolDown::CDecorator_CoolDown(const CDecorator_CoolDown& Prototype)
	: CBT_Decorator(Prototype)
	, m_fCoolDownTime{ Prototype.m_fCoolDownTime }
{
}

HRESULT CDecorator_CoolDown::Initialize_Prototype(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    return S_OK;
}

HRESULT CDecorator_CoolDown::Initialize(void* pArg)
{
	__super::Initialize(pArg);

	m_strNodeName = "Decorator_CoolDown";

	return S_OK;
}

BT_RESULT_TYPE CDecorator_CoolDown::Run(CGameObject* pOwner)
{
	if (m_bCoolDownFinished)
	{
		m_bCoolDownFinished = false;
		m_pGameInstance->Add_Timer("Timer_Decorator_CoolDown", m_fCoolDownTime, [&]() {m_bCoolDownFinished = true; });
		return BT_RESULT_SUCCESS;
	}
	else 
		return BT_RESULT_TYPE::BT_RESULT_FAIL;
}

rapidjson::Value CDecorator_CoolDown::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    Value ObjValue = __super::Save_JsonFile(Alloc);

	ObjValue.AddMember("m_fCoolDownTime", m_fCoolDownTime, Alloc);

	return ObjValue;
}

HRESULT CDecorator_CoolDown::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Load_JsonFile(pJsonValue, pTree);

    if (pJsonValue)
    {
        m_fCoolDownTime = (*pJsonValue)["m_fCoolDownTime"].GetFloat();
    }

    return S_OK;
}

CDecorator_CoolDown* CDecorator_CoolDown::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
	CDecorator_CoolDown* pInstance = new CDecorator_CoolDown;

	if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
	{
		MSG_BOX("Failed To Created : CDecorator_CoolDown");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CBT_Decorator* CDecorator_CoolDown::Clone(void* pArg)
{
	CDecorator_CoolDown* pInstance = new CDecorator_CoolDown(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed To Cloned : CDecorator_CoolDown");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CDecorator_CoolDown::Free()
{
	m_pGameInstance->Remove_Timer("Timer_Decorator_CoolDown");

	__super::Free();
}