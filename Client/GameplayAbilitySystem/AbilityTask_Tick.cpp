#include "AbilityTask_Tick.h"
#include "MovePointLight.h"
#include "GameplayAbilitySystemHeader.h"
#include "MonsterObject.h"
#include "Laser.h"

CAbilityTask_UpdateLight::CAbilityTask_UpdateLight()
{
    m_bTickingTask = true;
}

void CAbilityTask_UpdateLight::Activate()
{
}

void CAbilityTask_UpdateLight::Tick_Task(_float fTimeDelta)
{
    if (m_pMovePointLight)
    {
        _vector vCamPos = XMLoadFloat4(m_pGameInstance->Get_CamPosition());

        m_pMovePointLight->Update_Position(vCamPos, {1.f, 1.f, 1.f, 0.f});
    }
}

void CAbilityTask_UpdateLight::OnDestroy(bool bInOwnerFinished)
{
}

CAbilityTask_UpdateLight* CAbilityTask_UpdateLight::Create(IGameplayAbility* pAbility, CMovePointLight* pMovePointLight)
{
    CAbilityTask_UpdateLight* pTask = new CAbilityTask_UpdateLight();
    pTask->Init_Task(*pAbility);
    pTask->m_pMovePointLight = pMovePointLight;

    return pTask;
}

void CAbilityTask_UpdateLight::Free()
{
    __super::Free();

}

CAbilityTask_UpdateAimSpread::CAbilityTask_UpdateAimSpread()
{
    m_bTickingTask = true;
}

void CAbilityTask_UpdateAimSpread::Activate()
{
    _float fAimSpread;
    _bool bFound;
    fAimSpread = m_pAbilitySystemComponent->GetGameplayAttributeValue("fAimSpread", bFound);
    if (false == bFound)
    {
        End_Task();
        return;
    }

    switch (m_eWeaponType)
    {
    case Client::WEAPON_NONE:
        break;
    case Client::WEAPON_KNIFE:
        break;
    case Client::WEAPON_PISTOL:
        m_fSpreadMultiplier = 1.f;
        break;
    case Client::WEAPON_SHOTGUN:
        m_fSpreadMultiplier = 3.f;
        break;
    case Client::WEAPON_RIFLE:
        // 히트 스캔
        m_fSpreadMultiplier = 0.f;
        m_fMinSpread = 0.f;
        break;
    case Client::WEAPON_GRENADE:
        break;
    case Client::WEAPON_MAGNUM:
        break;
    case Client::WEAPON_ROCKET_LAUNCHER:
        break;
    case Client::WEAPON_SMG:
        break;
    case Client::WEAPON_END:
        break;
    default:
        break;
    }

    if (m_pAbilitySystemComponent->Has_MatchingGameplayTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_WALK)))
    {
        m_fFinalSpread = m_fMaxSpread;
    }
}

void CAbilityTask_UpdateAimSpread::Tick_Task(_float fTimeDelta)
{
    if (m_pAbilitySystemComponent->Has_MatchingGameplayTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_WALK)))
    {
        m_fFinalSpread += m_fWalkSpreadRate * fTimeDelta;
    }
    else if (m_bCanRecoverSpread)
    {
        m_fFinalSpread -= m_fRecoverRate * fTimeDelta;
    }

    m_fFinalSpread = clamp(m_fFinalSpread, m_fMinSpread, m_fMaxSpread);

    Apply_BreathAnimation();

    m_pAbilitySystemComponent->SetNumericAttributeBase("fAimSpread", m_fFinalSpread * m_fSpreadMultiplier);
}

void CAbilityTask_UpdateAimSpread::Init_Task(IGameplayAbility& ThisAbility)
{
    __super::Init_Task(ThisAbility);

    m_pShootAbility->m_OnShootDelegate.AddDynamic("CAbilityTask_UpdateAimSpread", this, &CAbilityTask_UpdateAimSpread::OnShootEventRecevied);
}

void CAbilityTask_UpdateAimSpread::OnDestroy(bool bInOwnerFinished)
{
    m_ActorInfo.pCompositeArmature->Deactivate_AnimLayer("", "Add_Breath");
}

void CAbilityTask_UpdateAimSpread::OnShootEventRecevied(_uint iWeaponType)
{
    m_bCanRecoverSpread = false;
    m_fFinalSpread = m_fMaxSpread;
    m_pGameInstance->Add_Timer("CAbilityTask_UpdateAimSpread", 1.f, [&]() 
        {
            m_bCanRecoverSpread = true; 
        });
}

