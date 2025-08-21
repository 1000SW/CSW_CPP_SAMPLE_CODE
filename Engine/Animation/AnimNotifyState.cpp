#include "AnimNotifyState.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "CompositeArmatrue.h"
#include "Animator.h"
#include "HealthObject.h"
#include "CharacterController.h"

using namespace rapidjson;

CAnimNotifyState::CAnimNotifyState()
	: m_pGameInstance{ CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
}

CAnimNotifyState::CAnimNotifyState(const CAnimNotifyState& Prototype)
	: m_pGameInstance{ Prototype.m_pGameInstance }
	, m_fTriggerTime { Prototype.m_fTriggerTime}
	, m_fEndTime{ Prototype.m_fEndTime }
	, m_strNotifyStateName{ Prototype.m_strNotifyStateName }
    , m_pOwner { Prototype.m_pOwner}
    , m_pArmature { Prototype.m_pArmature }
{
	Safe_AddRef(m_pGameInstance);
}

HRESULT CAnimNotifyState::Initialize_Prototype(const _string& strNotifyStateName, rapidjson::Value* pJsonValue)
{
	m_strNotifyStateName = strNotifyStateName;
	if (pJsonValue)
	{
		m_fTriggerTime = (*pJsonValue)["m_fTriggerTime"].GetFloat();
		m_fEndTime = (*pJsonValue)["m_fEndTime"].GetFloat();
	}
	else
	{
		m_fEndTime = 100.f;
	}

	return S_OK;
}

void CAnimNotifyState::Late_Initialize(CGameObject* pOwner)
{
    if (pOwner)
        m_pOwner = pOwner;
}

void CAnimNotifyState::Update(_float fCurrentTrackPos, _float fTimeDelta)
{
	if (!m_bTriggered && fCurrentTrackPos <= m_fEndTime && fCurrentTrackPos >= m_fTriggerTime)
	{
		m_bTriggered = true;
		if (m_fnBegin)
			m_fnBegin(fCurrentTrackPos);
	}
	else if (m_bTriggered && fCurrentTrackPos < m_fEndTime)
	{
		if (m_fnTick)
			m_fnTick(fTimeDelta, fCurrentTrackPos);
	}
	else if (m_bTriggered && fCurrentTrackPos >= m_fEndTime)
	{
		if (m_fnEnd)
			m_fnEnd(fCurrentTrackPos);

		m_bTriggered = false;
	}
}

void CAnimNotifyState::Reset(_float fCurrentTrackPos)
{
	if (m_bTriggered && fCurrentTrackPos >= m_fTriggerTime && m_fnEnd)
		m_fnEnd(fCurrentTrackPos);

	m_bTriggered = false;
}

rapidjson::Value CAnimNotifyState::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
	Value NotifyStateValue(kObjectType);

	NotifyStateValue.AddMember("m_strNotifyStateName", StringRef(m_strNotifyStateName.c_str()), Alloc);
	NotifyStateValue.AddMember("m_fTriggerTime", m_fTriggerTime, Alloc);
	NotifyStateValue.AddMember("m_fEndTime", m_fEndTime, Alloc);

	return NotifyStateValue;
}

_float CAnimNotifyState::Get_NotifyProgress_Ratio(_float fTrackPosition)
{
    _float fDuration = max(0.0001f, m_fEndTime - m_fTriggerTime);

    // 현재 트랙 위치가 노티파이 시작 시간보다 빠를 경우 0을 반환 (아직 시작 전)
    if (fTrackPosition < m_fTriggerTime)
        return 0.0f;

    // 현재 트랙 위치가 노티파이 종료 시간보다 늦을 경우 1을 반환 (이미 종료)
    if (fTrackPosition > m_fEndTime)
        return 1.0f;

    return (fTrackPosition - m_fTriggerTime) / fDuration;
}

_float CAnimNotifyState::Get_NotifyDuration()
{
    return max(0.0001f, m_fEndTime - m_fTriggerTime);
}

void CAnimNotifyState::Free()
{
	__super::Free();

	Safe_Release(m_pGameInstance);
}

CAnimNotifyState_Generic::CAnimNotifyState_Generic()
	: CAnimNotifyState()
{
}

CAnimNotifyState_Generic::CAnimNotifyState_Generic(const CAnimNotifyState_Generic& Prototype)
	:CAnimNotifyState(Prototype)
	, m_vSaveMovePos(Prototype.m_vSaveMovePos)
	, m_vMovePos(Prototype.m_vMovePos)
	, m_vAdjustDeltaDir(Prototype.m_vAdjustDeltaDir)
	, m_fPrevTrackPosition(Prototype.m_fPrevTrackPosition)
	, m_vSaveDirection(Prototype.m_vSaveDirection)
	, m_vTargetOffset(Prototype.m_vTargetOffset)
	, m_vFrameDirection(Prototype.m_vFrameDirection)
	, m_fPrevTrackPos(Prototype.m_fPrevTrackPos)
{
}

