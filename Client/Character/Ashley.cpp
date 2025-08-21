#include "Ashley.h"
#include "CompositeArmatrue.h"
#include "AnimInstance.h"
#include "AbilitySystemComponent.h"
#include "GameInstance.h"
#include "GameplayAbilitySystemHeader.h"
#include "Context.h"
#include "FSMHeader.h"
#include "Trigger_Socket.h"
#include "BehaviorTree.h"
#include "Reon.h"
#include "BehaviorTreeHeader.h"
#include "Simple_Factory.h"
#include "Bound_Sphere.h"

CAshley::CAshley(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    :CPlayerCharacter(pDevice, pContext)
{
}

CAshley::CAshley(const CAshley& Prototype)
    : CPlayerCharacter{ Prototype }
{
    m_iCharacterNumber = 1;
    m_iCustomMask = CT_ASHLEY;
}

HRESULT CAshley::Initialize(void* pArg)
{
    CGameObject::GAMEOBJECT_DESC desc{};

    m_vPlayerPos = { -6.f, -0.9f, -18.285f };
    XMStoreFloat3(&desc.vPosition, m_vPlayerPos);
    desc.vRotation = { 0.f, 86.f, 0.f };
    desc.fRotationPerSec = XM_PI;

    lstrcpy(desc.szGameObjectTag, L"Ashley");

    if (E_FAIL == __super::Initialize(&desc))
        return E_FAIL;

    if (E_FAIL == Ready_Components())
        return E_FAIL;

    if (E_FAIL == Ready_Materials())
        return E_FAIL;

    if (E_FAIL == Ready_Parts())
        return E_FAIL;

    if (E_FAIL == Ready_VariableContext())
        return E_FAIL;

    if (E_FAIL == Ready_GAS())
        return E_FAIL;

    if (E_FAIL == Ready_BehaviorTree())
        return E_FAIL;

    if (E_FAIL == Ready_Collision())
        return E_FAIL;

    if (E_FAIL == Ready_AnimLayers())
        return E_FAIL;

    m_pBound->Set_Mask(OT_NPC);

    m_pCompositeArmature->Select_Animation("Head", "cha100_10_cute_loop_facial", true);
    //m_pCompositeArmature->Select_Animation("Head", "cha1_general_1160_stand_loop", true);
    m_pCompositeArmature->Set_MotionSpeed("Head", 60.f);

    m_pGameInstance->Add_OnLevelStartedDelegate("CAshley::OnLevelstarted", [&](_uint iLevelIndex) {OnLevelStarted(iLevelIndex); });
    m_pGameInstance->Add_OnLevelEndedDelegate("CAshley::OnLevelEnded", [&](_uint iLevelIndex) {OnLevelEnded(iLevelIndex); });

#ifdef LOAD_UI
    m_pGameInstance->Register_Script_Target(this, SCRIPT_TARGET::ASHELY);
#endif

    m_pGameInstance->Cutscene_AddCharacter("Ashley", this);
    m_pBehaviorTree->Set_Active(false);

    m_CarryOffsetMatrix = XMMatrixRotationY(3.14f) * XMMatrixTranslation(0.056f, 0.038f, 0.128f);

    return S_OK;
}

float CAshley::Take_Damage(_float fDamageAmount, DamageEvent* pDamageEvent, CGameObject* pDamageInstigator, CGameObject* pDamageCauser)
{
    if (nullptr == pDamageEvent)
        return 0.f;

    string strStateKey = m_pBehaviorTree->Get_Blackboard()->Get_Variable<string>(KEY_BB_STATE);
    if (KEY_STATE_HIT == strStateKey || KEY_STATE_CAUTION == strStateKey)
        return 0.f;

    DamageEventRE* pDmgEvent = static_cast<DamageEventRE*>(pDamageEvent);
    if (pDmgEvent->pOverlappedComp)
    {
        if (pDamageInstigator == m_pReon)
            return 0.f;
    }

#ifdef LOAD_UI
    m_pGameInstance->Start_Script(26);
#endif

    string strBlackboardHitKeyValue = KEY_BB_HIT_NORMAL;
    m_pBehaviorTree->Set_BlackboardKey(KEY_BB_STATE, KEY_STATE_HIT);
    m_pBehaviorTree->Set_BlackboardKey(KEY_BB_HIT, strBlackboardHitKeyValue);
    m_strPrevStateKey = m_pBehaviorTree->Get_Blackboard()->Get_Variable<string>(KEY_BB_STATE);

    return 0.0f;
}

void CAshley::Receive_Event(const GameplayEventData* pEventData)
{
    __super::Receive_Event(pEventData);

    /* For St1 Ladder IteractionState*/
    if (pEventData->EventTag == GameplayTag(KEY_EVENT_CHANGE_INTERACTION_STATE))
    {
        if (nullptr == m_pTrigger)
            return;

        const CTrigger::TRIGGER_DATA& TriggerData = m_pTrigger->Get_TriggerData();
        if (TriggerData.vecFloat3Values.empty())
            return;
        Set_TargetPoint(XMVectorSetW(XMLoadFloat3(&TriggerData.vecFloat3Values[0]), 1.f));

        m_pBehaviorTree->Set_BlackboardKey(KEY_BB_STATE, KEY_STATE_INTERACTION);
        m_pBehaviorTree->Set_BlackboardKey(KEY_BB_INTERACTION, KEY_BB_INTERACTION_LADDER);
    }
    else if (pEventData->EventTag == GameplayTag(KEY_EVENT_ST1_RECEIVE_CALLING))
    {
        if (nullptr == m_pTrigger)
            return;
        
        /* 레온에게 트리거 데이터 전달 및 이벤트 전달 */
        m_pReon->Set_Trigger(m_pTrigger);
        m_pReon->Receive_Event(pEventData);
    }
    /* 이전 상태로 복귀하기 위함 */
    else if (pEventData->EventTag == GameplayTag(KEY_EVENT_ASHLEY_CHANGE_PREV_STATE))
    {
        m_pBehaviorTree->Set_BlackboardKey(KEY_BB_STATE, m_strPrevStateKey);
    }
    else if (pEventData->EventTag == GameplayTag(KEY_EVENT_ASHLEY_SET_CARRIED))
    {
        if (nullptr != m_pCarrierMatrix)
        {
            m_bIsCarried = true;

            m_pCCT->Apply_GhostMode(true);
            m_pCCT->Apply_Gravity(false);
        }
    }
    else if (pEventData->EventTag == GameplayTag(KEY_EVENT_ASHLEY_CAUTION_END))
    {
        DIR eDIR = static_cast<DIR>(pEventData->iUserData);
        if (DIR_FRONT == eDIR)
            m_pCompositeArmature->Select_Animation("", "cha1_caution_0163_stand_end_set_cha0");
        else if (DIR_BACK == eDIR)
            m_pCompositeArmature->Select_Animation("", "cha1_caution_0166_stand_B_end_set_cha0");

        m_bCautionEnded = true;
        m_pGameInstance->End_Interection(INTERECTION_TYPE::ASHLEY_GROGGY, this);

        m_OnGameplayEvent.Broadcast(pEventData);
    }
    else if (pEventData->EventTag == GameplayTag(KEY_EVENT_ASHLEY_CAUTION_START))
    {
        m_bCautionEnded = false;
    }
    else if (pEventData->EventTag == GameplayTag(KEY_EVENT_SET_CCT_GRAVITY_GHOST_MODE))
    {
        m_pCCT->Apply_GhostMode(true);
        m_pCCT->Apply_Gravity(false);
    }
    else if (pEventData->EventTag == GameplayTag(KEY_EVENT_UNSET_CCT_GRAVITY_GHOST_MODE))
    {
        m_pCCT->Apply_GhostMode(false);
        m_pCCT->Apply_Gravity(true);
    }
}

void CAshley::Play_CutSceneAnimation(string strPartKey, string strAnimationKey, _fvector vPosition, _fvector vLook, _bool bLoop, string strAnimLayerKey)
{
    m_pBehaviorTree->Set_Active(false);

    __super::Play_CutSceneAnimation(strPartKey, strAnimationKey, vPosition, vLook, bLoop);
}

void CAshley::Finish_CutScene()
{
    m_pBehaviorTree->Set_Active(true);
    if (LEVEL_HARBOR != m_pGameInstance->Get_CurrentLevel())
        m_pBehaviorTree->Set_BlackboardKey(KEY_BB_STATE, KEY_STATE_CHASING);
    else
        m_pBehaviorTree->Set_BlackboardKey(KEY_BB_STATE, KEY_STATE_IDLE);

    __super::Finish_CutScene();
}

ASHLEY_CARRY_START_TYPE CAshley::Carry_Ashley(CGameObject* pCarrier)
{
    if (nullptr == pCarrier)
        return ASHLEY_CARRY_START_TYPE::CARRY_FAIL;

    /* 바로 붙이면 안되고 에니메이션이 끝난다음에 붙여야 해서 시기를 뒤로 미룸 */
    /* Receive Event => KEY_EVENT_ASHLEY_CHANGE_SET_CARRIED 이벤트 태그로 처리함 */
    //m_bIsCarried = true;

    m_bCanTurn = false;

    string strStateKey = m_pBehaviorTree->Get_Blackboard()->Get_Variable<string>(KEY_BB_STATE);
    if (KEY_STATE_CHASING == strStateKey || KEY_STATE_IDLE == strStateKey || strStateKey ==  KEY_STATE_CAUTION)
    {
        m_pTransformCom->LookAt_FixYaw(pCarrier->Get_TransformCom()->Get_Position());

        m_pBehaviorTree->Set_BlackboardKey(KEY_BB_STATE, KEY_STATE_BEING_CARRIED);

        m_pTarget = pCarrier;

        m_pCarrierMatrix = pCarrier->Get_TransformCom()->Get_WorldMatrix_Ptr();

        /* 몬스터와 에니메이션 합을 맞춰야 함 */
        if (KEY_STATE_CAUTION == strStateKey)
        {
            m_pCompositeArmature->Select_Animation("", "chc0_jack_npc_0th_com_5700_caution_carry_up_F");
            return ASHLEY_CARRY_START_TYPE::CARRY_CAUTION;
        }
        else
        {
            m_pCompositeArmature->Select_Animation("", "chc0_jack_npc_0th_com_5110_carry_up_F_start_verA");
            return ASHLEY_CARRY_START_TYPE::CARRY_NORMAL;
        }
    }
    else
        return ASHLEY_CARRY_START_TYPE::CARRY_FAIL;
}

void CAshley::StopCarry_Ashley(ASHLEY_FREE_TYPE eType)
{
    m_pTarget = nullptr;

    m_pCarrierMatrix = nullptr;
    m_bIsCarried = false;
    m_pBehaviorTree->Set_BlackboardKey(KEY_BB_IS_CARRY_ENDED, true);

    m_bCanTurn = true;

    /* Sequence진입 후 CarryEnd를 다시 false로 Set*/
    m_pGameInstance->Add_Timer("CAshley::Receive_Event", 1.f, [&]() {m_pBehaviorTree->Set_BlackboardKey(KEY_BB_IS_CARRY_ENDED, false); });

    m_pCCT->Apply_GhostMode(false);
    m_pCCT->Apply_Gravity(true);

    switch (eType)
    {
    case Client::ASHLEY_FREE_TYPE::TYPE_DAMAGED:
        m_pCompositeArmature->Select_Animation("", "chc0_jack_npc_0th_com_5400_damage_carry_up_verA");
        break;
    case Client::ASHLEY_FREE_TYPE::TYPE_STEALTH_KILLED:
        m_pCompositeArmature->Select_Animation("", "chc0_jack_npc_0th_com_5921_wp5000_carry_knife_stealth_kill_B");
        break;
    case Client::ASHLEY_FREE_TYPE::TYPE_END:
        break;
    default:
        break;
    }

    GameplayEventData EventData;
    EventData.EventTag = GameplayTag(KEY_EVENT_ASHLEY_CARRY_END);
    EventData.iUserData = static_cast<int>(eType);

    // BT의 Task에게 끝났다는 신호를 줌
    m_OnGameplayEvent.Broadcast(&EventData);
}

HRESULT CAshley::Ready_Components()
{
    m_pNavigationCom->Set_ReachedThresholdSq(1.3f);

    m_pCompositeArmature->Set_BaseArmature(
        TEXT("Prototype_Component_Model_Ashley"),
        TEXT("Prototype_Component_Armature_Ashley"),
        LEVEL_STATIC);

    /** animation set setting */
    m_pCompositeArmature->Set_AnimationSet("",
        TEXT("Prototype_Component_AnimSet_Ashley"),
        LEVEL_STATIC);

    /** additional Head */
    m_pCompositeArmature->Add_Part("Head",
        TEXT("Prototype_Component_Model_AshleyHead"),
        TEXT("Prototype_Component_Armature_AshleyHead"),
        LEVEL_STATIC);

    m_pCompositeArmature->Set_AnimationSet("Head",
        TEXT("Prototype_Component_AnimSet_AshleyHead"),
        LEVEL_STATIC);

    /** addtional Hair */
    m_pCompositeArmature->Add_Part(
        "AshleyHair",
        TEXT("Prototype_Component_Model_AshleyHair"),
        TEXT("Prototype_Component_Armature_AshleyHair"),
        LEVEL_STATIC);

    /* Com_BehaviorTree */
    if (FAILED(__super::Add_Component(LEVEL_STATIC, KEY_PROTO_COM_BEHAVIOR_TREE,
        reinterpret_cast<CComponent**>(&m_pBehaviorTree), TEXT("Com_BehaviorTree"))))
        return E_FAIL;

    m_pCompositeArmature->Set_RootBoneTransform("root", m_pTransformCom, true);
    m_pCompositeArmature->Select_Animation("", "cha1_general_0160_stand_loop", true);
    m_pCompositeArmature->Set_MotionSpeed("", 60);

    m_pReon = m_pGameInstance->Get_Object<CReon>(LEVEL_STATIC, L"Layer_Player");
    m_pTarget = m_pReon;

    return S_OK;
}

HRESULT CAshley::Ready_GAS()
{
    m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    //m_pASC->Give_Ability("Move", CAbility_Walk::Create());
    //m_pASC->Give_Ability("Dash", CAbility_Dash::Create());
    //m_pASC->Give_Ability("Idle", CAbility_Idle::Create());
    m_pASC->Give_Ability("Crouching", CAbility_Crouching::Create());
    //m_pASC->Give_Ability("Hit", CAbility_Hit::Create());

    m_pASC->Give_Ability("Ladder", CAbility_Ladder::Create());
    m_pASC->Give_Ability("Fall", CAbility_Fall::Create());
    m_pASC->Give_Ability("StepUp", CAbility_StepUp::Create());

    m_pASC->m_OnTagAddedDelegate.AddDynamic("CAshley", this, &CAshley::OnASCTagAddReceived);
    m_pASC->m_OnTagRemovedDelegate.AddDynamic("CAshley", this, &CAshley::OnASCTagRemoveReceived);

    return S_OK;
}

HRESULT CAshley::Ready_Materials()
{
    // 메터리얼을 추가시킨다.
    queue<pair<wstring, wstring>> detailMapQueue;
    detailMapQueue.push({ TEXT("../Bin/Resources/Models/Player/Ashley/Detail/"), TEXT("msk4") });
    if (FAILED(Check_Materials(m_pCompositeArmature, &detailMapQueue)))
        return E_FAIL;

    // 미리 저장된 프리셋을 불러온다.
    if (FAILED(Load_Materials("../Bin/Resources/Materials/Player/Character_Ashley.yaml")))
        return E_FAIL;

    return S_OK;
}

HRESULT CAshley::Ready_BehaviorTree()
{
    m_strInteractionKey = KEY_BB_INTERACTION_LADDER;

    m_pBehaviorTree->Add_BlackboardKey(KEY_BB_STATE, make_pair(string(KEY_STATE_IDLE), TYPE_STRING));
    m_pBehaviorTree->Add_BlackboardKey(KEY_BB_INTERACTION, make_pair((string)m_strInteractionKey, TYPE_STRING));

    m_pBehaviorTree->Add_BlackboardKey(KEY_BB_ATTACH_TO_REON, make_pair(m_bAttachToReon, TYPE_BOOL));
    m_pBehaviorTree->Add_BlackboardKey(KEY_BB_ON_CHANGED_ATTACH, make_pair(false, TYPE_BOOL));

    m_pBehaviorTree->Add_BlackboardKey(KEY_BB_IS_IN_FRONT_OF_REON, make_pair(false, TYPE_BOOL));
    m_pBehaviorTree->Add_BlackboardKey(KEY_BB_IS_BEHIND_OF_REON, make_pair(false, TYPE_BOOL));
    m_pBehaviorTree->Add_BlackboardKey(KEY_BB_AIMING, make_pair(false, TYPE_BOOL));

    m_pBehaviorTree->Add_BlackboardKey(KEY_BB_ON_CHANGED_CROUCH, make_pair(false, TYPE_BOOL));
    m_pBehaviorTree->Add_BlackboardKey(KEY_BB_CROUCHING, make_pair(false, TYPE_BOOL));

    m_pBehaviorTree->Add_BlackboardKey(KEY_BB_HIT, make_pair(KEY_BB_HIT_NORMAL, TYPE_STRING));

    m_pBehaviorTree->Add_BlackboardKey(KEY_BB_IS_WALK_RANGE_KEY, make_pair(false, TYPE_BOOL));
    m_pBehaviorTree->Add_BlackboardKey(KEY_BB_IS_DASH_RANGE_KEY, make_pair(false, TYPE_BOOL));

    m_pBehaviorTree->Add_BlackboardKey(KEY_BB_IS_CARRY_ENDED, make_pair(m_bIsCarried, TYPE_BOOL));

    if (E_FAIL == m_pBehaviorTree->Load_JsonFile("../Bin/DataFiles/BehaviorTree/Ashley.json", "Ashley"))
        return E_FAIL;

    m_pBehaviorTree->Get_Blackboard()->Bind_Delegate(KEY_BB_STATE, [&]() {
        m_pGameInstance->Get_UI(L"Ashely_State")->On_UI_Trigger(); 
        if (KEY_STATE_CAUTION != m_pBehaviorTree->Get_Blackboard()->Get_Variable<string>(KEY_BB_STATE))
        {
            m_pGameInstance->End_Interection(INTERECTION_TYPE::ASHLEY_GROGGY, this);
        }

        });

    m_pBehaviorTree->Get_Blackboard()->m_OnPreValueChanged.AddDynamic("CAshley", this, &CAshley::OnBlackboardKeyPreChanged);

    return S_OK;
}

HRESULT CAshley::Ready_Parts()
{
    /* For Target Point(Prison => LadderPoint) */
    GAMEOBJECT_DESC ObjectDesc{};
    ObjectDesc.vPosition = { 0.9f, 0.1f, 11.664f };
    ObjectDesc.vOffset = { 0.f, 0.f, 0.f };
    ObjectDesc.fSpeedPerSec = 1.3f;
    ObjectDesc.fRotationPerSec = XM_PI * 0.4f;
    ObjectDesc.fRadius = 0.1f;
    ObjectDesc.iMask = OT_TRIGGER;
    ObjectDesc.boundType = BOUNDTYPE::AABB;
    ObjectDesc.BoundBox = BoundingBox({ 0.0f, 0.f, 0.f }, { 0.15f, 0.15f, 0.15f });

    // 리지드 박스의 형태는 동일
    ObjectDesc.eRigidType = RIGIDSHAPE::SPHERE;
    ObjectDesc.Physics_Info.bRotate = false;
    ObjectDesc.Physics_Info.bDynamic = false;
    ObjectDesc.Physics_Info.bSimulate = false;
    ObjectDesc.Physics_Info.bUseRigid = false;
    ObjectDesc.Physics_Info.bTrigger = true;

    lstrcpy(ObjectDesc.szGameObjectTag, L"Ashley_InteractionPoint");
    m_pInteractionPoint = dynamic_cast<CTargetPoint*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::TYPE_GAMEOBJECT, LEVEL_STATIC, TEXT("Prototype_GameObject_TargetPoint"), &ObjectDesc));
    if (nullptr == m_pInteractionPoint)
        return E_FAIL;
    m_pGameInstance->Add_GameObject(m_pInteractionPoint, LEVEL_STATIC, L"Layer_TargetPoint");

    /* Goal Point */
    ObjectDesc.eRigidType = RIGIDSHAPE::SPHERE;
    lstrcpy(ObjectDesc.szGameObjectTag, L"Ashley_GoalPoint");
    ObjectDesc.fRadius = 0.05f;
    ObjectDesc.BoundBox = BoundingBox({ 0.0f, 0.f, 0.f }, { 0.37f, 0.37f, 0.37f });
    m_pGoalPoint = dynamic_cast<CTargetPoint*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::TYPE_GAMEOBJECT, LEVEL_STATIC, TEXT("Prototype_GameObject_TargetPoint"), &ObjectDesc));
    if (nullptr == m_pGoalPoint)
        return E_FAIL;
    m_pGameInstance->Add_GameObject(m_pGoalPoint, LEVEL_STATIC, L"Layer_TargetPoint");

    /* Collision Sockets */
    vector<BONE_BIND> BindList = CMyUtils::Load_BoneBind("../Bin/Resources/ColInfo/Player/Ashley.yaml");
    if (BindList.empty() || BindList.size() < 2)
        return E_FAIL;

    /* TriggerSocket For CloseRange Distinguish */
    CTriggerSocket::TriggerSoket_DESC TriggerDesc{};

    TriggerDesc.strPartsName = BindList[0].strPartsName;
    TriggerDesc.strBoneName = BindList[0].strBoneName;

    TriggerDesc.eRigidType = BindList[0].eTypeRigid;

    TriggerDesc.vScale = BindList[0].vRigidScale;
    TriggerDesc.vOffsetTranslation = BindList[0].vTranslation;
    TriggerDesc.vOffsetRotation = BindList[0].vRotation;

    TriggerDesc.pCompositeArmature = m_pCompositeArmature;
    TriggerDesc.pObject = this;
    TriggerDesc.pOwner = this;
    TriggerDesc.pRealOwner = this;
    TriggerDesc.iMask = OT_NPC;

    CTriggerSocket* pSocket = dynamic_cast<CTriggerSocket*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::TYPE_GAMEOBJECT, LEVEL_STATIC, TEXT("Prototype_GameObject_TriggerSocket"), &TriggerDesc));
    if (pSocket)
        m_pCloseRangeTrigger = pSocket;

    /* TriggerSocket For Walk Range */
    TriggerDesc.strPartsName = BindList[1].strPartsName;
    TriggerDesc.strBoneName = BindList[1].strBoneName;

    TriggerDesc.eRigidType = BindList[1].eTypeRigid;

    TriggerDesc.vScale = BindList[1].vRigidScale;
    TriggerDesc.vOffsetTranslation = BindList[1].vTranslation;
    TriggerDesc.vOffsetRotation = BindList[1].vRotation;

    TriggerDesc.pCompositeArmature = m_pCompositeArmature;
    TriggerDesc.pObject = this;
    TriggerDesc.pOwner = this;
    TriggerDesc.pRealOwner = this;
    TriggerDesc.iMask = OT_NPC;

    pSocket = dynamic_cast<CTriggerSocket*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::TYPE_GAMEOBJECT, LEVEL_STATIC, TEXT("Prototype_GameObject_TriggerSocket"), &TriggerDesc));
    if (pSocket)
        m_pWalkRangeTrigger = pSocket;

    /* TriggerSocket For Dash Range */
    TriggerDesc.strPartsName = BindList[2].strPartsName;
    TriggerDesc.strBoneName = BindList[2].strBoneName;

    TriggerDesc.eRigidType = BindList[2].eTypeRigid;

    TriggerDesc.vScale = BindList[2].vRigidScale;
    TriggerDesc.vOffsetTranslation = BindList[2].vTranslation;
    TriggerDesc.vOffsetRotation = BindList[2].vRotation;

    TriggerDesc.pCompositeArmature = m_pCompositeArmature;
    TriggerDesc.pObject = this;
    TriggerDesc.pOwner = this;
    TriggerDesc.pRealOwner = this;
    TriggerDesc.iMask = OT_NPC;

    pSocket = dynamic_cast<CTriggerSocket*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::TYPE_GAMEOBJECT, LEVEL_STATIC, TEXT("Prototype_GameObject_TriggerSocket"), &TriggerDesc));
    if (pSocket)
        m_pDashRangeTrigger = pSocket;

    /* TriggerSocket For  */
    TriggerDesc.strPartsName = BindList[3].strPartsName;
    TriggerDesc.strBoneName = BindList[3].strBoneName;

    TriggerDesc.eRigidType = BindList[3].eTypeRigid;

    TriggerDesc.vScale = BindList[3].vRigidScale;
    TriggerDesc.vOffsetTranslation = BindList[3].vTranslation;
    TriggerDesc.vOffsetRotation = BindList[3].vRotation;

    TriggerDesc.pCompositeArmature = m_pCompositeArmature;
    TriggerDesc.pObject = this;
    TriggerDesc.pOwner = this;
    TriggerDesc.pRealOwner = this;
    TriggerDesc.iMask = OT_NPC;

    pSocket = dynamic_cast<CTriggerSocket*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::TYPE_GAMEOBJECT, LEVEL_STATIC, TEXT("Prototype_GameObject_TriggerSocket"), &TriggerDesc));
    if (pSocket)
        m_pCheckingInteractionPointTrigger = pSocket;


    return S_OK;
}

