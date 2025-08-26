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
// �������� �����Ƽ �ʱ�ȭ ���� ��� ����
// ======================
HRESULT CAbility_Aim::Late_Initialize()
{
    // Ŭ���̾�Ʈ���� ���� ĳ���� ������ �θ������� �ٿ� ĳ����
    m_pCharacter = dynamic_cast<CPlayerCharacter*>(m_ActorInfo.pCharacter);

    // ��ǲ �׼� ���ε� Ű ����
    m_iInputActionID = ACTION_AIM;

    // �����Ƽ �±� ����
    m_AbilityTags.AddTag(GameplayTag(KEY_STATE_AIMING));
    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_AIMING));

    // �����Ƽ �ߵ� �� ������ �±� ����
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_AIMING));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_AIMING));

    // �����Ƽ�� �ߵ��� �� ��ҽ�ų �����Ƽ �±� ����
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_CROUCHING));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_DASH));

    // �����Ƽ�� �ߵ��ϴµ� �䱸�� �±� ����
    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    // �����Ƽ�� �ߵ��� ���� �±� ����
    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));
    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_RELOAD));

    // �ٸ� �����Ƽ �ߵ��� �����ϴ� �±� ����
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_STATE_DASH));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_STATE_CROUCHING));

    // �ִϸ��̼� ��Ÿ�� �½�ũ ����(���� ��� �ִ� ��Ÿ�� �����)
    m_pHoldStartMontageTask = CAbilityTask_PlayMontageAndWait::Create(this, "", KEY_ANIMLAYER_UPPER,
        "");

    // ��Ÿ�� �½�ũ�� ��������Ʈ�� �Լ� ���� => ��Ÿ�� ����� �˸� �ޱ� ����
    m_pHoldStartMontageTask->m_OnMontageFinished.AddDynamic("CAbility_Aim", this, &CAbility_Aim::OnMontageTaskEndReceived);

    // ���� �������� �����ϱ� ���� �½�ũ
    m_pRotatePitchTask = CAbilityTask_LookCamPitch::Create(this, "Spine_2", 3.f, 60.f, 20.f);

    // ĳ���͸� ī�޶� �������� ������Ű�� ���� �½�ũ
    m_pTurnTask = CAT_FixDirection::Create(this, 10.f);

    // ������ ���� ���⿡�� Ű ��ǲ�� ���� �� ����� �����ϱ� ���� �½�ũ
    m_pInputTask = CAbilityTask_InputBinding::Create(this, DIK_V);
    m_pInputTask->m_OnInputPressed.AddDynamic("CAbility_Aim", this, &CAbility_Aim::OnAltPressed);

    // ���̹� ���� ���Ϳ��� ���� �̺�Ʈ�� �����ϱ� ���� �½�ũ
    m_pSendMonsterDodgeEventTask = CAbilityTask_SendMonsterDodgeEvent::Create(this);

    // ������ ���̹� ���� ������ ����Ʈ ����Ʈ�� �����ϱ� ���� �½�ũ
    m_pApplyLaserPointTask = CAbilityTask_ApplyLaserPoint::Create(this, m_pCharacter);

    // ����ź ���̹� ���� ���� ǥ�ø� �ϱ� ���� �½�ũ
    m_pApplyGrenadeRebornTask = CAbilityTask_ApplyGrenadeReborn::Create(this, m_pCharacter);

    /* ���� �����Ǵ� Shoot �����Ƽ�� ã��
    ź���� �½�ũ�� ���� �� Shoot �����Ƽ�� �����ؼ� �߻�� �̺�Ʈ ���޹޵��� �� */
    m_pGameInstance->Add_Timer("CAbility_Aim", 0.f, [&]()
        {
            CAbility_Shoot* pShootAbility = dynamic_cast<CAbility_Shoot*>(m_pASC->Find_Ability("Shoot"));
            if (pShootAbility)
                m_pAimSpreadTask = CAbilityTask_UpdateAimSpread::Create(this, pShootAbility);
        });


    return S_OK;
}

