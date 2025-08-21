#include "BT_Task.h"
#include "Blackboard.h"
#include "GameInstance.h"
#include "CompositeArmatrue.h"
#include "BT_Decorator.h"
#include "BehaviorTree.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbility.h"
#include "HealthObject.h"
#include "Animator.h"

IBT_Task::IBT_Task()
{
}

IBT_Task::IBT_Task(const IBT_Task& Prototype)
    : CBT_Node(Prototype)
    , m_bTickingTask(Prototype.m_bTickingTask)
{
}

HRESULT IBT_Task::Initialize(void* pArg)
{
    return S_OK;
}

void IBT_Task::PreExecuteTask()
{
    m_eTaskStatus = EBTTaskStatus::Active;
}

BT_RESULT_TYPE IBT_Task::ExecuteTask()
{
    return BT_RESULT_SUCCESS;
}

BT_RESULT_TYPE IBT_Task::Receive_Excute(CGameObject* pOwner)
{
    if (m_eTaskStatus == EBTTaskStatus::Inactive)
    {
        if (false == Checking_Decorator())
            return BT_RESULT_FAIL;

        for (auto& pService : m_Services)
            pService->PreExecuteService();

        PreExecuteTask();
        m_eResultType = ExecuteTask();
    }

    if (m_eResultType == BT_RESULT_CONTINUE)
    {
        m_pBehaviorTree->Set_LatentTask(this);
        return m_eResultType;
    }

    BT_RESULT_TYPE finalLateRunResult = m_eResultType;
    for (auto& pDecorator : m_Decorators)
    {
        BT_RESULT_TYPE eDecoratorResultType = pDecorator->Late_Run(pOwner);
        if (BT_RESULT_SUCCESS == eDecoratorResultType && BT_RESULT_FAIL != m_eResultType)
            finalLateRunResult = BT_RESULT_TYPE::BT_RESULT_SUCCESS;
        else if (BT_RESULT_FAIL == eDecoratorResultType)
            m_eResultType = BT_RESULT_TYPE::BT_RESULT_FAIL;
        else if (BT_RESULT_CONTINUE == eDecoratorResultType)
        {
            if (finalLateRunResult != BT_RESULT_SUCCESS && finalLateRunResult != BT_RESULT_FAIL)
                finalLateRunResult = BT_RESULT_TYPE::BT_RESULT_CONTINUE;
        }
    }

    m_eResultType = finalLateRunResult;
    if (BT_RESULT_CONTINUE != m_eResultType)
    {
        for (auto& pService : m_Services)
            pService->FinishService();

        OnTaskFinished(m_eResultType);
    }

    return m_eResultType;
}

void IBT_Task::FinishLatentTask(BT_RESULT_TYPE TaskResult)
{
    m_eTaskStatus = EBTTaskStatus::Finish;
    m_eResultType = TaskResult;

    m_pBehaviorTree->Set_LatentTask(nullptr);
}

void IBT_Task::FinishLatentAbort()
{
    AbortTask();
    m_eTaskStatus = EBTTaskStatus::Finish;
    m_eResultType = BT_RESULT_ABORT;
    m_pBehaviorTree->Set_LatentTask(nullptr);
}

void IBT_Task::TickTask(_float DeltaSeconds)
{
}

void IBT_Task::OnTaskFinished(BT_RESULT_TYPE TaskResult)
{
}

BT_RESULT_TYPE IBT_Task::AbortTask()
{
    return BT_RESULT_ABORT;
}

void IBT_Task::Request_Abort()
{
    for (auto& pService : m_Services)
        pService->FinishService();
}

void IBT_Task::Free()
{
    __super::Free();
}


CBT_Task_Wait::CBT_Task_Wait()
{
    m_bTickingTask = true;

    m_strClassName = "CBT_Task_Wait";
}

CBT_Task_Wait::CBT_Task_Wait(const CBT_Task_Wait& Prototype)
    : IBT_Task(Prototype)
    , m_fWaitTime{ Prototype.m_fWaitTime }
{
}

HRESULT CBT_Task_Wait::Initialize_Prototype(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Initialize_Prototype(pJsonValue, pTree);

    m_strNodeName = "Task_Wait";

    if (pJsonValue)
    {
        m_fWaitTime = (*pJsonValue)["m_fWaitTime"].GetFloat();
    }

    return S_OK;
}

