#include "AbilityTask_Tick.h"
#include "MovePointLight.h"
#include "GameplayAbilitySystemHeader.h"
#include "MonsterObject.h"
#include "Laser.h"

CAbilityTask_UpdateAimSpread::CAbilityTask_UpdateAimSpread()
{
    // Task Tick ��� Ȱ��ȭ
    m_bTickingTask = true;

    // ź���� ����� �ּ� ź������ ����
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
// �����Ƽ�� ���� ReadyForActivation ȣ��� �� ƽ �ߵ��Ǵ� �Լ�
// ======================
void CAbilityTask_UpdateAimSpread::Activate()
{
    // ź ���� ��Ʈ����Ʈ ã��
    _float fAimSpread;
    _bool bFound;
    fAimSpread = m_pAbilitySystemComponent->GetGameplayAttributeValue("fAimSpread", bFound);
    if (false == bFound)
    {
        End_Task();
        return;
    }

    // �����Ƽ �ߵ��� m_eWeaponType Set�Լ��� ���Թ���
    m_fSpreadMultiplier = m_SpreadMultipliers[m_eWeaponType];
    m_fMinSpread = m_MinSpreads[m_eWeaponType];

    // �÷��̾ �Ȱ��ִ� ���¿��ٸ� ź������ �ִ�
    if (m_pAbilitySystemComponent->Has_MatchingGameplayTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_WALK)))
    {
        m_fFinalSpread = m_fMaxSpread;
    }
}

// ======================
// �½�ũ�� Ȱ��ȭ������ �� �� ƽ ȣ��Ǵ� �Լ�
// ======================
void CAbilityTask_UpdateAimSpread::Tick_Task(_float fTimeDelta)
{
    // �Ȱ��ִ� ���̶�� ź���� ����
    if (m_pAbilitySystemComponent->Has_MatchingGameplayTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_WALK)))
    {
        m_fFinalSpread += m_fWalkSpreadRate * fTimeDelta;
    }
    // ź���� ȸ�� �����ϸ� ź���� ����
    else if (m_bCanRecoverSpread)
    {
        m_fFinalSpread -= m_fRecoverRate * fTimeDelta;
    }

    m_fFinalSpread = clamp(m_fFinalSpread, m_fMinSpread, m_fMaxSpread);

    m_pAbilitySystemComponent->SetNumericAttributeBase("fAimSpread", m_fFinalSpread * m_fSpreadMultiplier);
}

// ======================
// �����Ƽ �ʱ�ȭ ���� �ҷ����� �Լ�
// Shoot Ability�� ��������Ʈ�� �Լ� ���ε� ����
// ======================
void CAbilityTask_UpdateAimSpread::Init_Task(IGameplayAbility& ThisAbility)
{
    __super::Init_Task(ThisAbility);

    m_pShootAbility->m_OnShootDelegate.AddDynamic("CAbilityTask_UpdateAimSpread", this, &CAbilityTask_UpdateAimSpread::OnShootEventRecevied);
}

// ======================
// �����Ƽ ����� �ߵ��Ǵ� �Լ�
// ======================
void CAbilityTask_UpdateAimSpread::OnDestroy(bool bInOwnerFinished)
{
    m_ActorInfo.pCompositeArmature->Deactivate_AnimLayer("", "Add_Breath");
}

// ======================
// �߻��� ShootAbility�� ��������Ʈ�� ���� ȣ��Ǵ� �Լ�
// 1�ʵ��� ź���� ȸ�� ����
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