#include "AbilityTask_Tick.h"
#include "MovePointLight.h"
#include "GameplayAbilitySystemHeader.h"
#include "MonsterObject.h"
#include "Laser.h"

CAbilityTask_UpdateAimSpread::CAbilityTask_UpdateAimSpread()
{
    // Task Tick 기능 활성화
    m_bTickingTask = true;

    // 탄퍼짐 계수와 최소 탄퍼짐량 저장
    m_SpreadMultipliers[WEAPON_PISTOL] = 1.f;
    m_MinSpreads[WEAPON_PISTOL] = 0.13f;

    m_SpreadMultipliers[WEAPON_SHOTGUN] = 3.f;
    m_MinSpreads[WEAPON_SHOTGUN] = 0.33f;

    m_SpreadMultipliers[WEAPON_RIFLE] = 0.f;
    m_MinSpreads[WEAPON_RIFLE] = 0.f;

    m_SpreadMultipliers[WEAPON_SMG] = 1.5f;
    m_MinSpreads[WEAPON_SMG] = 0.22f;
}

// ======================
// 어빌리티에 의해 ReadyForActivation 호출시 한 틱 발동되는 함수
// ======================
void CAbilityTask_UpdateAimSpread::Activate()
{
    // 탄 퍼짐 어트리뷰트 찾기
    _float fAimSpread;
    _bool bFound;
    fAimSpread = m_pAbilitySystemComponent->GetGameplayAttributeValue("fAimSpread", bFound);
    if (false == bFound)
    {
        End_Task();
        return;
    }

    // 어빌리티 발동시 m_eWeaponType Set함수로 주입받음
    m_fSpreadMultiplier = m_SpreadMultipliers[m_eWeaponType];
    m_fMinSpread = m_MinSpreads[m_eWeaponType];

    // 플레이어가 걷고있는 상태였다면 탄퍼짐량 최대
    if (m_pAbilitySystemComponent->Has_MatchingGameplayTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_WALK)))
    {
        m_fFinalSpread = m_fMaxSpread;
    }
}

// ======================
// 태스크가 활성화상태일 때 매 틱 호출되는 함수
// ======================
void CAbilityTask_UpdateAimSpread::Tick_Task(_float fTimeDelta)
{
    // 걷고있는 중이라면 탄퍼짐 증가
    if (m_pAbilitySystemComponent->Has_MatchingGameplayTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_WALK)))
    {
        m_fFinalSpread += m_fWalkSpreadRate * fTimeDelta;
    }
    // 탄퍼짐 회복 가능하면 탄퍼짐 감소
    else if (m_bCanRecoverSpread)
    {
        m_fFinalSpread -= m_fRecoverRate * fTimeDelta;
    }

    m_fFinalSpread = clamp(m_fFinalSpread, m_fMinSpread, m_fMaxSpread);

    m_pAbilitySystemComponent->SetNumericAttributeBase("fAimSpread", m_fFinalSpread * m_fSpreadMultiplier);
}

// ======================
// 어빌리티 초기화 이후 불려지는 함수
// Shoot Ability의 델리게이트에 함수 바인딩 수행
// ======================
void CAbilityTask_UpdateAimSpread::Init_Task(IGameplayAbility& ThisAbility)
{
    __super::Init_Task(ThisAbility);

    m_pShootAbility->m_OnShootDelegate.AddDynamic("CAbilityTask_UpdateAimSpread", this, &CAbilityTask_UpdateAimSpread::OnShootEventRecevied);
}

// ======================
// 어빌리티 종료시 발동되는 함수
// ======================
void CAbilityTask_UpdateAimSpread::OnDestroy(bool bInOwnerFinished)
{
    m_ActorInfo.pCompositeArmature->Deactivate_AnimLayer("", "Add_Breath");
}

// ======================
// 발사후 ShootAbility의 델리게이트에 의해 호출되는 함수
// 1초동안 탄퍼짐 회복 금지
// ======================
void CAbilityTask_UpdateAimSpread::OnShootEventRecevied(_uint iWeaponType)
{
    m_bCanRecoverSpread = false;
    m_fFinalSpread = m_fMaxSpread;
    m_pGameInstance->Add_Timer("CAbilityTask_UpdateAimSpread", 1.f, [&]() 
        {
            m_bCanRecoverSpread = true; 
        });
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