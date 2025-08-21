#include "BTTask_Ashley.h"
#include "GameplayAbilitySystemHeader.h"
#include "BehaviorTreeHeader.h"
#include "Reon.h"

CBTTask_Ashley::CBTTask_Ashley()
{
}

HRESULT CBTTask_Ashley::Late_Initialize(void* pArg)
{
    CAshley* pAshley = dynamic_cast<CAshley*>(m_pOwner);
    if (nullptr == pAshley)
        return E_FAIL;

    m_pAshley = pAshley;

    CCompositeArmature* pComposite = m_pAshley->Get_CompositeArmature();
    if (nullptr == pComposite)
        return E_FAIL;
    m_pCompositeArmature = pComposite;

    CContext* pVariableContext = m_pAshley->Get_VariableContext();
    if (nullptr == pVariableContext)
        return E_FAIL;

    m_pAshleyContext = pVariableContext;

    if (false == m_pAshleyContext->Has_Variable("AttachToReonKey"))
        return E_FAIL;
    if (false == m_pAshleyContext->Has_Variable("IsInCloseRangeKey"))
        return E_FAIL;
    if (false == m_pAshleyContext->Has_Variable("IsInWalkRangeKey"))
        return E_FAIL;
    if (false == m_pAshleyContext->Has_Variable("IsInDashRangeKey"))
        return E_FAIL;

    m_pAttachToReon = m_pAshleyContext->Get_Variable<_bool*>("AttachToReonKey");
    m_pIsInCloseRange = m_pAshleyContext->Get_Variable<_bool*>("IsInCloseRangeKey");
    m_pIsInWalkRange = m_pAshleyContext->Get_Variable<_bool*>("IsInWalkRangeKey");
    m_pIsInDashRange = m_pAshleyContext->Get_Variable<_bool*>("IsInDashRangeKey");

    return S_OK;
}

void CBTTask_Ashley::Free()
{
    __super::Free();
}

CBTTask_TurnToTarget::CBTTask_TurnToTarget()
{
    m_strClassName = "CBTTask_TurnToTarget";
    m_bTickingTask = true;
}

HRESULT CBTTask_TurnToTarget::Initialize_Prototype(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Initialize_Prototype(pJsonValue, pTree);

    m_strNodeName = "CBTTask_TurnToTarget";

    return S_OK;
}

BT_RESULT_TYPE CBTTask_TurnToTarget::ExecuteTask()
{
    m_pAshley->Set_CanTurn(false);

    if (m_bInstance)
    {
        m_pAshley->Get_TransformCom()->Rotation_Dir_AtPoint(m_pAshley->Get_Target()->Get_TransformCom()->Get_Position());
        return BT_RESULT_SUCCESS;
    }

    // TurnSmooth To Interaction Point

    _vector vTargetPoistion = m_pAshley->Get_GoalPoint()->Get_TargetPoint();
    _float fRadian = 0.f;

    if (false == m_pAshley->Get_TransformCom()->IsLookingAtTarget(vTargetPoistion, 30.f, fRadian))
    {
        _float fAbsRadian = fabs(fRadian);
        if (fAbsRadian >= XMConvertToRadians(135))
        {
            // 타겟이 나를 기준으로 오른쪽에 있는 케이스
            if (fRadian >= 0)
                m_pCompositeArmature->Select_Animation("", "cha1_general_0181_turn_R180", true);
            else
                m_pCompositeArmature->Select_Animation("", "cha1_general_0180_turn_L180", true);
        }
        else if (fAbsRadian >= XMConvertToRadians(60.f))
        {
            if (fRadian >= 0)
                m_pCompositeArmature->Select_Animation("", "cha1_general_0185_turn_R90", true);
            else
                m_pCompositeArmature->Select_Animation("", "cha1_general_0184_turn_L90", true);
        }
        else if (fAbsRadian >= XMConvertToRadians(30.f))
        {
            if (fRadian >= 0)
                m_pCompositeArmature->Select_Animation("", "cha1_general_0182_turn_L45", true);
            else
                m_pCompositeArmature->Select_Animation("", "cha1_general_0183_turn_R45", true);
        }
    }
    else
        return BT_RESULT_SUCCESS;

    return BT_RESULT_CONTINUE;
}

