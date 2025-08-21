#include "Ability_Attack.h"
#include "GameplayAbilitySystemHeader.h"
#include "HealthObject.h"
#include "ParticleEmitter.h"
#include "MonsterObject.h"
#include "Reon.h"
#include "MonsterCommonObject.h"

HRESULT CAbility_Attack::Late_Initialize()
{
    return S_OK;
}

CAbility_Attack::CAbility_Attack()
{
}

CAbility_Aim::CAbility_Aim()
{

}

HRESULT CAbility_Aim::Late_Initialize()
{
    m_pCharacter = dynamic_cast<CPlayerCharacter*>(m_ActorInfo.pCharacter);

    m_iInputActionID = ACTION_AIM;

    GameplayTag tag(KEY_STATE_AIMING);

    m_AbilityTags.AddTag(tag);
    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_AIMING));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_AIMING));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_AIMING));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_CROUCHING));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_DASH));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));
    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_RELOAD));

    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_STATE_DASH));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_STATE_CROUCHING));

    m_pHoldStartMontageTask = CAbilityTask_PlayMontageAndWait::Create(this, "", KEY_ANIMLAYER_UPPER,
        "");

    m_pHoldStartMontageTask->m_OnMontageFinished.AddDynamic("CAbility_Aim", this, &CAbility_Aim::OnMontageTaskEndReceived);

    m_pRotatePitchTask = CAbilityTask_LookCamPitch::Create(this, "Spine_2", 3.f, 60.f, 20.f);

    m_pTurnTask = CAT_FixDirection::Create(this, 10.f);

    m_pInputTask = CAbilityTask_InputBinding::Create(this, DIK_V);
    m_pInputTask->m_OnInputPressed.AddDynamic("CAbility_Aim", this, &CAbility_Aim::OnAltPressed);

    m_pSendMonsterDodgeEventTask = CAbilityTask_SendMonsterDodgeEvent::Create(this);

    m_pApplyLaserPointTask = CAbilityTask_ApplyLaserPoint::Create(this, m_pCharacter);
    m_pApplyGrenadeRebornTask = CAbilityTask_ApplyGrenadeReborn::Create(this, m_pCharacter);

    /* 지연 생성되는 Shoot 어빌리티를 찾기 위함 */
    m_pGameInstance->Add_Timer("CAbility_Aim", 0.f, [&]()
        {
            CAbility_Shoot* pShootAbility = dynamic_cast<CAbility_Shoot*>(m_pASC->Find_Ability("Shoot"));
            if (pShootAbility)
                m_pAimSpreadTask = CAbilityTask_UpdateAimSpread::Create(this, pShootAbility);
        });


    return S_OK;
}

_bool CAbility_Aim::CanActivate_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
    /* 현재 소지하고 있는 무기가 나이프나 아무것도 안들고있는 상태에서 Hide한 WeaponType마저 없었다면 return false */
    if (m_pCharacter->Get_WeaponType() == WEAPON_NONE || m_pCharacter->Get_WeaponType() == WEAPON_KNIFE)
    {
        if (m_pCharacter->Get_HideWeaponType() == WEAPON_NONE || m_pCharacter->Get_HideWeaponType() == WEAPON_KNIFE)
            return false;
    }

    return true;
}

void CAbility_Aim::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_bStarted = false;

    if (m_pCharacter->Is_HidingWeapon())
    {
        GameplayEventData EventData;
        m_pASC->TriggerAbility_FromGameplayEvent("ChangeWeapon", nullptr, GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_AIMING), &EventData);
        End_Ability(pActorInfo);
        //m_pASC->TryActivate_Ability("ChangeWeapon");
    }
    else
    {
        OnStartActivate();
    }
}

void CAbility_Aim::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{
    m_pCharacter->Set_Aiming(false);

    m_ActorInfo.pAnimInstance->Play_Montage("", KEY_ANIMLAYER_UPPER, CGameManager::Parsing_Animation(m_ActorInfo, Parsing_WeaponName(m_pCharacter->Get_WeaponType()) + "0158_hold_end"));
    m_ActorInfo.pAnimInstance->Re_Enter_CurrentState();

    Hold_Weapon(m_pASC, "0158_hold_end", true);

    End_Ability(pActorInfo, false);
}

void CAbility_Aim::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    __super::End_Ability(pActorInfo, bWasCancelled);

    if (m_bStarted)
        CCamera_Manager::GetInstance()->On_Aiming(m_pCharacter->Get_WeaponType(), false);

#ifdef LOAD_UI
    CGameManager::GetInstance()->Set_MousePointer(false);
#endif
}

void CAbility_Aim::OnGameplayEvent(GameplayTag EventTag, const GameplayEventData* pTriggerEventData)
{
    /*if (EventTag == GameplayTag(KEY_ABILITY_COMBAT_WEAPON_CHANGE))
    {
        OnStartActivate();
    }*/
}

void CAbility_Aim::OnMontageTaskEndReceived(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
}

void CAbility_Aim::OnAltPressed(IAbilityTask* pTask)
{
    if (m_pCharacter->Get_WeaponType() == WEAPON_RIFLE)
    {
        m_iZoomLevel++;
        if (m_iZoomLevel > 2)
            m_iZoomLevel = 0;

        CCamera_Manager::GetInstance()->Set_ZoomLevel(m_iZoomLevel);
    }
}

void CAbility_Aim::OnStartActivate()
{
    m_bStarted = true;

    WeaponType eWeaponType = m_pCharacter->Get_WeaponType();

    CCamera_Manager::GetInstance()->On_Aiming(eWeaponType, true, 0.5f);

    // 뭔가 총을 들고있는 상태를 부여하려고 했었는데 굳이 싶다
    m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_GUN_ARMED));

#ifdef LOAD_UI
    CGameManager::GetInstance()->Set_MousePointer(true);
#endif

    /* Send Monster Dodge Event */
    m_pSendMonsterDodgeEventTask->ReadyForActivation();


    if (WEAPON_PISTOL == eWeaponType)
    {
#ifdef LOAD_EFFECT
        m_pApplyLaserPointTask->ReadyForActivation();
#endif
    }
    else if (WEAPON_GRENADE == eWeaponType || WEAPON_FLASHBANG == eWeaponType)
    {
#ifdef LOAD_EFFECT
        m_pApplyGrenadeRebornTask->ReadyForActivation();
#endif
    }

    /* 플레이어를 카메라 방향으로 세팅하고 에니메이션 상태를 재진입하게 만듦(조준상태의 에니메이션으로 전환) */
    m_pCharacter->Get_TransformCom()->Rotation_Dir(CMyUtils::Get_CamDirectionByDIR(DIR_FRONT));
    m_ActorInfo.pAnimInstance->Re_Enter_CurrentState();

    /* Checking Magnification Alt Pressed Task*/
    m_pInputTask->ReadyForActivation();

    /* Hold Start Montage */
    m_pHoldStartMontageTask->Set_AnimKey(CGameManager::Parsing_Animation(m_ActorInfo.pCharacter->Get_CharacterNumber(), Parsing_WeaponName(m_pCharacter->Get_WeaponType()) + "0140_hold_start"));
    m_pHoldStartMontageTask->ReadyForActivation();

    /* Ready Rotation Task*/
    m_pTurnTask->ReadyForActivation();
    m_pRotatePitchTask->ReadyForActivation();
    m_pAimSpreadTask->Set_WeaponType(m_pCharacter->Get_WeaponType());
    m_pAimSpreadTask->ReadyForActivation();

    /* 무기를 잡고있는 에니메이션 상태 제거 */
    Hold_Weapon(m_pASC, "0158_hold_end", false);
}

CAbility_Aim* CAbility_Aim::Create()
{
    return new CAbility_Aim();
}

void CAbility_Aim::Free()
{
    __super::Free();
}

CAbility_Shoot::CAbility_Shoot()
{

}