HRESULT CAshley::Ready_Collision()
{
    m_pInteractionPoint->Get_Bound()->Set_Mask(OT_TRIGGER);
    m_pCloseRangeTrigger->Get_Bound()->Set_Mask(OT_NPC);
    m_pWalkRangeTrigger->Get_Bound()->Set_Mask(OT_NPC);
    m_pDashRangeTrigger->Get_Bound()->Set_Mask(OT_NPC);

    m_pInteractionPoint->Get_Bound()->Add_CollisionCallBack(
        [&](CBound* pBound, COLLISION_INFO* pInfo)
        {
            //if (pBound->Get_ID() == m_pBound->Get_ID())
            if (pBound->Get_ID() == m_pCheckingInteractionPointTrigger->Get_Bound()->Get_ID())
                m_bCollideWithInteractionPoint = true;
        });

    m_pInteractionPoint->Get_Bound()->Add_CollisionExitCallBack(
        [&](CBound* pBound, COLLISION_INFO* pInfo)
        {
            //if (pBound->Get_ID() == m_pBound->Get_ID())
            if (pBound->Get_ID() == m_pCheckingInteractionPointTrigger->Get_Bound()->Get_ID())
                m_bCollideWithInteractionPoint = false;
        });

    m_pGoalPoint->Get_Bound()->Add_CollisionCallBack(
        [&](CBound* pBound, COLLISION_INFO* pInfo)
        {
            if (pBound->Get_ID() == m_pCheckingInteractionPointTrigger->Get_Bound()->Get_ID())
                m_bCollideWithGoalPoint = true;
        });

    m_pGoalPoint->Get_Bound()->Add_CollisionExitCallBack(
        [&](CBound* pBound, COLLISION_INFO* pInfo)
        {
            if (pBound->Get_ID() == m_pCheckingInteractionPointTrigger->Get_Bound()->Get_ID())
                m_bCollideWithGoalPoint = false;
        });

    m_pCloseRangeTrigger->Get_Bound()->Add_CollisionCallBack(
        [&](CBound* pBound, COLLISION_INFO* pInfo)
        {
            if (pBound->Get_Owner() == m_pReon)
            {
                m_bIsInCloseRange = true;
                if (KEY_STATE_CAUTION == m_pBehaviorTree->Get_Blackboard()->Get_Variable<string>(KEY_BB_STATE))
                {
                    if (false == m_bCautionEnded && m_pCompositeArmature->Get_CurrentAnimationName("") == "cha1_caution_0160_stand_loop")
                    {
                        m_pReon->Get_AbilitySystemComponent()->AddLooseGameplayTag(GameplayTag(KEY_STATE_HELPABLE));
#ifdef LOAD_UI
                        m_pGameInstance->Begin_Interection(INTERECTION_TYPE::ASHLEY_GROGGY, this);
#endif
                    }
                }
            }
        });

    m_pCloseRangeTrigger->Get_Bound()->Add_CollisionExitCallBack(
        [&](CBound* pBound, COLLISION_INFO* pInfo)
        {
            if (pBound->Get_Owner() == m_pReon)
            {
                m_bIsInCloseRange = false;

                if (KEY_STATE_CAUTION == m_pBehaviorTree->Get_Blackboard()->Get_Variable<string>(KEY_BB_STATE))
                {
                    m_pReon->Get_AbilitySystemComponent()->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_HELPABLE));
#ifdef LOAD_UI
                    m_pGameInstance->End_Interection(INTERECTION_TYPE::ASHLEY_GROGGY, this);
#endif
                }
            }
        });

    m_pWalkRangeTrigger->Get_Bound()->Add_CollisionCallBack(
        [&](CBound* pBound, COLLISION_INFO* pInfo)
        {
            if (pBound->Get_Owner() == m_pReon)
            {
                m_bIsInWalkRange = true;
                m_pBehaviorTree->Set_BlackboardKey(KEY_BB_IS_WALK_RANGE_KEY, m_bIsInWalkRange);
            }
        });

    m_pWalkRangeTrigger->Get_Bound()->Add_CollisionExitCallBack(
        [&](CBound* pBound, COLLISION_INFO* pInfo)
        {
            if (pBound->Get_Owner() == m_pReon)
            {
                m_bIsInWalkRange = false;
                m_pBehaviorTree->Set_BlackboardKey(KEY_BB_IS_WALK_RANGE_KEY, m_bIsInWalkRange);
            }
        });

    m_pDashRangeTrigger->Get_Bound()->Add_CollisionCallBack(
        [&](CBound* pBound, COLLISION_INFO* pInfo)
        {
            if (pBound->Get_Owner() == m_pReon)
            {
                m_bIsInDashRange = true;
                m_pBehaviorTree->Set_BlackboardKey(KEY_BB_IS_DASH_RANGE_KEY, m_bIsInDashRange);
            }
        });

    m_pDashRangeTrigger->Get_Bound()->Add_CollisionExitCallBack(
        [&](CBound* pBound, COLLISION_INFO* pInfo)
        {
            if (pBound->Get_Owner() == m_pReon)
            {
                m_bIsInDashRange = false;
                m_pBehaviorTree->Set_BlackboardKey(KEY_BB_IS_DASH_RANGE_KEY, m_bIsInDashRange);
            }
        });

    return S_OK;
}

