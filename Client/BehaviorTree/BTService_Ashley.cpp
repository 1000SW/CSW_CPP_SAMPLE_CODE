#include "BTService_Ashley.h"
#include "BehaviorTreeHeader.h"

CBTService_Ashley::CBTService_Ashley()
{
    m_strClassName = "CBTService_Ashley";
}

HRESULT CBTService_Ashley::Late_Initialize(void* pArg)
{
    CAshley* pAshley = dynamic_cast<CAshley*>(m_pOwner);
    if (nullptr == pAshley)
        return E_FAIL;

    m_pAshley = pAshley;

    return S_OK;
}

CBTService_UpdateGoalLocation::CBTService_UpdateGoalLocation()
{
    m_strClassName = "CBTService_UpdateGoalLocation";
}

void CBTService_UpdateGoalLocation::ExecuteService()
{
    // Ȱ��ȭ�� �� �� ���� ������ �Ҹ��� �ϱ� ����
    TickService(m_pGameInstance->Get_TimeDelta());
}

void CBTService_UpdateGoalLocation::TickService(_float DeltaSeconds)
{
    _vector vFinalPos;
    if (true == m_bDetermineFromReon)
    {
        CTransform* pReonTransformCom = m_pAshley->Get_Reon()->Get_TransformCom();
        _vector vReonPos = pReonTransformCom->Get_Position();

        _vector vReonRight = m_pAshley->Get_Reon()->Get_TransformCom()->Get_State(CTransform::STATE_RIGHT);
        _vector vReonUp = m_pAshley->Get_Reon()->Get_TransformCom()->Get_State(CTransform::STATE_UP);
        _vector vReonForward = m_pAshley->Get_Reon()->Get_TransformCom()->Get_State(CTransform::STATE_LOOK);

        vFinalPos = vReonPos +
            (vReonRight * m_vOffsetLength.x) +
            (vReonUp * m_vOffsetLength.y) +
            (vReonForward * m_vOffsetLength.z);
        m_pAshley->Get_GoalPoint()->Set_Position(vFinalPos);
    }
    else
    {
        vFinalPos = m_pAshley->Get_TargetPoint();
        m_pAshley->Get_GoalPoint()->Set_Position(vFinalPos);
    }

    _float fabsHeightDiff = fabs(vFinalPos.m128_f32[1] - m_pAshley->Get_TransformCom()->Get_Position().m128_f32[1]);

    _vector vCurrentPos = m_pAshley->Get_TransformCom()->Get_Position();

    /* ����� ������ �ְ� ���� ���̰� �� ���� �ʴ´ٸ� �׺���̼��� �̿��� ��ã�⸦ �������� �ʰ��� */
    if (true == m_pAshley->Get_IsInWalkRange() && fabsHeightDiff < 0.5f)
    {
        m_pAshley->Get_InteractionPoint()->Set_Position(vFinalPos);
    }
    else
    {
        // ��ã�� ��û
        if (m_pAshley->Get_Navigation()->Request_Path_Async(vCurrentPos, vFinalPos))
        {
            _vector vNextPos;
            if (m_pAshley->Get_Navigation()->Get_NextWayPoint(vCurrentPos, vNextPos))
            {
                m_pAshley->Get_InteractionPoint()->Set_Position(vNextPos);
            }
            /* �׺���̼��� ��ȯ�� Location�� ���ٸ� ���� ��ǥ���� ��ġ�� �̵� ��ġ ���� */
            else
            {
                m_pAshley->Get_InteractionPoint()->Set_Position(vFinalPos);
            }
        }
        else
            m_pAshley->Get_InteractionPoint()->Set_Position(vFinalPos);
    }

    /* ����� ������ Ű�� �ִٸ� �Ҹ��� ������ ������(������ ������ �����ߴ��� ���θ� ����) */
    if (false == m_strBlackboardKey.empty())
    {
        if (m_pBehaviorTree->Get_Blackboard()->Has_Variable(m_strBlackboardKey))
            m_pBehaviorTree->Set_BlackboardKey(m_strBlackboardKey, m_pAshley->Get_CollideWithGoalPoint());
    }
}

void CBTService_UpdateGoalLocation::OnServiceFinished(BT_RESULT_TYPE ServiceResult)
{
}

rapidjson::Value CBTService_UpdateGoalLocation::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    ObjValue.AddMember("m_bDetermineFromReon", m_bDetermineFromReon, Alloc);

    if (true == m_bDetermineFromReon)
    {
        ObjValue.AddMember("m_vOffsetLength.x", m_vOffsetLength.x, Alloc);
        ObjValue.AddMember("m_vOffsetLength.y", m_vOffsetLength.y, Alloc);
        ObjValue.AddMember("m_vOffsetLength.z", m_vOffsetLength.z, Alloc);
    }

    ObjValue.AddMember("m_strBlackboardKey", StringRef(m_strBlackboardKey.c_str()), Alloc);

    return ObjValue;
}