HRESULT CAbility_Shoot::Late_Initialize()
{
    m_pCharacter = dynamic_cast<CPlayerCharacter*>(m_ActorInfo.pCharacter);

    m_iInputActionID = ACTION_SHOOT;

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_SHOOTING));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_SHOOTING));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_AIMING));
    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_SHOOTABLE));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));
    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE));
    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_RELOAD));

    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_AIMING));

    m_ShootDelay[WEAPON_PISTOL] = 2.3f;
    m_ShootDelay[WEAPON_SHOTGUN] = 1.f;
    m_ShootDelay[WEAPON_RIFLE] = 1.f;
    m_ShootDelay[WEAPON_ROCKET_LAUNCHER] = 0.5f;
    m_ShootDelay[WEAPON_SMG] = 16.f;
    m_ShootDelay[WEAPON_MAGNUM] = 0.5f;
    m_ShootDelay[WEAPON_FLASHBANG] = 0.5f;
    m_ShootDelay[WEAPON_GRENADE] = 0.5f;

    /* Hold Start Montage */
    
    m_pGrenadeShootMontageTask = CAbilityTask_PlayMontageAndWait::Create(this, "", KEY_ANIMLAYER_DEFAULT, "");

    m_pGrenadeShootMontageTask->m_OnMontageFinished.AddDynamic("CAbility_Shoot", this, &CAbility_Shoot::OnGrenadeShootEnded);

    return S_OK;
}

_bool CAbility_Shoot::CanActivate_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
#ifdef LOAD_UI
    if (WEAPON_ROCKET_LAUNCHER == m_pCharacter->Get_WeaponType() || WEAPON_GRENADE == m_pCharacter->Get_WeaponType() || WEAPON_FLASHBANG == m_pCharacter->Get_WeaponType())
        return true;

    if (nullptr == m_pCharacter->Get_HoldWeapon())
        return false;

    if (false == m_pCharacter->Get_HoldWeapon()->Can_Shot())
    {
        m_pASC->TryActivate_Ability("Reload");
        return false;
    }
#endif

    return true;
}

bool CAbility_Shoot::Commit_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
#ifdef LOAD_UI
    CGameManager::GetInstance()->On_Shoot();

    CItem_Weapon* pWeapon = m_pCharacter->Get_HoldWeapon();
    if (pWeapon)
    {
        Determine_ShootDelay(m_pCharacter->Get_WeaponType(), pWeapon);

        //// 연사속도 정의
        //m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_SHOOTABLE));
        //m_pGameInstance->Add_Timer("CAbility_Shoot", pWeapon->Get_RapidOfFire(), [&]() {m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_SHOOTABLE)); });
    }

#else
    m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_SHOOTABLE));
    m_pGameInstance->Add_Timer("CAbility_Shoot", 0.13f, [&]() {m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_SHOOTABLE)); });
#endif

    return true;
}

void CAbility_Shoot::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    if (false == Commit_Ability(pActorInfo))
    {
        End_Ability(pActorInfo);
        return;
    }

    /* Play Weapon Animation*/
    WeaponType eWeaponType = m_pCharacter->Get_WeaponType();
    m_ActorInfo.pCompositeArmature->Select_Animation(Parsing_WeaponParts(eWeaponType), Parsing_WeaponNameNoneSuffix(eWeaponType) + "general_0512_Aim_Fire");

    /* For Rebound Anim, SMG의 경우에는 반동 적용하면 너무 이상해서 적용하지 않겠음... */
    if (WEAPON_SMG != eWeaponType)
        m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_ADD, CGameManager::Parsing_Animation(m_ActorInfo, Parsing_WeaponName(m_pCharacter->Get_WeaponType()) + "0500_shoot"), false);

    // 로켓 에니메이션의 경우 반동 에니메이션이 이상해서 강제로 멈춤
    if (m_pCharacter->Get_WeaponType() == WEAPON_ROCKET_LAUNCHER)
    {
        m_pGameInstance->Add_Timer("CAbility_Shoot_StopRocketLauncerAddAnim", 0.5f, [&]() {
            m_ActorInfo.pCompositeArmature->Deactivate_AnimLayer("", KEY_ANIMLAYER_ADD);
            });

#ifdef LOAD_UI
        /* 일시적인 코드 */
        //m_pGameInstance->Start_Script(19);
#endif
    }

    RAY ray;
    if (false == Get_BulletRay(ray))
    {
        End_Ability(pActorInfo);
        return;
    }

    /* Spawn Particle */
    switch (eWeaponType)
    {
    case Client::WEAPON_PISTOL:
    case Client::WEAPON_SHOTGUN:
    case Client::WEAPON_RIFLE:
    case Client::WEAPON_MAGNUM:
    case Client::WEAPON_SMG:
        Apply_GameplayCueToMuzzle();
        Apply_GameplayCueToHitPoint(ray);
        break;
    case Client::WEAPON_NONE:
    case Client::WEAPON_KNIFE:
    case Client::WEAPON_GRENADE:
    case Client::WEAPON_ROCKET_LAUNCHER:
    case Client::WEAPON_FLASHBANG:
    case Client::WEAPON_END:
    default:
        break;
    }

    /* Camera Rebound */
    CCamera_Manager::GetInstance()->On_WeaponFire(eWeaponType);

    /* 발사 로직 결정 */
    switch (eWeaponType)
    {
    case Client::WEAPON_PISTOL:
    case Client::WEAPON_RIFLE:
    case Client::WEAPON_MAGNUM:
    case Client::WEAPON_SMG:
    {
        m_pCharacter->PlaySoundEvent("event:/Weapon/SMG/Shoot", false, false, false, _float4x4{});
        ShootMonster(ray);
        break;
    }
    case Client::WEAPON_SHOTGUN:
    {
        // 샷건의 경우 중앙 1발 주변탄 12발 데미지
        ShootMonster(ray, 0.3568f);
        const vector<RAY>& BulletRays = Get_BulletRay(12);
        for (auto& Ray : BulletRays)
            ShootMonster(Ray, 0.0536f);
        break;
    }
    case Client::WEAPON_FLASHBANG:
    case Client::WEAPON_GRENADE:
    {
        _matrix matHand;
        if (true == m_ActorInfo.pCompositeArmature->Get_SocketTransform("", "R_Wep", matHand))
        {
            _matrix matCombined = matHand * m_pCharacter->Get_TransformCom()->Get_WorldMatrix();
            _vector vStartPos = XMVectorSetW(matCombined.r[3], 1.f);

            _vector vDot = XMVector3Dot(CMyUtils::Up_Vector, XMVector3Normalize(XMLoadFloat3(&ray.vDir)));
            /* 0 ~ 180도 사이의 Theta 범위에서 103 ~ 180도 사이의 범위면 내려다 본다고 생각하겠음 */
            _float fTheta = acosf(XMVectorGetX(vDot));
            constexpr _float kDownwardAngleThresholdDeg = 103.f;
            constexpr _float kDownwardAngleThresholdRad = XM_PI * kDownwardAngleThresholdDeg / 180.f;
            _bool bIsDownwardThrow;

            if (fTheta >= kDownwardAngleThresholdRad)
            {
                bIsDownwardThrow = true;
                m_pGrenadeShootMontageTask->Set_AnimKey(CGameManager::Parsing_Animation(m_ActorInfo, "wp5400H_1410_throwing_D"));
                m_pGrenadeShootMontageTask->ReadyForActivation();

                //m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_DEFAULT, CGameManager::Parsing_Animation(m_ActorInfo, "wp5400H_1410_throwing_D"));
            }
            else
            {
                bIsDownwardThrow = false;
                m_pGrenadeShootMontageTask->Set_AnimKey(CGameManager::Parsing_Animation(m_ActorInfo, "wp5400H_1400_throwing"));
                m_pGrenadeShootMontageTask->ReadyForActivation();

                //m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_DEFAULT, CGameManager::Parsing_Animation(m_ActorInfo, "wp5400H_1400_throwing"));
            }

            m_pASC->Cancel_Ability("Aim");

            /* 휴리스틱한 시간 설정 */
            m_pGameInstance->Add_Timer("CAbility_Shoot", 0.3f, [=]() {
                CGrenade::GRENADE_DESC GrenadeDesc{};
                GrenadeDesc.pInstigator = m_pCharacter;
                GrenadeDesc.vLook = ray.vDir;
                GrenadeDesc.bIsDownwardThrow = bIsDownwardThrow;
                XMStoreFloat3(&GrenadeDesc.vPosition, vStartPos);
                if (eWeaponType == WEAPON_GRENADE)
                {
                    GrenadeDesc.eGrandeType = CGrenade::GRENADE;
                    m_pGameInstance->Activate_PoolingObject(LEVEL_STATIC, L"Layer_Grenade", &GrenadeDesc);
                }
                else if (eWeaponType == WEAPON_FLASHBANG)
                {
                    GrenadeDesc.eGrandeType = CGrenade::GRENADE_FLASHBANG;
                    m_pGameInstance->Activate_PoolingObject(LEVEL_STATIC, L"Layer_Flashbang", &GrenadeDesc);
                }

                //m_pGameInstance->Add_GameObject(LEVEL_STATIC, KEY_PROTO_OBJ_GRENADE, m_pGameInstance->Get_CurrentLevel(), L"Layer_Projectile", &GrenadeDesc);

                });
        }

        break;
    }
    case Client::WEAPON_ROCKET_LAUNCHER:
    {
        CWorldItem::WorldItemDesc WorldItemDesc{};
        WorldItemDesc.pInstigator = m_pCharacter;
        WorldItemDesc.vLook = ray.vDir;

        _matrix matMuzzlePoint;
        if (true == m_ActorInfo.pCompositeArmature->Get_SocketTransform("RPG", "vfx_muzzle1", matMuzzlePoint))
        {
            _matrix matCombined = matMuzzlePoint * m_pCharacter->Get_TransformCom()->Get_WorldMatrix();
            _vector vStartPos = XMVectorSetW(matCombined.r[3], 1.f);
            XMStoreFloat3(&WorldItemDesc.vPosition, vStartPos);

            m_pGameInstance->Add_GameObject(LEVEL_STATIC, KEY_PROTO_OBJ_RPG, m_pGameInstance->Get_CurrentLevel(), L"Layer_Projectile", &WorldItemDesc);
        }

        break;
    }
    case Client::WEAPON_NONE:
    case Client::WEAPON_KNIFE:
    case Client::WEAPON_END:
    default:
        break;
    }

    /* Broadcast => 현재로써는 Spread Anim Task 한테만 메세지 전달 */
    m_OnShootDelegate.Broadcast(m_pCharacter->Get_WeaponType());

    if (eWeaponType != WEAPON_GRENADE && eWeaponType != WEAPON_FLASHBANG)
        End_Ability(pActorInfo);
}