void CBTTask_TurnToTarget::TickTask(_float DeltaSeconds)
{
    if (m_pCompositeArmature->Get_AnimEndNotify(""))
        FinishLatentTask(BT_RESULT_SUCCESS);
}

void CBTTask_TurnToTarget::OnTaskFinished(BT_RESULT_TYPE TaskResult)
{
    m_pAshley->Set_CanTurn(true);
}

BT_RESULT_TYPE CBTTask_TurnToTarget::AbortTask()
{
    m_pAshley->Set_CanTurn(true);
    return BT_RESULT_ABORT;
}

rapidjson::Value CBTTask_TurnToTarget::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    ObjValue.AddMember("m_bInstance", m_bInstance, Alloc);

    return ObjValue;
}

HRESULT CBTTask_TurnToTarget::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    if (pJsonValue)
    {
        if ((*pJsonValue).HasMember("m_bInstance"))
            m_bInstance = (*pJsonValue)["m_bInstance"].GetBool();
    }

    return S_OK;

}

CBTTask_TurnToTarget* CBTTask_TurnToTarget::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    CBTTask_TurnToTarget* pInstance = new CBTTask_TurnToTarget();

    if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
    {
        MSG_BOX("Failed To Created : CBTTask_TurnToTarget");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CBTTask_TurnToTarget::Free()
{
    __super::Free();
}


CBTTask_MoveToTarget::CBTTask_MoveToTarget()
{
    m_strClassName = "CBTTask_MoveToTarget";

    m_bTickingTask = true;
}

HRESULT CBTTask_MoveToTarget::Late_Initialize(void* pArg)
{
    __super::Late_Initialize(pArg);

    return S_OK;
}

BT_RESULT_TYPE CBTTask_MoveToTarget::ExecuteTask()
{
    /* Acceptance Radius를 갱신하지 않은 채 콜리전 검사를 하면 오류가 있어서 Tick에서 도착했는지 판단 */
    //if (m_pAshley->Get_CollideWithGoalPoint())
    //    return BT_RESULT_SUCCESS;

    m_pAshley->Set_CanTurn(false);

    m_pAshley->Get_GoalPoint()->Get_Bound()->Set_Extent({ m_fAcceptanceRadius, m_fAcceptanceRadius , m_fAcceptanceRadius });

    m_pAshley->Get_CompositeArmature()->Select_Animation("", m_strMoveAnimationKey, true);

    return BT_RESULT_CONTINUE;
}

void CBTTask_MoveToTarget::TickTask(_float DeltaSeconds)
{
    //static _float fDebug = 0.5f;
    //static _float fDebugRotationPerSec = 0.5f;
    //cout << "ReachedThresholdSq: " << fDebug << '\n' << "fDebugRotationPerSec: " << fDebugRotationPerSec << '\n';

    /* For  Debug */
    //if (m_pGameInstance->Key_Pressing(DIK_EQUALS))
    //{
    //    //m_fAcceptanceRadius += 0.01f;
    //    fDebug += 0.01f;
    //    m_pAshley->Get_Navigation()->Set_ReachedThresholdSq(fDebug);
    //}
    //if (m_pGameInstance->Key_Pressing(DIK_MINUS))
    //{
    //    //m_fAcceptanceRadius -= 0.01f;
    //    m_pAshley->Get_Navigation()->Set_ReachedThresholdSq(fDebug);
    //    fDebug -= 0.01f;
    //}

    //if (m_pGameInstance->Key_Pressing(DIK_O))
    //{
    //    fDebugRotationPerSec -= 0.01f;
    //    fDebugRotationPerSec = clamp(fDebugRotationPerSec, 0.f, 100.f);
    //    m_pAshley->Get_TransformCom()->Set_RotationPerSec(fDebugRotationPerSec);
    //}
    //if (m_pGameInstance->Key_Pressing(DIK_P))
    //{
    //    fDebugRotationPerSec += 0.01f;
    //    fDebugRotationPerSec = clamp(fDebugRotationPerSec, 0.f, 100.f);
    //    m_pAshley->Get_TransformCom()->Set_RotationPerSec(fDebugRotationPerSec);
    //}


    //m_pAshley->Get_GoalPoint()->Get_Bound()->Set_Extent({ m_fAcceptanceRadius , m_fAcceptanceRadius , m_fAcceptanceRadius });


    if (m_pAshley->Get_CollideWithGoalPoint())
        FinishLatentTask(BT_RESULT_SUCCESS);

    /* Way 포인트에 도달하지 않았을 경우에만 Way포인트를 향해서 회전 */

    if (m_bUseNavigation)
    {
        if (false == m_pAshley->Get_CollideWithInteractionPoint())
        {
            m_pAshley->Get_TransformCom()->Rotation_Dir_AtPoint(m_pAshley->Get_InteractionPoint()->Get_TargetPoint(), DeltaSeconds);

            //m_pAshley->Get_TransformCom()->Rotation_Dir_AtPoint(m_pAshley->Get_InteractionPoint()->Get_TargetPoint());
        }
    }
    else
    {
        m_pAshley->Get_TransformCom()->Rotation_Dir_AtPoint(m_pAshley->Get_GoalPoint()->Get_TargetPoint(), DeltaSeconds);
    }
}

void CBTTask_MoveToTarget::OnTaskFinished(BT_RESULT_TYPE TaskResult)
{
    if (true == m_pAshley->Get_AbilitySystemComponent()->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_CROUCHING)) ||  m_strMoveAnimationKey == "cha1_defence_4520_jog_loop_VerA")
    {
        m_pAshley->Get_CompositeArmature()->Select_Animation("", "cha1_general_1160_stand_loop", true);
    }
    else
    {
        m_pAshley->Get_CompositeArmature()->Select_Animation("", "cha1_general_0160_stand_loop", true);
    }

    m_pAshley->Set_CanTurn(true);
}