void CAbilityTask_UpdateAimSpread::Apply_BreathAnimation()
{
    if (m_fFinalSpread <= m_fMinSpread)
    {
        m_ActorInfo.pCompositeArmature->Deactivate_AnimLayer("", "Add_Breath");
    }
    else
    {
        string strBreathAddAnimKey = "";
        // 라이플의 경우 Danger Asset이 없어서 Danger상태에서도 평소랑 똑같은 BreathAnim을 유지하겠음
        if (CONDITION_GENERIC == m_pShootAbility->Get_PlayerCharacter()->Get_ConditionType() || WEAPON_RIFLE == m_pShootAbility->Get_PlayerCharacter()->Get_WeaponType())
            strBreathAddAnimKey = CGameManager::Parsing_Animation(m_ActorInfo, Parsing_WeaponName(m_pShootAbility->Get_PlayerCharacter()->Get_WeaponType()) + "0163_hold_breath_loop_add");
        else if (CONDITION_DANGER == m_pShootAbility->Get_PlayerCharacter()->Get_ConditionType())
            strBreathAddAnimKey = CGameManager::Parsing_Animation(m_ActorInfo, Parsing_WeaponName(m_pShootAbility->Get_PlayerCharacter()->Get_WeaponType()) + "0161_hold_danger_loop");

        if (strBreathAddAnimKey.size())
            m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", "Add_Breath", strBreathAddAnimKey, true);
    }
}

CAbilityTask_UpdateAimSpread* CAbilityTask_UpdateAimSpread::Create(IGameplayAbility* pAbility, CAbility_Shoot* pShootAbility)
{
    CAbilityTask_UpdateAimSpread* pTask = new CAbilityTask_UpdateAimSpread();
    pTask->m_pShootAbility = pShootAbility;
    pTask->Init_Task(*pAbility);

    return pTask;
}

void CAbilityTask_UpdateAimSpread::Free()
{
    __super::Free();
}

CAbilityTask_IsCallingEnd::CAbilityTask_IsCallingEnd()
{
    m_bTickingTask = true;
}

void CAbilityTask_IsCallingEnd::Activate()
{
}

void CAbilityTask_IsCallingEnd::Tick_Task(_float fTimeDelta)
{
    if (false == m_pGameInstance->Get_Playing_Script())
    {
        End_Task();
        /*m_pGameInstance->Add_Timer("Chapter",1.f ,[this]() 
            {
            });*/
        // End_Task();
    }
}
void CAbilityTask_IsCallingEnd::OnDestroy(bool bInOwnerFinished)
{
    m_OnEnded.Broadcast(this);
}
CAbilityTask_IsCallingEnd* CAbilityTask_IsCallingEnd::Create(IGameplayAbility* pAbility)
{
    CAbilityTask_IsCallingEnd* pTask = new CAbilityTask_IsCallingEnd();
    pTask->Init_Task(*pAbility);

    return pTask;
}

void CAbilityTask_IsCallingEnd::Free()
{
    __super::Free();
}

CAbilityTask_SetAnchorPoint::CAbilityTask_SetAnchorPoint()
{
    m_bTickingTask = true;
}

void CAbilityTask_SetAnchorPoint::Activate()
{
}

void CAbilityTask_SetAnchorPoint::Tick_Task(_float fTimeDelta)
{
    if (m_pAnchorPoint && m_pTargetMatrix)
    {
        _matrix matCombined;
        if (m_pTargetSocketMatrix)
            matCombined = XMLoadFloat4x4(m_pTargetSocketMatrix) * XMLoadFloat4x4(m_pTargetMatrix);
        else
            matCombined = XMLoadFloat4x4(m_pTargetMatrix);

        m_pAnchorPoint->Get_TransformCom()->Set_State_Notify(CTransform::STATE_POSITION, XMVectorSetW(matCombined.r[3], 1.f));
    }
}

void CAbilityTask_SetAnchorPoint::OnDestroy(bool bInOwnerFinished)
{
}