void CAnimNotifyState_Generic::Late_Initialize(CGameObject* pOwner)
{
    __super::Late_Initialize(pOwner);

    m_pComposite = m_pOwner->Get_Component<CCompositeArmature>();

    if (nullptr == m_pComposite)
        return;

    Bind_Function();
}

HRESULT CAnimNotifyState_Generic::Initialize_Prototype(const _string& strNotifyStateName, rapidjson::Value* pJsonValue)
{
    __super::Initialize_Prototype(strNotifyStateName, pJsonValue);

    if (pJsonValue)
    {
        if (m_strNotifyStateName == "ANS_MovePosLerp")
        {
            if ((*pJsonValue).HasMember("m_vSaveMovePos"))
            {
                Value& ArrValue = (*pJsonValue)["m_vSaveMovePos"];

                m_vSaveMovePos.x = ArrValue[0].GetFloat();
                m_vSaveMovePos.y = ArrValue[1].GetFloat();
                m_vSaveMovePos.z = ArrValue[2].GetFloat();
                
                m_vMovePos = XMLoadFloat3(&m_vSaveMovePos);
                m_vMovePos = XMVectorSetW(m_vMovePos, 1.f);
            }
        }
        else if (m_strNotifyStateName == "ANS_MoveRelativeDirection")
        {
            if ((*pJsonValue).HasMember("m_vSaveDirection"))
            {
                Value& ArrValue = (*pJsonValue)["m_vSaveDirection"];

                m_vSaveDirection.x = ArrValue[0].GetFloat();
                m_vSaveDirection.y = ArrValue[1].GetFloat();
                m_vSaveDirection.z = ArrValue[2].GetFloat();

                m_vTargetOffset = XMLoadFloat3(&m_vSaveDirection);
                m_vTargetOffset = XMVectorSetW(m_vTargetOffset, 1.f);
            }
        }
        else if (m_strNotifyStateName == "ANS_PlaySound")
        {
            if ((*pJsonValue).HasMember("m_strSoundTag"))
            {
                m_strSoundTag = (*pJsonValue)["m_strSoundTag"].GetString();
            }
            if ((*pJsonValue).HasMember("m_fActivationRate"))
            {
                m_fActivationRate = (*pJsonValue)["m_fActivationRate"].GetFloat();
            }
            if ((*pJsonValue).HasMember("m_bFadeOut"))
            {
                m_bFadeOut = (*pJsonValue)["m_bFadeOut"].GetBool();
            }
        }
    }
    
    return S_OK;
}

rapidjson::Value CAnimNotifyState_Generic::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
	Value NotifyStateValue(kObjectType);

	NotifyStateValue = __super::Save_JsonFile(Alloc);
	NotifyStateValue.AddMember("m_strNotifyStateClassName", "CAnimNotifyState_Generic", Alloc);

    if (m_strNotifyStateName == "ANS_MovePosLerp")
    {
        Value ArrayValue(kArrayType);

        ArrayValue.PushBack(m_vSaveMovePos.x, Alloc);
        ArrayValue.PushBack(m_vSaveMovePos.y, Alloc);
        ArrayValue.PushBack(m_vSaveMovePos.z, Alloc);

        NotifyStateValue.AddMember("m_vSaveMovePos", ArrayValue, Alloc);
    }
    else if (m_strNotifyStateName == "ANS_MoveRelativeDirection")
    {
        Value ArrayValue(kArrayType);

        ArrayValue.PushBack(m_vSaveDirection.x, Alloc);
        ArrayValue.PushBack(m_vSaveDirection.y, Alloc);
        ArrayValue.PushBack(m_vSaveDirection.z, Alloc);

        NotifyStateValue.AddMember("m_vSaveDirection", ArrayValue, Alloc);
    }
    else if (m_strNotifyStateName == "ANS_PlaySound")
    {
        NotifyStateValue.AddMember("m_strSoundTag", StringRef(m_strSoundTag.c_str()), Alloc);
        NotifyStateValue.AddMember("m_fActivationRate", m_fActivationRate, Alloc);
        NotifyStateValue.AddMember("m_bFadeOut", m_bFadeOut, Alloc);
    }

	return NotifyStateValue;
}

