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

// ======================
// 생성이후 어빌리티 초기화 관련 기능 수행
// ======================
HRESULT CAbility_Aim::Late_Initialize()
{
    // 클라이언트에서 만든 캐릭터 포인터 부모형으로 다운 캐스팅
    m_pCharacter = dynamic_cast<CPlayerCharacter*>(m_ActorInfo.pCharacter);

    // 인풋 액션 바인딩 키 지정
    m_iInputActionID = ACTION_AIM;

    // 어빌리티 태그 지정
    m_AbilityTags.AddTag(GameplayTag(KEY_STATE_AIMING));
    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_AIMING));

    // 어빌리티 발동 시 소유할 태그 지정
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_AIMING));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_AIMING));

    // 어빌리티가 발동될 때 취소시킬 어빌리티 태그 지정
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_CROUCHING));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_DASH));

    // 어빌리티를 발동하는데 요구할 태그 지정
    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    // 어빌리티를 발동을 막는 태그 지정
    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));
    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_RELOAD));

    // 다른 어빌리티 발동을 차단하는 태그 지정
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_STATE_DASH));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_STATE_CROUCHING));

    // 애니메이션 몽타주 태스크 생성(총을 드는 애님 몽타주 재생용)
    m_pHoldStartMontageTask = CAbilityTask_PlayMontageAndWait::Create(this, "", KEY_ANIMLAYER_UPPER,
        "");

    // 몽타주 태스크의 델리게이트에 함수 매핑 => 몽타주 종료시 알림 받기 위함
    m_pHoldStartMontageTask->m_OnMontageFinished.AddDynamic("CAbility_Aim", this, &CAbility_Aim::OnMontageTaskEndReceived);

    // 에임 오프셋을 적용하기 위한 태스크
    m_pRotatePitchTask = CAbilityTask_LookCamPitch::Create(this, "Spine_2", 3.f, 60.f, 20.f);

    // 캐릭터를 카메라 방향으로 고정시키기 위한 태스크
    m_pTurnTask = CAT_FixDirection::Create(this, 10.f);

    // 라이플 같은 무기에서 키 인풋을 통한 줌 기능을 수행하기 위한 태스크
    m_pInputTask = CAbilityTask_InputBinding::Create(this, DIK_V);
    m_pInputTask->m_OnInputPressed.AddDynamic("CAbility_Aim", this, &CAbility_Aim::OnAltPressed);

    // 에이밍 도중 몬스터에게 관련 이벤트를 전달하기 위한 태스크
    m_pSendMonsterDodgeEventTask = CAbilityTask_SendMonsterDodgeEvent::Create(this);

    // 권총의 에이밍 도중 레이저 사이트 이펙트를 적용하기 위한 태스크
    m_pApplyLaserPointTask = CAbilityTask_ApplyLaserPoint::Create(this, m_pCharacter);

    // 수류탄 에이밍 도중 궤적 표시를 하기 위한 태스크
    m_pApplyGrenadeRebornTask = CAbilityTask_ApplyGrenadeReborn::Create(this, m_pCharacter);

    /* 지연 생성되는 Shoot 어빌리티를 찾음
    탄퍼짐 태스크를 생성 및 Shoot 어빌리티를 전달해서 발사시 이벤트 전달받도록 함 */
    m_pGameInstance->Add_Timer("CAbility_Aim", 0.f, [&]()
        {
            CAbility_Shoot* pShootAbility = dynamic_cast<CAbility_Shoot*>(m_pASC->Find_Ability("Shoot"));
            if (pShootAbility)
                m_pAimSpreadTask = CAbilityTask_UpdateAimSpread::Create(this, pShootAbility);
        });


    return S_OK;
}

// ======================
// 어빌리티를 트리거할 때 태그 조건 체킹을 하기 전 불려지는 함수
// 유효한 무기를 들고있지 않다면 어빌리티 발동 실패
// ======================
_bool CAbility_Aim::CanActivate_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
    /* 현재 소지하고 있는 무기가 나이프 or 아무것도 안들고있는 상태에서 Hide한 WeaponType마저 없었다면 return false
      HideWeapon: 지형 낙하나 체술같은 이벤트 때문에 잠시 무기가 손에서 사라진 상태를 나타내기 위한 변수 */
    if (m_pCharacter->Get_WeaponType() == WEAPON_NONE || m_pCharacter->Get_WeaponType() == WEAPON_KNIFE)
    {
        if (m_pCharacter->Get_HideWeaponType() == WEAPON_NONE || m_pCharacter->Get_HideWeaponType() == WEAPON_KNIFE)
            return false;
    }

    return true;
}