HRESULT CAshley::Ready_AnimLayers()
{
    m_pCompositeArmature->Get_ArmatureByPart("")->Load_NotifyFile("../Bin/Datafiles/Animations/Player/Ashley.json", CSimple_Factory::Create_Notify, CSimple_Factory::Create_NotifyState);

    /* 전신 몽타주 슬롯*/
    vector<string> SplitBones = { "root" };
    vector<string> IgnoreBones;
    vector<string> SelectedBones;

    m_pCompositeArmature->Add_AnimLayer("", "Default", SplitBones, AnimationBlendMode::OVERWRITE, 0, IgnoreBones, 0.2f, 0.2f, true);

    // 양 다리 Bone Index 시작점
    SelectedBones = { "Spine_2" };
    IgnoreBones = { "L_Thigh", "R_Thigh" };
    m_pCompositeArmature->Add_AnimLayer("", "Upper", SelectedBones, AnimationBlendMode::OVERWRITE, 2, IgnoreBones, 0.3f, 0.33f, true);

    SelectedBones = { "Head" };
    IgnoreBones = {""};
    m_pCompositeArmature->Add_AnimLayer("", KEY_ANIMLAYER_ADD, SelectedBones, AnimationBlendMode::OVERWRITE, 10, IgnoreBones, 0.3f, 0.33f);

    return S_OK;
}

