#include "Ability_Action.h"
#include "GameplayAbilitySystemHeader.h"
#include "FlashLight.h"
#include "Ada.h"
#include "Wire.h"
#include "MonsterObject.h"
#include "DestructibleLootBox.h"
#include "BehaviorTreeHeader.h"
#include "MonsterCommonObject.h"

IAbility_Action::IAbility_Action()
{
}

HRESULT IAbility_Action::Late_Initialize()
{
    m_pPlayerCharacter = dynamic_cast<CPlayerCharacter*>(m_ActorInfo.pCharacter);
    
    m_pWeaponKey = m_ActorInfo.pContext->Get_Variable<_int*>("WeaponKey");

    return S_OK;
}

void IAbility_Action::Free()
{
    __super::Free();
}

CAbility_ChangeWeapon::CAbility_ChangeWeapon()
{
}

HRESULT CAbility_ChangeWeapon::Late_Initialize()
{
    __super::Late_Initialize();

    Load_AnimNames();

    m_iInputActionID = ACTION_CHANGE_WEAPON;

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_CHANGE));

    //m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_CHANGE));

    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_STATE_AIMING));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_HOLDING));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_STATE_CROUCHING));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_RELOAD));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_HOLDING));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_pWeaponKey = m_ActorInfo.pContext->Get_Variable<_int*>("WeaponKey");

    m_pPutAwayMontageTask = CAbilityTask_PlayMontageAndWait::Create(this,
        "", KEY_ANIMLAYER_UPPER, CGameManager::Parsing_Animation(m_ActorInfo, ""));
    m_pPutAwayMontageTask->m_OnMontageFinished.AddDynamic("CAbility_ChangeWeapon", this, &CAbility_ChangeWeapon::OnPutAwayMontageTaskEndReceived);

    m_pPutOutMontageTask = CAbilityTask_PlayMontageAndWait::Create(this,
        "", KEY_ANIMLAYER_UPPER, CGameManager::Parsing_Animation(m_ActorInfo, ""));
    m_pPutOutMontageTask->m_OnMontageFinished.AddDynamic("CAbility_ChangeWeapon", this, &CAbility_ChangeWeapon::OnPutOutMontageTaskEndReceived);

    return S_OK;
}

void CAbility_ChangeWeapon::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_bWeaponChanged = false;
    m_bRevertHideWeapon = false;

    /* 칼 Ready 에니메이션이 끝나거나 에임 어빌리티를 시작할 때 ChangeWeapon어빌리티를 트리거 시킴 => 숨겨져있던 무기 타입을 복구 시켜야 하는 상황임 */
    if (pTriggerEventData && (pTriggerEventData->EventTag == GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_READY) || pTriggerEventData->EventTag == GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_AIMING)))
    {
        m_bRevertHideWeapon = true;
    }

    /* 기본적으로 PutAway 몽타주를 먼저 수행한 후 그 다음 PutOut 몽타주 시작함. => PutOut 몽타주 시작 시 무기 교체 로직 수행 */
    if (m_PutAwayAnims[m_pPlayerCharacter->Get_WeaponType()].empty())
        Putout_Weapon();
    else
    {
        m_pPutAwayMontageTask->Set_AnimKey(m_PutAwayAnims[m_pPlayerCharacter->Get_WeaponType()]);
        m_pPutAwayMontageTask->ReadyForActivation();
    }
}

void CAbility_ChangeWeapon::InputPressed(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_ChangeWeapon::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{

}

void CAbility_ChangeWeapon::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_SHOOTABLE));

    if (false == m_bWeaponChanged)
        m_pPlayerCharacter->Change_Weapon(m_pPlayerCharacter->Get_ChangeWeaponType(), m_bRevertHideWeapon);

    __super::End_Ability(pActorInfo, bWasCancelled);

    // 에임키에 의해서 ChangeWeapon이 수행되었을 경우 다시 에임 어빌리티로 돌아가게 하기 위해서 마우스 우클릭 눌렀는지 체킹함
    if (m_pGameInstance->Mouse_Pressing(MOUSEKEYSTATE::DIM_RB))
        m_pASC->TryActivate_Ability("Aim");
}

void CAbility_ChangeWeapon::Load_AnimNames()
{
    m_PutAwayAnims[WEAPON_PISTOL] = CGameManager::Parsing_Animation(m_ActorInfo, "wp4000H_0551_put_away");
    m_PutAwayAnims[WEAPON_SHOTGUN] = CGameManager::Parsing_Animation(m_ActorInfo, "wp4100H_0551_put_away");
    /* 라이플은 PutAway가 없음 */
    m_PutAwayAnims[WEAPON_RIFLE] = CGameManager::Parsing_Animation(m_ActorInfo, "wp4100H_0551_put_away");
    m_PutAwayAnims[WEAPON_SMG] = CGameManager::Parsing_Animation(m_ActorInfo, "wp4200H_0551_put_away");
    m_PutAwayAnims[WEAPON_KNIFE] = CGameManager::Parsing_Animation(m_ActorInfo, "wp5000H_0551_put_away");
    m_PutAwayAnims[WEAPON_ROCKET_LAUNCHER] = CGameManager::Parsing_Animation(m_ActorInfo, "wp4900H_0551_put_away");
    m_PutAwayAnims[WEAPON_SMG] = CGameManager::Parsing_Animation(m_ActorInfo, "wp4100H_0551_put_away");

    m_PutOutAnims[WEAPON_PISTOL] = CGameManager::Parsing_Animation(m_ActorInfo, "wp4000H_0541_put_out");
    m_PutOutAnims[WEAPON_SHOTGUN] = CGameManager::Parsing_Animation(m_ActorInfo, "wp4100H_0541_put_out");
    m_PutOutAnims[WEAPON_SMG] = CGameManager::Parsing_Animation(m_ActorInfo, "wp4101H_0541_put_out");
    m_PutOutAnims[WEAPON_RIFLE] = CGameManager::Parsing_Animation(m_ActorInfo, "wp4400H_0543_put_out");
    m_PutAwayAnims[WEAPON_ROCKET_LAUNCHER] = CGameManager::Parsing_Animation(m_ActorInfo, "wp4900H_0541_put_out");
    // 나이프는 풋아웃이 없음
    m_PutOutAnims[WEAPON_KNIFE] = CGameManager::Parsing_Animation(m_ActorInfo, "wp5000H_0551_put_away");
    m_PutOutAnims[WEAPON_GRENADE] = CGameManager::Parsing_Animation(m_ActorInfo, "wp5400H_0541_put_out");
    m_PutOutAnims[WEAPON_FLASHBANG] = CGameManager::Parsing_Animation(m_ActorInfo, "wp5400H_0541_put_out");
}

void CAbility_ChangeWeapon::OnPutAwayMontageTaskEndReceived(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    Putout_Weapon();
}

void CAbility_ChangeWeapon::OnPutOutMontageTaskEndReceived(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
}

void CAbility_ChangeWeapon::Putout_Weapon()
{
    WeaponType eChangeWeaponType = m_pPlayerCharacter->Get_ChangeWeaponType();

    /*m_ActorInfo.pCompositeArmature->Select_Animation(Parsing_WeaponParts(eChangeWeaponType), Parsing_WeaponNameNoneSuffix(eChangeWeaponType) + "general_0543_put_out");*/

    m_pPlayerCharacter->Change_Weapon(m_pPlayerCharacter->Get_ChangeWeaponType(), m_bRevertHideWeapon);
    Hold_Weapon(m_pASC, "0158_hold_end", true);

    m_bWeaponChanged = true;

    // 델리게이트 사이에 연쇄적으로 몽타주가 호출되는 걸 막기 위함
    m_pGameInstance->Add_Timer("CAbility_ChangeWeapon", 0.0f, [&]()
        {
            //m_pPutOutMontageTask->Set_AnimKey(m_PutOutAnims[m_pPlayerCharacter->Get_WeaponType()]);
            //m_pPutOutMontageTask->ReadyForActivation();

            m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_UPPER, m_PutOutAnims[m_pPlayerCharacter->Get_WeaponType()]);
            End_Ability(&m_ActorInfo);
            //m_ActorInfo.pAnimInstance->Play_Montage("", KEY_ANIMLAYER_UPPER, m_PutOutAnims[m_pPlayerCharacter->Get_WeaponType()]);
        }
    );
}

