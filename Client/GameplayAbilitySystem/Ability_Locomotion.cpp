#include "Ability_Locomotion.h"
#include "AT_Move.h"
#include "GameplayAbilitySystemHeader.h"

CAbility_Walk::CAbility_Walk()
{
    m_iInputActionID = ACTION_MOVE;
}

HRESULT CAbility_Walk::Late_Initialize()
{
    m_AbilityTags.AddTag({ "Ability.Move" });

    m_pFixDirectionTask = CAT_FixDirection::Create(this, 3.7f);

    m_AbilityTags.AddTag(GameplayTag(KEY_STATE_WALK));
    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_WALK));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_WALK));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_WALK));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_STATE_IDLE));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_DASH));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_IDLE));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));
    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_WALKABLE));

    if (m_ActorInfo.pContext->Has_Variable("MoveKey"))
        m_pDirType = m_ActorInfo.pContext->Get_Variable<_int*>("MoveKey");

    auto WalkEndAnimLamda = [&]() {
        int iDirType = *m_pDirType;

        if (iDirType == DIR_END)
        {
            m_fAccTime += 0.01f;
            if (m_fAccTime >= m_fWalkEndThresholdTime)
                return true;
        }
        else
        {
            m_fAccTime = 0.f;
        }

        return false;
        };

    m_pCheckingConditionTask = CAbilityTask_CheckingCondition::Create(this);
    m_pCheckingConditionTask->m_fnLambda = WalkEndAnimLamda;
    m_pCheckingConditionTask->m_OnConditionCompleted.AddDynamic("CAbility_Walk", this, &CAbility_Walk::On_WalkEnded);

    return S_OK;
}

void CAbility_Walk::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_pFixDirectionTask->ReadyForActivation();
    m_pCheckingConditionTask->ReadyForActivation();

    //InputPressed(pActorInfo);
}

void CAbility_Walk::InputPressed(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_Walk::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{
    //End_Ability(pActorInfo);
}

void CAbility_Walk::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    __super::End_Ability(pActorInfo);


    m_pGameInstance->Add_Timer("CAbility_Walk", 0.f, [&]() {
        m_pASC->TryActivate_Ability("Idle");
        });
}

void CAbility_Walk::On_WalkEnded(IAbilityTask* pTask)
{
    if (pTask == m_pCheckingConditionTask)
    {
        m_pGameInstance->Add_Timer("CAbility_Walk_OnWalkEnded", 0.f, [&]() {
            m_pASC->TryActivate_Ability("Idle");
            });
    }

    End_Ability(&m_ActorInfo);
}

CAbility_Walk* CAbility_Walk::Create()
{
    CAbility_Walk* pInstance = new CAbility_Walk();

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CAbility_Walk");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CAbility_Walk::Free()
{
    __super::Free();
}

CAbility_Dash::CAbility_Dash()
{
}

HRESULT CAbility_Dash::Late_Initialize()
{
    m_iInputActionID = ACTION_DASH;

    m_AbilityTags.AddTag(GameplayTag(KEY_STATE_DASH));
    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_DASH));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_DASH));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_DASH));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_STATE_WALK));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_STATE_IDLE));
    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_STATE_CROUCHING));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_HOLDING));
    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_pTurnTask = CAT_MoveWithTurn::Create(this, 6.f);

    if (m_ActorInfo.pContext->Has_Variable("MoveKey"))
        m_pDirType = m_ActorInfo.pContext->Get_Variable<_int*>("MoveKey");

    if (nullptr == m_pDirType)
        return E_FAIL;

    m_pCheckingConditionTask = CAbilityTask_CheckingCondition::Create(this);

    auto lambda = [&]() -> bool {
        DIR eDIR = static_cast<DIR>(*m_pDirType);
        if (eDIR == DIR_END)
            return true;

        return false;
        };

    m_pCheckingConditionTask->m_fnLambda = lambda;
    m_pCheckingConditionTask->m_OnConditionCompleted.AddDynamic("CAbility_Dash", this, &CAbility_Dash::OnDashEnded);

    m_pDashMontageTask = CAbilityTask_PlayMontageAndWait::Create(this, "", "", CGameManager::Parsing_Animation(m_ActorInfo, "defence_0540_jog_end"));
    m_pDashMontageTask->m_OnMontageFinished.AddDynamic("CAbility_Dash", this, &CAbility_Dash::OnDashMontageEnded);

    m_iDashEndAnimIndex =  m_ActorInfo.pCompositeArmature->Get_AnimationIndexByName("", CGameManager::Parsing_Animation(m_ActorInfo, "defence_0540_jog_end"));

    m_pPlayerCharacter = dynamic_cast<CPlayerCharacter*>(m_ActorInfo.pCharacter);
    if (nullptr == m_pPlayerCharacter)
        return E_FAIL;

    return S_OK;
}