HRESULT CAshley::Ready_VariableContext()
{
    m_pVariableContext->Add_Variable<_bool*>("AttachToReonKey", &m_bAttachToReon);
    m_pVariableContext->Add_Variable<_bool*>("IsInCloseRangeKey", &m_bIsInCloseRange);
    m_pVariableContext->Add_Variable<_bool*>("IsInWalkRangeKey", &m_bIsInWalkRange);
    m_pVariableContext->Add_Variable<_bool*>("IsInDashRangeKey", &m_bIsInDashRange);

    return S_OK;
}

void CAshley::Update_BehaviorTreeValue(_float fTimeDelta)
{
    if (nullptr == m_pReon)
        return;

    BT_Debug_KeyInput();

    //_bool bReonCrouching = m_pReon->Get_Crouching();
    //Determine_Crouching(bReonCrouching);

    if (false == m_pBehaviorTree->Get_Active())
        return;

    if (m_bCanTurn)
    {
        _vector vReonPosition = m_pReon->Get_TransformCom()->Get_Position();
        _vector vPosition = m_pTransformCom->Get_Position();
        _vector vDir = vReonPosition - vPosition;
        _float fReonToAshleyDistance = XMVectorGetX(XMVector3Length(vDir));

        m_pTransformCom->Rotation_Dir_Smooth(XMVectorSetY(vDir, 0.f), fTimeDelta);
    }

    if (m_pBehaviorTree)
        m_pBehaviorTree->Update(fTimeDelta);
}