CAbility_ChangeWeapon* CAbility_ChangeWeapon::Create()
{
    return new CAbility_ChangeWeapon();
}

void CAbility_ChangeWeapon::Free()
{
    __super::Free();
}

CAbility_Reload::CAbility_Reload()
{
}

HRESULT CAbility_Reload::Late_Initialize()
{
    __super::Late_Initialize();

    m_iInputActionID = ACTION_RELOAD;

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_RELOAD));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_RELOAD));

    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_STATE_AIMING));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_READY));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_CROUCHING));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN_AIMING));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_pReloadStartMotange = CAbilityTask_PlayMontageAndWait::Create(this,
        "", "Upper_Priority", CGameManager::Parsing_Animation(m_ActorInfo, "wp4100H_0810_reload_loop"));
    m_pReloadStartMotange->m_OnMontageFinished.AddDynamic("CAbility_Reload", this, &CAbility_Reload::OnReloadStartMontageTaskEndReceived);

    /* 에니메이션 노티파이 이벤트로 장전이 끝나는 타이밍을 전달 받음!(라이플, 샷건같은 Reload Loop 에니메이션이 있는 아이 한정) */
    AbilityTriggerData TriggerData;
    TriggerData.TriggerTag = GameplayTag(KEY_EVENT_ON_RELOAD);

    m_AbilityTriggers.push_back(TriggerData);

    return S_OK;
}

void CAbility_Reload::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_iDebugCount = 3;

    if (false == Commit_Ability(pActorInfo))
    {
        End_Ability(pActorInfo);
        return;
    }

    WeaponType eWeaponType = m_pPlayerCharacter->Get_WeaponType();
    if (eWeaponType == WEAPON_KNIFE || eWeaponType == WEAPON_NONE)
        m_pPlayerCharacter->Change_Weapon(m_pPlayerCharacter->Get_HideWeaponType(), true);

    if (eWeaponType == WEAPON_RIFLE || eWeaponType == WEAPON_SHOTGUN)
        m_ActorInfo.pCompositeArmature->Select_Animation(Parsing_WeaponParts(eWeaponType), Parsing_WeaponNameNoneSuffix(eWeaponType) + "general_0805_Reload_Start");
    else
    {
        m_ActorInfo.pCompositeArmature->Select_Animation(Parsing_WeaponParts(eWeaponType), Parsing_WeaponNameNoneSuffix(eWeaponType) + "general_0810_Reload");
    }


    m_pReloadStartMotange->Set_AnimKey(Parsing_ReloadStartAnim(m_pPlayerCharacter->Get_WeaponType()));
    m_pReloadStartMotange->ReadyForActivation();
}

void CAbility_Reload::InputPressed(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_Reload::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{
}

_bool CAbility_Reload::CanActivate_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
    WeaponType eType = m_pPlayerCharacter->Get_WeaponType();
    switch (eType)
    {
    case Client::WEAPON_NONE:
    case Client::WEAPON_KNIFE:
        return Is_ReloadableGun(m_pPlayerCharacter->Get_HideWeaponType());
    case Client::WEAPON_PISTOL:
    case Client::WEAPON_SHOTGUN:
    case Client::WEAPON_RIFLE:
    case Client::WEAPON_MAGNUM:
    case Client::WEAPON_SMG:
        return true;
    case Client::WEAPON_GRENADE:
    case Client::WEAPON_FLASHBANG:
    case Client::WEAPON_ROCKET_LAUNCHER:
    case Client::WEAPON_END:
    default:
        return false;
    }

    return false;
}

bool CAbility_Reload::Commit_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
#ifdef LOAD_UI
    if (m_pPlayerCharacter->Get_HoldWeapon())
        return m_pPlayerCharacter->Get_HoldWeapon()->Can_Reload();
    else
        return false;
#endif

    return true;
}

void CAbility_Reload::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    if (bWasCancelled)
    {
        m_ActorInfo.pCompositeArmature->Deactivate_AnimLayer("", KEY_ANIMLAYER_UPPER);
    }

    __super::End_Ability(pActorInfo, bWasCancelled);
}

void CAbility_Reload::OnGameplayEvent(GameplayTag EventTag, const GameplayEventData* pTriggerEventData)
{
    /* Reload 에니메이션 노티파이로 전달받은 이벤트 */
    if (EventTag == GameplayTag(KEY_EVENT_ON_RELOAD))
    {
        if (OnReload())
            OnReloadEnd();
    }
}

string CAbility_Reload::Parsing_ReloadStartAnim(WeaponType eType)
{
    string strPrefix = m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_CROUCHING)) ? "1" : "0";

    if (eType == WEAPON_SHOTGUN || eType == WEAPON_RIFLE)
    {
        return CGameManager::Parsing_Animation(m_ActorInfo, Parsing_WeaponName(eType) + strPrefix + "805_reload_start");
    }
    else
        return CGameManager::Parsing_Animation(m_ActorInfo, Parsing_WeaponName(eType) + strPrefix + "810_reload");
}

string CAbility_Reload::Parsing_ReloadLoopAnim(WeaponType eType)
{
    string strPrefix = m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_CROUCHING)) ? "1" : "0";

    return CGameManager::Parsing_Animation(m_ActorInfo, Parsing_WeaponName(eType) + strPrefix + "810_reload_loop");
}

string CAbility_Reload::Parsing_ReloadEndAnim(WeaponType eType)
{
    string strPrefix = m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_CROUCHING)) ? "1" : "0";

    return CGameManager::Parsing_Animation(m_ActorInfo, Parsing_WeaponName(eType) + strPrefix + "815_reload_end");
}

void CAbility_Reload::OnReloadStartMontageTaskEndReceived(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    m_iDebugCount--;
    if (OnReload())
    {
        End_Ability(&m_ActorInfo);
        return;
    }
    else
    {
        /* Play Weapon Anim*/
        WeaponType eWeaponType = m_pPlayerCharacter->Get_WeaponType();
        m_ActorInfo.pCompositeArmature->Select_Animation(Parsing_WeaponParts(eWeaponType), Parsing_WeaponNameNoneSuffix(eWeaponType) + "general_0810_Reload_Loop");
       
        m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_UPPER, Parsing_ReloadLoopAnim(m_pPlayerCharacter->Get_WeaponType()), true);
    }
}

void CAbility_Reload::OnReloadEnd()
{
    WeaponType eWeaponType = m_pPlayerCharacter->Get_WeaponType();
    m_ActorInfo.pCompositeArmature->Select_Animation(Parsing_WeaponParts(eWeaponType), Parsing_WeaponNameNoneSuffix(eWeaponType) + "general_0815_Reload_End");

    m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_UPPER, Parsing_ReloadEndAnim(m_pPlayerCharacter->Get_WeaponType()));
    End_Ability(&m_ActorInfo);
}

_bool CAbility_Reload::Is_ReloadableGun(WeaponType eType)
{
    switch (eType)
    {
    case Client::WEAPON_NONE:
    case Client::WEAPON_KNIFE:
    case Client::WEAPON_GRENADE:
    case Client::WEAPON_FLASHBANG:
    case Client::WEAPON_END:
    case Client::WEAPON_ROCKET_LAUNCHER:
        return false;
    case Client::WEAPON_PISTOL:
    case Client::WEAPON_SHOTGUN:
    case Client::WEAPON_RIFLE:
    case Client::WEAPON_MAGNUM:
    case Client::WEAPON_SMG:
        return true;
    default:
        break;
    }

    return true;
}

_bool CAbility_Reload::OnReload()
{
    m_iDebugCount--;
#ifdef LOAD_UI
    CGameManager::GetInstance()->On_Reload();
#endif

    if (m_pPlayerCharacter->Get_WeaponType() != WEAPON_RIFLE && m_pPlayerCharacter->Get_WeaponType() != WEAPON_SHOTGUN)
        return true;
    else
    {
    #ifdef LOAD_UI
        if (false == m_pPlayerCharacter->Get_HoldWeapon()->Can_Reload())
    #else
        if (m_iDebugCount <= 0)
    #endif
        {
            m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_UPPER, Parsing_ReloadEndAnim(m_pPlayerCharacter->Get_WeaponType()));
            return true;
        }
        else
            return false;
    }

    return false;
}

CAbility_Reload* CAbility_Reload::Create()
{
    return new CAbility_Reload();
}

void CAbility_Reload::Free()
{
    __super::Free();
}

CAbility_Hit::CAbility_Hit()
{
}