BT_RESULT_TYPE CBTTask_MoveToTarget::AbortTask()
{
    m_pAshley->Set_CanTurn(true);
    return BT_RESULT_ABORT;
}

rapidjson::Value CBTTask_MoveToTarget::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    ObjValue.AddMember("m_strMoveAnimationKey", StringRef(m_strMoveAnimationKey.c_str()), Alloc);
    ObjValue.AddMember("m_fAcceptanceRadius", m_fAcceptanceRadius, Alloc);

    ObjValue.AddMember("m_bUseNavigation", m_bUseNavigation, Alloc);
    
    return ObjValue;
}

HRESULT CBTTask_MoveToTarget::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Load_JsonFile(pJsonValue, pTree);

    if (pJsonValue)
    {
        if ((*pJsonValue).HasMember("m_strMoveAnimationKey"))
            m_strMoveAnimationKey = (*pJsonValue)["m_strMoveAnimationKey"].GetString();

        if ((*pJsonValue).HasMember("m_fAcceptanceRadius"))
            m_fAcceptanceRadius = (*pJsonValue)["m_fAcceptanceRadius"].GetFloat();

        if ((*pJsonValue).HasMember("m_bUseNavigation"))
            m_bUseNavigation = (*pJsonValue)["m_bUseNavigation"].GetBool();
    }

    return S_OK;
}

CBTTask_MoveToTarget* CBTTask_MoveToTarget::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    CBTTask_MoveToTarget* pInstance = new CBTTask_MoveToTarget();

    if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
    {
        MSG_BOX("Failed To Created : CBTTask_MoveToTarget");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CBTTask_MoveToTarget::Free()
{
    __super::Free();
}

CBTTask_EnableTurn::CBTTask_EnableTurn()
{
    m_strClassName = "CBTTask_EnableTurn";
}

HRESULT CBTTask_EnableTurn::Initialize_Prototype(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Initialize_Prototype(pJsonValue, pTree);

    m_strNodeName = "CBTTask_EnableTurn";

    return S_OK;
}

BT_RESULT_TYPE CBTTask_EnableTurn::ExecuteTask()
{
    m_pAshley->Set_CanTurn(m_bEnableTurn);

    return BT_RESULT_SUCCESS;
}

rapidjson::Value CBTTask_EnableTurn::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    ObjValue.AddMember("m_bEnableTurn", m_bEnableTurn, Alloc);

    return ObjValue;
}