void CAshley::Priority_Update(_float fTimeDelta)
{
    if (m_pCloseRangeTrigger)
        m_pCloseRangeTrigger->Priority_Update(fTimeDelta);

    if (m_pWalkRangeTrigger)
        m_pWalkRangeTrigger->Priority_Update(fTimeDelta);

    if (m_pDashRangeTrigger)
        m_pDashRangeTrigger->Priority_Update(fTimeDelta);

    if (m_pCheckingInteractionPointTrigger)
        m_pCheckingInteractionPointTrigger->Priority_Update(fTimeDelta);


    Update_BehaviorTreeValue(fTimeDelta);

    __super::Priority_Update(fTimeDelta);
}

void CAshley::Update(_float fTimeDelta)
{
    // Update Navgation
    m_pNavigationCom->Set_IndexByNav(m_pTransformCom);
    m_pNavigationCom->Update(fTimeDelta);

    if (m_bIsCarried) 
    {
        m_pTransformCom->Set_WorldMatrix(m_CarryOffsetMatrix * m_pTarget->Get_TransformCom()->Get_WorldMatrix());
    }

    __super::Update(fTimeDelta);
}

void CAshley::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);
}

void CAshley::BT_Debug_KeyInput()
{
    if (m_pGameInstance->Key_Down(DIK_F1))
        m_pBehaviorTree->Toggle_Active();

    if (false == m_pBehaviorTree->Get_Active())
        return;

    if (m_pGameInstance->Key_Pressing(DIK_1) && m_pGameInstance->Key_Down(DIK_T))
        m_bCanTurn ^= 1;

    if (m_pGameInstance->Key_Pressing(DIK_1) && m_pGameInstance->Key_Down(DIK_G))
    {
        m_bGhostMode ^= 1;
        m_pCCT->Apply_GhostMode(m_bGhostMode);
    }

    if (m_pGameInstance->Key_Pressing(DIK_1) && m_pGameInstance->Key_Down(DIK_B))
    {
        m_pCCT->Apply_Gravity(WORLD_GRAVITY);
    }

    if (m_pGameInstance->Key_Pressing(DIK_1) && m_pGameInstance->Key_Down(DIK_O))
    {
        m_pCompositeArmature->Select_AnimLayer_Animation("", KEY_ANIMLAYER_ADD, "cha1_general2_1015_chap0_look_back_R");
    }

    if (m_pGameInstance->Key_Pressing(DIK_1) && m_pGameInstance->Key_Down(DIK_U))
    {
        /*m_pTarget = m_pGameInstance->Get_Object<CMonsterObject>(LEVEL_GAMEPLAY, L"Layer_Monster");
        if (m_pTarget)
        {
            m_pCompositeArmature->Select_Animation("", "chc0_jack_npc_0th_com_5700_caution_carry_up_F");

            m_pTarget->Get_Component<CCompositeArmature>()->Select_Animation("", "chc0_grapple_0th_com_5700_caution_carry_up_F");
        }*/
        
        m_pCompositeArmature->Select_AnimLayer_Animation ("", KEY_ANIMLAYER_ADD  ,"cha1_general2_0350_avoid_jog_L");
    }

    if (m_pGameInstance->Key_Pressing(DIK_1) && m_pGameInstance->Key_Down(DIK_Y))
    {
        m_pCompositeArmature->Select_Animation("", "cha1_general_0973_npc_dodge_LB");
    }


    if (m_pGameInstance->Key_Down(DIK_LCONTROL))
    {
        if (m_pBehaviorTree->Get_Blackboard()->Has_Variable(KEY_BB_STATE))
        {
            string strStateKey = m_pBehaviorTree->Get_Blackboard()->Get_Variable<string>(KEY_BB_STATE);

            if (KEY_STATE_CHASING == strStateKey)
            {
                m_bAttachToReon ^= 1;

                
                m_pBehaviorTree->Set_BlackboardKey(KEY_BB_ATTACH_TO_REON, m_bAttachToReon);
                m_pBehaviorTree->Set_BlackboardKey(KEY_BB_ON_CHANGED_ATTACH, true);

#ifdef LOAD_UI
                if (m_bAttachToReon)
                {
                    _int iRandomValue = CMyUtils::Get_RandomInt(36, 37);
                    m_pGameInstance->Start_Script(iRandomValue);
                }
                else
                {
                    _int iRandomValue = CMyUtils::Get_RandomInt(34, 35);
                    m_pGameInstance->Start_Script(iRandomValue);
                }

                m_pGameInstance->Get_UI(L"Ashely_State")->On_UI_Trigger();
#endif
            }
        }
    }

    if (m_pGameInstance->Key_Pressing(DIK_1) && m_pGameInstance->Key_Down(DIK_D))
    {
        m_pBehaviorTree->Set_BlackboardKey("StateKey", KEY_STATE_DEAD);
    }
    else if (m_pGameInstance->Key_Pressing(DIK_1) && m_pGameInstance->Key_Down(DIK_H))
    {
        m_pBehaviorTree->Set_BlackboardKey("StateKey", KEY_STATE_HIT);
    }
    else if (m_pGameInstance->Key_Pressing(DIK_1) && m_pGameInstance->Key_Down(DIK_C))
    {
        m_pBehaviorTree->Set_BlackboardKey("StateKey", KEY_STATE_CHASING);
    }
    else if (m_pGameInstance->Key_Pressing(DIK_1) && m_pGameInstance->Key_Down(DIK_N))
    {
        m_pBehaviorTree->Set_BlackboardKey("StateKey", KEY_STATE_IDLE);
    }
}