//BT_RESULT_TYPE CBT_Task_Wait::Run(CGameObject* pOwner)
//{
//    m_fAccTime += m_pGameInstance->Get_TimeDelta();
//    if (m_fAccTime >= m_fWaitTime)
//    {
//        m_fAccTime = 0.f;
//        return m_eResultType = BT_RESULT_TYPE::BT_RESULT_SUCCESS;
//    }
//
//    return m_eResultType = BT_RESULT_TYPE::BT_RESULT_CONTINUE;
//}

BT_RESULT_TYPE CBT_Task_Wait::ExecuteTask()
{
    return BT_RESULT_CONTINUE;
}

void CBT_Task_Wait::TickTask(_float DeltaSeconds)
{
    m_fAccTime += m_pGameInstance->Get_TimeDelta();
    if (m_fAccTime >= m_fWaitTime)
       FinishLatentTask(BT_RESULT_TYPE::BT_RESULT_SUCCESS);

}

void CBT_Task_Wait::OnTaskFinished(BT_RESULT_TYPE TaskResult)
{
    m_fAccTime = 0.f;
}

rapidjson::Value CBT_Task_Wait::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    ObjValue.AddMember("m_fWaitTime", m_fWaitTime, Alloc);

    return ObjValue;
}

CBT_Task_Wait* CBT_Task_Wait::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    CBT_Task_Wait* pInstance = new CBT_Task_Wait();

    if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
    {
        MSG_BOX("Failed To Created : CBT_Task_Wait");
        Safe_Release(pInstance);
    }

    return pInstance;
}

IBT_Task* CBT_Task_Wait::Clone(void* pArg)
{
    return new CBT_Task_Wait(*this);
}

void CBT_Task_Wait::Free()
{
    __super::Free();
}

CBT_Task_PlayAnimation::CBT_Task_PlayAnimation()
{
    m_strClassName = "CBT_Task_PlayAnimation";
}

CBT_Task_PlayAnimation::CBT_Task_PlayAnimation(const CBT_Task_PlayAnimation& Prototype)
    : IBT_Task(Prototype)
    , m_bLoop{ Prototype.m_bLoop }
    , m_strAnimationName{ Prototype.m_strAnimationName }
    , m_strAnimLayerName{ Prototype.m_strAnimLayerName }
    , m_strPartName{ Prototype.m_strPartName }
{
}

HRESULT CBT_Task_PlayAnimation::Initialize_Prototype(rapidjson::Value* pJsonValue, class CBehaviorTree* pTree)
{
    __super::Initialize_Prototype(pJsonValue, pTree);

    return S_OK;
}

HRESULT CBT_Task_PlayAnimation::Late_Initialize(void* pArg)
{
    if (nullptr == m_pCompositeArmatrue)
    {
        CCompositeArmature* pComposite = m_pBehaviorTree->Get_Owner()->Get_Component<CCompositeArmature>();
        if (nullptr == pComposite)
            return E_FAIL;

        m_pCompositeArmatrue = pComposite;
    }

    m_strDelegateKey = "CBT_Task_PlayAnimation" + to_string(m_pGameInstance->Get_UniqueID());

    return S_OK;
}