void CAbility_Shoot::InputPressed(const GameplayAbility_ActorInfo* pActorInfo)
{

}

void CAbility_Shoot::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_Shoot::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    //m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_SHOOTABLE));

    __super::End_Ability(pActorInfo, bWasCancelled);
}

constexpr int g_iMaxSample = 64;
static const _float2 diskSamples64[g_iMaxSample] =
{
    _float2(0.08588739595227426f, -0.030025691920664423f),
            _float2(0.00023643037646479872f, 0.20841231102873747f), _float2(-0.021014193513360258f, -0.041295564670943447f),
            _float2(0.17222130793060222f, 0.04208273481757199f), _float2(-0.10188002229260999f, 0.09867635981047702f),
            _float2(0.14228303321566757f, -0.2580966083703571f), _float2(0.1981280219522779f, 0.23339378133878957f),
            _float2(-0.3507565783285025f, -0.08514244901034494f), _float2(0.4109031442251335f, -0.1385572016757659f),
            _float2(-0.206881894838767f, 0.4355205101923277f), _float2(-0.07853732533126384f, -0.40242593228540835f),
            _float2(0.3160903818840148f, 0.21239359469104005f), _float2(-0.38171373492394534f, 0.07981848579134865f),
            _float2(0.20730755700310005f, -0.41694988161154317f), _float2(0.02116212593410295f, 0.452217832379357f),
            _float2(-0.4796904605571695f, -0.28651041940741073f), _float2(0.5297239385935364f, 0.021582930653477564f),
            _float2(-0.37273632966671305f, 0.37495166129580154f), _float2(-0.09981792946437304f, -0.5906748143084893f),
            _float2(0.2916051250135627f, 0.4431329722846758f), _float2(-0.48289435190476276f, 0.07681350373204565f),
            _float2(0.5187852940910572f, -0.3945141647783003f), _float2(-0.13963649020950908f, 0.49027792217786015f),
            _float2(-0.38061248065802f, -0.40308214832312955f), _float2(0.6158104812277763f, 0.0983407178807064f),
            _float2(-0.6196449607402823f, 0.23431768221667326f), _float2(0.2364557733641609f, -0.6229671072840033f),
            _float2(0.3005763530869763f, 0.6111111637098147f), _float2(-0.6384314720901211f, -0.1738869117858379f),
            _float2(0.573665374558835f, -0.15349124960603555f), _float2(-0.3196477554226972f, 0.7061658310018843f),
            _float2(-0.1831977675319068f, -0.5792765986262596f), _float2(0.6030483926071213f, 0.4205516346121553f),
            _float2(-0.7539032462944255f, 0.21931618991687488f), _float2(0.404646236896082f, -0.6316149350590375f),
            _float2(0.08658582362354603f, 0.7874143627737417f), _float2(-0.5005622663103718f, -0.4234605043899057f),
            _float2(0.7390320541648133f, -0.08070989534160762f), _float2(-0.6159218824886621f, 0.4341605161757036f),
            _float2(-0.03309358947453572f, -0.7000026169501067f), _float2(0.48803776980647995f, 0.6519132695663368f),
            _float2(-0.77524869240331f, -0.20524910902740462f), _float2(0.7399362385810723f, -0.44195229094163613f),
            _float2(-0.23965632319752905f, 0.7230660336872499f), _float2(-0.29944141213045217f, -0.7621092069740195f),
            _float2(0.7010758517114678f, 0.3812171234085407f), _float2(-0.7827431721781429f, 0.3506678790434763f),
            _float2(0.3590538137372583f, -0.6914906946153192f), _float2(0.18705594586863877f, 0.8722461169359508f),
            _float2(-0.8103576005204458f, -0.4865832198499747f), _float2(0.9524568869822917f, -0.2807021815875268f),
            _float2(-0.5434861760406469f, 0.8102944285691839f), _float2(-0.09371333179679404f, -0.9210740863451133f),
            _float2(0.7331372463675839f, 0.5308557721617637f), _float2(-0.8357615934512088f, 0.09499243670075716f),
            _float2(0.6528317322451158f, -0.7138708943030685f), _float2(-0.10113238393363477f, 0.9474249151791483f),
            _float2(-0.616352398484564f, -0.763336531568522f), _float2(0.9208229492320843f, 0.08175463425604013f),
            _float2(-0.7960583335354177f, 0.5466929855893787f), _float2(0.1380914715442238f, -0.9144556469794014f),
            _float2(0.40238820357725436f, 0.9331942124203743f), _float2(-0.9055908846548567f, -0.39092628874429447f),
            _float2(0.9515415333809746f, -0.3779463117824522f),
};