void CAbility_Dash::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_pTurnTask->ReadyForActivation();
    m_pCheckingConditionTask->ReadyForActivation();

    m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_WALKABLE));
}

void CAbility_Dash::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    __super::End_Ability(pActorInfo, bWasCancelled);

    if (bWasCancelled)
    {
        m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_WALKABLE));
    }
    m_pGameInstance->Add_Timer("CAbility_Dash", 0.f, [&]() {
        m_pASC->TryActivate_Ability("Idle");
        });
       //m_ActorInfo.pAnimInstance->Re_Enter_CurrentState();
}

void CAbility_Dash::OnDashEnded(IAbilityTask* pTask)
{
    if (m_bActivate)
    {
        if (pTask == m_pCheckingConditionTask)
        {
            if (CONDITION_GENERIC == m_pPlayerCharacter->Get_ConditionType())
                m_pDashMontageTask->Set_AnimKey(CGameManager::Parsing_Animation(m_ActorInfo, "defence_0540_jog_end"));
            else if (CONDITION_DANGER == m_pPlayerCharacter->Get_ConditionType())
                m_pDashMontageTask->Set_AnimKey(CGameManager::Parsing_Animation(m_ActorInfo, "danger_0540_jog_end"));

            m_pDashMontageTask->ReadyForActivation();
            m_ActorInfo.pCharacter->StopSoundEvent("event:/InteractVFX/StepSound", true);

            m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_WALKABLE));
        }
    }
}

void CAbility_Dash::OnDashMontageEnded(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    if (m_iDashEndAnimIndex == iAnimIndex && m_bActivate)
    {
        m_pGameInstance->Add_Timer("CAbility_Dash", 0.f, [&]() {
            m_pASC->TryActivate_Ability("Idle"); 
            });

    }

    End_Ability(&m_ActorInfo);
}

CAbility_Dash* CAbility_Dash::Create()
{
    CAbility_Dash* pInstance = new CAbility_Dash();

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CAbility_Dash");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CAbility_Dash::Free()
{
    __super::Free();

    //Safe_Release(m_pTurnTask);
}

CAbility_Crouching::CAbility_Crouching()
{
}

HRESULT CAbility_Crouching::Late_Initialize()
{
    m_pPlayerCharacter = dynamic_cast<CPlayerCharacter*>(m_ActorInfo.pCharacter);
    if (nullptr == m_pPlayerCharacter)
        return E_FAIL;

    m_iInputActionID = ACTION_CROUCHING;

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_CROUCHING));
    m_AbilityTags.AddTag(GameplayTag(KEY_STATE_CROUCHING));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_CROUCHING));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_CROUCHING));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_DASH));
    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_HOLDING));
    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

   
    return S_OK;
}

void CAbility_Crouching::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    CCamera_Manager::GetInstance()->On_TargetState(CAMTS_CROUCHING);

    m_ActorInfo.pCompositeArmature->Set_UseHairPhysics(false);

    m_ActorInfo.pCompositeArmature->Deactivate_AnimLayer("", KEY_ANIMLAYER_UPPER);
    if (CONDITION_GENERIC == m_pPlayerCharacter->Get_ConditionType())
    {
        m_ActorInfo.pCompositeArmature->Select_Animation("", CGameManager::Parsing_Animation(m_ActorInfo, "general_0169_switching_crouch"));
    }
    else if (CONDITION_DANGER == m_pPlayerCharacter->Get_ConditionType())
    {
        m_ActorInfo.pCompositeArmature->Select_Animation("", CGameManager::Parsing_Animation(m_ActorInfo, "danger_0169_switching_crouch"));
    }


}