BT_RESULT_TYPE CBT_Task_PlayAnimation::ExecuteTask()
{
    if (false == m_bWaitFinish)
    {
        if (m_strAnimLayerName.empty())
        {
            if (E_FAIL == m_pCompositeArmatrue->Select_Animation(m_strPartName, m_strAnimationName, m_bLoop))
                return BT_RESULT_FAIL;
        }
        else
        {
            if (E_FAIL == m_pCompositeArmatrue->Select_AnimLayer_Animation(m_strPartName, m_strAnimLayerName, m_strAnimationName, m_bLoop))
                return BT_RESULT_FAIL;
        }

        return BT_RESULT_SUCCESS;
    }
    else
    {
        if (m_strAnimLayerName == "")
        {
            if (E_FAIL == m_pCompositeArmatrue->Select_Animation(m_strPartName, m_strAnimationName, m_bLoop))
                return BT_RESULT_FAIL;
            else
            {
                m_iAnimIndex = m_pCompositeArmatrue->Get_AnimationIndexByName(m_strPartName, m_strAnimationName);
                if (m_iAnimIndex != -1)
                {
                    m_pAnimatorDelegate = &m_pCompositeArmatrue->Get_Animator(m_strPartName, m_strAnimLayerName)->m_OnAnimFinished[m_iAnimIndex];
                    m_pAnimatorDelegate->Remove(m_strDelegateKey);
                    m_pAnimatorDelegate->AddDynamic(m_strDelegateKey, this, &CBT_Task_PlayAnimation::OnAnimFinished);
                }
            }
        }
        else
        {
            if (E_FAIL == m_pCompositeArmatrue->Select_AnimLayer_Animation(m_strPartName, m_strAnimLayerName, m_strAnimationName, m_bLoop))
                return BT_RESULT_FAIL;
            else
            {
                m_iAnimIndex = m_pCompositeArmatrue->Get_AnimationIndexByName(m_strPartName, m_strAnimationName);
                if (m_iAnimIndex != -1)
                {
                    m_pAnimatorDelegate = &m_pCompositeArmatrue->Get_Animator(m_strPartName, m_strAnimLayerName)->m_OnAnimFinished[m_iAnimIndex];
                    m_pAnimatorDelegate->Remove(m_strDelegateKey);
                    m_pAnimatorDelegate->AddDynamic(m_strDelegateKey, this, &CBT_Task_PlayAnimation::OnAnimFinished);
                }
            }
        }

        return BT_RESULT_CONTINUE;
    }
}

void CBT_Task_PlayAnimation::TickTask(_float DeltaSeconds)
{
    
}

void CBT_Task_PlayAnimation::OnTaskFinished(BT_RESULT_TYPE TaskResult)
{
    if (m_pAnimatorDelegate) 
        m_pAnimatorDelegate->Remove(m_strDelegateKey);

    //m_pGameInstance->Add_Timer("CAbilityTask_PlayMontageAndWait", 0.f, [this]() {if (m_pAnimatorDelegate) m_pAnimatorDelegate->Remove(m_strDelegateKey); });
}

rapidjson::Value CBT_Task_PlayAnimation::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    ObjValue.AddMember("m_bLoop", m_bLoop, Alloc);
    ObjValue.AddMember("m_strPartName", StringRef(m_strPartName.c_str()), Alloc);
    ObjValue.AddMember("m_strAnimLayerName", StringRef(m_strAnimLayerName.c_str()), Alloc);
    ObjValue.AddMember("m_strAnimationName", StringRef(m_strAnimationName.c_str()), Alloc);
    ObjValue.AddMember("m_bWaitFinish", m_bWaitFinish, Alloc);

    return ObjValue;
}

HRESULT CBT_Task_PlayAnimation::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Load_JsonFile(pJsonValue, pTree);

    if (pJsonValue)
    {
        if ((*pJsonValue).HasMember("m_strPartName"))
            m_strPartName = (*pJsonValue)["m_strPartName"].GetString();

        if ((*pJsonValue).HasMember("m_strAnimLayerName"))
            m_strAnimLayerName = (*pJsonValue)["m_strAnimLayerName"].GetString();

        if ((*pJsonValue).HasMember("m_strAnimationName"))
            m_strAnimationName = (*pJsonValue)["m_strAnimationName"].GetString();

        if ((*pJsonValue).HasMember("m_bLoop"))
            m_bLoop = (*pJsonValue)["m_bLoop"].GetBool();

        if ((*pJsonValue).HasMember("m_bWaitFinish"))
            m_bWaitFinish = (*pJsonValue)["m_bWaitFinish"].GetBool();
    }

    m_strNodeName = "Task_PlayAnimation_" + m_strAnimationName;

    return S_OK;
}

void CBT_Task_PlayAnimation::OnAnimFinished(CAnimator* pAnimator, _uint iAnimIndex)
{
    if (m_iAnimIndex == iAnimIndex)
    {
        m_pGameInstance->Add_Timer("CBT_Task_PlayAnimation::OnAnimFinished", 0.f, [&]() {FinishLatentTask(BT_RESULT_SUCCESS); });
    }
}

CBT_Task_PlayAnimation* CBT_Task_PlayAnimation::Create(rapidjson::Value* pJsonValue, class CBehaviorTree* pTree)
{
    CBT_Task_PlayAnimation* pInstance = new CBT_Task_PlayAnimation();

    if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
    {
        MSG_BOX("Failed To Created : CBT_Task_PlayAnimation");
        Safe_Release(pInstance);
    }

    return pInstance;
}