void CAbility_Shoot::Apply_GameplayCueToMuzzle()
{
    ///* Spawn Bullet Particle? */
    //WeaponType eWeponType = m_pCharacter->Get_WeaponType();
    //switch (eWeponType)
    //{
    //case Client::WEAPON_RIFLE:
    //case Client::WEAPON_PISTOL:
    //case Client::WEAPON_SMG:
    //case Client::WEAPON_MAGNUM:
    //    m_pCharacter->Get_Bullet()->Fire();
    //    break;
    //case Client::WEAPON_NONE:
    //case Client::WEAPON_KNIFE:
    //case Client::WEAPON_SHOTGUN:
    //case Client::WEAPON_GRENADE:
    //case Client::WEAPON_ROCKET_LAUNCHER:
    //case Client::WEAPON_END:
    //default:
    //    break;
    //}

    // Get Muzzle Point
    CParticleEmitter* pEmitter = nullptr;

    _matrix matMuzzle, matCombined;
    m_ActorInfo.pCompositeArmature->Get_SocketTransform(Parsing_WeaponParts(m_pCharacter->Get_WeaponType()), "vfx_muzzle1", matMuzzle);
    matCombined = matMuzzle * m_pCharacter->Get_TransformCom()->Get_WorldMatrix();
    _vector vMuzzlePos = matCombined.r[3];

    /* 기존에 사용했던 Gas Emitter가 있었던 경우(처음 발사한 케이스가 아닌 경우)*/
    if (CParticleEmitter* pGasEmitter = m_pCharacter->Get_MuzzleGasEmitter())
    {
        m_pGameInstance->Remove_ParticleEmitter(pGasEmitter);
        ///* 기존에 재생중이던 가스 에미터가 죽어있다면 새로운 에미터를 캐릭터에게 Set 해줌*/
        //if (true == m_pCharacter->Get_MuzzleGasEmitter()->Is_Dead())
        //{
        //    m_pGameInstance->Spawn_Particle(L"Muzzle_Smoke", pEmitter);
        //    if (pEmitter)
        //        m_pCharacter->Set_MuzzleGasEmitter(pEmitter);
        //}
        ///* 살아있다면 해당 파티클을 재사용 */
        //else
        //{
        //    pGasEmitter->Replay();
        //}
    }
    /* 처음 발사한 케이스 */
    //else
    //{
    m_pGameInstance->Spawn_Particle(L"Muzzle_Smoke", pEmitter);
    if (pEmitter)
    {
        /* 상시로 위치를 업데이트를 해야하기 때문에 플레이어 캐릭터에게 에미터 전달 */
        m_pCharacter->Set_MuzzleGasEmitter(pEmitter);
    }
    /* }*/

    if (WEAPON_RIFLE == m_pCharacter->Get_WeaponType())
        return;

    /* Activate MuzzleLight */

    CMovePointLight* pMuzzlePointLight = m_pCharacter->Get_PointLight(CPlayerCharacter::POINTLIGHT_MUZZLE);
    if (pMuzzlePointLight)
    {
        pMuzzlePointLight->Update_Position(vMuzzlePos);
        pMuzzlePointLight->Blink(0.1f, { 4.f, 3.f, 1.f, 0.f });
    }

    /* 머즐 디스토션 */
    m_pCharacter->Turn_ON_Muzzle();

    _matrix matParticle = CMyUtils::Convert_WorldMatrix2ParticleWorldMatrix(matCombined);

    ///* Spawn Particle to Muzzle*/
    m_pGameInstance->Spawn_Particle(L"Muzzle", pEmitter);
    if (pEmitter)
        pEmitter->Set_WorldMatrix(matParticle);

    WeaponType eWeaponType = m_pCharacter->Get_WeaponType();
    if (WEAPON_SHOTGUN == eWeaponType)
        m_pGameInstance->Spawn_Particle(L"Muzzle_ShotGun_Dust", pEmitter);
    else if (WEAPON_PISTOL == eWeaponType || WEAPON_SMG == eWeaponType)
        m_pGameInstance->Spawn_Particle(L"HandGunShot", pEmitter);
    else
        m_pGameInstance->Spawn_Particle(L"Muzzle_Dust", pEmitter);

    if (pEmitter)
        pEmitter->Set_WorldMatrix(matParticle);
}

void CAbility_Shoot::Apply_GameplayCueToHitPoint(RAY ray)
{
    CParticleEmitter* pEmitter = nullptr;

    /* For Static Object */
    RAYCAST_INFO Info[16];
    if (m_pGameInstance->Query_RayCastMulti(ray, 15.f, Info, 1))
    {
        /* Spawn Particle To HitPoint*/
        m_pGameInstance->Spawn_Particle(L"Spark", pEmitter);
        if (pEmitter)
            pEmitter->Set_WorldMatrix(XMMatrixTranslationFromVector(Info[0].vPosition));

        /* Activate Light to HitPoint */
        CMovePointLight* pHitPointLight = m_pCharacter->Get_PointLight(CPlayerCharacter::POINTLIGHT_HITPOINT);
        if (pHitPointLight)
        {
            pHitPointLight->Update_Position(Info[0].vPosition);
            pHitPointLight->Blink(0.1f, { 1.3f, 1.3f, 1.3f, 0.f });
        }

        DamageEventRE DamageEvent;

        for (int i = 0; i < 16; i++)
        {
            CRigidBody* pRigid = m_pGameInstance->Get_RigidByActor(Info[i].pActor);
            if (pRigid)
            {
                pRigid->Get_Bound()->Get_Owner()->Take_Damage(200.f, &DamageEvent, m_ActorInfo.pCharacter, m_ActorInfo.pCharacter);
            }
        }
    }
}

void CAbility_Shoot::ShootMonster(RAY ray, _float fDamageMultiplier, _bool bDebug)
{
    // Determine Damage
    _float  fDamage;
#ifdef LOAD_UI
    fDamage = m_pCharacter->Get_HoldWeapon()->Get_Atk() * 200.f * fDamageMultiplier;
#else
    fDamage = 200.f * fDamageMultiplier;
#endif

#ifdef _DEBUG
    if (bDebug)
        m_pGameInstance->Spawn_Ray(ray, 23.f, 13.f);
#endif

    /* For Monster */
    vector<CAST_BIND> vecColInfos;
    if (m_pGameInstance->RayCast_Rigid(ray, 75.f, vecColInfos, (1 << OT_MONSTER) | (1 << OT_STATIC) /*| (1 << OT_TRIGGER)*/))
    {
        _vector vCharacterPos = m_pCharacter->Get_TransformCom()->Get_Position();

        // vecConInfos를 오름차순 정렬
        std::sort(vecColInfos.begin(), vecColInfos.end(), [&](const CAST_BIND& a, const CAST_BIND& b) {
            _float distA = XMVectorGetX(XMVector3Length(a.point.vContactPoint - vCharacterPos));
            _float distB = XMVectorGetX(XMVector3Length(b.point.vContactPoint - vCharacterPos));
            return distA < distB;
            });

        CGameObject* pTarget = { nullptr };

        for (auto& Info : vecColInfos)
        {
            if (Info.pBound->Get_ID() == m_ActorInfo.pBound->Get_ID())
                continue;

            /* 가장 가까운 거리를 가지는 대상을 타겟으로 삼음(한 타겟에 충돌체가 여러개 있을 수 있으므로 모든 충돌체에게 데미지를 주기 위한 과정 */
            if (nullptr == pTarget)
                pTarget = Info.pBound->Get_Owner();
            else
            {
                if (pTarget != Info.pBound->Get_Owner())
                    continue;
            }

            COLLISION_INFO SendInfo;
            SendInfo.point.resize(1);
            SendInfo.point[0] = Info.point;

            DamageEventRE DamageEvent;
            DamageEvent.ColInfo = SendInfo;
            DamageEvent.fDamage = fDamage;
            DamageEvent.eType = DAMAGE_RAY;
            DamageEvent.Ray = ray;
            DamageEvent.pOverlappedComp = Info.pBound;
            DamageEvent.eReactionType = REACTION_NONE;

            Info.pBound->Get_Owner()->Take_Damage(fDamage, &DamageEvent, m_ActorInfo.pCharacter, m_ActorInfo.pCharacter);
        }
    }
}