CBTTask_EnableTurn* CBTTask_EnableTurn::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    CBTTask_EnableTurn* pInstance = new CBTTask_EnableTurn();

    if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
    {
        MSG_BOX("Failed To Created : CBTTask_EnableTurn");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CBTTask_EnableTurn::Free()
{
    __super::Free();
}

CBTTask_Ashley_MoveClose::CBTTask_Ashley_MoveClose()
{
    m_bTickingTask = true;

    m_strClassName = "CBTTask_Ashley_MoveClose";
}

HRESULT CBTTask_Ashley_MoveClose::Initialize_Prototype(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Initialize_Prototype(pJsonValue, pTree);

    m_strNodeName = "CBTTask_Ashley_MoveClose";

    return S_OK;
}

HRESULT CBTTask_Ashley_MoveClose::Late_Initialize(void* pArg)
{
    __super::Late_Initialize(pArg);

    return S_OK;
}

BT_RESULT_TYPE CBTTask_Ashley_MoveClose::ExecuteTask()
{
    return BT_RESULT_CONTINUE;
}

void CBTTask_Ashley_MoveClose::TickTask(_float fTimeDelta)
{
    _bool IsInCloseRange = *m_pIsInCloseRange;
    _bool IsInWalkRange = *m_pIsInWalkRange;
    _bool IsInDashRange = *m_pIsInDashRange;

    if (IsInCloseRange)
    {
        m_pAshley->Get_CompositeArmature()->Select_Animation("", "cha1_general_0160_stand_loop", true);
        FinishLatentTask(BT_RESULT_SUCCESS);
    }
    else if (IsInWalkRange)
        m_pAshley->Get_CompositeArmature()->Select_Animation("", "cha1_general_0403_walk_F_loop", true);
    else
        m_pAshley->Get_CompositeArmature()->Select_Animation("", "cha1_general_0520_jog_loop_VerA", true);
}

void CBTTask_Ashley_MoveClose::OnTaskFinished(BT_RESULT_TYPE TaskResult)
{
}

rapidjson::Value CBTTask_Ashley_MoveClose::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    return ObjValue;
}

CBTTask_Ashley_MoveClose* CBTTask_Ashley_MoveClose::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    CBTTask_Ashley_MoveClose* pInstance = new CBTTask_Ashley_MoveClose();

    if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
    {
        MSG_BOX("Failed To Created : CBTTask_Ashley_MoveClose");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CBTTask_Ashley_MoveClose::Free()
{
    __super::Free();
}

CBTTask_Ashley_MoveKeepDistance::CBTTask_Ashley_MoveKeepDistance()
{
    m_bTickingTask = true;

    m_strClassName = "CBTTask_Ashley_MoveKeepDistance";
}

HRESULT CBTTask_Ashley_MoveKeepDistance::Initialize_Prototype(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Initialize_Prototype(pJsonValue, pTree);

    return S_OK;
}

HRESULT CBTTask_Ashley_MoveKeepDistance::Late_Initialize(void* pArg)
{
    __super::Late_Initialize(pArg);

    return S_OK;
}

BT_RESULT_TYPE CBTTask_Ashley_MoveKeepDistance::ExecuteTask()
{
    return BT_RESULT_CONTINUE;
}

void CBTTask_Ashley_MoveKeepDistance::TickTask(_float fTimeDelta)
{
    _bool IsInWalkRange = *m_pIsInWalkRange;
    _bool IsInDashRange = *m_pIsInDashRange;
    
    if (IsInWalkRange)
    {
        // 뒤로가는 에니메이션
        m_pAshley->Get_CompositeArmature()->Select_Animation("", "cha1_defence_2720_walk_B_loop_VerA", true);
    }
    else if (IsInDashRange)
    {
        m_pAshley->Get_CompositeArmature()->Select_Animation("", "cha1_general_0160_stand_loop", true);
        FinishLatentTask(BT_RESULT_SUCCESS);
    }
    else
        m_pAshley->Get_CompositeArmature()->Select_Animation("", "cha1_general_0520_jog_loop_VerA", true);
}

void CBTTask_Ashley_MoveKeepDistance::OnTaskFinished(BT_RESULT_TYPE TaskResult)
{
}

rapidjson::Value CBTTask_Ashley_MoveKeepDistance::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    return ObjValue;
}