IBT_Task* CBT_Task_PlayAnimation::Clone(void* pArg)
{
    return new CBT_Task_PlayAnimation(*this);
}

void CBT_Task_PlayAnimation::Free()
{
    __super::Free();
}

CBT_Task_MoveTo::CBT_Task_MoveTo()
{
    m_bTickingTask = true;

    m_strClassName = "CBT_Task_MoveTo";
}

CBT_Task_MoveTo::CBT_Task_MoveTo(const CBT_Task_MoveTo& Prototype)
    : IBT_Task(Prototype)
    , m_pairBlackboardValue{ Prototype.m_pairBlackboardValue }
    , m_fAcceptableRadius{ Prototype.m_fAcceptableRadius }
{

}

HRESULT CBT_Task_MoveTo::Initialize_Prototype(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Initialize_Prototype(pJsonValue, pTree);

    m_strNodeName = "Task_MoveTo";

    if (pJsonValue)
    {
        rapidjson::Value& PairValue = (*pJsonValue)["m_pairBlackboardValue"].GetObj();
        m_pairBlackboardValue = { PairValue["first"].GetString(), (TYPES)PairValue["second"].GetInt() };

        m_fAcceptableRadius = (*pJsonValue)["m_fAcceptableRadius"].GetFloat();
    }

    return S_OK;
}

BT_RESULT_TYPE CBT_Task_MoveTo::Run(CGameObject* pOwner)
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
    if (m_fAcceptableRadius >= fDistance)
        return BT_RESULT_SUCCESS;

    return BT_RESULT_TYPE::BT_RESULT_CONTINUE;
}

HRESULT CBT_Task_MoveTo::Late_Initialize(void* pArg)
{
    m_pNavigation = m_pOwner->Get_Component<CNavigation>();
    if (nullptr == m_pNavigation)
        return E_FAIL;

    return S_OK;
}

BT_RESULT_TYPE CBT_Task_MoveTo::ExecuteTask()
{
    return BT_RESULT_CONTINUE;
}

void CBT_Task_MoveTo::TickTask(_float DeltaSeconds)
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

    /* Checking Finish Condition */
    _vector vCurrentPos = m_pOwner->Get_TransformCom()->Get_Position();
    _float fDistance = XMVectorGetX(XMVector4Length(m_vGoalLocation - vCurrentPos));
    if (m_fAcceptableRadius >= fDistance)
        FinishLatentTask(BT_RESULT_SUCCESS);

    //// 대상과 나의 위치
    //_vector vTargetPos = m_vGoalLocation;
    //_vector vCurrentPos = m_pTransformCom->Get_State(CTransform::STATE_POSITION);

    //// 길찾기 요청
    //if (m_bChasing && m_pNavigationCom->Request_Path_Async(vCurrentPos, vTargetPos))
    //{
    //    _vector vNextPos;
    //    if (m_pNavigationCom->Get_NextWayPoint(vCurrentPos, vNextPos))
    //    {
    //        const _float fSpeed = 4.f; // 이동 속도

    //        // 다음 위치로 이동
    //        _vector vDirection = XMVector3Normalize(vNextPos - vCurrentPos);
    //        vNextPos = vCurrentPos + vDirection * fTimeDelta * fSpeed;
    //        m_pTransformCom->Set_State_Notify(CTransform::STATE_POSITION, vNextPos);
    //    }
    //}

 
}

rapidjson::Value CBT_Task_MoveTo::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    ObjValue.AddMember("m_fAcceptableRadius", m_fAcceptableRadius, Alloc);

    rapidjson::Value PairValue(rapidjson::kObjectType);

    rapidjson::Value strValue;
    strValue.SetString(m_pairBlackboardValue.first.c_str(), Alloc);
    PairValue.AddMember("first", strValue, Alloc);
    PairValue.AddMember("second", m_pairBlackboardValue.second, Alloc);
    ObjValue.AddMember("m_pairBlackboardValue", PairValue, Alloc);

    return ObjValue;
}