void CAbility_Crouching::InputPressed(const GameplayAbility_ActorInfo* pActorInfo)
{
    // 마찬가지임
    m_ActorInfo.pCompositeArmature->Deactivate_AnimLayer("", KEY_ANIMLAYER_UPPER);

    if (CONDITION_GENERIC == m_pPlayerCharacter->Get_ConditionType())
    {
        m_ActorInfo.pCompositeArmature->Select_Animation("", CGameManager::Parsing_Animation(m_ActorInfo, "general_1169_switching_stand"));
    }
    else if (CONDITION_DANGER == m_pPlayerCharacter->Get_ConditionType())
    {
        m_ActorInfo.pCompositeArmature->Select_Animation("", CGameManager::Parsing_Animation(m_ActorInfo, "danger_1169_switching_stand"));
    }

    End_Ability(pActorInfo);
}

void CAbility_Crouching::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_Crouching::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    m_ActorInfo.pCompositeArmature->Set_UseHairPhysics(true);

    dynamic_cast<CPlayerCharacter*>(m_ActorInfo.pCharacter)->Set_Crouching(false);
    CCamera_Manager::GetInstance()->On_TargetState(CAMTS_DEFAULT);
    m_ActorInfo.pAnimInstance->Re_Enter_CurrentState();

    __super::End_Ability(pActorInfo, bWasCancelled);
}

CAbility_Crouching* CAbility_Crouching::Create()
{
    CAbility_Crouching* pInstance = new CAbility_Crouching();

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CAbility_Crouching");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CAbility_Crouching::Free()
{
    __super::Free();
}

CAbility_Idle::CAbility_Idle()
{
}

HRESULT CAbility_Idle::Late_Initialize()
{
    m_iInputActionID = ACTION_IDLE;

    m_AbilityTags.AddTag(GameplayTag(KEY_STATE_IDLE));
    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_IDLE));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_IDLE));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_IDLE));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_WALK));
    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_DASH));

    //m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    return S_OK;
}

void CAbility_Idle::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
}

void CAbility_Idle::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_Idle::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    __super::End_Ability(pActorInfo, bWasCancelled);
}

CAbility_Idle* CAbility_Idle::Create()
{
    CAbility_Idle* pInstance = new CAbility_Idle();

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CAbility_Idle");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CAbility_Idle::Free()
{
    __super::Free();
}

CAbility_Ladder::CAbility_Ladder()
{
}

HRESULT CAbility_Ladder::Late_Initialize()
{
    __super::Late_Initialize();

    m_iInputActionID = ACTION_LADDER;

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_TRAVERSAL_LADDER));

    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_LADDERUSEABLE));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_pLadderStartMotange = CAbilityTask_PlayMontageAndWait::Create(this, "", "Default", CGameManager::Parsing_Animation(m_ActorInfo, "other_0220_climb_ladder_start"));
    m_pLadderStartMotange->m_OnMontageFinished.AddDynamic("CAbility_Ladder", this, &CAbility_Ladder::OnStartMontageTaskEndReceived);

    m_pLadderClimbL_Motange = CAbilityTask_PlayMontageAndWait::Create(this, "", "Default", CGameManager::Parsing_Animation(m_ActorInfo, "other_0223_climb_ladder_L"));
    m_pLadderClimbL_Motange->m_OnMontageFinished.AddDynamic("CAbility_Ladder", this, &CAbility_Ladder::OnClimbLMontageTaskEndReceived);

    m_pLadderClimbR_Motange = CAbilityTask_PlayMontageAndWait::Create(this, "", "Default", CGameManager::Parsing_Animation(m_ActorInfo, "other_0224_climb_ladder_R"));
    m_pLadderClimbR_Motange->m_OnMontageFinished.AddDynamic("CAbility_Ladder", this, &CAbility_Ladder::OnClimbRMontageTaskEndReceived);

    m_pPlayerCharacter = dynamic_cast<CPlayerCharacter*>(m_ActorInfo.pCharacter);
    if (nullptr == m_pPlayerCharacter)
        return E_FAIL;

    return S_OK;
}

void CAbility_Ladder::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    CCamera_Manager::GetInstance()->On_TargetState(CAMTS_LADDER);
    m_ActorInfo.pCCT->Apply_Gravity(false);

    if (CTrigger* pTrigger = m_ActorInfo.pCharacter->Get_Trigger())
    {
        const CTrigger::TRIGGER_DATA& TriggerData = pTrigger->Get_TriggerData();
        if (TriggerData.vecFloat3Values.size())
        {
            _vector vDirection = XMLoadFloat3(&TriggerData.vecFloat3Values[0]);
            m_ActorInfo.pCharacter->Get_TransformCom()->Rotation_Dir(XMVectorSetY(vDirection, 0.f));
            m_ActorInfo.pCharacter->Get_TransformCom()->Set_State_Notify(CTransform::STATE_POSITION, pTrigger->Get_TransformCom()->Get_Position());
        }
    }

    m_pPlayerCharacter->Change_Weapon(WEAPON_NONE);

    m_pLadderStartMotange->ReadyForActivation();
}