HRESULT CAbility_Hit::Late_Initialize()
{
    __super::Late_Initialize();

    m_iInputActionID = ACTION_RELOAD;

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_REATION_HIT));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_HIT));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_REATION_HIT));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION));
    // 홀드 나이프는 취소하면 패링 상태가 취소되서 일단 따로 분리해서 취소시킴
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_GUN));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_READY));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_INTERACTION));

    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION));

    //m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_pHitMotange = CAbilityTask_PlayMontageAndWait::Create(this,
        "", "Default", CGameManager::Parsing_Animation(m_ActorInfo, "damage_0105_small_FU"));
    m_pHitMotange->m_OnMontageFinished.AddDynamic("CAbility_Hit", this, &CAbility_Hit::OnMontageTaskEndReceived);

    m_QTETask = CAbilityTask_ApplyQTE::Create(this);
    m_QTETask->m_OnEventDelegate.AddDynamic("CAbility_Hit", this, &CAbility_Hit::OnQTEkeyEnded);

    return S_OK;
}

void CAbility_Hit::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_bUseHitMontage = true;
    m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_ActorInfo.pCharacter->Set_CanTurn(false);

    // 패리 상태로 맞은 케이스
    if (m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_PARRYABLE)))
    {
        m_pPlayerCharacter->Change_Weapon(WEAPON_KNIFE);

        m_pHitMotange->Set_AnimKey(CGameManager::Parsing_Animation(m_ActorInfo, "wp5000H_0146_parry"));

        /* Spawn Particle*/
        SpawnParryParticle();
    }

    // 일반적으로(패리 없이) 맞은 케이스
    else if (pTriggerEventData)
    {
        m_pPlayerCharacter->Change_Weapon(WEAPON_NONE);

        DamageEventRE* pDmgEventRe = static_cast<DamageEventRE*>(pTriggerEventData->pUserData);
        if (nullptr == pDmgEventRe)
            return;

        m_ActorInfo.pCharacter->Set_Target(pDmgEventRe->pInstigator);

        //#ifdef  LOAD_UI
        GameplayAttribute* pAttribute = m_pASC->Get_AttributeSet()->Get_Attribute("CurrentHP");
        if (pAttribute)
        {
            _float fCurrentHP = pAttribute->GetBaseValue();
            m_pASC->SetNumericAttributeBase("CurrentHP", fCurrentHP - pDmgEventRe->fDamage);
        }
        //#endif //  LOAD_UI

                /* Reaction Type이 정해진 데미지 타입이었다면 정해진 Reaction 타입을 토대로 맞는 방향같은 것을 고려해서 HitAnimation 결정 및 선택 */
        if (REACTION_NONE != pDmgEventRe->eReactionType)
        {
            /* Joint Attack의 리액션 타입이었다면 그에 맞춘 로직 수행*/
            if (REACTION_JOINTATTACK == pDmgEventRe->eReactionType)
            {
                if (JOINT_HOLD == pDmgEventRe->eJointPoint)
                {
                    JOINT_HOLD_TYPE eHoldType = static_cast<JOINT_HOLD_TYPE>(pDmgEventRe->iUserData);
                    m_bUseHitMontage = false;
                    m_QTETask->ReadyForActivation();
                    m_pInstigatorMonster = dynamic_cast<CMonsterObject*>(pDmgEventRe->pInstigator);

                    GameplayEventData EventData;
                    EventData.EventTag = GameplayTag("Event.Monster.BeginHoldGrappling");
                    m_pInstigatorMonster->Receive_Event(&EventData);

                    switch (eHoldType)
                    {
                    case Client::JOINT_HOLD_TYPE::TYPE_FRONT:
                        m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_DEFAULT, "chc0_jack_pl_0th_com_0050_hold_F_start");
                        break;
                    case Client::JOINT_HOLD_TYPE::TYPE_BACK:
                        m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_DEFAULT, "chc0_jack_pl_0th_com_0051_hold_B_start");
                        break;
                    default:
                        break;
                    }
                }
            }
            else
            {
                _vector vSrcPos = m_pPlayerCharacter->Get_TransformCom()->Get_Position();

                _float3 vColCenter = pDmgEventRe->pOverlappedComp->Get_Center();
                _vector vDstPos = XMLoadFloat3(&vColCenter);

                DIR eHitDiection = Calculate_HitDirection(vSrcPos, vDstPos, m_pPlayerCharacter->Get_TransformCom()->Get_State(CTransform::STATE_LOOK));

                const string& strHitAnimKey = Get_HitAnimationKey(eHitDiection, pDmgEventRe->eReactionType);
                if (false == DealWithHitAnimation(strHitAnimKey, pDmgEventRe->eReactionType, pTriggerEventData))
                    Hit_Normal();
            }
        }

        /* 몬스터에게 공격을 받았을 경우 지금 현재로써는 ReactionType을 건네받는 게 없기 때문에
        내부적으로 알아서 몬스터타입에 따라서 HitReaction 및 HitAnimation을 결정함 */
        else
        {
            const auto& tags = pTriggerEventData->InstigatorTags.Get_GameplayTags();

            string strTargetAnimKey = "";

            if (tags.size())
                strTargetAnimKey = tags.begin()->m_strTag;

            if (false == DealWithMonster(strTargetAnimKey, pTriggerEventData, *pDmgEventRe))
                Hit_Normal();
        }
    }

    if (m_bUseHitMontage)
        m_pHitMotange->ReadyForActivation();
}

void CAbility_Hit::InputPressed(const GameplayAbility_ActorInfo* pActorInfo)
{

}

void CAbility_Hit::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_Hit::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_ActorInfo.pCharacter->Set_CanTurn(true);

    __super::End_Ability(pActorInfo, bWasCancelled);
}