CAbilityTask_SetAnchorPoint* CAbilityTask_SetAnchorPoint::Create(IGameplayAbility* pAbility, CGameObject* pAnchorPoint, const _float4x4* pTargetMatrix, const _float4x4* pTargetSocketMatrix)
{
    CAbilityTask_SetAnchorPoint* pTask = new CAbilityTask_SetAnchorPoint();
    pTask->m_pTargetMatrix = pTargetMatrix;
    pTask->m_pTargetSocketMatrix = pTargetSocketMatrix;
    pTask->m_pAnchorPoint = pAnchorPoint;
    pTask->Init_Task(*pAbility);

    return pTask;
}

void CAbilityTask_SetAnchorPoint::Free()
{
    __super::Free();
}

CAbilityTask_SendMonsterDodgeEvent::CAbilityTask_SendMonsterDodgeEvent()
{
    m_bTickingTask = true;
}

void CAbilityTask_SendMonsterDodgeEvent::Activate()
{
    m_fAccTime = 0.f;
}

void CAbilityTask_SendMonsterDodgeEvent::Tick_Task(_float fTimeDelta)
{
    m_fAccTime += fTimeDelta;
    if (m_fAccTime < m_fSendCoolTime)
        return;
    /* 쿨타임 도달할 때 한 번만 발동*/
    else
    {
        m_fAccTime = 0.f;

        RAY CamRay = CMyUtils::Make_CamRay(m_pGameInstance);

        vector<CAST_BIND> vecColInfos;
        // 거리가 17이내인 몬스터들에게만 RayCast를 수행하여서 메세지를 전달하겠음 
        if (m_pGameInstance->RayCast_Rigid(CamRay, 17.f, vecColInfos, (1 << OT_MONSTER)))
        {
            for (auto& BindData : vecColInfos)
            {
                CMonsterObject* pMonsterObj = dynamic_cast<CMonsterObject*>(BindData.pBound->Get_Owner());
                if (nullptr == pMonsterObj)
                    continue;

                GameplayEventData EventData;
                EventData.EventTag = GameplayTag(KEY_EVENT_MONSTER_DODGE_EVENT);
                EventData.pUserData = &CamRay;
                pMonsterObj->Receive_Event(&EventData);
            }
        }
    }
}

void CAbilityTask_SendMonsterDodgeEvent::OnDestroy(bool bInOwnerFinished)
{
}

CAbilityTask_SendMonsterDodgeEvent* CAbilityTask_SendMonsterDodgeEvent::Create(IGameplayAbility* pAbility, _float fSendCoolTime)
{
    CAbilityTask_SendMonsterDodgeEvent* pTask = new CAbilityTask_SendMonsterDodgeEvent();
    pTask->Init_Task(*pAbility);

    // clamp For Safety
    fSendCoolTime = max(fSendCoolTime, 0.1f);
    pTask->m_fSendCoolTime = fSendCoolTime;

    return pTask;
}

void CAbilityTask_SendMonsterDodgeEvent::Free()
{
    __super::Free();
}

CAbilityTask_ApplyLaserPoint::CAbilityTask_ApplyLaserPoint()
{
    m_bTickingTask = true;
}

void CAbilityTask_ApplyLaserPoint::Activate()
{
    m_fAccTime = 0.f;
}