HRESULT CBT_Task_MoveTo::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Load_JsonFile(pJsonValue, pTree);

    m_strNodeName = "Task_MoveTo";

    if (pJsonValue)
    {
        rapidjson::Value& PairValue = (*pJsonValue)["m_pairBlackboardValue"].GetObj();
        m_pairBlackboardValue = { PairValue["first"].GetString(), (TYPES)PairValue["second"].GetInt() };

        m_fAcceptableRadius = (*pJsonValue)["m_fAcceptableRadius"].GetFloat();
    }

    return S_OK;
}

CBT_Task_MoveTo* CBT_Task_MoveTo::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    CBT_Task_MoveTo* pInstance = new CBT_Task_MoveTo();

    if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
    {
        MSG_BOX("Failed To Created : CBT_Task_MoveTo");
        Safe_Release(pInstance);
    }

    return pInstance;
}

IBT_Task* CBT_Task_MoveTo::Clone(void* pArg)
{
    return new CBT_Task_MoveTo(*this);
}

void CBT_Task_MoveTo::Free()
{
    __super::Free();
}

CBT_Task_FinishWithResult::CBT_Task_FinishWithResult()
{
    m_strClassName = "CBT_Task_FinishWithResult";
}

CBT_Task_FinishWithResult::CBT_Task_FinishWithResult(const CBT_Task_FinishWithResult& Prototype)
    : IBT_Task(Prototype)
    , m_eReturnResultType{ Prototype.m_eReturnResultType }
{

}

BT_RESULT_TYPE CBT_Task_FinishWithResult::ExecuteTask()
{
    return m_eReturnResultType;
}

rapidjson::Value CBT_Task_FinishWithResult::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    ObjValue.AddMember("m_eReturnResultType", m_eReturnResultType, Alloc);

    return ObjValue;
}

HRESULT CBT_Task_FinishWithResult::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    if (pJsonValue)
    {
        m_eReturnResultType = (BT_RESULT_TYPE)(*pJsonValue)["m_eReturnResultType"].GetInt();
    }

    return S_OK;
}

CBT_Task_FinishWithResult* CBT_Task_FinishWithResult::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    CBT_Task_FinishWithResult* pInstance = new CBT_Task_FinishWithResult();

    if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
    {
        MSG_BOX("Failed To Created : CBT_Task_FinishWithResult");
        Safe_Release(pInstance);
    }

    return pInstance;
}

IBT_Task* CBT_Task_FinishWithResult::Clone(void* pArg)
{
    return new CBT_Task_FinishWithResult(*this);
}

void CBT_Task_FinishWithResult::Free()
{
    __super::Free();
}

CBT_Task_Activate_Ability::CBT_Task_Activate_Ability()
{
    m_strClassName = "CBT_Task_Activate_Ability";
}

CBT_Task_Activate_Ability::CBT_Task_Activate_Ability(const CBT_Task_Activate_Ability& Prototype)
    : IBT_Task(Prototype)
{
}

HRESULT CBT_Task_Activate_Ability::Late_Initialize(void* pArg)
{
    CAbilitySystemComponent* pASC = m_pBehaviorTree->Get_Owner()->Get_Component<CAbilitySystemComponent>();
    if (nullptr == pASC)
        return E_FAIL;

    m_pASC = pASC;

    IGameplayAbility* pAbility = m_pASC->Find_Ability(m_strAbilityKey);
    if (nullptr == pAbility)
        return E_FAIL;

    m_pAbility = pAbility;

    return S_OK;
}

BT_RESULT_TYPE CBT_Task_Activate_Ability::ExecuteTask()
{
    if (false == m_pASC->TryActivate_Ability(m_strAbilityKey))
        return BT_RESULT_FAIL;

    // 시작하고 바로 종료한 케이스 체킹
    if (false == m_pAbility->IsActivate())
        return BT_RESULT_SUCCESS;
    else
    {
        m_pAbility->m_AbilityDelegate.AddDynamic("CBT_Task_Activate_Ability" + to_string(m_iNodeIndex), this, &CBT_Task_Activate_Ability::OnAbilityBroadcast);
    }

    return BT_RESULT_CONTINUE;
}

void CBT_Task_Activate_Ability::OnTaskFinished(BT_RESULT_TYPE TaskResult)
{
    m_pGameInstance->Add_Timer("CBT_Task_Activate_Ability", 0.f, [&]() {m_pAbility->m_AbilityDelegate.Remove("CBT_Task_Activate_Ability" + to_string(m_iNodeIndex)); });
}