_bool CAbility_Hit::DealWithMonster(string strTargetAnimKey, const GameplayEventData* pTriggerEventData, const DamageEventRE& DamageEventRE)
{
    CMonsterObject* pMonster = dynamic_cast<CMonsterObject*>(pTriggerEventData->pInstigator);

    if (nullptr == pMonster)
        return false;

    string strAnimkey = "";
    wstring wstrParticleKey = L"";

    _vector vMyPosition = m_pPlayerCharacter->Get_TransformCom()->Get_Position();
    
    DIR eHitDirection = DIR_END;

    // 기본적으로 상/하/좌/우 네 방향만 체킹함 원래는 충돌체 위치기반 충돌 방향체킹이었는데 잘 안맞아서 타겟 위치 기반으로 바꿈
    eHitDirection = Calculate_HitDirection(m_pPlayerCharacter->Get_TransformCom()->Get_Position(), /*pTriggerEventData->vTargetPoint*/ pMonster->Get_TransformCom()->Get_Position(), m_pPlayerCharacter->Get_TransformCom()->Get_State(CTransform::STATE_LOOK));

    /* 현재로써는 시간이 없어 몬스터의 에니메이션 종류에 상관없이 몬스터 타입에 따라서 Reaction Type을 결정하는 로직으로 감. 여유가 된다면 몬스터의 에니메이션 종류에 따라서 ReactionType을 결정하는 식으로 가는게 맞음 */
    ReactionType eReactionType = REACTION_SMALL;
    CMonsterObject::MONSTERTYPE eMonsterType = pMonster->Get_MonsterType();

    switch (eMonsterType)
    {
    case Client::CMonsterObject::MONSTER_COMMON:
        eReactionType = REACTION_SMALL;
        break;
    case Client::CMonsterObject::MONSTER_GARADOR:
    {
        _int iRandomValue = CMyUtils::Get_RandomInt(0, 1);
        eReactionType = iRandomValue == 0 ? REACTION_MEDIUM : REACTION_LARGE;
        break;
    }
    case Client::CMonsterObject::MONSTER_SADDLER:
    {
        _int iRandomValue = CMyUtils::Get_RandomInt(0, 1);
        
        eReactionType = iRandomValue == 0 ? REACTION_EXTRALARGE : REACTION_LARGE;

        static const std::unordered_set<string> SaddlerBeamAnimSet = {
    "chf8_attack_0100_beam_verA",
    "chf8_attack_0110_beam_swing_verA",
    "chf8_attack_0111_beam_swing_verB",
    "chf8_attack_0112_beam_swing_return_verB",
    "chf8_attack_5100_beam_verA"
    "chf8_attack_5110_beam_swing_verA"
    "chf8_attack_5111_beam_swing_verB"
    "chf8_attack_5112_beam_swing_return_verB"
    "chf8_attack_5113_beam_triple_verA"
    "chf8_attack_0112_beam_swing_return_verB"
    "chf8_attack_0113_beam_triple_verA"
        };

        static const std::unordered_set<string> SaddlerStampAnimSet = {
   "chf8_attack_0051_dive_stamp_verA",
   "chf8_attack_0050_double_stamp_verA",
   "chf8_attack_0072_strike_vertical_armD_verA",
        };

        if (SaddlerBeamAnimSet.contains(strTargetAnimKey))
        {
            eReactionType = REACTION_EXTRALARGE;

            /* Spawn Poison Particle */
            CParticleEmitter* pEmitter = { nullptr };
            m_pGameInstance->Spawn_Particle(L"Poisoned", pEmitter);
            if (pEmitter)
                pEmitter->Set_WorldMatrix(m_pPlayerCharacter->Get_TransformCom()->Get_WorldMatrix());
        }
        else if (SaddlerStampAnimSet.contains(strTargetAnimKey))
        {
            eReactionType = REACTION_EXTRALARGE;
        }
        else if ("chf8_attack_0080_strike_thrust_armR_verA" == strTargetAnimKey)
        {
            eReactionType = REACTION_LARGE;
        }
    }
    break;
    case Client::CMonsterObject::MONSTER_NOVISTADOR:
        eReactionType = REACTION_SMALL;
        break;
    case Client::CMonsterObject::MONSTER_END:
        break;
    default:
        break;
    }

    const string strHitAnimKey = Get_HitAnimationKey(eHitDirection, eReactionType);

    if (strHitAnimKey.size())
    {
        // Vignette 이펙트 Danger일떄는 소환하지않음
        if (CONDITION_DANGER != m_pPlayerCharacter->Get_ConditionType())
            CGameManager::GetInstance()->Hit_Damage();

        if (eReactionType == REACTION_EXTRALARGE || eReactionType == REACTION_LARGE)
        {
            /* Spawn Particle */
            CParticleEmitter* pEmitter = { nullptr };
            m_pGameInstance->Spawn_Particle(L"Blood_Out", pEmitter);
            if (pEmitter)
                pEmitter->Set_WorldMatrix(CMyUtils::Get_WorldMatrixFromContactPoint(pTriggerEventData->pContext->point[0]));
        }

        _int iAnimIndex = m_ActorInfo.pCompositeArmature->Get_AnimationIndexByName("", strHitAnimKey);

        if (-1 != iAnimIndex)
            m_pHitMotange->Set_AnimKey(strHitAnimKey);
        else
            m_pHitMotange->Set_AnimKey(CGameManager::Parsing_Animation(m_ActorInfo, "damage_0105_small_FU"));

        return true;
    }

    return false;
}


DIR CAbility_Hit::Calculate_HitDirection(_fvector vSrcPos, _fvector vDstPos, _fvector vLook)
{
    _vector vToAttacker = XMVector3Normalize(vDstPos - vSrcPos);      // 공격자 → 피격자 방향
    _vector vForward = XMVector3Normalize(vLook);                      // 피격자 정면
    _vector vRight = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), vForward)); // 우측 벡터

    _float fFrontDot = XMVectorGetX(XMVector3Dot(vForward, vToAttacker));
    _float fRightDot = XMVectorGetX(XMVector3Dot(vRight, vToAttacker));

    cout << fFrontDot << endl;
    cout << fRightDot << endl;

    if (fFrontDot > 0.707f)      // cos(45도) 이상이면 앞
        return DIR_FRONT;
    else if (fFrontDot < -0.707f)
        return DIR_BACK;
    else if (fRightDot > 0.f)
        return DIR_RIGHT;
    else
        return DIR_LEFT;

    return DIR_END;
}

_bool CAbility_Hit::DealWithHitAnimation(const string& strHitAnimKey, ReactionType eReactionType, const GameplayEventData* pTriggerEventData)
{
    if (strHitAnimKey.size())
    {
        // Vignette 이펙트 Danger일떄는 소환하지않음
        if (CONDITION_DANGER != m_pPlayerCharacter->Get_ConditionType())
            CGameManager::GetInstance()->Hit_Damage();

        if (eReactionType == REACTION_EXTRALARGE || eReactionType == REACTION_LARGE)
        {
            /* Spawn Particle */
            CParticleEmitter* pEmitter = { nullptr };
            m_pGameInstance->Spawn_Particle(L"Blood_Out", pEmitter);
            if (pEmitter)
                pEmitter->Set_WorldMatrix(CMyUtils::Get_WorldMatrixFromContactPoint(pTriggerEventData->pContext->point[0]));
        }

        _int iAnimIndex = m_ActorInfo.pCompositeArmature->Get_AnimationIndexByName("", strHitAnimKey);

        if (-1 != iAnimIndex)
            m_pHitMotange->Set_AnimKey(strHitAnimKey);
        else
            m_pHitMotange->Set_AnimKey(CGameManager::Parsing_Animation(m_ActorInfo, "damage_0105_small_FU"));

        return true;
    }

    return false;



   /* ReactionType eReactionType = DamageEventRE.eReactionType;
    string strHitAnimKey = "";

    const string strHitAnimationKey = Get_HitAnimationKey(eHitDirection, eReactionType);*/
}

string CAbility_Hit::Get_HitAnimationKey(DIR eHitDirection, ReactionType eReactionType)
{
    switch (eReactionType)
    {
    case Client::REACTION_NONE:
        break;
    case Client::REACTION_SMALL:
        switch (eHitDirection)
        {
        case Engine::DIR_LEFT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0101_small_LU");
        case Engine::DIR_RIGHT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0103_small_RU");
        case Engine::DIR_FRONT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0105_small_FU");
        case Engine::DIR_BACK:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0106_small_B");
        default:
            break;
        }
        break;
    case Client::REACTION_MEDIUM:
        switch (eHitDirection)
        {
        case Engine::DIR_LEFT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0111_medium_LU");
        case Engine::DIR_RIGHT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0113_medium_RU");
        case Engine::DIR_FRONT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0115_medium_FU");
        case Engine::DIR_BACK:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0116_medium_B");
        default:
            break;
        }
        break;
    case Client::REACTION_LARGE:
        switch (eHitDirection)
        {
        case Engine::DIR_LEFT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0121_large_LU");
        case Engine::DIR_RIGHT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0123_large_RU");
        case Engine::DIR_FRONT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0125_large_FU");
        case Engine::DIR_BACK:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0126_large_B");
        default:
            break;
        }
        break;
    case Client::REACTION_EXTRALARGE:
        switch (eHitDirection)
        {
        case Engine::DIR_LEFT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0130_extraLarge_L");
        case Engine::DIR_RIGHT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0132_extraLarge_R");
        case Engine::DIR_FRONT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0134_extraLarge_F");
        case Engine::DIR_BACK:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0136_extraLarge_B");
        default:
            break;
        }
        break;
    case Client::REACTION_KICK:
    case Client::REACTION_BLOWNAWAY:
        switch (eHitDirection)
        {
        case Engine::DIR_LEFT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0150_blowoff_FL");
        case Engine::DIR_RIGHT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0152_blowoff_FR");
        case Engine::DIR_FRONT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0160_blowoff_F");
        case Engine::DIR_BACK:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0158_blowoff_BR");
        default:
            break;
        }
        break;
    case Client::REACTION_STRIKE:
        switch (eHitDirection)
        {
        case Engine::DIR_LEFT:
        case Engine::DIR_RIGHT:
        case Engine::DIR_FRONT:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0140_strike_F");
        case Engine::DIR_BACK:
            return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0141_strike_B");
        default:
            break;
        }
        break;
    case Client::REACTION_JOINTATTACK:
        break;
    case Client::REACTION_BURN:
        return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0504_burn_end");
        break;
    case Client::REACTION_EXPLOSION:
        return CGameManager::Parsing_Animation(m_ActorInfo, "damage_0170_explosion_FL");
        break;
    case Client::REACTION_END:
        break;
    default:
        break;
    }

    return "";
}

void CAbility_Hit::Hit_Normal()
{
    m_pHitMotange->Set_AnimKey(CGameManager::Parsing_Animation(m_ActorInfo, "damage_0105_small_FU"));
    vector<string> strIgnoreAbilites = { "Hit", "FlashLight" };
    m_pASC->Cancel_AllAbilities(strIgnoreAbilites);
}