HRESULT CBTService_UpdateGoalLocation::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Load_JsonFile(pJsonValue, pTree);

    if (pJsonValue)
    {
        if ((*pJsonValue).HasMember("m_bDetermineFromReon"))
            m_bDetermineFromReon = (*pJsonValue)["m_bDetermineFromReon"].GetBool();

        if (true == m_bDetermineFromReon)
        {
            if ((*pJsonValue).HasMember("m_vOffsetLength.x"))
                m_vOffsetLength.x = (*pJsonValue)["m_vOffsetLength.x"].GetFloat();
            if ((*pJsonValue).HasMember("m_vOffsetLength.y"))
                m_vOffsetLength.y = (*pJsonValue)["m_vOffsetLength.y"].GetFloat();
            if ((*pJsonValue).HasMember("m_vOffsetLength.z"))
                m_vOffsetLength.z = (*pJsonValue)["m_vOffsetLength.z"].GetFloat();
        }
        if ((*pJsonValue).HasMember("m_strBlackboardKey"))
            m_strBlackboardKey = (*pJsonValue)["m_strBlackboardKey"].GetString();
    }

    return S_OK;
}

CBTService_UpdateGoalLocation* CBTService_UpdateGoalLocation::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    return new CBTService_UpdateGoalLocation();
}

void CBTService_UpdateGoalLocation::Free()
{
    __super::Free();
}

CBTService_CheckingConditionInfrontOfTarget::CBTService_CheckingConditionInfrontOfTarget()
{
    m_strClassName = "CBTService_CheckingConditionInfrontOfTarget";

    m_strBlackboardKey = KEY_BB_IS_IN_FRONT_OF_REON;
}

void CBTService_CheckingConditionInfrontOfTarget::ExecuteService()
{
}

void CBTService_CheckingConditionInfrontOfTarget::TickService(_float DeltaSeconds)
{
    CTransform* pReonCom = m_pAshley->Get_Reon()->Get_TransformCom();

    _vector vReonPos = pReonCom->Get_Position();
    _vector vReonLook = pReonCom->Get_State(CTransform::STATE_LOOK);

    _vector vMyPos = m_pOwner->Get_TransformCom()->Get_Position();

    _vector vReon2AshleyDirection = XMVector3Normalize(vMyPos - vReonPos);

    _float fDotResult = XMVectorGetX(XMVector3Dot(vReon2AshleyDirection, vReonLook));
    fDotResult = fmaxf(-1.0f, fminf(fDotResult, 1.0f));
    _float fReon2AshleyRadian = acosf(fDotResult);

    /* ī�޶��� ���⺤�Ϳ� ī�޶�� ���������� ���⺤�Ϳ��� ������ 100������ �۾Ҵٸ� ���鿡 �־��ٰ� �Ǵ� */
    if (XMConvertToRadians(100.f) >= fReon2AshleyRadian)
    {
        if (m_pBlackboard->Has_Variable(m_strBlackboardKey))
            m_pBlackboard->Set_Variable(m_strBlackboardKey, true);
    }
    else
    {
        if (m_pBlackboard->Has_Variable(m_strBlackboardKey))
            m_pBlackboard->Set_Variable(m_strBlackboardKey, false);
    }
}

void CBTService_CheckingConditionInfrontOfTarget::OnServiceFinished(BT_RESULT_TYPE ServiceResult)
{
}

CBTService_CheckingConditionInfrontOfTarget* CBTService_CheckingConditionInfrontOfTarget::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    return new CBTService_CheckingConditionInfrontOfTarget();
}

void CBTService_CheckingConditionInfrontOfTarget::Free()
{
    __super::Free();
}

CBTService_UpdateCrouchCondition::CBTService_UpdateCrouchCondition()
{
    m_strClassName = "CBTService_UpdateCrouchCondition";

    m_fTickInterval = 2.5f;
}

void CBTService_UpdateCrouchCondition::ExecuteService()
{
}

void CBTService_UpdateCrouchCondition::TickService(_float DeltaSeconds)
{
    if (false == m_pBehaviorTree->Get_Blackboard()->Has_Variable(KEY_BB_CROUCHING))
        return;

    _bool bCrouchState = m_pBehaviorTree->Get_Blackboard()->Get_Variable<_bool>(KEY_BB_CROUCHING);
    _bool bReonCrouchState = m_pAshley->Get_Reon()->Get_AbilitySystemComponent()->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_CROUCHING));

    m_pBehaviorTree->Set_BlackboardKey(KEY_BB_ON_CHANGED_CROUCH,
        bool(bCrouchState ^ bReonCrouchState));
}

void CBTService_UpdateCrouchCondition::OnServiceFinished(BT_RESULT_TYPE ServiceResult)
{
}

CBTService_UpdateCrouchCondition* CBTService_UpdateCrouchCondition::Create(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    return new CBTService_UpdateCrouchCondition();
}

void CBTService_UpdateCrouchCondition::Free()
{
    __super::Free();
}