rapidjson::Value CBT_Task_Activate_Ability::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    ObjValue.AddMember("m_strAbilityKey", StringRef(m_strAbilityKey.c_str()), Alloc);

    return ObjValue;
}

HRESULT CBT_Task_Activate_Ability::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    if (E_FAIL == __super::Load_JsonFile(pJsonValue, pTree))
        return E_FAIL;

    if (pJsonValue)
    {
        if ((*pJsonValue).HasMember("m_strAbilityKey"))
            m_strAbilityKey = (*pJsonValue)["m_strAbilityKey"].GetString();
    }

    return S_OK;
}

void CBT_Task_Activate_Ability::OnAbilityBroadcast(IGameplayAbility* pAbility, _uint iEventType)
{
    IGameplayAbility::Delegate_EventType eAbilityDelegateType = static_cast<IGameplayAbility::Delegate_EventType>(iEventType);
    if (eAbilityDelegateType == IGameplayAbility::TYPE_END_ABILITY)
        FinishLatentTask(BT_RESULT_SUCCESS);
    else if (eAbilityDelegateType == IGameplayAbility::TYPE_ACTIVATE_FAIL ||
        eAbilityDelegateType == IGameplayAbility::TYPE_CANCEL)
    {
        FinishLatentTask(BT_RESULT_FAIL);
    }
}

CBT_Task_Activate_Ability* CBT_Task_Activate_Ability::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    CBT_Task_Activate_Ability* pInstance = new CBT_Task_Activate_Ability();

    if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
    {
        MSG_BOX("Failed To Created : CBT_Task_Activate_Ability");
        Safe_Release(pInstance);
    }

    return pInstance;
}

IBT_Task* CBT_Task_Activate_Ability::Clone(void* pArg)
{
    return new CBT_Task_Activate_Ability(*this);
}

void CBT_Task_Activate_Ability::Free()
{
    __super::Free();
}

CBT_Task_SetBlackboardKey::CBT_Task_SetBlackboardKey()
{
    m_strClassName = "CBT_Task_SetBlackboardKey";
}

BT_RESULT_TYPE CBT_Task_SetBlackboardKey::ExecuteTask()
{
    if (E_FAIL == m_pBehaviorTree->Set_BlackboardKey(m_pairBlackboardValue.first, m_Value))
        return BT_RESULT_FAIL;

    return BT_RESULT_SUCCESS;
}

rapidjson::Value CBT_Task_SetBlackboardKey::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

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
        MSG_BOX("CBT_Task_SetBlackboardKey : Save Error Type Value!");
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

HRESULT CBT_Task_SetBlackboardKey::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    if (E_FAIL == __super::Load_JsonFile(pJsonValue, pTree))
        return E_FAIL;

    if (pJsonValue)
    {
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
            MSG_BOX("CBT_Task_SetBlackboardKey : Load Error Type Value!");
            break;
        }
    }

    return S_OK;
}

CBT_Task_SetBlackboardKey* CBT_Task_SetBlackboardKey::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    return new CBT_Task_SetBlackboardKey();
}

void CBT_Task_SetBlackboardKey::Free()
{
    __super::Free();
}

CBT_Task_WaitUntilAnimFinished::CBT_Task_WaitUntilAnimFinished()
{
    m_strClassName = "CBT_Task_WaitUntilAnimFinished";

    m_bTickingTask = true;
}

HRESULT CBT_Task_WaitUntilAnimFinished::Late_Initialize(void* pArg)
{
    CCompositeArmature* pComposite = m_pBehaviorTree->Get_Owner()->Get_Component<CCompositeArmature>();
    if (nullptr == pComposite)
        return E_FAIL;

    m_pCompositeArmatrue = pComposite;

    return S_OK;
}

BT_RESULT_TYPE CBT_Task_WaitUntilAnimFinished::ExecuteTask()
{
    return BT_RESULT_CONTINUE;
}

void CBT_Task_WaitUntilAnimFinished::TickTask(_float DeltaSeconds)
{
    if (true == m_pCompositeArmatrue->Get_AnimEndNotify(""))
        FinishLatentTask(BT_RESULT_SUCCESS);
}

CBT_Task_WaitUntilAnimFinished* CBT_Task_WaitUntilAnimFinished::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    return new CBT_Task_WaitUntilAnimFinished();
}

void CBT_Task_WaitUntilAnimFinished::Free()
{
    __super::Free();
}