void CAbility_Hit::OnMontageTaskEndReceived(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    End_Ability(&m_ActorInfo, false);
}

void CAbility_Hit::SpawnParryParticle()
{
    CParticleEmitter* pSpakleEmitter = nullptr;
    CParticleEmitter* pParringEmitter = nullptr;

    m_pGameInstance->Spawn_Particle(L"Spark", pSpakleEmitter);
    m_pGameInstance->Spawn_Particle(L"Parring_Bright", pParringEmitter);
    if (pSpakleEmitter && pParringEmitter)
    {
        _matrix matWeapon, matCombined;
        m_ActorInfo.pCompositeArmature->Get_SocketTransform("", "R_Wep", matWeapon);
        matCombined = XMMatrixScaling(100.f, 100.f, 100.f) * matWeapon * m_ActorInfo.pCharacter->Get_TransformCom()->Get_WorldMatrix();
        pSpakleEmitter->Set_WorldMatrix(matCombined);

        pParringEmitter->Set_WorldMatrix(XMMatrixTranslationFromVector(matCombined.r[3]));
    }
}

void CAbility_Hit::OnQTEkeyEnded(IAbilityTask* pTask, _uint iEventType)
{
    IAbilityTask::Task_Delegate_EventType eDelgateEventType = static_cast<IAbilityTask::Task_Delegate_EventType>(iEventType);
    if (IAbilityTask::TYPE_END_TASK == eDelgateEventType)
    {
        if (m_pInstigatorMonster)
        {
            CMonsterCommonObject* pCommonMonster = dynamic_cast<CMonsterCommonObject*>(m_pInstigatorMonster);
            if (pCommonMonster)
                pCommonMonster->Unset_Grappling();
        }

        m_pHitMotange->Set_AnimKey("chc0_jack_pl_0th_com_0070_hold_escape");
        m_pHitMotange->ReadyForActivation();
    }
}

CAbility_Hit* CAbility_Hit::Create()
{
    return new CAbility_Hit();
}

void CAbility_Hit::Free()
{
    __super::Free();
}

CAbility_FlashLight::CAbility_FlashLight()
{
}

HRESULT CAbility_FlashLight::Late_Initialize()
{
    __super::Late_Initialize();

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_UTILITY_FLASHLIGHT));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_UTILITY_FLASHLIGHT));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_pFixDirTask = CAT_FixDirection::Create(this);

    m_pFlashLight = m_pPlayerCharacter->Get_FlashLight();
    m_pMovePointLight = m_pPlayerCharacter->Get_PointLight(CPlayerCharacter::POINTLIGHT_ATTACHED);

    m_pUpdateLightTask = CAbilityTask_UpdateLight::Create(this, m_pMovePointLight);

    /*AbilityTriggerData TriggerData;
    TriggerData.TriggerTag = GameplayTag(KEY_ABILITY_UTILITY_FLASHLIGHT);
    m_AbilityTriggers.push_back(TriggerData);*/

    return S_OK;
}

void CAbility_FlashLight::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", "LeftArm", CGameManager::Parsing_Animation(m_ActorInfo, "light_0010_arm_1"), true);

    m_ActorInfo.pCompositeArmature->Activate_SubPart("FlashLight", true);


    m_pFixDirTask->ReadyForActivation();
    m_pFlashLight->Turn_On(true);

    m_pMovePointLight->Turn_On(true);

    m_pUpdateLightTask->ReadyForActivation();
}

void CAbility_FlashLight::InputPressed(const GameplayAbility_ActorInfo* pActorInfo)
{

}

void CAbility_FlashLight::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_FlashLight::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", "LeftArm", CGameManager::Parsing_Animation(m_ActorInfo, "general_0100_light_off"), false);
    m_pFlashLight->Turn_On(false);

    m_ActorInfo.pCompositeArmature->Activate_SubPart("FlashLight", false);

    if (m_pMovePointLight)
        m_pMovePointLight->Turn_On(false);

    __super::End_Ability(pActorInfo, bWasCancelled);
}

void CAbility_FlashLight::OnMontageTaskEndReceived(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
}

void CAbility_FlashLight::OnInputPressed(IAbilityTask* pTask)
{
    m_ActorInfo.pCompositeArmature->Get_AnimLayerData("", "LeftArm")->pAnimator->Set_CurrentTime(0.f);
    m_ActorInfo.pCompositeArmature->Get_AnimLayerData("", "LeftArm")->pAnimator->Set_Play(false);
}

void CAbility_FlashLight::OnInputReleased(IAbilityTask* pTask)
{
    m_ActorInfo.pCompositeArmature->Get_AnimLayerData("", "LeftArm")->pAnimator->Set_Play(true);
}

CAbility_FlashLight* CAbility_FlashLight::Create()
{
    return new CAbility_FlashLight();
}

void CAbility_FlashLight::Free()
{
    __super::Free();
}

CAbility_WireAction::CAbility_WireAction()
{
}

HRESULT CAbility_WireAction::Late_Initialize()
{
    __super::Late_Initialize();

    m_pAda = dynamic_cast<CAda*>(m_pPlayerCharacter);
    if (nullptr == m_pAda)
        return E_FAIL;

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_TRAVERSAL_WIRE));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_TRAVERSAL_WIRE));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_WIRE_TRIGGERABLE));
    
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_STATE_HIT));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));
    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_WIRE_TRIGGERABLE));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_pWireActionMotange = CAbilityTask_PlayMontageAndWait::Create(this, "", KEY_ANIMLAYER_DEFAULT, "cha8_Hookshot_0100_move_up_VerA");
    m_pLandingMontage = CAbilityTask_PlayMontageAndWait::Create(this, "", KEY_ANIMLAYER_DEFAULT, "");

    m_pWireActionMotange->m_OnMontageFinished.AddDynamic("CAbility_WireAction", this, &CAbility_WireAction::OnMontageTaskEndReceived);
    m_pLandingMontage->m_OnMontageFinished.AddDynamic("CAbility_WireAction", this, &CAbility_WireAction::OnLandingMontageTaskEndReceived);

    /* Wire에 의해서 트리거 되는 이벤트, 와이어가 목표 지점에 도달했을 경우 이벤트가 트리거 된다 */
    AbilityTriggerData TriggerData;
    TriggerData.TriggerTag = GameplayTag(KEY_EVENT_OnWireHit);

    m_pIsFallingTask = CAbilityTask_IsFalling::Create(this);
    m_pIsFallingTask->m_OnFallingTaskEnded.AddDynamic("CAbility_WireAction", this, &CAbility_WireAction::OnFalling);

    m_AbilityTriggers.push_back(TriggerData);

    m_pSetAnchorPointTask = CAbilityTask_SetAnchorPoint::Create(this, m_pAda->Get_AnchorPoint());

    return S_OK;
}