HRESULT CAnimNotifyState_Generic::Bind_Function()
{
    if (m_strNotifyStateName == "ANS_Root_FlipZ")
    {
        m_fnBegin = [&](_float fTrackPosition)
            {
                m_pArmature->Set_RootTranslationFlipZ(true);
            };

        m_fnEnd = [&](_float fTrackPosition)
            {
                m_pArmature->Set_RootTranslationFlipZ(false);
            };
    }
    else if (m_strNotifyStateName == "ANS_Root_FlipX")
    {
        m_fnBegin = [&](_float fTrackPosition)
            {
                m_pArmature->Set_RootTranslationFlipX(true);
            };

        m_fnEnd = [&](_float fTrackPosition)
            {
                m_pArmature->Set_RootTranslationFlipX(false);
            };
    }
    else if (m_strNotifyStateName == "ANS_Disable_Gravity")
    {
        if (nullptr == m_pOwner->Get_RigidBody())
            return E_FAIL;

        m_fnBegin = [&](_float fTrackPosition)
            {
                m_pOwner->Get_RigidBody()->Set_Gravity(false);
            };

        m_fnEnd = [&](_float fTrackPosition)
            {
                m_pOwner->Get_RigidBody()->Set_Gravity(true);
            };
    }
    else if (m_strNotifyStateName == "ANS_AnimationChangable")
    {
        m_fnBegin = [&](_float fTrackPosition)
            {
                m_pAnimator->Set_AnimationChangable(false);
            };
        m_fnEnd = [&](_float fTrackPosition)
            {
                m_pAnimator->Set_AnimationChangable(true);
            };
    }
    else if (m_strNotifyStateName == "ANS_SkipAnimation")
    {
        m_fnBegin = [&](_float fTrackPosition)
            {
                m_pAnimator->Set_CurrentTime(m_fEndTime);
            };
    }
    else if (m_strNotifyStateName == "ANS_ApplyRootVelocity")
    {
        m_fnBegin = [&](_float fTrackPosition)
            {
                m_pArmature->Set_ApplyRootBoneTranslationVelocity(true);
            };
        m_fnEnd = [&](_float fTrackPosition)
            {
                m_pArmature->Set_ApplyRootBoneTranslationVelocity(false);
                CRigidBody* pRigid = m_pOwner->Get_RigidBody();
                if (pRigid)
                {
                    pRigid->Set_Velocity({ 0, 0, 0, 0 });
                }
            };
    }
    else if (m_strNotifyStateName == "ANS_MovePosLerp")
    {
        m_fnBegin = [&](_float fTrackPosition)
            {
                m_vAdjustDeltaDir = m_vMovePos / Get_NotifyDuration();

                m_fPrevTrackPosition = m_fTriggerTime;
            };


        m_fnTick = [&](_float fTimeDelta, _float fTrackPosition)
            {
                _float fDeltaTrack = fTrackPosition - m_fPrevTrackPosition;

                _vector vFrameMove = m_vAdjustDeltaDir * fDeltaTrack;

                m_pOwner->Get_TransformCom()->Add_Pos(vFrameMove);

                m_fPrevTrackPosition = fTrackPosition;
            };
    }
    else if (m_strNotifyStateName == "ANS_MoveRelativeDirection")
    {
        m_fnBegin = [&](_float fTrackPosition)
            {
                if (!m_pOwner || !m_pOwner->Get_TransformCom())
                    return;

                _matrix matRot = XMMatrixRotationQuaternion(m_pOwner->Get_TransformCom()->Get_Rotation());
                
                _vector vWorldOffsetDirection = XMVector3TransformNormal(m_vTargetOffset, matRot);

                _vector vCurrentOwnerPosition = m_pOwner->Get_TransformCom()->Get_State(CTransform::STATE_POSITION);

                m_vFrameDirection = vWorldOffsetDirection / Get_NotifyDuration();
                m_fPrevTrackPos = m_fTriggerTime;
            };

        m_fnTick = [&](_float fTimeDelta, _float fTrackPosition)
            {
                _float fDeltaTrack = fTrackPosition - m_fPrevTrackPos;

                _vector vFrameMove = m_vFrameDirection * fDeltaTrack;

                m_pOwner->Get_TransformCom()->Add_Pos(vFrameMove);

                m_fPrevTrackPos = fTrackPosition;
            };
    }
    else if (m_strNotifyStateName == "ANS_PlaySound")
    {
        m_fnBegin = [&](_float fTrackPosition)
            {
                m_pOwner->PlaySoundEvent(m_strSoundTag, false, false, false, _float4x4{});

            };

        m_fnEnd = [&](_float fTrackPosition)
            {
                m_pOwner->StopSoundEvent(m_strSoundTag, m_bFadeOut);
            };
    }

    return S_OK;
}