_bool CAbility_Shoot::Get_BulletRay(RAY& OutRay)
{
    int iRandomValue = CMyUtils::Get_RandomInt(0, g_iMaxSample);

    _bool bFound = false;
    _float fSpread = m_pASC->GetNumericAttributeBase("fAimSpread", bFound);

    // 0~1 사이의 Value 적절한 방향으로 나가도록 보정
    fSpread *= 0.005f;
    if (bFound)
    {
        XMStoreFloat3(&OutRay.vOrigin, XMLoadFloat4(m_pGameInstance->Get_CamPosition()));

        _vector vShootDir = XMLoadFloat4(m_pGameInstance->Get_CamDirection());
        _vector vRight = XMVector3Cross(vShootDir, XMVectorSet(0, 1, 0, 0));
        _vector vUp = XMVector3Cross(vRight, vShootDir);

        _vector vBullterDir = Compute_BulletDirection(vShootDir, vRight, vUp, fSpread, diskSamples64[iRandomValue]);
        XMStoreFloat3(&OutRay.vDir, vBullterDir);
    }

    return bFound;
}

vector<RAY> CAbility_Shoot::Get_BulletRay(_uint iBulletCount)
{
    vector<RAY> Rays;
    Rays.resize(iBulletCount);

    int iRandomValue = CMyUtils::Get_RandomInt(0, g_iMaxSample);

    _bool bFound = false;
    _float fSpread = m_pASC->GetGameplayAttributeValue("fAimSpread", bFound);
    fSpread *= 0.01f;

    if (bFound)
    {
        _vector vShootDir = XMLoadFloat4(m_pGameInstance->Get_CamDirection());
        _vector vRight = XMVector3Cross(vShootDir, XMVectorSet(0, 1, 0, 0));
        _vector vUp = XMVector3Cross(vRight, vShootDir);

        for (_uint i = 0; i < iBulletCount; i++)
        {
            _vector vBullterDir = Compute_BulletDirection(vShootDir, vRight, vUp, fSpread, diskSamples64[iRandomValue]);
            XMStoreFloat3(&Rays[i].vOrigin, XMLoadFloat4(m_pGameInstance->Get_CamPosition()));
            XMStoreFloat3(&Rays[i].vDir, vBullterDir);

            iRandomValue++;
            if (iRandomValue >= g_iMaxSample)
                iRandomValue = 0;
        }
    }

    return Rays;
}

_vector CAbility_Shoot::Compute_BulletDirection(_fvector vDirection, _fvector vRight, _vector vUp, _float fSpread, _float2 fSampleValue)
{
    return XMVector3Normalize(vDirection + vRight * fSpread * fSampleValue.x + vUp * fSpread * fSampleValue.y);
}

_bool CAbility_Shoot::Determine_ShootDelay(WeaponType eWeaponType, CItem_Weapon* pWeapon)
{
    // 연사속도 정의
    _float finalRPS = max(1.f, pWeapon->Get_RapidOfFire()) * m_ShootDelay[eWeaponType];

    m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_SHOOTABLE));
    m_pGameInstance->Add_Timer("CAbility_Shoot", max(0.13f, 1.0f / finalRPS), [&]() {m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_SHOOTABLE)); });

    return true;
}

void CAbility_Shoot::OnGrenadeShootEnded(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    End_Ability(&m_ActorInfo);
}

CAbility_Shoot* CAbility_Shoot::Create()
{
    return new CAbility_Shoot();
}

void CAbility_Shoot::Free()
{
    __super::Free();
}

CAbility_Knife::CAbility_Knife()
{
}

HRESULT CAbility_Knife::Late_Initialize()
{
    m_iInputActionID = ACTION_KNIFE;

    GameplayTag tag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_READY);

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_READY));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_READY));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_STATE_CROUCHING));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_CHANGE));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_HOLDING));
    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_AIMING));

    m_pCharacter = dynamic_cast<CPlayerCharacter*>(m_ActorInfo.pCharacter);

    m_pComboA_MontageTask = CAbilityTask_PlayMontageAndWait::Create(this, "", KEY_ANIMLAYER_DEFAULT,
        CGameManager::Parsing_Animation(m_ActorInfo, "wp5000H_0630_attack_1st"));
    m_pComboA_MontageTask->m_OnMontageFinished.AddDynamic("Knife", this, &CAbility_Knife::OnMontageTaskEndReceived);

    m_pComboB_MontageTask = CAbilityTask_PlayMontageAndWait::Create(this, "", KEY_ANIMLAYER_DEFAULT,
        CGameManager::Parsing_Animation(m_ActorInfo, "wp5000H_0640_attack_2nd"));
    m_pComboB_MontageTask->m_OnMontageFinished.AddDynamic("Knife", this, &CAbility_Knife::OnMontageTaskEndReceived);

    m_pComboC_MontageTask = CAbilityTask_PlayMontageAndWait::Create(this, "", KEY_ANIMLAYER_DEFAULT,
        CGameManager::Parsing_Animation(m_ActorInfo, "wp5000H_0650_attack_3rd"));
    m_pComboC_MontageTask->m_OnMontageFinished.AddDynamic("Knife", this, &CAbility_Knife::OnMontageTaskEndReceived);

    m_pDashAttackMontageTask = CAbilityTask_PlayMontageAndWait::Create(this, "", KEY_ANIMLAYER_DEFAULT,
        CGameManager::Parsing_Animation(m_ActorInfo, "wp5000H_0660_attack_jog"));
    m_pDashAttackMontageTask->m_OnMontageFinished.AddDynamic("Knife", this, &CAbility_Knife::OnMontageTaskEndReceived);

    return S_OK;
}

void CAbility_Knife::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_pCharacter->Set_HoldKnife(false);

    m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_GUN_ARMED));

    if (nullptr == pTriggerEventData)
    {
        m_pCharacter->Change_Weapon(WEAPON_KNIFE);

        m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_UNOCCUPIED));
        if (m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_DASH)))
            Excute_DashAttack();
        else
        {
            m_pComboA_MontageTask->ReadyForActivation();
        }

        m_iComboIndex = 1;
    }
    else
    {
        if (pTriggerEventData->EventTag == GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_HOLDING))
        {
            m_bAttackable = true;
        }
    }

    m_pCharacter->Set_Crouching(false);
    m_pCharacter->Set_DashActivate(false);

    m_pGameInstance->Remove_Timer("Timer_Knife");
    m_pGameInstance->Add_Timer("Timer_Knife", m_fKnifePutAwayTime, [=]() {End_Ability(pActorInfo); });
}

void CAbility_Knife::InputPressed(const GameplayAbility_ActorInfo* pActorInfo)
{
    if (false == m_bAttackable)
        return;

    m_pCharacter->Change_Weapon(WEAPON_KNIFE);
    m_pASC->Cancel_Ability("Crouching");

    m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    if (m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_DASH)))
        Excute_DashAttack();
    else
    {
        if (m_bAttackable)
        {
            if (m_iComboIndex == 0)
            {
                m_pComboA_MontageTask->ReadyForActivation();
                m_iComboIndex++;
            }
            else if (m_iComboIndex == 1)
            {
                m_pComboB_MontageTask->ReadyForActivation();
                m_iComboIndex++;
            }
            else if (m_iComboIndex == 2)
            {
                m_pComboC_MontageTask->ReadyForActivation();
                m_iComboIndex = 1;
            }
        }
    }

    m_bAttackable = false;
    m_pCharacter->Set_Crouching(false);
    m_pCharacter->Set_DashActivate(false);

    m_pGameInstance->Remove_Timer("Timer_Knife");
    m_pGameInstance->Add_Timer("Timer_Knife", m_fKnifePutAwayTime, [=]()
        {
            if (m_bActivate)
            {
                End_Ability(&m_ActorInfo);
            }
        });
}