void CAbility_WireAction::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    if (nullptr == pTriggerEventData)
        End_Ability(pActorInfo);

    if (pTriggerEventData->EventTag == GameplayTag(KEY_EVENT_OnHookShot))
    {
        m_ActorInfo.pCCT->Apply_GhostMode(true);
        m_ActorInfo.pCCT->Apply_Gravity(false);

        _vector vTargetPoint;
        
        // 지형과 상호작용하는 케이스
        if (WIRETARGET_STATIC == pTriggerEventData->iUserData)
        {
            CTrigger* pTrigger = m_pAda->Get_Trigger();
            if (nullptr == pTrigger)
            {
                End_Ability(pActorInfo);
                return;
            }
            const CTrigger::TRIGGER_DATA& TriggerData = pTrigger->Get_TriggerData();

            if (TriggerData.vecFloat3Values.empty() || TriggerData.vecIntValues.empty())
            {
                End_Ability(pActorInfo);
                return;
            }

            _float3 vStaticPoint = TriggerData.vecFloat3Values[0];
            vTargetPoint = XMLoadFloat3(&vStaticPoint);

            WirePointType eWirePointType = static_cast<WirePointType>(TriggerData.vecIntValues[0]);

            // Determine Animation Type...
            switch (eWirePointType)
            {
            case Client::WIREPOINT_UP:
                m_strWireAnimationName = "cha8_Hookshot_0102_move_up_VerC";
                break;
            case Client::WIREPOINT_LONG:
                m_strWireAnimationName = "cha8_Hookshot_0210_move_long_VerA";
                m_ActorInfo.pCCT->Apply_GhostMode(false);
                m_ActorInfo.pCCT->Apply_Gravity(true);
                break;
            case Client::WIREPOINT_SHORT:
                m_strWireAnimationName = "cha8_Hookshot_0200_move_short_VerA";
                m_ActorInfo.pCCT->Apply_GhostMode(false);
                break;
            case Client::WIREPOINT_END:
                break;
            default:
                break;
            }

            CCamera_Manager::GetInstance()->On_TargetState(CameraTargetState::CAMTS_WIREACTIVE);
        }

        // 몬스터와 충돌하는 경우...
        else if (WIRETARGET_MONSTER == pTriggerEventData->iUserData)
        {
            if (nullptr == pTriggerEventData->pTarget)
            {
                End_Ability(pActorInfo);
                return;
            }
            CCompositeArmature* pTargetComposite = pTriggerEventData->pTarget->Get_Component<CCompositeArmature>();
            if (nullptr == pTargetComposite)
            {
                End_Ability(pActorInfo);
                return;
            }

            _matrix matNeckSocket;
            if (false == pTargetComposite->Get_SocketTransform("", "Neck_0", matNeckSocket))
            {
                if (nullptr == pTargetComposite)
                {
                    End_Ability(pActorInfo);
                    return;
                }
            }

            _matrix matCombined = matNeckSocket * pTriggerEventData->pTarget->Get_TransformCom()->Get_WorldMatrix();

            vTargetPoint = matCombined.r[3];

            m_pSetAnchorPointTask->Set_TargetMatrix(pTriggerEventData->pTarget->Get_TransformCom()->Get_WorldMatrix_Ptr());
            m_pSetAnchorPointTask->Set_TargetSocketMatrix(pTargetComposite->Get_SocketMatrixPtr("", "Neck_0"));

            m_pSetAnchorPointTask->ReadyForActivation();

            m_strWireAnimationName = "cha8_Hookshot_0905_shoot_jump_fatal_VerA";
        }

        vTargetPoint = XMVectorSetW(vTargetPoint, 1.f);

        /* Wire가 타겟에 닿을때까지 에니메이션 상태 머신 정지*/
        m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_DEFAULT, "cha8_Hookshot_0000_shoot");
        m_ActorInfo.pAnimInstance->Pause_FSM(true);

        // 와이어의 발사. 와이어가 Hit될때까지 정지함
        m_pPlayerCharacter->Set_TargetPoint(vTargetPoint);
        m_pAda->Get_Wire()->On_Activate();
        m_pAda->Get_Wire()->Set_TargetPoint(vTargetPoint);
        if (m_pAda->Get_AnchorPoint())
            m_pAda->Get_AnchorPoint()->Get_TransformCom()->Set_State_Notify(CTransform::STATE_POSITION, vTargetPoint);

        m_pAda->Get_TransformCom()->LookAt_FixYaw(vTargetPoint);
        m_pAda->Get_CompositeArmature()->Activate_Part("WireGun", true);
    }

    m_pAda->Change_Weapon(WEAPON_NONE);

    //CCamera_Manager::GetInstance()->On_WireActive(true);
}

void CAbility_WireAction::InputPressed(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_WireAction::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_WireAction::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    m_ActorInfo.pCCT->Apply_GhostMode(false);
    m_ActorInfo.pCCT->Apply_Gravity(true);

    m_ActorInfo.pAnimInstance->Pause_FSM(false);

    m_ActorInfo.pCompositeArmature->Select_Animation("", CGameManager::Parsing_Animation(m_ActorInfo, "general_0160_stand_loop"), true);

    m_pAda->Get_CompositeArmature()->Activate_Part("WireGun", false);

    //CCamera_Manager::GetInstance()->On_WireActive(false);
    CCamera_Manager::GetInstance()->On_TargetState(CameraTargetState::CAMTS_DEFAULT);

    m_pAda->Set_Trigger(nullptr);

    __super::End_Ability(pActorInfo, bWasCancelled);
}

void CAbility_WireAction::OnGameplayEvent(GameplayTag EventTag, const GameplayEventData* pTriggerEventData)
{
    if (EventTag == GameplayTag(KEY_EVENT_OnWireHit))
    {
        m_pWireActionMotange->Set_AnimKey(m_strWireAnimationName);
        m_pWireActionMotange->ReadyForActivation();
    }
}

void CAbility_WireAction::OnMontageTaskEndReceived(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    if (iAnimIndex == m_ActorInfo.pCompositeArmature->Get_AnimationIndexByName("", "cha8_Hookshot_0102_move_up_VerC"))
    {
        const CTrigger::TRIGGER_DATA& TriggerData = m_pAda->Get_Trigger()->Get_TriggerData();
        if (TriggerData.vecIntValues.size() >= 2)
        {
            //m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_DEFAULT, "cha8_Hookshot_0051_Move_Landing_VerB");
            m_ActorInfo.pCCT->Apply_GhostMode(false);
            m_ActorInfo.pCCT->Apply_Gravity(true);
            m_pLandingMontage->Set_AnimKey("cha8_Hookshot_0051_Move_Landing_VerB");
            m_pLandingMontage->ReadyForActivation();
        }
        else
        {
            m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_DEFAULT, "cha8_Hookshot_0060_Move_Landing_window_start");

            m_pIsFallingTask->ReadyForActivation();
        }
    }
    else if (iAnimIndex == m_ActorInfo.pCompositeArmature->Get_AnimationIndexByName("", "cha8_Hookshot_0210_move_long_VerA"))
    {
        //m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_DEFAULT, "cha8_Hookshot_0050_Move_Landing_VerA");
        m_ActorInfo.pCCT->Apply_GhostMode(false);
        m_ActorInfo.pCCT->Apply_Gravity(true);
        m_pLandingMontage->Set_AnimKey("cha8_Hookshot_0050_Move_Landing_VerA");
        m_pLandingMontage->ReadyForActivation();
    }
    else
        End_Ability(&m_ActorInfo);
}

void CAbility_WireAction::OnLandingMontageTaskEndReceived(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    End_Ability(&m_ActorInfo);
}

void CAbility_WireAction::OnFalling(IAbilityTask* pTask)
{
    if (pTask)
    {
        m_ActorInfo.pCCT->Apply_GhostMode(false);
        m_ActorInfo.pCCT->Apply_Gravity(true);

        m_pLandingMontage->Set_AnimKey("cha8_Hookshot_0061_Move_Landing_window_end");
        m_pLandingMontage->ReadyForActivation();

        //m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_DEFAULT, "cha8_Hookshot_0061_Move_Landing_window_end");
    }

    End_Ability(&m_ActorInfo);
}

CAbility_WireAction* CAbility_WireAction::Create()
{
    return new CAbility_WireAction();
}

void CAbility_WireAction::Free()
{
    __super::Free();
}

CAbility_Calling::CAbility_Calling()
{
}

HRESULT CAbility_Calling::Late_Initialize()
{
    __super::Late_Initialize();

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_INTERACTION_CALLING));

    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_TRAVERSAL));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_DASH));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_REACTION));

   /* AbilityTriggerData TriggerData;
    TriggerData.TriggerTag = GameplayTag(KEY_ABILITY_INTERACTION_CALLING);

    m_AbilityTriggers.push_back(TriggerData);*/

    m_pIsCallingEndTask = CAbilityTask_IsCallingEnd::Create(this);
    m_pIsCallingEndTask->m_OnEnded.AddDynamic("CAbility_Calling", this, &CAbility_Calling::OnCallingEnded);

    return S_OK;
}

void CAbility_Calling::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
#ifdef LOAD_UI
    if (CTrigger* pTrigger = m_pPlayerCharacter->Get_Trigger())
    {
        const CTrigger::TRIGGER_DATA& TriggerData = pTrigger->Get_TriggerData();
        if (TriggerData.vecIntValues.empty())
        {
            End_Ability(pActorInfo);
            return;
        }
        
        // 60(평소) => 45
        m_ActorInfo.pCompositeArmature->Set_MotionSpeed("", 45.f);

        m_pGameInstance->Start_Script(TriggerData.vecIntValues[0]);
        m_pIsCallingEndTask->ReadyForActivation();
    }
#else
    m_pGameInstance->Add_Timer("CAbility_Calling_Debug", 13.f, [&]() {if(m_bActivate)End_Ability(pActorInfo); });