CAnimNotifyState* CAnimNotifyState_Generic::Create(const _string& strNotifyStateName, rapidjson::Value* pJsonValue)
{
	CAnimNotifyState_Generic* pInstance = new CAnimNotifyState_Generic();

	if (FAILED(pInstance->Initialize_Prototype(strNotifyStateName, pJsonValue)))
	{
		MSG_BOX("Failed To Created : CAnimNotifyState_Generic");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CAnimNotifyState* CAnimNotifyState_Generic::Clone(void* pArg)
{
	return new CAnimNotifyState_Generic(*this);
}

void CAnimNotifyState_Generic::Free()
{
	__super::Free();
}

CAnimNotifyState_HealthObject::CAnimNotifyState_HealthObject()
    :CAnimNotifyState()
{
}

CAnimNotifyState_HealthObject::CAnimNotifyState_HealthObject(const CAnimNotifyState_HealthObject& Prototype)
    : CAnimNotifyState(Prototype)
    , m_bFixRootBoneTranslation(Prototype.m_bFixRootBoneTranslation)
    , m_vStartPoint(Prototype.m_vStartPoint)
    , m_vTargetPoint(Prototype.m_vTargetPoint)
    , m_vStartQuaternion(Prototype.m_vStartQuaternion)
    , m_vTargetQuaternion(Prototype.m_vTargetQuaternion)
    , m_fDistanceOffset(Prototype.m_fDistanceOffset)
    , m_vAdjustDeltaDir(Prototype.m_vAdjustDeltaDir)
    , m_vRootmotionFinalPoint_Store(Prototype.m_vRootmotionFinalPoint_Store)
    , m_vRootMotionFinalPoint(Prototype.m_vRootMotionFinalPoint)
    , m_fPrevTrackPosition(Prototype.m_fPrevTrackPosition)
    , m_fDuration(Prototype.m_fDuration)
    , m_bApplyCDF(Prototype.m_bApplyCDF)
    , m_bFaceToFace(Prototype.m_bFaceToFace)
    , m_vSaveOffsetPos(Prototype.m_vSaveOffsetPos)
    , m_fYaw(Prototype.m_fYaw)
    , m_vOffsetPos(Prototype.m_vOffsetPos)
    , m_vTransformOffsetPos(Prototype.m_vTransformOffsetPos)
    , m_fLerpTime(Prototype.m_fLerpTime)
    , m_fAccTime(Prototype.m_fAccTime)
    , m_vStartPos(Prototype.m_vStartPos)
    , m_vDstPos(Prototype.m_vDstPos)
    , m_vMoveSpeed(Prototype.m_vMoveSpeed)
    , m_vStartQuat(Prototype.m_vStartQuat)
    , m_vDstQuat(Prototype.m_vDstQuat)
    , m_vRotSpeed(Prototype.m_vRotSpeed)
{
}

HRESULT CAnimNotifyState_HealthObject::Initialize_Prototype(const _string& strNotifyStateName, rapidjson::Value* pJsonValue)
{
    __super::Initialize_Prototype(strNotifyStateName, pJsonValue);

    if (pJsonValue)
    {
        if (m_strNotifyStateName == "ANS_HealthObj_MoveToTarget" || m_strNotifyStateName == "ANS_HealthObj_MotionWarping")
        {
            m_fDistanceOffset = (*pJsonValue)["m_fDistanceOffset"].GetFloat();
        }

        if (m_strNotifyStateName == "ANS_HealthObj_RootMotion2TargetPoint")
        {
            if ((*pJsonValue).HasMember("m_vRootmotionFinalPoint_Store"))
            {
                Value& ArrValue = (*pJsonValue)["m_vRootmotionFinalPoint_Store"];

                m_vRootmotionFinalPoint_Store.x = ArrValue[0].GetFloat();
                m_vRootmotionFinalPoint_Store.y = ArrValue[1].GetFloat();
                m_vRootmotionFinalPoint_Store.z = ArrValue[2].GetFloat();
            }
            if ((*pJsonValue).HasMember("m_bApplyCDF"))
            {
                m_bApplyCDF = (*pJsonValue)["m_bApplyCDF"].GetBool();
            }
        }
        
        else if ((*pJsonValue).HasMember("ANS_HealthObj_MotionWarping") || (*pJsonValue).HasMember("ANS_HealthObj_MoveToTarget"))
        {
            if ((*pJsonValue).HasMember("m_fDistanceOffset"))
            {
                m_fDistanceOffset = (*pJsonValue)["m_fDistanceOffset"].GetFloat();
            }
        }
        else if (m_strNotifyStateName == "ANS_HealthObj_JointAttack")
        {
            if ((*pJsonValue).HasMember("m_vSaveOffsetPos"))
            {
                Value& ArrValue = (*pJsonValue)["m_vSaveOffsetPos"];

                m_vSaveOffsetPos.x = ArrValue[0].GetFloat();
                m_vSaveOffsetPos.y = ArrValue[1].GetFloat();
                m_vSaveOffsetPos.z = ArrValue[2].GetFloat();

                m_vOffsetPos = XMLoadFloat3(&m_vSaveOffsetPos);
                m_vOffsetPos = XMVectorSetW(m_vOffsetPos, 1.f);
            }

            if ((*pJsonValue).HasMember("m_fYaw"))
                m_fYaw = XMConvertToRadians((*pJsonValue)["m_fYaw"].GetFloat());
        }

    }
    return S_OK;
}

void CAnimNotifyState_HealthObject::Late_Initialize(CGameObject* pOwner)
{
    __super::Late_Initialize(pOwner);

    m_pComposite = m_pOwner->Get_Component<CCompositeArmature>();

    if (nullptr == m_pComposite)
        return;

    m_pHealthObject = dynamic_cast<CHealthObject*>(m_pOwner);

    if (nullptr == m_pHealthObject)
        return;

    if (nullptr == m_pAnimator)
        return;

    m_pCCT = m_pHealthObject->Get_Component<CCharacterController>();

    Bind_Function();
}

rapidjson::Value CAnimNotifyState_HealthObject::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    Value NotifyStateValue(kObjectType);

    NotifyStateValue = __super::Save_JsonFile(Alloc);
    NotifyStateValue.AddMember("m_strNotifyStateClassName", "CAnimNotifyState_HealthObject", Alloc);

    if (m_strNotifyStateName == "ANS_HealthObj_MoveToTarget" || m_strNotifyStateName == "ANS_HealthObj_MotionWarping")
    {
        NotifyStateValue.AddMember("m_fDistanceOffset", m_fDistanceOffset, Alloc);
    }
    else if (m_strNotifyStateName == "ANS_HealthObj_RootMotion2TargetPoint")
    {
        Value ArrayValue(kArrayType);

        ArrayValue.PushBack(m_vRootmotionFinalPoint_Store.x, Alloc);
        ArrayValue.PushBack(m_vRootmotionFinalPoint_Store.y, Alloc);
        ArrayValue.PushBack(m_vRootmotionFinalPoint_Store.z, Alloc);
        NotifyStateValue.AddMember("m_vRootmotionFinalPoint_Store", ArrayValue, Alloc);

        NotifyStateValue.AddMember("m_bApplyCDF", m_bApplyCDF, Alloc);
    }
    else if (m_strNotifyStateName == "ANS_HealthObj_JointAttack")
    {
        Value ArrayValue(kArrayType);

        ArrayValue.PushBack(m_vSaveOffsetPos.x, Alloc);
        ArrayValue.PushBack(m_vSaveOffsetPos.y, Alloc);
        ArrayValue.PushBack(m_vSaveOffsetPos.z, Alloc);

        NotifyStateValue.AddMember("m_vSaveOffsetPos", ArrayValue, Alloc);
        NotifyStateValue.AddMember("m_fYaw", XMConvertToDegrees(m_fYaw), Alloc);
    }

    return NotifyStateValue;
}

_float CAnimNotifyState_HealthObject::ultraSimpleCDF(_float x)
{
    return 0.5 + 0.5 * tanh(0.7978845608 * (x + 0.044715 * x * x * x));
}

HRESULT CAnimNotifyState_HealthObject::Bind_Function()
{
    if (m_strNotifyStateName == "ANS_HealthObj_Invoke_AttackBehavior")
    {
        m_fnBegin = [&](_float fTrackPosition)
            {
                m_pHealthObject->Invoke_AttackBehavior(true, m_pAnimator->Get_CurrentAnimIndex(), m_pAnimator);
            };

        m_fnEnd = [&](_float fTrackPosition)
            {
                m_pHealthObject->Invoke_AttackBehavior(false, m_pAnimator->Get_CurrentAnimIndex(), m_pAnimator);
            };
    }
    else if (m_strNotifyStateName == "ANS_HealthObj_MotionWarping")
    {
        // Begin
        m_fnBegin = [&](_float fTrackPosition)
            {
                if (nullptr == m_pHealthObject->Get_Target())
                    return;

                m_vStartPoint = m_pHealthObject->Get_TransformCom()->Get_Position();

                _vector vToTarget = m_pHealthObject->Get_Target()->Get_TransformCom()->Get_Position() - m_vStartPoint;

                if (XMVector3Equal(vToTarget, XMVectorZero()))
                    vToTarget = XMVectorSet(0.f, 0.f, 1.f, 0.f);
                else
                    vToTarget = XMVector3Normalize(vToTarget);

                m_vTargetPoint = m_pHealthObject->Get_Target()->Get_TransformCom()->Get_Position() - vToTarget * m_fDistanceOffset;
                m_vTargetPoint = XMVectorSetW(m_vTargetPoint, 1.f);

                // XZ 평면에서 방향만 고려
                vToTarget = XMVectorSet(XMVectorGetX(vToTarget), 0.f, XMVectorGetZ(vToTarget), 0.f);

                // 회전 정보 저장
                m_vStartQuaternion = m_pHealthObject->Get_TransformCom()->Get_Rotation();

                // Y축 기준 Yaw 회전 쿼터니언 계산
                _vector vForward = XMVectorSet(0.f, 0.f, 1.f, 0.f); // 기본 전방 벡터
                _float fDot = XMVectorGetX(XMVector3Dot(vForward, vToTarget));
                fDot = clamp(fDot, -1.f, 1.f); // acosf 안정성

                _float fYaw = acosf(fDot);

                _vector vCross = XMVector3Cross(vForward, vToTarget);
                if (XMVectorGetY(vCross) < 0.f)
                    fYaw *= -1.f;

                m_vTargetQuaternion = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), fYaw);
            };

        // Tick
        m_fnTick = [&](_float fTimeDelta, _float fTrackPosition)
            {
                if (nullptr == m_pHealthObject->Get_Target())
                    return;

                _float fRatio = (fTrackPosition - m_fTriggerTime) / max(m_fEndTime - m_fTriggerTime, 0.0001f);
                fRatio = clamp(fRatio, 0.f, 1.f);

                // 위치 보간
                _vector vNewPos = XMVectorLerp(m_vStartPoint, m_vTargetPoint, fRatio);
                m_pHealthObject->Get_TransformCom()->Set_State_Notify(CTransform::STATE_POSITION, XMVectorSetW(vNewPos, 1.f));

                // 회전 보간
                _vector qNewRot = XMQuaternionSlerp(m_vStartQuaternion, m_vTargetQuaternion, fRatio);
                qNewRot = XMQuaternionNormalize(qNewRot);
                m_pHealthObject->Get_TransformCom()->Rotation(qNewRot);
            };

        m_fnEnd = [&](_float fTrackPosition)
            {
                if (nullptr == m_pHealthObject->Get_Target())
                    return;

                m_pArmature->Set_FixRootBoneTranslation(false);
            };
    }
    else if (m_strNotifyStateName == "ANS_HealthObj_MoveToTarget")
    {
        m_fnBegin = [&](_float fTrackPosition)
            {
                if (nullptr == m_pHealthObject->Get_Target())
                    return;

                m_vStartPoint = m_pHealthObject->Get_TransformCom()->Get_Position();

                // 헬스 오브젝트 → 타겟 방향
                _vector vToTarget =  m_pHealthObject->Get_Target()->Get_TransformCom()->Get_Position() - m_vStartPoint;

                vToTarget = XMVector3Normalize(vToTarget);

                m_vTargetPoint = m_pHealthObject->Get_Target()->Get_TransformCom()->Get_Position()
                    - vToTarget * m_fDistanceOffset;

                m_vTargetPoint = XMVectorSetW(m_vTargetPoint, 1.f);
            };

        m_fnTick = [&](_float fTimeDelta, _float fTrackPosition)
            {
                if (nullptr == m_pHealthObject->Get_Target())
                    return;

                _float fDuration = m_fEndTime - m_fTriggerTime;
                if (fDuration <= 0.0001f) fDuration = 0.0001f;

                _float fRatio = (fTrackPosition - m_fTriggerTime) / fDuration;
                fRatio = clamp(fRatio, 0.0f, 1.0f);

                _vector vNewPos = XMVectorLerp(m_vStartPoint, m_vTargetPoint, fRatio);
                m_pHealthObject->Get_TransformCom()->Set_State_Notify(CTransform::STATE_POSITION, XMVectorSetW(vNewPos, 1.f));
            };

        m_fnEnd = [&](_float fTrackPosition)
            {
                if (nullptr == m_pHealthObject->Get_Target())
                    return;
                m_pHealthObject->Get_TransformCom()->Set_State_Notify(CTransform::STATE_POSITION, m_vTargetPoint);
            };
    }

    else if (m_strNotifyStateName == "ANS_HealthObj_RootMotion2TargetPoint")
    {
        m_fnBegin = [&](_float fTrackPosition)
            {
                m_vTargetPoint = m_pHealthObject->Get_TargetPoint();

                m_vRootMotionFinalPoint = XMLoadFloat4(&m_vRootmotionFinalPoint_Store);
                m_vRootMotionFinalPoint = XMVector3TransformCoord(m_vRootMotionFinalPoint, m_pHealthObject->Get_TransformCom()->Get_WorldMatrix());

                _vector vDiffDir = m_vTargetPoint - m_vRootMotionFinalPoint;

                m_fDuration = m_fEndTime - m_fTriggerTime;
                m_fDuration = max(m_fEndTime - m_fTriggerTime, 0.0001f);

                m_vAdjustDeltaDir = vDiffDir / m_fDuration;

                m_fPrevTrackPosition = m_fTriggerTime;
            };

        m_fnTick = [&](_float fTimeDelta, _float fTrackPosition)
            {
                if (m_bApplyCDF)
                {
                    _float fProgress =  m_pArmature->Get_AnimationProgress();
                    _float fCDF = ultraSimpleCDF(fProgress);

                    m_pHealthObject->Get_TransformCom()->Add_Pos(fCDF* m_vAdjustDeltaDir);
                }
                else
                {
                    _float fDeltaTrack = fTrackPosition - m_fPrevTrackPosition;

                    // 현재 위치에 더하기
                    m_pHealthObject->Get_TransformCom()->Add_Pos(fDeltaTrack* m_vAdjustDeltaDir);

                    m_fPrevTrackPosition = fTrackPosition;
                }
            };

        m_fnEnd = [&](_float fTrackPosition)
            {
            };
    }
    else if (m_strNotifyStateName == "ANS_HealthObj_JointAttack")
    {
        if (nullptr == m_pCCT)
            return E_FAIL;

        m_fnBegin = [&](_float fTrackPosition) {
            if (CGameObject* pTarget = m_pHealthObject->Get_Target())
            {
                m_fAccTime = 0.f;
                m_vStartPos = m_pHealthObject->Get_TransformCom()->Get_Position();
                m_vStartQuat = m_pHealthObject->Get_TransformCom()->Get_Rotation();

                _vector vTargetPoint = pTarget->Get_TransformCom()->Get_Position();
                m_vTargetPoint = vTargetPoint;

                _vector vTargetLook = pTarget->Get_TransformCom()->Get_State(CTransform::STATE_LOOK);
                vTargetLook = XMVector3Normalize(vTargetLook);

                _vector vOffsetLocal = m_vOffsetPos;

                // 타겟의 방향 벡터 얻기
                _vector vLook = pTarget->Get_TransformCom()->Get_State(CTransform::STATE_LOOK);  // z방향
                _vector vRight = pTarget->Get_TransformCom()->Get_State(CTransform::STATE_RIGHT); // x방향
                _vector vUp = pTarget->Get_TransformCom()->Get_State(CTransform::STATE_UP); // y방향

                vLook = XMVector3Normalize(vLook);
                vRight = XMVector3Normalize(vRight);
                vUp = XMVector3Normalize(vUp);

                // 오프셋을 방향 벡터 기준으로 직접 조립
                _vector vRotatedOffset =
                    vRight * XMVectorGetX(vOffsetLocal) +
                    vUp * XMVectorGetY(vOffsetLocal) +
                    vLook * XMVectorGetZ(vOffsetLocal);

                // 최종 위치
                _vector vTargetPos = pTarget->Get_TransformCom()->Get_Position();
                m_vDstPos = vTargetPos + vRotatedOffset;

                //m_pHealthObject->Get_TransformCom()->Set_State_Notify(CTransform::STATE_POSITION, m_vDstPos);
                _vector vDir = vTargetPoint - m_vDstPos;
                vDir = XMVectorSetY(vDir, 0.f);
                vDir = XMVector3Normalize(vDir);

                _matrix matYaw = XMMatrixRotationAxis(XMVectorSet(0, 1, 0, 0), m_fYaw);
                _vector vYawedDir = XMVector3TransformNormal(vDir, matYaw);
                //m_pHealthObject->Get_TransformCom()->Rotation_Dir(vDir);
                m_pHealthObject->Get_TransformCom()->Rotation_Dir(vYawedDir);
                m_pHealthObject->Get_TransformCom()->Add_Rotation(XMVectorSet(0, 1, 0, 0), m_fYaw);

                //이동 속도 설정
                m_fAccTime = 0.f;
                m_vMoveSpeed = (m_vDstPos - m_vStartPos) / m_fLerpTime;

                //m_fAccTime = 0.f;
                //m_vStartPos = m_pHealthObject->Get_TransformCom()->Get_Position();
                //m_vStartQuat = m_pHealthObject->Get_TransformCom()->Get_Rotation();

                //_vector vTargetPoint = pTarget->Get_TransformCom()->Get_Position();
                //m_vTargetPoint = vTargetPoint;

                //// 먼저 회전 방향 결정 및 적용
                //_vector vDir = vTargetPoint - m_vStartPos;
                //vDir = XMVectorSetY(vDir, 0.f);
                //vDir = XMVector3Normalize(vDir);

                //m_pHealthObject->Get_TransformCom()->Rotation_Dir(vDir);
                //m_pHealthObject->Get_TransformCom()->Add_Rotation(XMVectorSet(0, 1, 0, 0), m_fYaw);

                //// 이 시점에 방향 벡터를 다시 받아야 Yaw가 반영됨
                //_vector vLook = m_pHealthObject->Get_TransformCom()->Get_State(CTransform::STATE_LOOK);
                //_vector vRight = m_pHealthObject->Get_TransformCom()->Get_State(CTransform::STATE_RIGHT);
                //_vector vUp = m_pHealthObject->Get_TransformCom()->Get_State(CTransform::STATE_UP);

                //vLook = XMVector3Normalize(vLook);
                //vRight = XMVector3Normalize(vRight);
                //vUp = XMVector3Normalize(vUp);

                //// 오프셋 조립 (Yaw 적용 이후의 로컬축 기준)
                //_vector vOffsetLocal = m_vOffsetPos;
                //_vector vRotatedOffset =
                //    vRight * XMVectorGetX(vOffsetLocal) +
                //    vUp * XMVectorGetY(vOffsetLocal) +
                //    vLook * XMVectorGetZ(vOffsetLocal);

                //// 최종 위치
                //_vector vTargetPos = vTargetPoint;
                //m_vDstPos = vTargetPos + vRotatedOffset;

                //m_pHealthObject->Get_TransformCom()->Set_State_Notify(CTransform::STATE_POSITION, m_vDstPos);


            }
            };

        m_fnTick = [&](_float fTimeDelta, _float fTrackPosition) {
            if (m_fAccTime <= m_fLerpTime)
            {
                m_pHealthObject->Get_TransformCom()->Add_Pos(m_vMoveSpeed * fTimeDelta);

                m_fAccTime += fTimeDelta;
            }

            };

        m_fnEnd = [&](_float fTrackPosition) {
            };
    }

    else if (m_strNotifyStateName == "ANS_HealthObj_RotateToTarget")
    {
        m_fnTick = [&](_float fTimeDelta, _float fTrackPosition) {
            if (m_fAccTime <= m_fLerpTime)
            {
                if (m_pHealthObject->Get_Target())
                {
                    m_pHealthObject->Get_TransformCom()->Rotation_Dir_AtPoint(m_pHealthObject->Get_Target()->Get_TransformCom()->Get_Position(), fTimeDelta);
                }
            }

            };

    }



	return S_OK;
}