void CAshley::Determine_Crouching(_bool bCrouch)
{

}

void CAshley::OnLevelStarted(_uint iLevelIndex)
{
    if (iLevelIndex == LEVEL_GAMEPLAY)
    {
        m_pBehaviorTree->Set_Active(true);
        m_pBehaviorTree->Set_BlackboardKey(KEY_BB_STATE, KEY_STATE_IDLE);

        m_pTransformCom->Set_State_Notify(CTransform::STATE_POSITION, XMVectorSet(10.878f, -5.542f, 3.344f, 1.f));
        XMVECTOR quat = XMQuaternionRotationRollPitchYaw(
            XMConvertToRadians(0.f),     // pitch (X축 회전)
            XMConvertToRadians(-90.f),   // yaw (Y축 회전)
            XMConvertToRadians(0.f)      // roll (Z축 회전)
        );
        m_pTransformCom->Rotation(quat);

        m_pCCT->Renew_Controller();
    }

    else if (iLevelIndex == LEVEL_HARBOR)
    {
        m_pCompositeArmature->Select_Animation("", CGameManager::Parsing_Animation(m_iCharacterNumber, "general_0160_stand_loop"));

        XMVECTOR quat = XMQuaternionRotationRollPitchYaw(
            XMConvertToRadians(0.f),     // pitch (X축 회전)
            XMConvertToRadians(-82.f),   // yaw (Y축 회전)
            XMConvertToRadians(0.f)      // roll (Z축 회전)
        );

        m_pTransformCom->Rotation(quat);

        Get_TransformCom()->Set_State_Notify(CTransform::STATE_POSITION, XMVectorSet(29.140f, 5.731f, 15.332f, 1.f));
        Get_CharacterController()->Apply_GhostMode(false);
        Get_CharacterController()->Apply_Gravity(true);
        m_pCCT->Renew_Controller();
        m_bCanTurn = false;
        Get_BehavioreTree()->Set_BlackboardKey(KEY_BB_STATE, KEY_STATE_IDLE);

        m_pBehaviorTree->Set_Active(false);
    }

    else if (iLevelIndex == LEVEL_VILLAGE)
    {
        m_pBehaviorTree->Set_Active(true);
        m_pBehaviorTree->Set_BlackboardKey(KEY_BB_STATE, KEY_STATE_CHASING);
        m_bCanTurn = true;

        m_pCompositeArmature->Select_Animation("", CGameManager::Parsing_Animation(m_iCharacterNumber, "general_0160_stand_loop"));

        XMVECTOR quat = XMQuaternionRotationRollPitchYaw(
            XMConvertToRadians(0.f),     // pitch (X축 회전)
            XMConvertToRadians(81.f),   // yaw (Y축 회전)
            XMConvertToRadians(0.f)      // roll (Z축 회전)
        );

        m_pTransformCom->Rotation(quat);

        Get_TransformCom()->Set_State_Notify(CTransform::STATE_POSITION, XMVectorSet(-38.05f, 0.434f, -10.07f, 1.f));
        Get_CharacterController()->Apply_GhostMode(false);
        Get_CharacterController()->Apply_Gravity(true);
        m_pCCT->Renew_Controller();

        m_pCompositeArmature->Set_UseHairPhysics(false);
        m_pGameInstance->Add_Timer("CAshley::OnLevelStarted", 3.f, [=]() {
            m_pCompositeArmature->Set_UseHairPhysics(true);
            });
    }
}