void CAbilityTask_ApplyLaserPoint::Tick_Task(_float fTimeDelta)
{
    m_fAccTime += fTimeDelta;
    //if (m_fAccTime < m_fSendCoolTime)
    if (0)
        return;
    /* 쿨타임 도달할 때 한 번만 발동*/
    else
    {
        m_fAccTime = 0.f;

        // 레이저 포인터가 위치할 포인트
        _vector vAnchorPoint = XMVectorZero();
        _float fMinDist = 1e9;

        RAY CamRay = CMyUtils::Make_CamRay(m_pGameInstance);

        vector<CAST_BIND> vecColInfos;
        if (m_pGameInstance->RayCast_Rigid(CamRay, 50.f, vecColInfos, (1 << OT_MONSTER | 1 << OT_STATIC | 1 << OT_DESTRUCTIBLE | 1 << OT_BOSSMONSTER)))
        {
            if (false == vecColInfos.empty())
            {
                _vector vCamPos = XMLoadFloat4(m_pGameInstance->Get_CamPosition());

                // vecConInfos를 오름차순 정렬
                std::sort(vecColInfos.begin(), vecColInfos.end(), [&](const CAST_BIND& a, const CAST_BIND& b) {
                    _float distA = XMVectorGetX(XMVector3Length(a.point.vContactPoint - vCamPos));
                    _float distB = XMVectorGetX(XMVector3Length(b.point.vContactPoint - vCamPos));
                    return distA < distB; 
                    });

                CAST_BIND& closestHit = vecColInfos[0];

                vAnchorPoint = closestHit.point.vContactPoint;
                fMinDist = XMVectorGetX(XMVector3Length(XMLoadFloat3(&CamRay.vOrigin) - vAnchorPoint));
            }
        }

        /* Query Only Static Object */
        RAYCAST_INFO Info[16];
        if (m_pGameInstance->Query_RayCastMulti(CamRay, 50.f, Info, 1))
        {
            for (int i = 0; i < 16; i++)
            {
                if (nullptr == Info[i].pActor)
                    continue;

                if (fMinDist > Info[i].fDistance)
                {
                    fMinDist = Info[i].fDistance;
                    vAnchorPoint = Info[i].vPosition;
                }
            }
        }

        // 충돌 대상을 찾지 못한 케이스
        if (1e9 != fMinDist)
        {
            m_pPlayerCharacter->Get_Laser()->Set_AcnchorPos(vAnchorPoint);
            m_pPlayerCharacter->Get_Laser()->TurnOn(true);
        }
        else
            m_pPlayerCharacter->Get_Laser()->TurnOn(false);
    }
}

void CAbilityTask_ApplyLaserPoint::OnDestroy(bool bInOwnerFinished)
{
    m_pPlayerCharacter->Get_Laser()->TurnOn(false);
}

CAbilityTask_ApplyLaserPoint* CAbilityTask_ApplyLaserPoint::Create(IGameplayAbility* pAbility, CPlayerCharacter* pPlayerCharacter, _float fSendCoolTime)
{
    CAbilityTask_ApplyLaserPoint* pTask = new CAbilityTask_ApplyLaserPoint();
    pTask->Init_Task(*pAbility);

    // clamp For Safety
    fSendCoolTime = max(fSendCoolTime, 0.0001f);
    pTask->m_fSendCoolTime = fSendCoolTime;
    pTask->m_pPlayerCharacter = pPlayerCharacter;

    return pTask;
}

void CAbilityTask_ApplyLaserPoint::Free()
{
    __super::Free();
}

CAbilityTask_ApplyGrenadeReborn::CAbilityTask_ApplyGrenadeReborn()
{
    m_bTickingTask = true;
}

void CAbilityTask_ApplyGrenadeReborn::Activate()
{
    m_fAccTime = 0.f;
}

void CAbilityTask_ApplyGrenadeReborn::Tick_Task(_float fTimeDelta)
{
    m_fAccTime += fTimeDelta;
    if (m_fAccTime < m_fSendCoolTime)
        return;
    /* 쿨타임 도달할 때 한 번만 발동*/
    else
    {
        m_fAccTime = 0.f;

        RAY CamRay = CMyUtils::Make_CamRay(m_pGameInstance);

        _matrix matHand;
        if (true == m_ActorInfo.pCompositeArmature->Get_SocketTransform("", "R_Wep", matHand))
        {
            _matrix matCombined = matHand * m_pPlayerCharacter->Get_TransformCom()->Get_WorldMatrix();
            _vector vStartPos = XMVectorSetW(matCombined.r[3], 1.f);

            _vector vDot = XMVector3Dot(CMyUtils::Up_Vector, XMVector3Normalize(XMLoadFloat3(&CamRay.vDir)));
            /* 0 ~ 180도 사이의 Theta 범위에서 103 ~ 180도 사이의 범위면 내려다 본다고 생각하겠음 */
            _float fTheta = acosf(XMVectorGetX(vDot));
            constexpr _float kDownwardAngleThresholdDeg = 103.f;
            constexpr _float kDownwardAngleThresholdRad = XM_PI * kDownwardAngleThresholdDeg / 180.f;
            _bool bIsDownwardThrow;

            if (fTheta >= kDownwardAngleThresholdRad)
                bIsDownwardThrow = true;
            else
                bIsDownwardThrow = false;

            _vector vNormalizedCamLook = XMVector3Normalize(XMLoadFloat4(m_pGameInstance->Get_CamDirection()));

            if (false == bIsDownwardThrow)
            {
                // 시작 위치와 목표 위치를 설정
                _vector start = vStartPos;
                _vector target = start + vNormalizedCamLook * m_fThrowDistance;

                _float T = 1.2f;               // 목표 위치까지 도달하는 시간 (초)

                CGameManager::GetInstance()->Control_Ribbon(true, start, CMyUtils::Get_InitialVelocityFromTargetPoint(start, target, 1.2f));
                //m_pRigid->Set_Velocity(CMyUtils::Get_InitialVelocityFromTargetPoint(start, target, 1.2f));
            }
            else
            {
                // 아래로 던지기 (위로 올라갔다가 내려오는 궤적)
                  // 카메라 방향의 수평 성분만 추출
                _vector vCamLookHorizontal = XMVectorSetY(vNormalizedCamLook, 0.f);
                vCamLookHorizontal = XMVector3Normalize(vCamLookHorizontal);

                // 초기 속도 설정
                _float fHorizontalSpeed = 20.f;  // 수평 속도 (더 강하게)
                _float fVerticalSpeed = -5.f;    // 수직 속도 (아래로, 약하게)

                // 수평 속도 + 수직 속도 조합
                _vector vInitialVelocity = vCamLookHorizontal * fHorizontalSpeed;
                vInitialVelocity = XMVectorSetY(vInitialVelocity, fVerticalSpeed);

                CGameManager::GetInstance()->Control_Ribbon(true, vStartPos, vInitialVelocity, true);
            }
        }
    }
}