CBTTask_Ashley_MoveKeepDistance* CBTTask_Ashley_MoveKeepDistance::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    CBTTask_Ashley_MoveKeepDistance* pInstance = new CBTTask_Ashley_MoveKeepDistance();

    if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
    {
        MSG_BOX("Failed To Created : CBTTask_Ashley_MoveKeepDistance");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CBTTask_Ashley_MoveKeepDistance::Free()
{
    __super::Free();
}

CBTTask_SendGameplayEvent::CBTTask_SendGameplayEvent()
{
    m_strClassName = "CBTTask_SendGameplayEvent";
}

HRESULT CBTTask_SendGameplayEvent::Initialize_Prototype(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Initialize_Prototype(pJsonValue, pTree);

    return S_OK;
}

HRESULT CBTTask_SendGameplayEvent::Late_Initialize(void* pArg)
{
    __super::Late_Initialize(pArg);

    return S_OK;
}

BT_RESULT_TYPE CBTTask_SendGameplayEvent::ExecuteTask()
{
    GameplayEventData EventData;
    EventData.EventTag = GameplayTag(m_strEventTag);

    if (false == m_bSendSelf)
    {
        if (m_pAshley->Get_Reon()->Get_AbilitySystemComponent()->HandleGameplayEvent(GameplayTag(m_strEventTag), &EventData))
            return BT_RESULT_SUCCESS;
        else
            return BT_RESULT_FAIL;
    }
    else
    {
        m_pAshley->Receive_Event(&EventData);
        return BT_RESULT_SUCCESS;
    }

    return BT_RESULT_FAIL;
}

rapidjson::Value CBTTask_SendGameplayEvent::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    ObjValue.AddMember("m_strEventTag", StringRef(m_strEventTag.c_str()), Alloc);
    ObjValue.AddMember("m_bSendSelf", m_bSendSelf, Alloc);
    
    return ObjValue;
}

HRESULT CBTTask_SendGameplayEvent::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Load_JsonFile(pJsonValue, pTree);

    if (pJsonValue)
    {
        if ((*pJsonValue).HasMember("m_strEventTag"))
            m_strEventTag = (*pJsonValue)["m_strEventTag"].GetString();
        if ((*pJsonValue).HasMember("m_bSendSelf"))
            m_bSendSelf = (*pJsonValue)["m_bSendSelf"].GetBool();
    }

    return S_OK;
}

CBTTask_SendGameplayEvent* CBTTask_SendGameplayEvent::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    CBTTask_SendGameplayEvent* pInstance = new CBTTask_SendGameplayEvent();

    if (FAILED(pInstance->Initialize_Prototype(pJsonValue, pTree)))
    {
        MSG_BOX("Failed To Created : CBTTask_SendGameplayEvent");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CBTTask_SendGameplayEvent::Free()
{
    __super::Free();
}

CBTTask_MoveToGoalLocationByCrouching::CBTTask_MoveToGoalLocationByCrouching()
{
    m_strClassName = "CBTTask_MoveToGoalLocationByCrouching";
}

BT_RESULT_TYPE CBTTask_MoveToGoalLocationByCrouching::ExecuteTask()
{
    m_pAshley->Get_CompositeArmature()->Select_Animation("", "cha1_general_1403_walk_F_loop", true);
    return BT_RESULT_SUCCESS;
}

void CBTTask_MoveToGoalLocationByCrouching::TickTask(_float fTimeDelta)
{
    _vector vAshleyPos = m_pAshley->Get_TransformCom()->Get_Position();
    _vector vGoalLocation = m_pBehaviorTree->Get_Blackboard()->Get_Variable<_vector>(KEY_BB_GOAL_LOACATION);
    _float fDistance = XMVectorGetX(XMVector3Length(vAshleyPos - vGoalLocation));
    
    if (fDistance <= m_fAcceptanceRadius)
    {
        m_pAshley->Get_CompositeArmature()->Select_Animation("", "cha1_general_1160_stand_loop", true);
    }
    else
    {
        m_pAshley->Get_CompositeArmature()->Select_Animation("", "cha1_general_1403_walk_F_loop", true);
    }

    m_pAshley->Get_TransformCom()->Rotation_Dir_AtPoint(vGoalLocation);
}

rapidjson::Value CBTTask_MoveToGoalLocationByCrouching::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    ObjValue.AddMember("m_fAcceptanceRadius", m_fAcceptanceRadius, Alloc);

    return ObjValue;
}