void CAshley::OnLevelEnded(_uint iLevelIndex)
{
    if (iLevelIndex != LEVEL_LOADING)
    {
        Get_AbilitySystemComponent()->Cancel_AllAbilities();
        Get_CharacterController()->Apply_GhostMode(true);
        Get_CharacterController()->Apply_Gravity(false);
        Get_BehavioreTree()->Set_BlackboardKey("StateKey", KEY_STATE_IDLE);
    }
}

void CAshley::OnASCTagAddReceived(const GameplayTagContainer& TagContainer)
{
    if (TagContainer.HasTag(GameplayTag(KEY_STATE_CROUCHING)))
    {
        m_bCrouching = true;

        m_pBehaviorTree->Set_BlackboardKey(KEY_BB_CROUCHING, true);
    }
}

void CAshley::OnASCTagRemoveReceived(const GameplayTagContainer& TagContainer)
{
    if (TagContainer.HasTag(GameplayTag(KEY_STATE_CROUCHING)))
    {
        m_bCrouching = false;

        m_pBehaviorTree->Set_BlackboardKey(KEY_BB_CROUCHING, false);
    }
}

void CAshley::OnBlackboardKeyPreChanged(string strKey)
{
    if (strKey == KEY_BB_STATE)
    {
        if (KEY_STATE_CAUTION == m_pBehaviorTree->Get_Blackboard()->Get_Variable<string>(strKey))
        {
            
        }

        else if (KEY_STATE_BEING_CARRIED == m_pBehaviorTree->Get_Blackboard()->Get_Variable<string>(strKey))
        {
            //StopCarry_Ashley(ASHLEY_FREE_TYPE::TYPE_STEALTH_KILLED);

            m_bCautionEnded = true;
        }
    }
}

CAshley* CAshley::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CAshley* pInstance = new CAshley(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CAshley");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CAshley::Clone(void* pArg)
{
    CAshley* pInstance = new CAshley(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Cloned : CAshley");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CAshley::Free()
{
    __super::Free();

    Safe_Release(m_pBehaviorTree);
}