#endif
    m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", "LeftArm", CGameManager::Parsing_Animation(m_ActorInfo, "general2_0810_mobile_loop"));

    if (m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_ABILITY_UTILITY_FLASHLIGHT)))
    {
        m_ActorInfo.pCompositeArmature->Activate_SubPart("FlashLight", false);
    }
}

void CAbility_Calling::InputPressed(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_Calling::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_Calling::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    m_ActorInfo.pCompositeArmature->Set_MotionSpeed("", 60.f);

#ifdef LOAD_UI
    m_pGameInstance->Add_Timer("Chapter", 1.f, [this]()
        {
            CGameInstance::GetInstance()->Get_UI(L"Chapter")->On_UI_Trigger(true);
        });
#endif

    m_ActorInfo.pCompositeArmature->Deactivate_AnimLayer("", "LeftArm");
    if (m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_ABILITY_UTILITY_FLASHLIGHT)))
    {
        m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", "LeftArm", CGameManager::Parsing_Animation(m_ActorInfo, "light_0010_arm_1"), true);

        m_ActorInfo.pCompositeArmature->Activate_SubPart("FlashLight", true);
    }

    __super::End_Ability(pActorInfo, bWasCancelled);
}

void CAbility_Calling::OnGameplayEvent(GameplayTag EventTag, const GameplayEventData* pTriggerEventData)
{

}

void CAbility_Calling::OnCallingEnded(IAbilityTask* pTask)
{
    if (pTask == m_pIsCallingEndTask)
    {
        m_pGameInstance->Add_Timer("CAbility_Calling", 0.f, [&]() {End_Ability(&m_ActorInfo); });
    }
}

CAbility_Calling* CAbility_Calling::Create()
{
    return new CAbility_Calling();
}

void CAbility_Calling::Free()
{
    __super::Free();
}

CAbility_DestroyItem::CAbility_DestroyItem()
{
    m_iInputActionID = ACTION_DESTROY_ITEM;
}

HRESULT CAbility_DestroyItem::Late_Initialize()
{
    __super::Late_Initialize();

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_INTERACTION_DESTORY_ITEM));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_INTERACTION_DESTORY_ITEM));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_ITEM_DESTROYABLE));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_ITEM_DESTROYABLE));
    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT));

    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_TRAVERSAL));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION));
   
    m_pDestroyItemMontage = CAbilityTask_PlayMontageAndWait::Create(this,
        "", KEY_ANIMLAYER_DEFAULT, CGameManager::Parsing_Animation(m_ActorInfo, "general2_0530_attack_lower"));
    m_pDestroyItemMontage->m_OnMontageFinished.AddDynamic("CAbility_DestroyItem", this, &CAbility_DestroyItem::OnDestroyMontageEnded);

    return S_OK;
}

void CAbility_DestroyItem::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    if (nullptr == m_pPlayerCharacter->Get_DestructibleRootBox())
    {
        End_Ability(pActorInfo);
        return;
    }

    m_ActorInfo.pCharacter->Set_CanTurn(false);

    if (CDestructibleLootBox* pRootBox = m_pPlayerCharacter->Get_DestructibleRootBox())
    {
        m_pPlayerCharacter->Get_TransformCom()->LookAt_FixYaw(pRootBox->Get_TransformCom()->Get_Position());
        m_pDestroyItemMontage->ReadyForActivation();
        m_pPlayerCharacter->Set_DestructibleRootBox(nullptr);
        pActorInfo->pCompositeArmature->Get_BaseArmature()->Set_FixRootBoneTranslation(true);
    }
    else
        End_Ability(pActorInfo);
}

void CAbility_DestroyItem::InputPressed(const GameplayAbility_ActorInfo* pActorInfo)
{
    
}

void CAbility_DestroyItem::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    m_ActorInfo.pCharacter->Set_CanTurn(true);

    pActorInfo->pCompositeArmature->Get_BaseArmature()->Set_FixRootBoneTranslation(false);
    pActorInfo->pCharacter->Set_Target(nullptr);

    __super::End_Ability(pActorInfo, bWasCancelled);
}

void CAbility_DestroyItem::OnDestroyMontageEnded(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    End_Ability(&m_ActorInfo);
}

CAbility_DestroyItem* CAbility_DestroyItem::Create()
{
    return new CAbility_DestroyItem();
}

void CAbility_DestroyItem::Free()
{
    __super::Free();
}

CAbility_ObtainItem::CAbility_ObtainItem()
{
    m_iInputActionID = ACTION_PICKUP_ITEM;
}

HRESULT CAbility_ObtainItem::Late_Initialize()
{
    __super::Late_Initialize();

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_INTERACTION_PICKUP_ITEM));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_INTERACTION_PICKUP_ITEM));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_ITEM_OBTAINABLE));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_ITEM_OBTAINABLE));
    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT));

    m_pPickUpItemMontage = CAbilityTask_PlayMontageAndWait::Create(this,
        "", KEY_ANIMLAYER_UPPER, CGameManager::Parsing_Animation(m_ActorInfo, "general_0810_obtain"));
    m_pPickUpItemMontage->m_OnMontageFinished.AddDynamic("CAbility_ObtainItem", this, &CAbility_ObtainItem::OnPickUpMontageEnded);

    return S_OK;
}

void CAbility_ObtainItem::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_pPickUpItemMontage->ReadyForActivation();
}

void CAbility_ObtainItem::InputPressed(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_ObtainItem::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    __super::End_Ability(pActorInfo, bWasCancelled);
}

void CAbility_ObtainItem::OnPickUpMontageEnded(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    End_Ability(&m_ActorInfo);
}

CAbility_ObtainItem* CAbility_ObtainItem::Create()
{
    return new CAbility_ObtainItem();
}

void CAbility_ObtainItem::Free()
{
    __super::Free();
}

CAbility_QTE::CAbility_QTE()
{
    m_iInputActionID = ACTION_QTE;
}

HRESULT CAbility_QTE::Late_Initialize()
{
    __super::Late_Initialize();

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_QTE));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_QTE));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_QTE_TRIGGERABLE));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_QTE_TRIGGERABLE));
    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT));

    m_pQTEMontage = CAbilityTask_PlayMontageAndWait::Create(this,
        "", KEY_ANIMLAYER_DEFAULT, CGameManager::Parsing_Animation(m_ActorInfo, "enemy_0002_escape_B_VerC"));
    m_pQTEMontage->m_OnMontageFinished.AddDynamic("CAbility_QTE", this, &CAbility_QTE::OnQTEMontageEnded);

    return S_OK;
}

void CAbility_QTE::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_pQTEMontage->ReadyForActivation();
}

void CAbility_QTE::InputPressed(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_QTE::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    __super::End_Ability(pActorInfo, bWasCancelled);
}

void CAbility_QTE::OnQTEMontageEnded(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    End_Ability(&m_ActorInfo);
}

CAbility_QTE* CAbility_QTE::Create()
{
    return new CAbility_QTE();
}

void CAbility_QTE::Free()
{
    __super::Free();
}

CAbility_MoveUp::CAbility_MoveUp()
{
    m_iInputActionID = ACTION_MOVEUP;
}

HRESULT CAbility_MoveUp::Late_Initialize()
{
    __super::Late_Initialize();

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_INTERACTION_MOVEUP));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_INTERACTION_MOVEUP));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_CAN_MOVE_UP));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_CAN_MOVE_UP));
    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_REACTION));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_REACTION));

    m_pCallingMontage = CAbilityTask_PlayMontageAndWait::Create(this,
        "", KEY_ANIMLAYER_DEFAULT, "cha0_sm84_702_0010");
    m_pCallingMontage->m_OnMontageFinished.AddDynamic("CAbility_MoveUp", this, &CAbility_MoveUp::OnCallingMontageEnded);

    m_pMoveUpMontage = CAbilityTask_PlayMontageAndWait::Create(this,
        "", KEY_ANIMLAYER_DEFAULT, "cha0_exception_0034_coop_over"); 
    m_pMoveUpMontage->m_OnMontageFinished.AddDynamic("CAbility_MoveUp", this, &CAbility_MoveUp::OnMoveUpMontageEnded);

    // 지연 생성되는 에슐리를 추적하기 위함
    m_pGameInstance->Add_Timer("CAbility_MoveUp::Late_Initialize", 0.f, [&]() {m_pAshley = m_pGameInstance->Get_Object<CAshley>(LEVEL_STATIC, L"Layer_Player"); });
    //m_pAshley = m_pGameInstance->Get_Object<CAshley>(LEVEL_STATIC, L"Layer_Player");
    
    AbilityTriggerData TriggerData;
    TriggerData.TriggerTag = GameplayTag(KEY_EVENT_ASHLEY_CAN_MOVEUP);

    m_AbilityTriggers.push_back(TriggerData);

    return S_OK;
}