CAnimNotifyState* CAnimNotifyState_HealthObject::Create(const _string& strNotifyStateName, rapidjson::Value* pJsonValue)
{
    CAnimNotifyState_HealthObject* pInstance = new CAnimNotifyState_HealthObject();

    if (FAILED(pInstance->Initialize_Prototype(strNotifyStateName, pJsonValue)))
    {
        MSG_BOX("Failed To Created : CAnimNotifyState_HealthObject");
        Safe_Release(pInstance);
    }

    return pInstance;

}

CAnimNotifyState* CAnimNotifyState_HealthObject::Clone(void* pArg)
{
    return new CAnimNotifyState_HealthObject(*this);
}

void CAnimNotifyState_HealthObject::Free()
{
    __super::Free();
}

CAnimNotifyState_CharacterController::CAnimNotifyState_CharacterController()
    :CAnimNotifyState()
{
    Safe_AddRef(m_pGameInstance);
}

CAnimNotifyState_CharacterController::CAnimNotifyState_CharacterController(const CAnimNotifyState_CharacterController& Prototype)
    : CAnimNotifyState(Prototype)
    , m_bApplyGravity(Prototype.m_bApplyGravity)
{
}

HRESULT CAnimNotifyState_CharacterController::Initialize_Prototype(const _string& strNotifyStateName, rapidjson::Value* pJsonValue)
{
    __super::Initialize_Prototype(strNotifyStateName, pJsonValue);

    return S_OK;
}