void CAbilityTask_ApplyGrenadeReborn::OnDestroy(bool bInOwnerFinished)
{
    CGameManager::GetInstance()->Control_Ribbon(false, {}, {});
}

CAbilityTask_ApplyGrenadeReborn* CAbilityTask_ApplyGrenadeReborn::Create(IGameplayAbility* pAbility, CPlayerCharacter* pPlayerCharacter, _float fSendCoolTime)
{
    CAbilityTask_ApplyGrenadeReborn* pTask = new CAbilityTask_ApplyGrenadeReborn();
    pTask->Init_Task(*pAbility);

    // clamp For Safety
    fSendCoolTime = max(fSendCoolTime, 0.1f);
    pTask->m_fSendCoolTime = fSendCoolTime;
    pTask->m_pPlayerCharacter = pPlayerCharacter;

    return pTask;
}

void CAbilityTask_ApplyGrenadeReborn::Free()
{
    __super::Free();

}

CAbilityTask_ApplyQTE::CAbilityTask_ApplyQTE()
{
    m_bTickingTask = true;
}

void CAbilityTask_ApplyQTE::Activate()
{
    m_fGaugeProgress = 0.f;
}

void CAbilityTask_ApplyQTE::Tick_Task(_float fTimeDelta)
{
    if (true == m_pDefaultAnimLayerAnimator->Get_EndNotify())
    {
        End_Task();
        return;
    }

    m_fGaugeProgress -= fTimeDelta * m_fGaugeDecaySpeed;
    m_fGaugeProgress = max(0.f, m_fGaugeProgress);

    if (m_pGameInstance->Key_Down(m_InputKey))
    {
        m_fGaugeProgress += m_fGaugePerPress;
        m_fGaugeProgress = min(1.f, m_fGaugeProgress);

#ifdef LOAD_UI
        m_pGameInstance->Get_UI(L"Interection_QTE_Catch_Circle")->Set_HpRatio(m_fGaugeProgress) ;
#endif

        if (m_fGaugeProgress >= 1.f)
            End_Task();
    }
}

void CAbilityTask_ApplyQTE::OnDestroy(bool bInOwnerFinished)
{
}

CAbilityTask_ApplyQTE* CAbilityTask_ApplyQTE::Create(IGameplayAbility* pAbility, _ubyte DIKKey)
{
    CAbilityTask_ApplyQTE* pTask = new CAbilityTask_ApplyQTE();
    pTask->Init_Task(*pAbility);
    pTask->m_InputKey = DIKKey;
    pTask->m_pDefaultAnimLayerAnimator = pTask->m_ActorInfo.pCompositeArmature->Get_Animator("", KEY_ANIMLAYER_DEFAULT);
    if (nullptr == pTask->m_pDefaultAnimLayerAnimator)
        return nullptr;

    return pTask;
}

void CAbilityTask_ApplyQTE::Free()
{
    __super::Free();
}