// ======================
// 어빌리티 발동시 한 틱 호출되는 함수
// ======================
void CAbility_Aim::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_bStarted = false;

    // 현재 무기를 숨기고 있는 상태라면 숨긴 무기를 복귀시키는 어빌리티 수행
    if (m_pCharacter->Is_HidingWeapon())
    {
        // EventData를 통해 어빌리티를 트리거해서 컨텍스트를 전달시킴
        GameplayEventData EventData;
        EventData.EventTag = GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_AIMING);
        m_pASC->TriggerAbility_FromGameplayEvent("ChangeWeapon", nullptr, GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_AIMING), &EventData);
        End_Ability(pActorInfo);
    }
    else
    {
        OnStartActivate();
    }
}

// ======================
// 바인딩된 키의 인풋을 뗄 때 발동되는 함수
// 마우스 우클릭을 떼면 발동함
// ======================
void CAbility_Aim::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{
    // 조준 종료 애니메이션 수행 
    m_ActorInfo.pAnimInstance->Play_Montage("", KEY_ANIMLAYER_UPPER, CGameManager::Parsing_Animation(m_ActorInfo, Parsing_WeaponName(m_pCharacter->Get_WeaponType()) + "0158_hold_end"));

    // 애니메이션 상태 머신에 자신의 상태로 재전이를 하도록 시킴 => 총을 들고 있지 않은 애니메이션 상태로 복귀 시킴 
    m_ActorInfo.pAnimInstance->Re_Enter_CurrentState();

    // 캐싱 포즈로 팔 애니메이션을 고정시키기 위함(총을 가만히 들고있는 애니메이션 에셋이 없어 재장전 중인 애니메이션에서 포즈를 캐싱 및 적용해서 팔에 무기를 자연스럽게 붙임)
    Hold_Weapon(m_pASC, "0158_hold_end", true);
    
    // 어빌리티 종료
    End_Ability(pActorInfo, false);
}

void CAbility_Aim::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    __super::End_Ability(pActorInfo, bWasCancelled);

    CCamera_Manager::GetInstance()->On_Aiming(m_pCharacter->Get_WeaponType(), false);

#ifdef LOAD_UI
    CGameManager::GetInstance()->Set_MousePointer(false);
#endif
}

void CAbility_Aim::OnMontageTaskEndReceived(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    // 아무런 이벤트 수행하지 않음
}

// ======================
// InputTask에 의한 함수 호출
// 줌 변경 기능 수행
// ======================
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
    WeaponType eWeaponType = m_pCharacter->Get_WeaponType();

    CCamera_Manager::GetInstance()->On_Aiming(eWeaponType, true, 0.5f);

#ifdef LOAD_UI
    CGameManager::GetInstance()->Set_MousePointer(true);
#endif

    // 몬스터에게 회피 이벤트 전달 태스크 호출
    m_pSendMonsterDodgeEventTask->ReadyForActivation();

    // 이펙트 관련 태스크 호출
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

    // 플레이어를 카메라 방향으로 세팅하고 에니메이션 상태를 재진입하게 만듦(조준상태의 에니메이션으로 전환) 
    m_pCharacter->Get_TransformCom()->Rotation_Dir(CMyUtils::Get_CamDirectionByDIR(DIR_FRONT));
    m_ActorInfo.pAnimInstance->Re_Enter_CurrentState();

    // 줌 상태 변경을 체킹하기 위한 태스크 호출 
    m_iZoomLevel = 0;
    m_pInputTask->ReadyForActivation();

    // Hold Start Montage 호출
    m_pHoldStartMontageTask->Set_AnimKey(CGameManager::Parsing_Animation(m_ActorInfo.pCharacter->Get_CharacterNumber(), Parsing_WeaponName(m_pCharacter->Get_WeaponType()) + "0140_hold_start"));
    m_pHoldStartMontageTask->ReadyForActivation();

    // 플레이어 회전 고정/에임오프셋 태스크 호출 
    m_pTurnTask->ReadyForActivation();
    m_pRotatePitchTask->ReadyForActivation();

    // 탄 퍼짐 태스크 호출 
    m_pAimSpreadTask->Set_WeaponType(m_pCharacter->Get_WeaponType());
    m_pAimSpreadTask->ReadyForActivation();

    /* 무기를 잡고있는 애니메이션 상태 제거 */
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