// ======================
// �����Ƽ�� Ʈ������ �� �±� ���� üŷ�� �ϱ� �� �ҷ����� �Լ�
// ��ȿ�� ���⸦ ������� �ʴٸ� �����Ƽ �ߵ� ����
// ======================
_bool CAbility_Aim::CanActivate_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
    /* ���� �����ϰ� �ִ� ���Ⱑ ������ or �ƹ��͵� �ȵ���ִ� ���¿��� Hide�� WeaponType���� �����ٸ� return false
      HideWeapon: ���� ���ϳ� ü������ �̺�Ʈ ������ ��� ���Ⱑ �տ��� ����� ���¸� ��Ÿ���� ���� ���� */
    if (m_pCharacter->Get_WeaponType() == WEAPON_NONE || m_pCharacter->Get_WeaponType() == WEAPON_KNIFE)
    {
        if (m_pCharacter->Get_HideWeaponType() == WEAPON_NONE || m_pCharacter->Get_HideWeaponType() == WEAPON_KNIFE)
            return false;
    }

    return true;
}

// ======================
// �����Ƽ �ߵ��� �� ƽ ȣ��Ǵ� �Լ�
// ======================
void CAbility_Aim::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_bStarted = false;

    // ���� ���⸦ ����� �ִ� ���¶�� ���� ���⸦ ���ͽ�Ű�� �����Ƽ ����
    if (m_pCharacter->Is_HidingWeapon())
    {
        // EventData�� ���� �����Ƽ�� Ʈ�����ؼ� ���ؽ�Ʈ�� ���޽�Ŵ
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
// ���ε��� Ű�� ��ǲ�� �� �� �ߵ��Ǵ� �Լ�
// ���콺 ��Ŭ���� ���� �ߵ���
// ======================
void CAbility_Aim::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{
    // ���� ���� �ִϸ��̼� ���� 
    m_ActorInfo.pAnimInstance->Play_Montage("", KEY_ANIMLAYER_UPPER, CGameManager::Parsing_Animation(m_ActorInfo, Parsing_WeaponName(m_pCharacter->Get_WeaponType()) + "0158_hold_end"));

    // �ִϸ��̼� ���� �ӽſ� �ڽ��� ���·� �����̸� �ϵ��� ��Ŵ => ���� ��� ���� ���� �ִϸ��̼� ���·� ���� ��Ŵ 
    m_ActorInfo.pAnimInstance->Re_Enter_CurrentState();

    // ĳ�� ����� �� �ִϸ��̼��� ������Ű�� ����(���� ������ ����ִ� �ִϸ��̼� ������ ���� ������ ���� �ִϸ��̼ǿ��� ��� ĳ�� �� �����ؼ� �ȿ� ���⸦ �ڿ������� ����)
    Hold_Weapon(m_pASC, "0158_hold_end", true);
    
    // �����Ƽ ����
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
    // �ƹ��� �̺�Ʈ �������� ����
}

// ======================
// InputTask�� ���� �Լ� ȣ��
// �� ���� ��� ����
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

    // ���Ϳ��� ȸ�� �̺�Ʈ ���� �½�ũ ȣ��
    m_pSendMonsterDodgeEventTask->ReadyForActivation();

    // ����Ʈ ���� �½�ũ ȣ��
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

    // �÷��̾ ī�޶� �������� �����ϰ� ���ϸ��̼� ���¸� �������ϰ� ����(���ػ����� ���ϸ��̼����� ��ȯ) 
    m_pCharacter->Get_TransformCom()->Rotation_Dir(CMyUtils::Get_CamDirectionByDIR(DIR_FRONT));
    m_ActorInfo.pAnimInstance->Re_Enter_CurrentState();

    // �� ���� ������ üŷ�ϱ� ���� �½�ũ ȣ�� 
    m_iZoomLevel = 0;
    m_pInputTask->ReadyForActivation();

    // Hold Start Montage ȣ��
    m_pHoldStartMontageTask->Set_AnimKey(CGameManager::Parsing_Animation(m_ActorInfo.pCharacter->Get_CharacterNumber(), Parsing_WeaponName(m_pCharacter->Get_WeaponType()) + "0140_hold_start"));
    m_pHoldStartMontageTask->ReadyForActivation();

    // �÷��̾� ȸ�� ����/���ӿ����� �½�ũ ȣ�� 
    m_pTurnTask->ReadyForActivation();
    m_pRotatePitchTask->ReadyForActivation();

    // ź ���� �½�ũ ȣ�� 
    m_pAimSpreadTask->Set_WeaponType(m_pCharacter->Get_WeaponType());
    m_pAimSpreadTask->ReadyForActivation();

    /* ���⸦ ����ִ� �ִϸ��̼� ���� ���� */
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