void CAbility_Knife::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{

}

void CAbility_Knife::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_bAttackable = false;
    m_pGameInstance->Remove_Timer("Timer_Knife");

    if (false == bWasCancelled)
    {
        m_pCharacter->Set_ChangeWeaponType(m_pCharacter->Get_HideWeaponType());
        GameplayEventData EventData;
        m_pASC->TriggerAbility_FromGameplayEvent("ChangeWeapon", &m_ActorInfo, GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_READY), &EventData);
    }

    __super::End_Ability(pActorInfo, bWasCancelled);
}

void CAbility_Knife::OnMontageTaskEndReceived(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_bAttackable = true;
}

void CAbility_Knife::Excute_DashAttack()
{
    //m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_UNOCCUPIED));
    m_pASC->Cancel_Ability("Dash");
    m_pDashAttackMontageTask->ReadyForActivation();
    m_iComboIndex = 1;
}

CAbility_Knife* CAbility_Knife::Create()
{
    return new CAbility_Knife();
}

void CAbility_Knife::Free()
{
    __super::Free();
}

CAbility_HoldingKnife::CAbility_HoldingKnife()
{
}

HRESULT CAbility_HoldingKnife::Late_Initialize()
{
    m_iInputActionID = ACTION_HOLDING_KNIFE;

    m_pCharacter = dynamic_cast<CPlayerCharacter*>(m_ActorInfo.pCharacter);

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_HOLDING));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_HOLDING));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_PARRYABLE));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_STATE_CROUCHING));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_STATE_DASH));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_READY));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_STATE_GUN));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_CHANGE));

    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_STATE_DASH));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_STATE_CROUCHING));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_READY));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_AIMING));
    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_SHOOTING));
    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    /* For Hold Start Montage*/
    m_pMontageTask = CAbilityTask_PlayMontageAndWait::Create(this, "", KEY_ANIMLAYER_UPPER, CGameManager::Parsing_Animation(m_ActorInfo, "wp5000H_0138_hold_start"));
    m_pMontageTask->m_OnMontageFinished.AddDynamic("CAbility_HoldingKnife", this, &CAbility_HoldingKnife::OnHoldStartMontageTaskEndReceived);

    /* For Pierce Attack Montage*/
    m_pAttackMontageTask = CAbilityTask_PlayMontageAndWait::Create(this, "", "", CGameManager::Parsing_Animation(m_ActorInfo, "wp5000H_0600_attack"));
    m_pAttackMontageTask->m_OnMontageFinished.AddDynamic("CAbility_HoldingKnife", this, &CAbility_HoldingKnife::OnAttackMontageTaskEndReceived);

    /* For Input Task*/
    m_pInputTask = CAbilityTask_InputBinding::Create(this, MOUSEKEYSTATE::DIM_LB);
    m_pInputTask->m_OnInputPressed.AddDynamic("CAbility_HoldingKnife", this, &CAbility_HoldingKnife::OnTaskInputPressed);

    /* For Look Cam Pitch Task*/
    m_pLookCamPitchTask = CAbilityTask_LookCamPitch::Create(this, "Spine_2", 3.f, 60.f, 20.f);

    //m_pNeckLookCamPitchTask = CAbilityTask_LookCamPitch::Create(this, "Neck_0", 3.f, -XM_PI * 0.1f, XM_PI * 0.03f);
    //m_pLShoulderLookCamPitchTask = CAbilityTask_LookCamPitch::Create(this, "L_Shoulder", 3.f, -XM_PI * 0.17f, XM_PI * 0.07f);
    //m_pRShoulderLookCamPitchTask = CAbilityTask_LookCamPitch::Create(this, "R_Shoulder", 3.f, -XM_PI * 0.17f, XM_PI * 0.07f);

    m_pTurnTask = CAT_FixDirection::Create(this, 10.f);

    return S_OK;
}

void CAbility_HoldingKnife::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_bAttackable = true;
    m_bParryEnd = false;

    if (CONDITION_GENERIC == m_pCharacter->Get_ConditionType())
    {
        m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", "Add_Breath", CGameManager::Parsing_Animation(m_ActorInfo, "wp5000H_0163_hold_breath_loop_add"), true);
    }
    else if (CONDITION_DANGER == m_pCharacter->Get_ConditionType())
    {
        m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", "Add_Breath", CGameManager::Parsing_Animation(m_ActorInfo, "wp5000H_0161_hold_danger_loop"), true);
    }

    /* DeActivate Gun Arm State*/
    m_ActorInfo.pCompositeArmature->Deactivate_AnimLayer("", "BothArms");
    m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_GUN_ARMED));
    m_pCharacter->Change_Weapon(WEAPON_KNIFE);

    /* Set Parrying Statte*/
    m_pGameInstance->Remove_Timer("CAbility_HoldingKnife");
    m_pGameInstance->Add_Timer("CAbility_HoldingKnife", 0.23f, [&]()
        {
            if (false == this->IsActivate())
                m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_PARRYABLE));
        });

    /* Renew Anim State */
    m_ActorInfo.pAnimInstance->Re_Enter_CurrentState();

    /* Ready Hold Knife Start Montage */
    m_pMontageTask->Set_AnimLayerKey(KEY_ANIMLAYER_UPPER);
    m_pMontageTask->Set_AnimKey(CGameManager::Parsing_Animation(m_ActorInfo, "wp5000H_0138_hold_start"));
    m_pMontageTask->ReadyForActivation();

    /* Bind Input Event to Pierce Attack */
    m_pInputTask->ReadyForActivation();

    /* For Look Cam Pitch*/
    m_pLookCamPitchTask->ReadyForActivation();

    //m_pNeckLookCamPitchTask->ReadyForActivation();
    //m_pLShoulderLookCamPitchTask->ReadyForActivation();
    //m_pRShoulderLookCamPitchTask->ReadyForActivation();

    m_bHoldEnded = false;

    m_pTurnTask->ReadyForActivation();
}

void CAbility_HoldingKnife::InputPressed(const GameplayAbility_ActorInfo* pActorInfo)
{
    m_bHoldEnded = false;
}

void CAbility_HoldingKnife::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{
    m_bHoldEnded = true;

    if (m_bAttackable)
    {
        OnHoldEndStarted();
    }
}

void CAbility_HoldingKnife::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    __super::End_Ability(pActorInfo);

    m_ActorInfo.pCompositeArmature->Deactivate_AnimLayer("", "Add_Breath");

    m_pCharacter->Set_HoldKnife(false);

    if (false == bWasCancelled)
    {
        GameplayEventData EventData;
        m_pASC->TriggerAbility_FromGameplayEvent("Knife", &m_ActorInfo, GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_HOLDING), &EventData);
    }
}

void CAbility_HoldingKnife::OnHoldStartMontageTaskEndReceived(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    if (!m_bParryEnd)
    {
        m_bParryEnd = true;
        m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_PARRYABLE));
    }
}

void CAbility_HoldingKnife::OnAttackMontageTaskEndReceived(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    if (m_ActorInfo.pCompositeArmature->Get_AnimEndNotify(""))
        m_ActorInfo.pAnimInstance->Re_Enter_CurrentState();

    if (m_bHoldEnded)
    {
        OnHoldEndStarted();
    }

    m_bAttackable = true;
}

void CAbility_HoldingKnife::OnHoldEndStarted()
{
    m_ActorInfo.pCompositeArmature->Deactivate_AnimLayer("", KEY_ANIMLAYER_UPPER);
    m_ActorInfo.pCompositeArmature->Select_Animation("", CGameManager::Parsing_Animation(m_ActorInfo, "wp5000H_0158_hold_end"));
    End_Ability(&m_ActorInfo);
}