HRESULT CBTTask_MoveToGoalLocationByCrouching::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Load_JsonFile(pJsonValue, pTree);

    if (pJsonValue)
    {
        if ((*pJsonValue).HasMember("m_fAcceptanceRadius"))
            m_fAcceptanceRadius = (*pJsonValue)["m_fAcceptanceRadius"].GetFloat();
    }

    return S_OK;
}

CBTTask_MoveToGoalLocationByCrouching* CBTTask_MoveToGoalLocationByCrouching::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    return new CBTTask_MoveToGoalLocationByCrouching();
}

void CBTTask_MoveToGoalLocationByCrouching::Free()
{
    __super::Free();
}

CBTTask_ActivateAbilityLocalInputPressed::CBTTask_ActivateAbilityLocalInputPressed()
{
    m_strClassName = "CBTTask_ActivateAbilityLocalInputPressed";
}

BT_RESULT_TYPE CBTTask_ActivateAbilityLocalInputPressed::ExecuteTask()
{
    m_pAshley->Get_AbilitySystemComponent()->AbilityLocalInputPressed(m_eActionType);
    return BT_RESULT_SUCCESS;
}

void CBTTask_ActivateAbilityLocalInputPressed::TickTask(_float fTimeDelta)
{
}

rapidjson::Value CBTTask_ActivateAbilityLocalInputPressed::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    ObjValue.AddMember("m_eActionType", m_eActionType, Alloc);

    return ObjValue;
}

HRESULT CBTTask_ActivateAbilityLocalInputPressed::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Load_JsonFile(pJsonValue, pTree);

    if (pJsonValue)
    {
        if ((*pJsonValue).HasMember("m_eActionType"))
            m_eActionType = static_cast<ActionType>((*pJsonValue)["m_eActionType"].GetInt());
    }

    return S_OK;
}

CBTTask_ActivateAbilityLocalInputPressed* CBTTask_ActivateAbilityLocalInputPressed::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    return new CBTTask_ActivateAbilityLocalInputPressed;
}

void CBTTask_ActivateAbilityLocalInputPressed::Free()
{
    __super::Free();
}

CBTTask_CheckingTargetInFrontOf::CBTTask_CheckingTargetInFrontOf()
{
    m_strClassName = "CBTTask_CheckingTargetInFrontOf";
}

BT_RESULT_TYPE CBTTask_CheckingTargetInFrontOf::ExecuteTask()
{
    _vector vCamPos = XMLoadFloat4(m_pGameInstance->Get_CamPosition());
    _vector vCamDir = XMVector3Normalize(XMLoadFloat4(m_pGameInstance->Get_CamDirection()));

    _vector vMyPos = m_pOwner->Get_TransformCom()->Get_Position();

    _vector vCam2OwnerDirection = XMVector3Normalize(vMyPos - vCamPos);

    _float fDotResult = XMVectorGetX(XMVector3Dot(vCam2OwnerDirection, vCamDir));
    fDotResult = fmaxf(-1.0f, fminf(fDotResult, 1.0f));
    _float fCam2OwnerRadian = acosf(fDotResult);

    /* 카메라의 방향벡터와 카메라와 에슐리와의 방향벡터와의 각도가 100도보다 작았다면 정면에 있었다고 판단 */
    if (XMConvertToRadians(100.f) >= fCam2OwnerRadian)
        return BT_RESULT_SUCCESS;
    else
        return BT_RESULT_FAIL;
}