void CAbility_MoveUp::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    // 1.레온의 Position과 Look을 Trigger에게 받아서 Set, 에슐리도 트리거 데이터를 기반으로 가야할 Interaction Point를 지정함
    // 2.에슐리가 InteractionPoint에 도달했다면 OnGameplayEvent함수를 Ashley를 통해서 받게됨 => 동시에 에니메이션을 수행함
    // 3.에니메이션이 모두 끝난다면 레온의 어빌리티는 종료하고 에슐리는 이후 문을 열게 됨
    
    m_pPlayerCharacter->Change_Weapon(WEAPON_NONE);

    if (CTrigger* pTrigger = m_pPlayerCharacter->Get_Trigger())
    {
        const CTrigger::TRIGGER_DATA& TriggerData = pTrigger->Get_TriggerData();
        // 첫 번째 벨류는 레온의 Look벡터, 두 번째 벨류는 에슐리가 담을 넘은 후 이동할 포인트임
        if (TriggerData.vecFloat3Values.size() != 2)
        {
            End_Ability(pActorInfo);
            return;
        }

        m_pPlayerCharacter->Set_Target(m_pAshley);
        m_pPlayerCharacter->Get_TransformCom()->Set_State_Notify(CTransform::STATE_POSITION, pTrigger->Get_TransformCom()->Get_Position());
        m_pPlayerCharacter->Get_TransformCom()->Rotation_Dir(XMVectorSetY(XMLoadFloat3(&TriggerData.vecFloat3Values[0]), 0.f));

        /* Set Ashley Interaction Point */
        _vector vLook = XMVector3Normalize(m_pPlayerCharacter->Get_TransformCom()->Get_State(CTransform::STATE_LOOK));
        m_pAshley->Get_InteractionPoint()->Get_TransformCom()->Set_State_Notify(CTransform::STATE_POSITION, XMVectorSetW(pTrigger->Get_TransformCom()->Get_Position() - vLook * 0.3f, 1.f));
        // 문의 위치를 TargetPoint로 지정
        m_pAshley->Set_TargetPoint(XMVectorSetW(XMLoadFloat3(&TriggerData.vecFloat3Values[1]), 1.f));

        m_pCallingMontage->ReadyForActivation();

        dynamic_cast<CReon*>(m_pPlayerCharacter)->Turn_On_HairSystem(false);
    }
}

void CAbility_MoveUp::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    dynamic_cast<CReon*>(m_pPlayerCharacter)->Turn_On_HairSystem(true);

    __super::End_Ability(pActorInfo, bWasCancelled);
}

void CAbility_MoveUp::OnGameplayEvent(GameplayTag EventTag, const GameplayEventData* pTriggerEventData)
{
    /* 에슐리가 레온의 등 뒤로 왔을 때 메세지를 받고 합동기 수행 */
    if (EventTag == GameplayTag(KEY_EVENT_ASHLEY_CAN_MOVEUP))
    {
        m_pMoveUpMontage->ReadyForActivation();
    }
}

void CAbility_MoveUp::OnMoveUpMontageEnded(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    End_Ability(&m_ActorInfo);
}

void CAbility_MoveUp::OnCallingMontageEnded(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_DEFAULT, "cha0_sm84_702_0003_Loop", true);

    // 에슐리에게 상호작용을 돌입한다는 상태를 부여 및 어떤 상호작용을 할 것인지 부여
    m_pAshley->Get_BehavioreTree()->Set_BlackboardKey(KEY_BB_STATE, KEY_STATE_INTERACTION);



    m_pAshley->Get_BehavioreTree()->Set_BlackboardKey(KEY_BB_INTERACTION, KEY_BB_INTERACTION_MOVEUP);
}

CAbility_MoveUp* CAbility_MoveUp::Create()
{
    return new CAbility_MoveUp();
}

void CAbility_MoveUp::Free()
{
    __super::Free();
}

CAbility_DoorControl::CAbility_DoorControl()
{
    m_iInputActionID = ACTION_OPENDOOR;
}

HRESULT CAbility_DoorControl::Late_Initialize()
{
    __super::Late_Initialize();

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_INTERACTION_OPEN_DOOR));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_INTERACTION_OPEN_DOOR));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_DOOR_OPENABLE));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_DOOR_OPENABLE));
    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON));

    m_pOpenDoorMontage = CAbilityTask_PlayMontageAndWait::Create(this,
        "", KEY_ANIMLAYER_UPPER, CGameManager::Parsing_Animation(m_ActorInfo, "other_0104_open_door_R"));
    m_pOpenDoorMontage->m_OnMontageFinished.AddDynamic("CAbility_DoorControl", this, &CAbility_DoorControl::OnOpenDoorMontageEnded);

    return S_OK;
}

void CAbility_DoorControl::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_pOpenDoorMontage->ReadyForActivation();
}

void CAbility_DoorControl::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    __super::End_Ability(pActorInfo, bWasCancelled);
}

void CAbility_DoorControl::OnOpenDoorMontageEnded(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    End_Ability(&m_ActorInfo);
}

CAbility_DoorControl* CAbility_DoorControl::Create()
{
    return new CAbility_DoorControl();
}

void CAbility_DoorControl::Free()
{
    __super::Free();
}

CAbility_HelpAshley::CAbility_HelpAshley()
{
    m_iInputActionID = ACTION_HELP_ASHLEY;
}

HRESULT CAbility_HelpAshley::Late_Initialize()
{
    __super::Late_Initialize();

    m_pReon = dynamic_cast<CReon*>(m_ActorInfo.pCharacter);
    if (nullptr == m_pReon)
        return E_FAIL;

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_INTERACTION_HELP_ASHLEY));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_INTERACTION_HELP_ASHLEY));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_HELPABLE));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_HELPABLE));
    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT));

    m_pHelpMontage = CAbilityTask_PlayMontageAndWait::Create(this,
        "", KEY_ANIMLAYER_DEFAULT, CGameManager::Parsing_Animation(m_ActorInfo, ""));
    m_pHelpMontage->m_OnMontageFinished.AddDynamic("CAbility_HelpAshley", this, &CAbility_HelpAshley::OnHelpMontageEnded);

    return S_OK;
}

void CAbility_HelpAshley::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_pPlayerCharacter->Change_Weapon(WEAPON_NONE);
    
    CAshley* pAshley = m_pReon->Get_Ashley();
    if (nullptr == pAshley)
    {
        End_Ability(pActorInfo);
        return;
    }

#ifdef LOAD_UI
    _int iRandoValue = CMyUtils::Get_RandomInt(0, 1);
    m_pGameInstance->Start_Script(iRandoValue == 0 ? 27 : 28);
#endif

    m_pReon->Set_Target(pAshley);

    GameplayEventData EventData;
    _float fOutDegree = 0.f;
    _vector vDir =  pAshley->Get_TransformCom()->Get_Position() - m_pReon->Get_TransformCom()->Get_Position();
    if (CMyUtils::IsFace2Face(vDir, pAshley->Get_TransformCom()->Get_State(CTransform::STATE_LOOK), fOutDegree))
    {
        EventData.iUserData = DIR_FRONT;
        m_pHelpMontage->Set_AnimKey("cha0_caution_0163_stand_end_set_cha1");
    }
    else
    {
        EventData.iUserData = DIR_BACK;
        m_pHelpMontage->Set_AnimKey("cha0_caution_0166_stand_B_end_set_cha1");
    }

    m_pHelpMontage->ReadyForActivation();

    EventData.EventTag = GameplayTag(KEY_EVENT_ASHLEY_CAUTION_END);
    pAshley->Receive_Event(&EventData);
}

void CAbility_HelpAshley::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    __super::End_Ability(pActorInfo, bWasCancelled);
}

void CAbility_HelpAshley::OnHelpMontageEnded(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    End_Ability(&m_ActorInfo);
}

CAbility_HelpAshley* CAbility_HelpAshley::Create()
{
    return new CAbility_HelpAshley();
}

void CAbility_HelpAshley::Free()
{
    __super::Free();
}