void CAnimNotifyState_CharacterController::Late_Initialize(CGameObject* pOwner)
{
    __super::Late_Initialize(pOwner);

    m_pComposite = m_pOwner->Get_Component<CCompositeArmature>();

    if (nullptr == m_pComposite)
        return;

    m_pCCT = m_pOwner->Get_Component<CCharacterController>();

    if (nullptr == m_pCCT)
        return;

    if (nullptr == m_pAnimator)
        return;

    Bind_Function();
}

rapidjson::Value CAnimNotifyState_CharacterController::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    Value NotifyStateValue = __super::Save_JsonFile(Alloc);
    NotifyStateValue.AddMember("m_strNotifyStateClassName", "CAnimNotifyState_CharacterController", Alloc);

    return NotifyStateValue;
}

HRESULT CAnimNotifyState_CharacterController::Bind_Function()
{
    if (m_strNotifyStateName == "ANS_CCT_DisableGravity")
    {
        m_fnBegin = [&](_float fTrackPosition)
            {
                m_pCCT->Apply_Gravity(false);
            };

        m_fnEnd = [&](_float fTrackPosition)
            {
                m_pCCT->Apply_Gravity(true);
            };
    }
    else if (m_strNotifyStateName == "ANS_CCT_ApplyGhostMode")
    {
        m_fnBegin = [&](_float fTrackPosition)
            {
                m_pCCT->Apply_GhostMode(true);
            };

        m_fnEnd = [&](_float fTrackPosition)
            {
                m_pCCT->Apply_GhostMode(false);
            };
    }
    else if (m_strNotifyStateName == "ANS_CCT_DisableGravityApplyGhost")
    {
        m_fnBegin = [&](_float fTrackPosition)
            {
                m_pCCT->Apply_Gravity(false);
                m_pCCT->Apply_GhostMode(true);

                m_pGameInstance->Remove_Timer("CAnimNotifyState_CharacterController::ANS_CCT_DisableGravityApplyGhost" + to_string(m_pOwner->Get_ObjectID()));
            };

        m_fnEnd = [&](_float fTrackPosition)
            {
                m_pCCT->Apply_GhostMode(false);
                //m_pCCT->Apply_Gravity(true);
                //m_pCCT->Renew_Controller();
                m_pGameInstance->Add_Timer("CAnimNotifyState_CharacterController::ANS_CCT_DisableGravityApplyGhost" + to_string(m_pOwner->Get_ObjectID()), 0.0f, [&]() { m_pCCT->Apply_Gravity(true); });
            };
    }

    return S_OK;
}

CAnimNotifyState* CAnimNotifyState_CharacterController::Create(const _string& strNotifyStateName, rapidjson::Value* pJsonValue)
{
    CAnimNotifyState_CharacterController* pInstance = new CAnimNotifyState_CharacterController();

    if (FAILED(pInstance->Initialize_Prototype(strNotifyStateName, pJsonValue)))
    {
        MSG_BOX("Failed To Created : CAnimNotifyState_CharacterController");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CAnimNotifyState* CAnimNotifyState_CharacterController::Clone(void* pArg)
{
    return new CAnimNotifyState_CharacterController(*this);
}

void CAnimNotifyState_CharacterController::Free()
{
    __super::Free();
}