void CBTTask_CheckingTargetInFrontOf::TickTask(_float fTimeDelta)
{
}

rapidjson::Value CBTTask_CheckingTargetInFrontOf::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    return ObjValue;
}

HRESULT CBTTask_CheckingTargetInFrontOf::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Load_JsonFile(pJsonValue, pTree);

    return S_OK;
}

CBTTask_CheckingTargetInFrontOf* CBTTask_CheckingTargetInFrontOf::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    return new CBTTask_CheckingTargetInFrontOf;
}

void CBTTask_CheckingTargetInFrontOf::Free()
{
    __super::Free();
}

CBTTask_ImitatePlayerMove::CBTTask_ImitatePlayerMove()
{
    m_strClassName = "CBTTask_ImitatePlayerMove";

    m_bTickingTask = true;
}

HRESULT CBTTask_ImitatePlayerMove::Late_Initialize(void* pArg)
{
    __super::Late_Initialize(pArg);

    CContext* pReonVariableContext = m_pAshley->Get_Reon()->Get_VariableContext();
    if (nullptr == pReonVariableContext)
        return E_FAIL;

    if (pReonVariableContext->Has_Variable("MoveKey"))
        m_pReonInputDirType = pReonVariableContext->Get_Variable<_int*>("MoveKey");

    if (nullptr == m_pReonInputDirType)
        return E_FAIL;

    m_WalkAnims[DIR_FRONT] = "cha1_general_0403_walk_F_loop";
    m_WalkAnims[DIR_FRONTRIGHT] = "cha1_general_0423_walk_LF_loop";
    m_WalkAnims[DIR_RIGHT] = "cha1_general_0473_walk_R_loop";
    m_WalkAnims[DIR_BACKRIGHT] = "cha1_general_0453_walk_BR_loop";
    m_WalkAnims[DIR_BACK] = "cha1_general_0443_walk_B_loop";
    m_WalkAnims[DIR_BACKLEFT] = "cha1_general_0463_walk_RB_loop";
    m_WalkAnims[DIR_LEFT] = "cha1_general_0433_walk_L_loop";
    m_WalkAnims[DIR_FRONTLEFT] = "cha1_general_0413_walk_FL_loop";
    m_WalkAnims[DIR_END] = "cha1_general_0160_stand_loop";

    m_fOriginRotatePerSec = m_pAshley->Get_TransformCom()->Get_RotationPerSec();

    return S_OK;
}

BT_RESULT_TYPE CBTTask_ImitatePlayerMove::ExecuteTask()
{
    m_pAshley->Get_TransformCom()->Set_RotationPerSec(XM_PI * 0.6f);
    //m_pAshley->Get_TransformCom()->LookAt_FixYaw(m_pAshley->Get_Reon()->Get_TransformCom()->Get_Position());

    return BT_RESULT_CONTINUE;
}

void CBTTask_ImitatePlayerMove::TickTask(_float fTimeDelta)
{
    CReon* pReon = m_pAshley->Get_Reon();

    if (pReon->Get_AbilitySystemComponent()->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_WALK)))
    {
        m_pAshley->Get_CompositeArmature()->Select_Animation("", m_WalkAnims[*m_pReonInputDirType], true);
    }
    else
        m_pAshley->Get_CompositeArmature()->Select_Animation("", "cha1_general_0160_stand_loop", true);
}

void CBTTask_ImitatePlayerMove::OnTaskFinished(BT_RESULT_TYPE TaskResult)
{
    m_pAshley->Get_TransformCom()->Set_RotationPerSec(m_fOriginRotatePerSec);

    m_pAshley->Get_CompositeArmature()->Select_Animation("", "cha1_general_0160_stand_loop", true);
}