void CAbility_HoldingKnife::OnTaskInputPressed(IAbilityTask* pTask)
{
    if (m_bAttackable)
    {
        m_pAttackMontageTask->ReadyForActivation();

        m_ActorInfo.pAnimInstance->Pause_FSM_OneTick();

        m_bAttackable = false;
    }
}

CAbility_HoldingKnife* CAbility_HoldingKnife::Create()
{
    return new CAbility_HoldingKnife();
}

void CAbility_HoldingKnife::Free()
{
    __super::Free();
}

CAbility_JointAttack::CAbility_JointAttack()
{
}

HRESULT CAbility_JointAttack::Late_Initialize()
{
    m_iInputActionID = ACTION_JOINT_ATTACK;

    m_pCharacter = dynamic_cast<CPlayerCharacter*>(m_ActorInfo.pCharacter);

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_JOINT_ATTACK));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION));

    //m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_GRAPPLEABLE))

    /* For Grapple Montage*/
    m_pJointAttackMontageTask = CAbilityTask_PlayMontageAndWait::Create(this, "", KEY_ANIMLAYER_DEFAULT, "");
    m_pJointAttackMontageTask->m_OnMontageFinished.AddDynamic("CAbility_JointAttack", this, &CAbility_JointAttack::OnJointAttackMontageEnded);

    m_pAdditionalJointAttackMontageTask = CAbilityTask_PlayMontageAndWait::Create(this, "", KEY_ANIMLAYER_DEFAULT, "");
    m_pAdditionalJointAttackMontageTask->m_OnMontageFinished.AddDynamic("CAbility_JointAttack", this, &CAbility_JointAttack::OnAdditionalJointAttackMontageEnded);


    return S_OK;
}

_bool CAbility_JointAttack::CanActivate_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
    if (m_pCharacter->Get_TargetMonsters().empty() && m_pCharacter->Get_KnifeTargetMonsters().empty())
        return false;

    /*set<CMonsterObject*> Monsters = m_pCharacter->Get_TargetMonsters();

    if (Monsters.empty())
        return false;

    _float fMinDistanceSq = 1e9;

    CMonsterObject* pTargetMonster = { nullptr };

    for (auto& pMonster : Monsters)
    {
        _vector vDir = pMonster->Get_TransformCom()->Get_Position() - m_pCharacter->Get_TransformCom()->Get_Position();
        _float fDistanceSq = XMVectorGetX(XMVector3LengthSq(vDir));
        if (fMinDistanceSq > fDistanceSq)
        {
            fMinDistanceSq = fDistanceSq;
            pTargetMonster = pMonster;
        }
    }

    if (fMinDistanceSq > m_fJointDistThresholdSq)
        return false;

    m_pCharacter->Set_Target(pTargetMonster);*/

    return true;
}

void CAbility_JointAttack::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    Determine_JointAttackTarget(pTriggerEventData);

    CGameObject* pTarget = m_pCharacter->Get_Target();
    CMonsterObject* pMonster = dynamic_cast<CMonsterObject*>(pTarget);
    if (nullptr == pTarget || nullptr == pMonster)
    {
        End_Ability(pActorInfo);
        return;
    }

    m_bInstantDamage = false;
    m_eChangeWeaponType = WEAPON_NONE;
    m_strJointAttackAnimKey = "";

    if (m_bIsInputPressedF)
        m_strJointAttackAnimKey = Determine_JointAttackValue(pMonster, m_bInstantDamage);
    else
        m_strJointAttackAnimKey = Determine_KnifeJointAttackValue(pMonster, m_bInstantDamage);

    if (m_strJointAttackAnimKey.empty())
    {
        End_Ability(pActorInfo);
        return;
    }

    m_pJointAttackMontageTask->Set_AnimKey(m_strJointAttackAnimKey);
    if (m_bInstantDamage)
    {
        pMonster->Take_Damage(1000.f, &m_DamageEvent, m_pCharacter, m_pCharacter);
    }

    _vector vMyPos = m_pCharacter->Get_TransformCom()->Get_Position();
    _vector vMonsterPos = pTarget->Get_TransformCom()->Get_Position();
    _vector vMonsterLook = pTarget->Get_TransformCom()->Get_State(CTransform::STATE_LOOK);

    m_pCharacter->Change_Weapon(m_eChangeWeaponType);

    m_pCharacter->Get_TransformCom()->LookAt_FixYaw(vMonsterPos);
    m_pJointAttackMontageTask->ReadyForActivation();
}

void CAbility_JointAttack::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    __super::End_Ability(pActorInfo, bWasCancelled);
    m_pASC->TryActivate_Ability("Idle");
}

void CAbility_JointAttack::OnJointAttackMontageEnded(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    if (m_ActorInfo.pCompositeArmature->Get_AnimationNameByIndex("", iAnimIndex) == string("chd0_jack_pl_1000_Stealth_Kill"))
    {
        m_pAdditionalJointAttackMontageTask->Set_AnimKey("chd0_jack_pl_1002_Stealth_Kill_end");
        m_pAdditionalJointAttackMontageTask->ReadyForActivation();
    }
    else
        End_Ability(&m_ActorInfo);
}

void CAbility_JointAttack::OnAdditionalJointAttackMontageEnded(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    End_Ability(&m_ActorInfo);
}

const string CAbility_JointAttack::Determine_JointAttackValue(CMonsterObject* pMonster, _bool& bOutInstanceDamage)
{
    string strJointAttackAnimKey = "";

    // MonsterType을 갖고와서 알맞은 로직을 결정
    int iCharacterType = m_pCharacter->Get_CharacterNumber();
    const _float fTargetDistance = XMVectorGetX(XMVector3Length(m_pCharacter->Get_TransformCom()->Get_Position() - pMonster->Get_TransformCom()->Get_Position()));

    CMonsterObject::MONSTERTYPE eMonsterType = pMonster->Get_MonsterType();
    switch (eMonsterType)
    {
    case Client::CMonsterObject::MONSTER_COMMON:
        /* Ada Wire Attack(일정 거리 이상이어야 와이어 어택을 수행함)*/
        if (iCharacterType == 8 && fTargetDistance >= m_fMiddleAttackRange)
        {
            GameplayEventData EventData;
            EventData.iUserData = WIRETARGET_MONSTER;
            EventData.pTarget = pMonster;
            EventData.EventTag = GameplayTag(KEY_EVENT_OnHookShot);

            m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_WIRE_TRIGGERABLE));
            m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_UNOCCUPIED));
            m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_OCCUPIED));
            m_pASC->TriggerAbility_FromGameplayEvent("Wire", nullptr, GameplayTag(KEY_EVENT_OnHookShot), &EventData);

            End_Ability(&m_ActorInfo);
            return "";
        }
        /* 정면 발차기(공용)*/
        else if (Is_Face2Face(m_pCharacter->Get_TransformCom()->Get_State(CTransform::STATE_LOOK), pMonster->Get_TransformCom()->Get_State(CTransform::STATE_LOOK)))
        {
            if ((fTargetDistance <= m_fCloseAttackRange) && 0 == iCharacterType)
                strJointAttackAnimKey = CGameManager::GetInstance()->Parsing_Animation(m_ActorInfo, "general_0900_fatal_VerA");
            else if (fTargetDistance <= m_fMiddleAttackRange)
                strJointAttackAnimKey = CGameManager::GetInstance()->Parsing_Animation(m_ActorInfo, "general_0902_fatal_VerB");
        }
        /* 뒤 */
        else
        {
            if (iCharacterType == 0)
            {
                if (fTargetDistance <= m_fCloseAttackRange)
                {
                    strJointAttackAnimKey = "chc0_jack_pl_0th_com_1200_Leon_backdrop";
                    m_DamageEvent.eJointPoint = JOINT_BACKDROP;
                    bOutInstanceDamage = true;
                }
                else if (fTargetDistance <= m_fMiddleAttackRange)
                {
                    strJointAttackAnimKey = CGameManager::GetInstance()->Parsing_Animation(m_ActorInfo, "general_0902_fatal_VerB");
                    bOutInstanceDamage = false;
                }
            }
            else if (iCharacterType == 8)
            {
                if (fTargetDistance <= m_fMiddleAttackRange)
                    strJointAttackAnimKey = CGameManager::GetInstance()->Parsing_Animation(m_ActorInfo, "general_0908_fatal_VerE");
                bOutInstanceDamage = false;
            }
        }
        break;
    case Client::CMonsterObject::MONSTER_GARADOR:
    {
        _bool bIsFace2Face = Is_Face2Face(m_pCharacter->Get_TransformCom()->Get_State(CTransform::STATE_LOOK), pMonster->Get_TransformCom()->Get_State(CTransform::STATE_LOOK));

        if (fTargetDistance <= m_fMiddleAttackRange && false == bIsFace2Face)
        {
            m_eChangeWeaponType = WEAPON_KNIFE;
            strJointAttackAnimKey = "chd0_jack_pl_1000_Stealth_Kill";
            bOutInstanceDamage = true;
        }
        break;
    }
    case Client::CMonsterObject::MONSTER_SADDLER:
        if (fTargetDistance <= m_fSaddlerAttackRange)
        {
            bOutInstanceDamage = true;
            strJointAttackAnimKey = "chf8_jack_pl_1000_knife_head";
            m_eChangeWeaponType = WEAPON_KNIFE;
        }
        break;
    case Client::CMonsterObject::MONSTER_NOVISTADOR:
        break;
    case Client::CMonsterObject::MONSTER_END:
        break;
    default:
        break;
    }

    m_DamageEvent.eType = DAMAGE_NORMAL;
    m_DamageEvent.fDamage = 1000.f;
    m_DamageEvent.eReactionType = REACTION_JOINTATTACK;
    m_DamageEvent.strInstigatorAnimKey = m_strJointAttackAnimKey;
    m_DamageEvent.pOverlappedComp = m_pCharacter->Get_Bound();
    // 현재로써는 F키로 누르는건 JOINTPOINT 벨류를 딱히 정하지 않아도 된듯 => 칼로 찌를때는 구분해서 주겠음
    m_DamageEvent.eJointPoint = JOINT_BACKDROP;

    return strJointAttackAnimKey;
}