void CAbility_Ladder::InputPressed(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_Ladder::InputReleased(const GameplayAbility_ActorInfo* pActorInfo)
{
}

void CAbility_Ladder::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    __super::End_Ability(pActorInfo, bWasCancelled);

    CCamera_Manager::GetInstance()->On_TargetState(CAMTS_DEFAULT);

    if (false == m_pLadderClimbL_Motange->IsFinished())
        m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", "Default", CGameManager::Parsing_Animation(m_ActorInfo, "other_0225_climb_ladder_L_end"));
    else 
    {
        m_ActorInfo.pCompositeArmature->Select_AnimLayer_Animation("", "Default", CGameManager::Parsing_Animation(m_ActorInfo, "other_0226_climb_ladder_R_end"));
    }
}

void CAbility_Ladder::OnStartMontageTaskEndReceived(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    if (false == m_bActivate)
        return;

    m_pGameInstance->Add_Timer("CAbility_Ladder", 0.f, [&]() {m_pLadderClimbL_Motange->ReadyForActivation(); });
}

void CAbility_Ladder::OnClimbLMontageTaskEndReceived(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    if (false == m_bActivate)
        return;

    m_pGameInstance->Add_Timer("CAbility_Ladder", 0.f, [&]() {m_pLadderClimbR_Motange->ReadyForActivation(); });
}

void CAbility_Ladder::OnClimbRMontageTaskEndReceived(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    if (false == m_bActivate)
        return;

    m_pGameInstance->Add_Timer("CAbility_Ladder", 0.f, [&]() {m_pLadderClimbL_Motange->ReadyForActivation(); });
}

CAbility_Ladder* CAbility_Ladder::Create()
{
    return new CAbility_Ladder();
}

void CAbility_Ladder::Free()
{
    __super::Free();
}

CAbility_Fall::CAbility_Fall()
{
}

HRESULT CAbility_Fall::Late_Initialize()
{
    __super::Late_Initialize();

    m_iInputActionID = ACTION_FALL;

    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_TRAVERSAL_FALL));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));
    // 어빌리티가 끝날때 상태를 같이 제거하기 위함
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_FALLABLE));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT));

    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_COMBAT));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_REACTION));
    m_BlockAbilitiesWithTag.AddTag(GameplayTag(KEY_ABILITY_UTILITY));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_pIsFallingTask = CAbilityTask_IsFalling::Create(this);

    m_pIsFallingTask->m_OnFallingTaskEnded.AddDynamic("CAbility_Fall", this, &CAbility_Fall::OnFallingEnded);

    m_pPlayerCharacter = dynamic_cast<CPlayerCharacter*>(m_ActorInfo.pCharacter);
    if (nullptr == m_pPlayerCharacter)
        return E_FAIL;

    return S_OK;
}

void CAbility_Fall::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    if (pTriggerEventData)
    {
        if (CTrigger* pTrigger = dynamic_cast<CTrigger*>(pTriggerEventData->pTarget))
        {
            const CTrigger::TRIGGER_DATA& TriggerData = pTrigger->Get_TriggerData();

            if (TriggerData.vecFloat3Values.empty() || TriggerData.vecIntValues.empty())
            {
                End_Ability(pActorInfo);
                return;
            }

            _vector vDirection = XMLoadFloat3(&TriggerData.vecFloat3Values[0]);
            m_ActorInfo.pCharacter->Get_TransformCom()->Rotation_Dir(XMVectorSetY(vDirection, 0.f));

            LandType eLandType = static_cast<LandType>(TriggerData.vecIntValues[0]);
            switch (eLandType)
            {
            case Client::LAND_LOW:
                m_strLandAnimKey = "general_0323_land_low";
                break;
            case Client::LAND_MIDDLE:
                m_strLandAnimKey = "general_0341_land_middle";
                break;
            case Client::LAND_LARGE:
                m_strLandAnimKey = "general_0340_land_large";
                break;
            case Client::LAND_HIGHEST:
                m_strLandAnimKey = "general_0342_land_highest";
                break;
            default:
                m_strLandAnimKey = "general_0323_land_low";
                break;
            }
        }
    }

    m_ActorInfo.pCCT->Apply_Gravity(true);

    m_ActorInfo.pAnimInstance->Play_Montage("", "Default", CGameManager::Parsing_Animation(m_ActorInfo, "general_0321_jump_down"));

    m_pPlayerCharacter->Change_Weapon(WEAPON_NONE);

    m_pGameInstance->Add_Timer("CAbility_Fall", 0.55f, [&]() {m_pIsFallingTask->ReadyForActivation(); });
}