BT_RESULT_TYPE CBTTask_ImitatePlayerMove::AbortTask()
{
    m_pAshley->Get_CompositeArmature()->Select_Animation("", "cha1_general_0160_stand_loop", true);

    return BT_RESULT_ABORT;
}

CBTTask_ImitatePlayerMove* CBTTask_ImitatePlayerMove::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    return new CBTTask_ImitatePlayerMove;
}

void CBTTask_ImitatePlayerMove::Free()
{
    __super::Free();
}

CBTTask_WaitUntilReceiveGameplayEvent::CBTTask_WaitUntilReceiveGameplayEvent()
{
    m_strClassName = "CBTTask_WaitUntilReceiveGameplayEvent";
}

BT_RESULT_TYPE CBTTask_WaitUntilReceiveGameplayEvent::ExecuteTask()
{
    m_pAshley->m_OnGameplayEvent.AddDynamic("CBTTask_WaitUntilReceiveGameplayEvent" + to_string(m_iNodeIndex), this, &CBTTask_WaitUntilReceiveGameplayEvent::OnGameplayEvent);

    return BT_RESULT_CONTINUE;
}

void CBTTask_WaitUntilReceiveGameplayEvent::OnTaskFinished(BT_RESULT_TYPE TaskResult)
{
    m_pAshley->m_OnGameplayEvent.Remove("CBTTask_WaitUntilReceiveGameplayEvent" + to_string(m_iNodeIndex));
}

BT_RESULT_TYPE CBTTask_WaitUntilReceiveGameplayEvent::AbortTask()
{
    m_pAshley->m_OnGameplayEvent.Remove("CBTTask_WaitUntilReceiveGameplayEvent" + to_string(m_iNodeIndex));
    return BT_RESULT_ABORT;
}

rapidjson::Value CBTTask_WaitUntilReceiveGameplayEvent::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    ObjValue.AddMember("m_EventTag", StringRef(m_EventTag.m_strTag.c_str()), Alloc);

    return ObjValue;
}

HRESULT CBTTask_WaitUntilReceiveGameplayEvent::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Load_JsonFile(pJsonValue, pTree);

    if (pJsonValue)
    {
        if ((*pJsonValue).HasMember("m_EventTag"))
            m_EventTag = GameplayTag((*pJsonValue)["m_EventTag"].GetString());
    }

    return S_OK;
}

void CBTTask_WaitUntilReceiveGameplayEvent::OnGameplayEvent(const GameplayEventData* pEventData)
{
    if (nullptr == pEventData)
        return;

    if (m_EventTag == pEventData->EventTag)
    {
        FinishLatentTask(BT_RESULT_SUCCESS);
    }
}

CBTTask_WaitUntilReceiveGameplayEvent* CBTTask_WaitUntilReceiveGameplayEvent::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    return new CBTTask_WaitUntilReceiveGameplayEvent;
}

void CBTTask_WaitUntilReceiveGameplayEvent::Free()
{
    __super::Free();
}

CBTTask_ActivateScript::CBTTask_ActivateScript()
{
    m_strClassName = "CBTTask_ActivateScript";
}

BT_RESULT_TYPE CBTTask_ActivateScript::ExecuteTask()
{
#ifdef LOAD_UI
    if (true == m_pGameInstance->Start_Script(m_iScriptIndex))
        return BT_RESULT_SUCCESS;
    else
        return BT_RESULT_FAIL;
#else
    return BT_RESULT_SUCCESS;
#endif
}

rapidjson::Value CBTTask_ActivateScript::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    ObjValue.AddMember("m_iScriptIndex", m_iScriptIndex, Alloc);

    return ObjValue;
   
}

HRESULT CBTTask_ActivateScript::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Load_JsonFile(pJsonValue, pTree);

    if (pJsonValue)
    {
        if ((*pJsonValue).HasMember("m_iScriptIndex"))
            m_iScriptIndex = (*pJsonValue)["m_iScriptIndex"].GetInt();
    }

    return S_OK;
}

CBTTask_ActivateScript* CBTTask_ActivateScript::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    return new CBTTask_ActivateScript;
}

void CBTTask_ActivateScript::Free()
{
    __super::Free();
}