const string CAbility_JointAttack::Determine_KnifeJointAttackValue(CMonsterObject* pMonster, _bool& bOutInstanceDamage)
{
    string strJointAttackAnimKey = "";
    m_eChangeWeaponType = WEAPON_KNIFE;

    // MonsterType을 갖고와서 알맞은 로직을 결정
    int iCharacterType = m_pCharacter->Get_CharacterNumber();
    const _float fTargetDistance = XMVectorGetX(XMVector3Length(m_pCharacter->Get_TransformCom()->Get_Position() - pMonster->Get_TransformCom()->Get_Position()));

    CMonsterObject::MONSTERTYPE eMonsterType = pMonster->Get_MonsterType();
    switch (eMonsterType)
    {
    case Client::CMonsterObject::MONSTER_COMMON:
    {
        if (iCharacterType == 0)
        {
            if (fTargetDistance <= m_fCloseAttackRange)
            {
                m_DamageEvent.eJointPoint = JOINT_STEALTHKILL;
                bOutInstanceDamage = true;
                CReon* pReon = dynamic_cast<CReon*>(m_pCharacter);

                CMonsterCommonObject* pCommonMonster = dynamic_cast<CMonsterCommonObject*>(pMonster);
                if (pCommonMonster)
                {
                    if (false == pCommonMonster->Get_GrappleAshley())
                        strJointAttackAnimKey = "chc0_jack_pl_0th_com_1101_knife_stealth_kill_B";
                    else
                        strJointAttackAnimKey = "chc0_jack_pl_0th_com_5921_wp5000_carry_knife_stealth_kill_B";
                }
            }
        }
    }
    break;
    case Client::CMonsterObject::MONSTER_GARADOR:
    {
        _bool bIsFace2Face = Is_Face2Face(m_pCharacter->Get_TransformCom()->Get_State(CTransform::STATE_LOOK), pMonster->Get_TransformCom()->Get_State(CTransform::STATE_LOOK));

        if (fTargetDistance <= m_fMiddleAttackRange && false == bIsFace2Face)
        {
            m_eChangeWeaponType = WEAPON_KNIFE;
            strJointAttackAnimKey = "chd0_jack_pl_1000_Stealth_Kill";
            bOutInstanceDamage = true;
        }
        break;
    }
    case Client::CMonsterObject::MONSTER_SADDLER:
        if (fTargetDistance <= m_fSaddlerAttackRange)
        {
            bOutInstanceDamage = true;
            strJointAttackAnimKey = "chf8_jack_pl_1000_knife_head";
            m_eChangeWeaponType = WEAPON_KNIFE;
        }
        break;
    case Client::CMonsterObject::MONSTER_NOVISTADOR:
        break;
    case Client::CMonsterObject::MONSTER_END:
        break;
    default:
        break;
    }

    m_DamageEvent.eType = DAMAGE_NORMAL;
    m_DamageEvent.fDamage = 1000.f;
    m_DamageEvent.eReactionType = REACTION_JOINTATTACK;
    m_DamageEvent.strInstigatorAnimKey = m_strJointAttackAnimKey;
    m_DamageEvent.pOverlappedComp = m_pCharacter->Get_Bound();

    return strJointAttackAnimKey;
}

_bool CAbility_JointAttack::Is_Face2Face(_fvector vSrcLook, _fvector vDstLook)
{
    return XMVectorGetX(XMVector3Dot(XMVector3Normalize(vSrcLook), XMVector3Normalize(vDstLook))) <= 0.25f;
}

_bool CAbility_JointAttack::Is_Face2Face(_fvector vSrcLook, _fvector vDstLook, _float fToleranceAngle)
{
    return XMVectorGetX(XMVector3Dot(XMVector3Normalize(vSrcLook), XMVector3Normalize(vDstLook))) <= fToleranceAngle;
}

void CAbility_JointAttack::Determine_JointAttackTarget(const GameplayEventData* pTriggerEventData)
{
    if (nullptr == pTriggerEventData)
        return;

    if (pTriggerEventData->EventTag == GameplayTag(KEY_EVENT_JOINT_ATTACK_INPUT_F))
    {
        m_bIsInputPressedF = true;
        Select_TargetMonster(m_pCharacter->Get_TargetMonsters());
    }
    else if (pTriggerEventData->EventTag == GameplayTag(KEY_EVENT_JOINT_ATTACK_INPUT_LB))
    {
        m_bIsInputPressedF = false;
        Select_TargetMonster(m_pCharacter->Get_KnifeTargetMonsters());
    }
}

void CAbility_JointAttack::Select_TargetMonster(const set<CMonsterObject*>& TargetMonsters)
{
    if (TargetMonsters.empty())
        return;

    _float fMinDistanceSq = 1e9;

    CMonsterObject* pTargetMonster = { nullptr };

    for (auto& pMonster : TargetMonsters)
    {
        _vector vDir = pMonster->Get_TransformCom()->Get_Position() - m_pCharacter->Get_TransformCom()->Get_Position();
        _float fDistanceSq = XMVectorGetX(XMVector3LengthSq(vDir));
        if (fMinDistanceSq > fDistanceSq)
        {
            fMinDistanceSq = fDistanceSq;
            pTargetMonster = pMonster;
        }
    }

    if (fMinDistanceSq > m_fJointDistThresholdSq)
        return;

    m_pCharacter->Set_Target(pTargetMonster);
}

CAbility_JointAttack* CAbility_JointAttack::Create()
{
    return new CAbility_JointAttack();
}

void CAbility_JointAttack::Free()
{
    __super::Free();
}