void CAbility_Fall::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    __super::End_Ability(pActorInfo, bWasCancelled);

    m_pGameInstance->Add_Timer("CAbility_Fall", 0.f, [&]() { m_ActorInfo.pAnimInstance->Play_Montage("", "Default", CGameManager::Parsing_Animation(m_ActorInfo, m_strLandAnimKey)); });
}

void CAbility_Fall::OnFallingEnded(IAbilityTask* pFallingTask)
{
    End_Ability(&m_ActorInfo, false);
}   

CAbility_Fall* CAbility_Fall::Create()
{
    return new CAbility_Fall();

}

void CAbility_Fall::Free()
{
    __super::Free();
}

CAbility_StepUp::CAbility_StepUp()
{

}

HRESULT CAbility_StepUp::Late_Initialize()
{
    m_AbilityTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_TRAVERSAL_STEPUP));

    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_WALK));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));
    m_ActivationOwnedTags.AddTag(GameplayTag(KEY_STATE_CLIMBABLE));

    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_HIT));
    m_ActivationBlockedTags.AddTag(GameplayTag(KEY_STATE_OCCUPIED));

    m_ActivationRequiredTags.AddTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    m_CancelAbilitiesWithTags.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT));

    /*AbilityTriggerData TriggerData;
    TriggerData.TriggerTag = GameplayTag(KEY_EVENT_ON_CLIMB);
    m_AbilityTriggers.push_back(TriggerData);*/

    m_pStepUpMontageTask = CAbilityTask_PlayMontageAndWait::Create(this, "", KEY_ANIMLAYER_DEFAULT, "");
    m_pStepUpMontageTask->m_OnMontageFinished.AddDynamic("CAbility_StepUp", this, &CAbility_StepUp::OnStepUpMontageEnded);

    m_pPlayerCharacter = dynamic_cast<CPlayerCharacter*>(m_ActorInfo.pCharacter);
    if (nullptr == m_pPlayerCharacter)
        return E_FAIL;

    return S_OK;
}

void CAbility_StepUp::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    if (pTriggerEventData)
    {
        m_ActorInfo.pCCT->Apply_Gravity(false);

        if (CTrigger* pTrigger = dynamic_cast<CTrigger*>(pTriggerEventData->pTarget))
        {
            const CTrigger::TRIGGER_DATA& TriggerData = pTrigger->Get_TriggerData();

            if (TriggerData.vecFloat3Values.empty() || TriggerData.vecIntValues.empty())
            {
                End_Ability(pActorInfo);
                return;
            }

            _vector vDirection =  XMLoadFloat3(&TriggerData.vecFloat3Values[0]);
            m_ActorInfo.pCharacter->Get_TransformCom()->Rotation_Dir(XMVectorSetY(vDirection, 0.f));

            _int iStepUpType = TriggerData.vecIntValues[0];
            if (STEPUP_1F == iStepUpType)
            {
                m_pStepUpMontageTask->Set_AnimKey(CGameManager::Parsing_Animation(m_ActorInfo, m_strStepUp_1F_AnimKey));
            }
            else if (STEPUP_2F == iStepUpType)
            {
                m_pStepUpMontageTask->Set_AnimKey(CGameManager::Parsing_Animation(m_ActorInfo, m_strStepUp_2F_AnimKey));
            }
            m_pStepUpMontageTask->ReadyForActivation();
        }
    }

    m_pPlayerCharacter->Change_Weapon(WEAPON_NONE);

    m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_UNOCCUPIED));
}

void CAbility_StepUp::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    __super::End_Ability(pActorInfo, bWasCancelled);
}

void CAbility_StepUp::OnStepUpMontageEnded(CAbilityTask_PlayMontageAndWait* pMontageTask, _uint iAnimIndex)
{
    End_Ability(&m_ActorInfo, false);
}

CAbility_StepUp* CAbility_StepUp::Create()
{
    return new CAbility_StepUp();
}

void CAbility_StepUp::Free()
{
    __super::Free();
}
