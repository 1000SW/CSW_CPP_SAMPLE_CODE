#include "Reon.h"
#include "CompositeArmatrue.h"
#include "AnimInstance.h"
#include "AbilitySystemComponent.h"
#include "GameInstance.h"
#include "GameplayAbilitySystemHeader.h"
#include "Context.h"
#include "FSMHeader.h"
#include "Trigger_Socket.h"
#include "MonsterCommonObject.h"
#include "FlashLight.h"
#include "Simple_Factory.h"
#include "DestructibleLootBox.h"
#include "MovePointLight.h"
#include "DistortEffect.h"
#include "Bullet.h"
#include "Laser.h"
#include "BehaviorTreeHeader.h"

CReon::CReon(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    :CPlayerCharacter(pDevice, pContext)
{

}

CReon::CReon(const CReon& Prototype)
    : CPlayerCharacter{ Prototype }
{
    m_iCharacterNumber = 0;
    m_iCustomMask = CT_REON;
}

void CReon::Update_Particle()
{
    if (m_pDustEmitter)
    {
        m_pDustEmitter->Set_WorldMatrix(m_pTransformCom->Get_WorldMatrix());

        if (m_pDustEmitter->Is_Dead())
        {
            cout << "Reon Dust Emitter is Dead" << endl;
        }
    }

    if (m_pMuzzleGasEmitter && false == m_pMuzzleGasEmitter->Is_Dead())
    {
        _matrix matMuzzleSocket;
        if (true == m_pCompositeArmature->Get_SocketTransform(Parsing_WeaponParts(m_eWeaponType), "vfx_muzzle1", matMuzzleSocket))
        {
            _matrix matCombined = matMuzzleSocket * m_pTransformCom->Get_WorldMatrix();
            m_pMuzzleGasEmitter->Set_WorldMatrix(XMMatrixScaling(100.f, 100.f, 100.f) * matCombined);
        }
    }
}

HRESULT CReon::Initialize(void* pArg)
{
    CGameObject::GAMEOBJECT_DESC desc{};

    // For Prision
    m_vPlayerPos = { -13.f, -4.2f, -8.6f }; 

    XMStoreFloat3(&desc.vPosition, m_vPlayerPos);
    desc.fRotationPerSec = XM_PI * 0.4f;
    
    lstrcpy(desc.szGameObjectTag, L"Reon");

    if (E_FAIL == __super::Initialize(&desc))
        return E_FAIL;

    if (E_FAIL == Ready_Components())
        return E_FAIL;

    if (E_FAIL == Ready_Parts())
        return E_FAIL;

    if (E_FAIL == Ready_AnimStateMachine())
        return E_FAIL;

    if (E_FAIL == Ready_AnimLayers())
        return E_FAIL;

    if (E_FAIL == Ready_GAS())
        return E_FAIL;

    if (E_FAIL == Ready_Attribute())
        return E_FAIL;

    if (E_FAIL == Ready_Materials())
        return E_FAIL;

    if (E_FAIL == Ready_Collision())
        return E_FAIL;

    if (E_FAIL == Ready_Particle())
        return E_FAIL;

    if (E_FAIL == Ready_AnimSet())
        return E_FAIL;

    m_pGameInstance->Add_OnLevelStartedDelegate("CReon::OnLevelstarted", [&](_uint iLevelIndex) {OnLevelStarted(iLevelIndex); });
    m_pGameInstance->Add_OnLevelEndedDelegate("CReon", [&](_uint iLevelIndex) {OnLevelEnded(iLevelIndex); });

#ifdef LOAD_UI
    //For.UI
    m_pGameInstance->Register_Script_Target(this, SCRIPT_TARGET::REON);
    m_pGameInstance->Swap_Player_UI(this);
    m_pGameInstance->Update_Player_Hp(1000, 2000);
#endif

    m_bInputEnabled = false;
    
    // 헤어를 등록한다.
    m_pGameInstance->Add_Timer("Late_Hair",
        2.3f, [&]() { 
            Initialize_HairSystem(); 
            m_bInputEnabled = true;
        });

    // 컨트롤러 빙의 요청
    CGameManager::GetInstance()->Change_CurrentCharacter(this);

    // 컬링 사이즈 조절
    m_pBound->Set_CullingSize(2.f);

    m_pGameInstance->Cutscene_AddCharacter("Reon", this);
    m_iCustomMask = CT_REON;

    // 레온보다 지연 생성되는 에슐리를 찾기 위함
    m_pGameInstance->Add_Timer("CReon_Initialize", 0.1f, [&]() {
        m_pAshley = m_pGameInstance->Get_Object<CAshley>(LEVEL_STATIC, L"Layer_Player");
        });

    /* 합동기 후 레온과 에슐리의의 대사가 필요해서 지연 출력 */
    m_bCanShowScript = false;

    return S_OK;
}

HRESULT CReon::Ready_Components()
{
    /* Com_Shader For SubModel */
    if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_Component_Common_VtxMesh"),
        reinterpret_cast<CComponent**>(&m_pModelShader), TEXT("Com_Shader"))))
        return E_FAIL;

    return S_OK;
}

HRESULT CReon::Ready_GAS()
{
    m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_UNOCCUPIED));
    m_pASC->AddLooseGameplayTag(GameplayTag(KEY_STATE_WALKABLE));

    /* For Gameplay */
    m_pASC->Give_Ability("Move", CAbility_Walk::Create());
    m_pASC->Give_Ability("Crouching", CAbility_Crouching::Create());
    m_pASC->Give_Ability("Dash", CAbility_Dash::Create());
    m_pASC->Give_Ability("Idle", CAbility_Idle::Create());

    m_pASC->Give_Ability("Aim", CAbility_Aim::Create());
    m_pASC->Give_Ability("Shoot", CAbility_Shoot::Create());
    m_pASC->Give_Ability("Knife", CAbility_Knife::Create());
    m_pASC->Give_Ability("HoldingKnife", CAbility_HoldingKnife::Create());

    m_pASC->Give_Ability("ChangeWeapon", CAbility_ChangeWeapon::Create());
    m_pASC->Give_Ability("Reload", CAbility_Reload::Create());

    m_pASC->Give_Ability("Hit", CAbility_Hit::Create());
    m_pASC->Give_Ability("FlashLight", CAbility_FlashLight::Create());

    m_pASC->Give_Ability("Ladder", CAbility_Ladder::Create());
    m_pASC->Give_Ability("Fall", CAbility_Fall::Create());
    m_pASC->Give_Ability("StepUp", CAbility_StepUp::Create());

    m_pASC->Give_Ability("Calling", CAbility_Calling::Create());

    m_pASC->Give_Ability("JointAttack", CAbility_JointAttack::Create());

    m_pASC->Give_Ability("DestroyItem", CAbility_DestroyItem::Create());
    m_pASC->Give_Ability("ObtainItem", CAbility_ObtainItem::Create());

    m_pASC->Give_Ability("QTE", CAbility_QTE::Create());
    //m_pASC->Give_Ability("DoorControl", CAbility_DoorControl::Create());
    m_pASC->Give_Ability("MoveUp", CAbility_MoveUp::Create());

    m_pASC->Give_Ability("HelpAshley", CAbility_HelpAshley::Create());

    m_pASC->m_OnTagAddedDelegate.AddDynamic("CReon", this, &CReon::OnASCTagAddReceived);
    m_pASC->m_OnTagRemovedDelegate.AddDynamic("CReon", this, &CReon::OnASCTagRemoveReceived);

    return S_OK;
}

HRESULT CReon::Ready_Parts()
{
    m_pCompositeArmature->Set_BaseArmature(
        TEXT("Prototype_Component_Model_Reon"),
        TEXT("Prototype_Component_Armature_Reon"),
        LEVEL_STATIC);

    /** animation set setting */
    m_pCompositeArmature->Set_AnimationSet("",
        TEXT("Prototype_Component_AnimSet_Reon"),
        LEVEL_STATIC);

    m_pAnimInstance->Set_MainAnimator(m_pCompositeArmature->Get_Animator(""));

    /** additional Head */
    m_pCompositeArmature->Add_Part("Head",
        TEXT("Prototype_Component_Model_ReonHead"),
        TEXT("Prototype_Component_Armature_ReonHead"),
        LEVEL_STATIC);

    m_pCompositeArmature->Set_AnimationSet("Head",
        TEXT("Prototype_Component_AnimSet_ReonHead"),
        LEVEL_STATIC);

    /** addtional Hair */
    m_pCompositeArmature->Add_Part(
        "ReonHair",
        TEXT("Prototype_Component_Model_ReonHair"),
        TEXT("Prototype_Component_Armature_ReonHair"),
        LEVEL_STATIC);

    /* Pistol */
    m_pCompositeArmature->Add_Part(
        "Pistol",
        TEXT("Prototype_Component_Model_SG-09R"),
        TEXT("Prototype_Component_Armature_SG-09R"),
        LEVEL_STATIC);

    m_pCompositeArmature->Set_AnimationSet("Pistol",
        TEXT("Prototype_Component_AnimSet_SG-09R"),
        LEVEL_STATIC);

    m_pCompositeArmature->Activate_Part("Pistol", false);
    m_pCompositeArmature->Set_MotionSpeed("Pistol", 60.f);

    /* Shotgun */
    m_pCompositeArmature->Add_Part(
        "Shotgun",
        TEXT("Prototype_Component_Model_W-870"),
        TEXT("Prototype_Component_Armature_W-870"),
        LEVEL_STATIC);

    /* Back Shotgun */
    _matrix matOffsetTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationRollPitchYaw(XMConvertToRadians(-45.000f), XMConvertToRadians(90.000f), -XMConvertToRadians(0.f)) * XMMatrixTranslation(0.f, 0.f, 0.100f);
    m_pCompositeArmature->Add_ToSlot("", "Back_Shotgun", "Wep_chain_00", L"Prototype_Component_Model_W-870", m_pModelShader, 0, matOffsetTransform, LEVEL_STATIC);
    m_pCompositeArmature->Activate_SubPart("Back_Shotgun", false);

    //m_pCompositeArmature->Set_AnimationSet("Shotgun",
    //    TEXT("Prototype_Component_AnimSet_W-870"),
    //    LEVEL_STATIC);

    m_pCompositeArmature->Activate_Part("Shotgun", false);

    ///* Rifle */
    m_pCompositeArmature->Add_Part(
        "Rifle",
        TEXT("Prototype_Component_Model_SR-M1903"),
        TEXT("Prototype_Component_Armature_SR-M1903"),
        LEVEL_STATIC);

    m_pCompositeArmature->Set_AnimationSet("Rifle",
        TEXT("Prototype_Component_AnimSet_SR-M1903"),
        LEVEL_STATIC);

    m_pCompositeArmature->Set_MotionSpeed("Rifle", 60.f);

    m_pCompositeArmature->Activate_Part("Rifle", false);

    /* Back Rifle */
    matOffsetTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationRollPitchYaw(XMConvertToRadians(-55.000f), XMConvertToRadians(90.000f), -XMConvertToRadians(0.f)) * XMMatrixTranslation(0.095f, 0.018f, 0.075f);
    m_pCompositeArmature->Add_ToSlot("", "Back_Rifle", "Wep_chain_00", L"Prototype_Component_Model_SR-M1903", m_pModelShader, 0, matOffsetTransform, LEVEL_STATIC);
    m_pCompositeArmature->Activate_SubPart("Back_Rifle", false);

    /* Combat Knife */
    _matrix matModifyTransform = XMMatrixRotationRollPitchYaw(XMConvertToRadians(2.f), XMConvertToRadians(30.f), -XMConvertToRadians(80.f)) * XMMatrixTranslation(-0.085f, -0.032f, -0.015f);
    m_pCompositeArmature->Add_ToSlot("", "Knife", "R_Wep", KEY_PROTO_MODEL_COMBAT_KNIFE, m_pModelShader, 0, matModifyTransform, LEVEL_STATIC);
    m_pCompositeArmature->Activate_SubPart("Knife", false);

    /* Grenade */
    _matrix matGrandeModifyTransform = XMMatrixRotationRollPitchYaw(XM_PI, XMConvertToRadians(-65.f), XM_PIDIV2) * XMMatrixTranslation(-0.080f, -0.040f, -0.010f);

    m_pCompositeArmature->Add_ToSlot("", "Grenade", "R_Wep", KEY_PROTO_MODEL_GRENADE, m_pModelShader, 0, matGrandeModifyTransform, LEVEL_STATIC);
    m_pCompositeArmature->Activate_SubPart("Grenade", false);

    /* FlashBang */
    m_pCompositeArmature->Add_ToSlot("", "FlashBang", "R_Wep", KEY_PROTO_MODEL_FLASHBANG, m_pModelShader, 0, matGrandeModifyTransform, LEVEL_STATIC);
    m_pCompositeArmature->Activate_SubPart("FlashBang", false);

    /* FlashLight */
    m_pCompositeArmature->Add_ToSlot("", "FlashLight", "L_Wep", KEY_PROTO_MODEL_FLASHLIGHT, m_pModelShader, 0,
        XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationRollPitchYaw(XMConvertToRadians(-66.f), XMConvertToRadians(45.f), XMConvertToRadians(27.f)) * XMMatrixTranslation(0.08f, 0.f, -0.12f)
        , LEVEL_PRISON);
    m_pCompositeArmature->Activate_SubPart("FlashLight", false);
    
    /* RPG */
    m_pCompositeArmature->Add_Part(
        "RPG",
        TEXT("Prototype_Component_Model_RPG"),
        TEXT("Prototype_Component_Armature_RPG"),
        LEVEL_STATIC);
    m_pCompositeArmature->Activate_Part("RPG", false);

    /* SMG */
    m_pCompositeArmature->Add_Part(
        "SMG",
        TEXT("Prototype_Component_Model_SMG"),
        TEXT("Prototype_Component_Armature_SMG"),
        LEVEL_STATIC);
    m_pCompositeArmature->Set_AnimationSet("SMG",
        TEXT("Prototype_Component_AnimSet_SMG"),
        LEVEL_STATIC);
    m_pCompositeArmature->Set_MotionSpeed("SMG", 60.f);

    m_pCompositeArmature->Activate_Part("SMG", false);

    /* Back SMG */
    matOffsetTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationRollPitchYaw(XMConvertToRadians(-60.f), XMConvertToRadians(90.000f), -XMConvertToRadians(0.f)) * XMMatrixTranslation(-0.07f, -0.2f, 0.08f);
    m_pCompositeArmature->Add_ToSlot("", "Back_SMG", "Wep_chain_00", L"Prototype_Component_Model_SMG", m_pModelShader, 0, matOffsetTransform, LEVEL_STATIC);
    m_pCompositeArmature->Activate_SubPart("Back_SMG", false);


    /* Collision Sockets */
    vector<BONE_BIND> BindList = CMyUtils::Load_BoneBind("../Bin/Resources/ColInfo/Player/Reon.yaml");
    if (BindList.empty())
        return E_FAIL;

    /* TriggerSocket For Knife Collider */
    CTriggerSocket::TriggerSoket_DESC TriggerDesc{};

    TriggerDesc.strPartsName = BindList[0].strPartsName;
    TriggerDesc.strBoneName = BindList[0].strBoneName;

    TriggerDesc.eRigidType = BindList[0].eTypeRigid;

    TriggerDesc.vScale = BindList[0].vRigidScale;
    TriggerDesc.vOffsetTranslation = BindList[0].vTranslation;
    TriggerDesc.vOffsetRotation = BindList[0].vRotation;

    TriggerDesc.fSpeedPerSec = 0.9f;
    TriggerDesc.fRotationPerSec = XM_PI * 0.4f;
    TriggerDesc.fRadius = 1.f;

    TriggerDesc.pCompositeArmature = m_pCompositeArmature;
    TriggerDesc.pObject = this;
    TriggerDesc.pOwner = this;
    TriggerDesc.pRealOwner = this;
    TriggerDesc.iMask = OT_PLAYER;

    CTriggerSocket* pSocket = dynamic_cast<CTriggerSocket*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::TYPE_GAMEOBJECT, LEVEL_STATIC, TEXT("Prototype_GameObject_TriggerSocket"), &TriggerDesc));
    m_pTriggerKnife = pSocket;

    /* TriggerSocket For Leg Collider*/
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
    TriggerDesc.iMask = OT_PLAYER;

    pSocket = dynamic_cast<CTriggerSocket*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::TYPE_GAMEOBJECT, LEVEL_STATIC, TEXT("Prototype_GameObject_TriggerSocket"), &TriggerDesc));
    m_pTriggerLeg = pSocket;

    /* Etc */
    m_pCompositeArmature->Set_RootBoneTransform("root", m_pTransformCom, true);
    m_pCompositeArmature->Select_Animation("", "cha0_general_0160_stand_loop", true);
    m_pCompositeArmature->Set_MotionSpeed("", 60.f);

    CFlashLight::FLASHLIGHT_DESC flashDesc;
    flashDesc.wstrGoboInnerPath = L"../Bin/Resources/Textures/Effect/Tex_Light/tex_capcom_light_ring_0000_ALPG.dds";
    flashDesc.wstrGoboOuterPath = L"../Bin/Resources/Textures/Effect/Tex_Light/tex_capcom_light_ring_0002_ALPG.dds";
    flashDesc.wstrGoboMiddlePath = L"../Bin/Resources/Textures/Effect/Tex_Light/tex_capcom_light_ring_0004_ALPG.dds";

    flashDesc.SocketMatrix = const_cast<_float4x4*>(m_pCompositeArmature->Get_SocketMatrixPtr("", "L_Wep"));
    flashDesc.pParentObject = this;

    m_pGameInstance->Add_GameObject(LEVEL_PRISON, L"Flash_Light", LEVEL_PRISON, L"Layer_FlashLight", &flashDesc);
    m_pFlashLight = m_pGameInstance->Get_Object<CFlashLight>(LEVEL_PRISON, L"Layer_FlashLight");
    if (m_pFlashLight)
        m_pFlashLight->Turn_On(false);

    m_MovePointLights.resize(POINTLIGHT_END);

    CMovePointLight::FOLLOWPOINT_DESC Desc;
    Desc.radius = 3.f;
    lstrcpy(Desc.szGameObjectTag, TEXT("ReonLight01"));
    m_pGameInstance->Add_GameObject(LEVEL_STATIC, L"MovePointLight", LEVEL_STATIC, L"Layer_Light", &Desc);
    m_MovePointLights[POINTLIGHT_HITPOINT] = dynamic_cast<CMovePointLight*>(m_pGameInstance->Get_LastObject(LEVEL_STATIC, L"Layer_Light"));
    Safe_AddRef(m_MovePointLights[POINTLIGHT_HITPOINT]);

    Desc.radius = 2.f;
    Desc.fFallOffStart = 2.f;
    lstrcpy(Desc.szGameObjectTag, TEXT("ReonLight02"));
    m_pGameInstance->Add_GameObject(LEVEL_STATIC, L"MovePointLight", LEVEL_STATIC, L"Layer_Light", &Desc);
    m_MovePointLights[POINTLIGHT_ATTACHED] = dynamic_cast<CMovePointLight*>(m_pGameInstance->Get_LastObject(LEVEL_STATIC, L"Layer_Light"));
    Safe_AddRef(m_MovePointLights[POINTLIGHT_ATTACHED]);

    Desc.radius = 13.f;
    lstrcpy(Desc.szGameObjectTag, TEXT("ReonLight03"));
    m_pGameInstance->Add_GameObject(LEVEL_STATIC, L"MovePointLight", LEVEL_STATIC, L"Layer_Light", &Desc);
    m_MovePointLights[POINTLIGHT_MUZZLE] = dynamic_cast<CMovePointLight*>(m_pGameInstance->Get_LastObject(LEVEL_STATIC, L"Layer_Light"));
    Safe_AddRef(m_MovePointLights[POINTLIGHT_MUZZLE]);


    // Distortion Effect 를 단다
    CDistortEffect::DISTORT_DESC objDesc;
    lstrcpy(objDesc.szGameObjectTag, TEXT("Distortion_Reon"));
    objDesc.boundType = BOUNDTYPE::NONE;
    objDesc.eRigidType = RIGIDSHAPE::NONE;

    objDesc.SocketMatrix = const_cast<_float4x4*>(m_pCompositeArmature->Get_SocketMatrixPtr("", "R_Wep"));
    objDesc.pParentObject = this;

    // 디스토션 머즐을 장착
    m_pDistortMuzzle = dynamic_cast<CDistortEffect*>(m_pGameInstance->
        Clone_Prototype(PROTOTYPE::TYPE_GAMEOBJECT, LEVEL_STATIC, TEXT("Prototype_GameObject_Distortion"), &objDesc));
    if (nullptr == m_pDistortMuzzle)
        return E_FAIL;
    m_pGameInstance->Add_GameObject(m_pDistortMuzzle, LEVEL_STATIC, TEXT("Layer_Effect"));

    // Bullet Effect 를 단다
    // lstrcpy(objDesc.szGameObjectTag, TEXT("Bullet_Reon"));
    // m_pBullet = dynamic_cast<CBullet*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::TYPE_GAMEOBJECT, 
    //     LEVEL_STATIC, TEXT("Prototype_GameObject_Bullet"), &objDesc));
    // if (nullptr == m_pBullet)
    //     return E_FAIL;
    // m_pGameInstance->Add_GameObject(m_pBullet, LEVEL_STATIC, TEXT("Layer_Effect"));
    
    // Laser Effect 를 단다
    lstrcpy(objDesc.szGameObjectTag, TEXT("Laser_Reon"));
    m_pLaser = dynamic_cast<CLaser*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::TYPE_GAMEOBJECT, 
        LEVEL_STATIC, TEXT("Prototype_GameObject_Laser"), &objDesc));
    if (nullptr == m_pLaser)
        return E_FAIL;
    m_pGameInstance->Add_GameObject(m_pLaser, LEVEL_STATIC, TEXT("Layer_Effect"));

    //if (FAILED(m_pGameInstance->Add_GameObject(LEVEL_STATIC, TEXT("Prototype_GameObject_Distortion"),
    //    LEVEL_STATIC, L"Layer_Effect", &objDesc)))
    //    return E_FAIL;
    //m_pDistortMuzzle = m_pGameInstance->Get_Object<CDistortEffect>(LEVEL_STATIC, L"Layer_Effect");

    return S_OK;
}

HRESULT CReon::Ready_AnimStateMachine()
{
    m_pVariableContext->Add_Variable<CCompositeArmature*>("ArmatureKey", m_pCompositeArmature);
    m_pVariableContext->Add_Variable<CAnimInstance*>("AnimInstanceKey", m_pAnimInstance);

    m_pVariableContext->Add_Variable<_bool*>("DashKey", &m_bDashActivate);
    m_pVariableContext->Add_Variable<_bool*>("FallingKey", &m_bFalling);
    m_pVariableContext->Add_Variable<_bool*>("CrouchingKey", &m_bCrouching);
    m_pVariableContext->Add_Variable<_bool*>("AimingKey", &m_bAiming);
    m_pVariableContext->Add_Variable<_bool*>("HoldingKnifeKey", &m_bHoldingKnife);

    m_pVariableContext->Add_Variable<_int*>("MoveKey", (_int*)&m_eInputDIR);
    m_pVariableContext->Add_Variable<_int*>("WeaponKey", (_int*)&m_eWeaponType);
    m_pVariableContext->Add_Variable<_int*>("ConditionKey", (_int*)&m_eConditionType);
    m_pVariableContext->Add_Variable<_int*>("FootStepKey", (_int*)&m_eFootStepType);

    m_pVariableContext->Add_Variable<CAbilitySystemComponent*>("ASCKey", m_pASC);

    m_pVariableContext->Add_Variable<_int>("CharacterKey", m_iCharacterNumber);


    /* Ready AnimStateMachine */
    CFSM* pFSM = CFSM::Create(m_pDevice, m_pContext);
    CAnimState* pAnimState;
    CFSM_Transition* pTransition;
    function<bool(CContext*)> fnTransitionLogic;

    // Idle, Walk, Falling

    /* For Idle State*/
    pAnimState = CAnimStateIdle::Create(m_pVariableContext);

    GameplayTagContainer TagContainer;

    // Idle -> Walk
    TagContainer.AddTag(GameplayTag("State.Walk"));
    pTransition = CAnim_Transition_Query_ASC::Create(m_pASC, TagContainer);
    TagContainer.Clear();
    pAnimState->Add_Transition(STATE_WALK, pTransition);

    /* Add State to FSM And Set Initial State*/
    pFSM->Add_State(STATE_IDLE, pAnimState);
    pFSM->Set_IntialState(pAnimState);

    /* For Walk State */
    pAnimState = CAnimStateWalk::Create(m_pVariableContext);

    TagContainer.AddTag(GameplayTag(KEY_STATE_IDLE));
    pTransition = CAnim_Transition_Query_ASC::Create(m_pASC, TagContainer);
    TagContainer.Clear();
    pAnimState->Add_Transition(STATE_IDLE, pTransition);

    TagContainer.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_DASH));
    pTransition = CAnim_Transition_Query_ASC::Create(m_pASC, TagContainer);
    TagContainer.Clear();
    pAnimState->Add_Transition(STATE_DASH, pTransition);

    pFSM->Add_State(STATE_WALK, pAnimState);

    /* For Dash State */
    pAnimState = CAnimStateDash::Create(m_pVariableContext);

    TagContainer.AddTag(GameplayTag(KEY_STATE_IDLE));
    pTransition = CAnim_Transition_Query_ASC::Create(m_pASC, TagContainer);
    TagContainer.Clear();
    pAnimState->Add_Transition(STATE_IDLE, pTransition);

    TagContainer.AddTag(GameplayTag(KEY_ABILITY_MOVEMENT_LOCOMOTION_WALK));
    pTransition = CAnim_Transition_Query_ASC::Create(m_pASC, TagContainer);
    TagContainer.Clear();
    pAnimState->Add_Transition(STATE_WALK, pTransition);

    pFSM->Add_State(STATE_DASH, pAnimState);

    /* Add to AnimInstance */
    m_pAnimInstance->Add_FSM("MainStateMachine", pFSM);

    return S_OK;
}

HRESULT CReon::Ready_AnimStateMachine_Upper()
{
    CFSM* pFSM = CFSM::Create(m_pDevice, m_pContext);
    CAnimState* pAnimState;
    CFSM_Transition* pTransition;
    GameplayTagContainer TagContainer;

    // Stand, Crouch, Aim

    /* For Stand State*/
    pAnimState = CAnimState_Upper_Stand::Create(m_pVariableContext);

    /* Stand => Aiming*/
    TagContainer.AddTag(GameplayTag(KEY_STATE_AIMING));
    pTransition = CAnim_Transition_Query_ASC::Create(m_pASC, TagContainer);
    TagContainer.Clear();
    pAnimState->Add_Transition(UAS_AIM, pTransition);

    /* Stand => Crouch*/
    TagContainer.AddTag(GameplayTag(KEY_STATE_CROUCHING));
    pTransition = CAnim_Transition_Query_ASC::Create(m_pASC, TagContainer);
    TagContainer.Clear();
    pAnimState->Add_Transition(UAS_CROUCH, pTransition);

    /* Add State to FSM And Set Initial State */
    pFSM->Add_State(UAS_STAND, pAnimState);
    pFSM->Set_IntialState(pAnimState);

    /* For Aim State */
    pAnimState = CAnimState_Upper_Aim::Create(m_pVariableContext);

    /* Aim => Stand */
    TagContainer.AddTag(GameplayTag(KEY_STATE_AIMING));
    pTransition = CAnim_Transition_Query_ASC::Create(m_pASC, TagContainer, true);
    pAnimState->Add_Transition(STATE_IDLE, pTransition);

    pFSM->Add_State(UAS_AIM, pAnimState);

    /* For Crouch State */
    pAnimState = CAnimState_Upper_Crouch::Create(m_pVariableContext);

    /* Crouch => Aim */
    TagContainer.AddTag(GameplayTag(KEY_STATE_AIMING));
    pTransition = CAnim_Transition_Query_ASC::Create(m_pASC, TagContainer);
    TagContainer.Clear();
    pAnimState->Add_Transition(UAS_AIM, pTransition);

    /* Crouch => Stand*/
    TagContainer.AddTag(GameplayTag(KEY_STATE_CROUCHING));
    pTransition = CAnim_Transition_Query_ASC::Create(m_pASC, TagContainer, true);
    TagContainer.Clear();
    pAnimState->Add_Transition(UAS_STAND, pTransition);

    pFSM->Add_State(UAS_CROUCH, pAnimState);

    // 최종적으로 FSM을 추가한다. 
    m_pAnimInstance->Add_SubFSM(pFSM);

    return S_OK;
}

HRESULT CReon::Ready_AnimLayers()
{
    /* Composite Armature Notify */
    m_pCompositeArmature->Get_ArmatureByPart("")->Load_NotifyFile("../Bin/Datafiles/Animations/Player/Reon.json", CSimple_Factory::Create_Notify, CSimple_Factory::Create_NotifyState);

    // Priority의 숫자가 높을 수록 에님 레이어의 적용 순서가 먼저 적용되고 낮을 수록 나중에 적용됨

    /* 전신 몽타주 슬롯*/
    vector<string> SplitBones = {"root"};
    vector<string> IgnoreBones;
    vector<string> SelectedBones;

    m_pCompositeArmature->Add_AnimLayer("", "Default", SplitBones, AnimationBlendMode::OVERWRITE, 0, IgnoreBones, 0.2f, 0.2f, true);

    /* 상체 몽타주 슬롯 */
    // 양 다리 Bone Index 시작점
    SelectedBones = { "Spine_2"};
    IgnoreBones = { "L_Thigh", "R_Thigh" };
    m_pCompositeArmature->Add_AnimLayer("", "Upper", SelectedBones, AnimationBlendMode::OVERWRITE, 2, IgnoreBones, 0.3f, 0.33f, true);

    SelectedBones = { "Spine_2" };
    IgnoreBones = { "L_Thigh", "R_Thigh" };
    m_pCompositeArmature->Add_AnimLayer("", "Upper_Priority", SelectedBones, AnimationBlendMode::OVERWRITE, -4, IgnoreBones, 0.3f, 0.33f, true);

    /* Add 에니메이션 전용 슬롯*/    // 반동 에니메이션 재생 시 무기가 팔에 부착되지않아 양 어깨를 무시하는 본 목록에 추가함
    SelectedBones = { "root" };
    IgnoreBones = { "R_Shoulder", "L_Shoulder" };
    m_pCompositeArmature->Add_AnimLayer("", "Add", SelectedBones, AnimationBlendMode::ADDTIVE, 1, IgnoreBones, 0.2f, 0.2f, true);

    SelectedBones = { "root" };
    //IgnoreBones = { "R_Shoulder", "L_Shoulder" };
    m_pCompositeArmature->Add_AnimLayer("", "Add_Breath", SelectedBones, AnimationBlendMode::ADDTIVE, 100, {}, 0.2f, 0.2f);

    /*SelectedBones = { "root" };
    IgnoreBones = { "R_Shoulder", "L_Shoulder" };
    m_pCompositeArmature->Add_AnimLayer("", "Add", SelectedBones, AnimationBlendMode::ADDTIVE, 1, IgnoreBones);*/

    /* 왼팔 슬롯 For 손전등 */
    m_pCompositeArmature->Add_AnimLayer("", "LeftArm", "L_Shoulder", AnimationBlendMode::OVERWRITE, -3);

    /* 왼팔 + 오른팔 슬롯 주로 무기 가만히 들고있을 때 사용할 레이어임*/
    SelectedBones = { "L_Shoulder", "R_Shoulder" };
    m_pCompositeArmature->Add_AnimLayer("", "BothArms", SelectedBones, AnimationBlendMode::OVERWRITE, 10, {}, 0, 0);

    return S_OK;
}

HRESULT CReon::Ready_Materials()
{
    // 메터리얼을 추가시킨다.
    queue<pair<wstring, wstring>> detailMapQueue;
    detailMapQueue.push({ TEXT("../Bin/Resources/Models/Enm/Cha000_00/"), TEXT("msk4") });
    if (FAILED(Check_Materials(m_pCompositeArmature, &detailMapQueue)))
        return E_FAIL;

    // 미리 저장된 프리셋을 불러온다.
    if (FAILED(Load_Materials("../Bin/Resources/Materials/Player/ReonMat.yaml")))
        return E_FAIL;

    return S_OK;
}

HRESULT CReon::Ready_Collision()
{
    if (nullptr == m_pTriggerKnife || nullptr == m_pTriggerLeg)
        return E_FAIL;
    
    m_pTriggerKnife->Get_Bound()->Set_Mask(OT_PLAYER);
    m_pTriggerLeg->Get_Bound()->Set_Mask(OT_PLAYER);

    m_pTriggerKnife->Get_Bound()->Add_CollisionCallBack(
        [&](CBound* pBound, COLLISION_INFO* pInfo)
        {
            if (m_bKnifeOn)
            {
                DamageEventRE DamageEvent;
                DamageEvent.ColInfo = *pInfo;

                DamageEvent.eType = DAMAGE_NORMAL;
                DamageEvent.pOverlappedComp = pBound;

                if (m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_PARRYABLE)))
                    DamageEvent.eReactionType = REACTION_PARRY;
                else
                    DamageEvent.eReactionType = REACTION_NONE;

                pBound->Get_Owner()->Take_Damage(200.f, &DamageEvent, this, m_pTriggerKnife);
            }
        });

    m_pTriggerLeg->Get_Bound()->Add_CollisionCallBack(
        [&](CBound* pBound, COLLISION_INFO* pInfo)
        {
            if (m_bKickable)
            {
                m_DamegeInfo.pOverlappedComp = pBound;
                pBound->Get_Owner()->Take_Damage(m_DamegeInfo.fDamage, &m_DamegeInfo, this, m_pTriggerLeg);
                m_DamegeInfo.pOverlappedComp = nullptr;
            }
            else if (m_bAttacktable)
            {
                DamageEventRE DamEventRE;
                DamEventRE.pOverlappedComp = pBound;
                // 지형지물과만 상호작용하기위한 타입 설정
                DamEventRE.eType = DAMAGE_END;
                DamEventRE.eReactionType = REACTION_END;
                pBound->Get_Owner()->Take_Damage(200.f, &DamEventRE, this, m_pTriggerLeg);
            }
        });

    return S_OK;
}

HRESULT CReon::Ready_Attribute()
{
    m_pAttributeSet->Init_Value("fLimitHP", 2500.f);
    m_pAttributeSet->Init_Value("MaxHP", 2000.f);
    m_pAttributeSet->Init_Value("CurrentHP", 1000.f);
    m_pAttributeSet->Init_Value("fAimSpread", 1.f);

    return S_OK;
}

HRESULT CReon::Ready_Particle()
{
    m_pGameInstance->Add_Timer("CReon", 0.1f, [&]() {m_pGameInstance->Spawn_Particle(L"Dust", m_pDustEmitter); });

    return S_OK;
}

void CReon::Initialize_HairSystem()
{
    // 헤어 조인트 설정 - 더 부드럽고 자연스러운 값으로 조정
    HAIR_JOINT_DESC jointDesc;
    jointDesc.fSwingLimit1 = XM_PI / 6.0f;      // 30도 (더 자연스러운 움직임)
    jointDesc.fSwingLimit2 = XM_PI / 6.0f;
    jointDesc.fTwistLimitLower = -XM_PI / 12.0f; // -15도
    jointDesc.fTwistLimitUpper = XM_PI / 12.0f;  // 15도
    jointDesc.fStiffness = 50.0f;    // 강성 감소 (더 유연하게)
    jointDesc.fDamping = 5.0f;        // 댐핑 감소 (더 자연스럽게)
    jointDesc.fRestitution = 0.05f;   // 반발력 감소

    // 헤어 본 정보 설정
    // _float radius _float length;

     // 헤어 체인별로 그룹화하여 추가
    const _float baseRadius = 0.015f;  // 기본 반지름
    const _float tipRadius = 0.008f;   // 끝부분 반지름
    float dynamicRadius =  tipRadius;
    float capsuleLength = 0.3f; 

    vector<HAIR_BONE_INFO> hairBones;
    hairBones.push_back({ "hair00_chain_00", "", baseRadius, 0.04f, true});
    hairBones.push_back({ "hair00_chain_01", "hair00_chain_00", dynamicRadius, capsuleLength, false});
    hairBones.push_back({ "hair01_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair01_chain_01", "hair01_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair02_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair02_chain_01", "hair02_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair03_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair03_chain_01", "hair03_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair04_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair04_chain_01", "hair04_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair05_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair05_chain_01", "hair05_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair06_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair06_chain_01", "hair06_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair07_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair07_chain_01", "hair07_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair08_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair08_chain_01", "hair08_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair09_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair09_chain_01", "hair09_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair10_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair10_chain_01", "hair10_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair11_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair11_chain_01", "hair11_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair12_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair12_chain_01", "hair12_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair13_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair13_chain_01", "hair13_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair14_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair14_chain_01", "hair14_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair15_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair15_chain_01", "hair15_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair16_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair16_chain_01", "hair16_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair17_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair17_chain_01", "hair17_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair18_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair18_chain_01", "hair18_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair19_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair19_chain_01", "hair19_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair20_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair20_chain_01", "hair20_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair21_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair21_chain_01", "hair21_chain_00", dynamicRadius, capsuleLength, false });
    hairBones.push_back({ "hair22_chain_00", "", baseRadius, 0.04f, true });
    hairBones.push_back({ "hair22_chain_01", "hair22_chain_00", dynamicRadius, capsuleLength, false });

    m_pCompositeArmature->Add_HairSystem("ReonHair", hairBones, jointDesc);
}

void CReon::Turn_On_HairSystem(_bool bOn)
{
    m_pCompositeArmature->Set_UseHairPhysics(bOn);
}

void CReon::Priority_Update(_float fTimeDelta)
{
    if (m_pTriggerKnife) 
        m_pTriggerKnife->Priority_Update(fTimeDelta);

    if (m_pTriggerLeg)
        m_pTriggerLeg->Priority_Update(fTimeDelta);

    Update_Particle();

    //// Debug Code
    //if(m_pGameInstance->Key_Down(DIK_Y))
    //{
    //    Get_CharacterController()->Apply_Gravity(false);
    //}

    __super::Priority_Update(fTimeDelta);
}

void CReon::Key_Input()
{
    DebugKey_Input();

#ifdef LOAD_UI
    if (CGameManager::GetInstance()->Get_isOpen())
        return;
#endif

    if (!m_bInputEnabled)
        return;

    m_ePrevInputDIR = m_eInputDIR;

    if (m_pGameInstance->Key_Pressing(DIK_W) && m_pGameInstance->Key_Pressing(DIK_D))
        m_eInputDIR = DIR_FRONTRIGHT;
    else if (m_pGameInstance->Key_Pressing(DIK_W) && m_pGameInstance->Key_Pressing(DIK_A))
        m_eInputDIR = DIR_FRONTLEFT;
    else if (m_pGameInstance->Key_Pressing(DIK_W))
        m_eInputDIR = DIR_FRONT;
    else if (m_pGameInstance->Key_Pressing(DIK_S) && m_pGameInstance->Key_Pressing(DIK_D))
        m_eInputDIR = DIR_BACKRIGHT;
    else if (m_pGameInstance->Key_Pressing(DIK_S) && m_pGameInstance->Key_Pressing(DIK_A))
        m_eInputDIR = DIR_BACKLEFT;
    else if (m_pGameInstance->Key_Pressing(DIK_S))
        m_eInputDIR = DIR_BACK;

    else if (m_pGameInstance->Key_Pressing(DIK_D))
        m_eInputDIR = DIR_RIGHT;
    else if (m_pGameInstance->Key_Pressing(DIK_A))
        m_eInputDIR = DIR_LEFT;
    else
        m_eInputDIR = DIR_END;

    Ability_KeyInput();
}

void CReon::Ability_KeyInput()
{
    if (m_pGameInstance->Key_Down(DIK_LCONTROL))
    {
        m_pASC->AbilityLocalInputPressed(ACTION_HELP_ASHLEY);
    }

    if (m_pGameInstance->Key_Down(DIK_F))
    {
        if (m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_ITEM_OBTAINABLE)))
            m_pASC->AbilityLocalInputPressed(ACTION_PICKUP_ITEM);
        else if (m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_ITEM_DESTROYABLE)))
            m_pASC->AbilityLocalInputPressed(ACTION_DESTROY_ITEM);
        else if (m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_LADDERUSEABLE)))
            m_pASC->AbilityLocalInputPressed(ACTION_LADDER);
        else if (m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_CLIMBABLE)))
        {
            if (m_pTrigger && m_pTrigger->Get_TriggerData().vecIntValues.size())
            {
                GameplayEventData EventData;
                EventData.EventTag = GameplayTag(KEY_EVENT_ON_CLIMB);
                EventData.pTarget = m_pTrigger;
                m_pASC->TriggerAbility_FromGameplayEvent("StepUp", nullptr, EventData.EventTag, &EventData);
            }
        }
        else if (m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_FALLABLE)))
        {
            if (m_pTrigger)
            {
                GameplayEventData EventData;
                EventData.EventTag = GameplayTag(KEY_EVENT_ON_FALLING);
                EventData.pTarget = m_pTrigger;
                m_pASC->TriggerAbility_FromGameplayEvent("Fall", nullptr, EventData.EventTag, &EventData);
            }
        }

        else if (m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_CAN_MOVE_UP)))
        {
            m_pASC->AbilityLocalInputPressed(ACTION_MOVEUP);
        }
        else if (false == m_TargetMonsters.empty())
        {
            GameplayTag EventTag = GameplayTag(KEY_EVENT_JOINT_ATTACK_INPUT_F);
            GameplayEventData EventData;
            EventData.EventTag = EventTag;
            m_pASC->TriggerAbility_FromGameplayEvent("JointAttack", nullptr, EventTag, &EventData);
            //m_pASC->TryActivate_Ability("JointAttack");
        }
    }

#ifdef LOAD_UI
    for (int i = DIK_1; i <= DIK_7; i++)
    {
        if (m_pGameInstance->Key_Down(i))
        {
            Change_Weapon_InteractWithUI(i);
            m_pASC->AbilityLocalInputPressed(ACTION_CHANGE_WEAPON);
            break;
        }
    }

    if (m_pGameInstance->Key_Down(DIK_8))
    {
        m_eChangeWeaponType = WEAPON_ROCKET_LAUNCHER;
        m_pASC->AbilityLocalInputPressed(ACTION_CHANGE_WEAPON);
    }

#else
    if (m_pGameInstance->Key_Down(DIK_1))
    {
        m_eChangeWeaponType = WEAPON_PISTOL;
        m_pASC->AbilityLocalInputPressed(ACTION_CHANGE_WEAPON);
    }

    else if (m_pGameInstance->Key_Down(DIK_2))
    {
        m_eChangeWeaponType = WEAPON_SMG;
        m_pASC->AbilityLocalInputPressed(ACTION_CHANGE_WEAPON);
    }
    else if (m_pGameInstance->Key_Down(DIK_3))
    {
        m_eChangeWeaponType = WEAPON_SHOTGUN;
        m_pASC->AbilityLocalInputPressed(ACTION_CHANGE_WEAPON);
    }
    else if (m_pGameInstance->Key_Down(DIK_4))
    {
        m_eChangeWeaponType = WEAPON_RIFLE;
        m_pASC->AbilityLocalInputPressed(ACTION_CHANGE_WEAPON);
    }
    else if (m_pGameInstance->Key_Down(DIK_5))
    {
        m_eChangeWeaponType = WEAPON_GRENADE;
        m_pASC->AbilityLocalInputPressed(ACTION_CHANGE_WEAPON);
    }
    else if (m_pGameInstance->Key_Down(DIK_6))
    {
        m_eChangeWeaponType = WEAPON_FLASHBANG;
        m_pASC->AbilityLocalInputPressed(ACTION_CHANGE_WEAPON);
    }
    else if (m_pGameInstance->Key_Down(DIK_7))
    {
        m_eChangeWeaponType = WEAPON_GRENADE;
        m_pASC->AbilityLocalInputPressed(ACTION_CHANGE_WEAPON);
    }
    else if (m_pGameInstance->Key_Down(DIK_8))
    {
        m_eChangeWeaponType = WEAPON_ROCKET_LAUNCHER;
        m_pASC->AbilityLocalInputPressed(ACTION_CHANGE_WEAPON);
    }

#endif // LOAD_UI

    if (m_pGameInstance->Key_Down(DIK_R))
    {
        m_pASC->AbilityLocalInputPressed(ACTION_RELOAD);
    }

    if (m_pGameInstance->Key_Down(DIK_E))
    {
        if (m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_QTE_TRIGGERABLE)))
            m_pASC->AbilityLocalInputPressed(ACTION_QTE);
        else
            m_pASC->AbilityLocalInputPressed(ACTION_CROUCHING);
    }

    if (m_eInputDIR != DIR_END)
        m_pASC->AbilityLocalInputPressed(ACTION_MOVE);
    else if (m_ePrevInputDIR != DIR_END && m_eInputDIR == DIR_END)
        m_pASC->AbilityLocalInputPressed(ACTION_IDLE);

    if (m_eInputDIR != DIR_END && m_pGameInstance->Key_Down(DIK_LSHIFT))
        m_pASC->AbilityLocalInputPressed(ACTION_DASH);

    if (m_pGameInstance->Mouse_Down(MOUSEKEYSTATE::DIM_RB))
    {
        m_pASC->AbilityLocalInputPressed(ACTION_AIM);
    }

    else if (m_pGameInstance->Mouse_Up(MOUSEKEYSTATE::DIM_RB))
    {
        m_pASC->AbilityLocalInputReleased(ACTION_AIM);
    }

    if (m_pGameInstance->Mouse_Down(MOUSEKEYSTATE::DIM_LB))
    {
        if (m_StealthKillTargetMonsters.size())
        {
            GameplayTag EventTag = GameplayTag(KEY_EVENT_JOINT_ATTACK_INPUT_LB);
            GameplayEventData EventData;
            EventData.EventTag = EventTag;
            m_pASC->TriggerAbility_FromGameplayEvent("JointAttack", nullptr, EventTag, &EventData);
        }
        else if (false == m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_AIMING)))
            m_pASC->AbilityLocalInputPressed(ACTION_KNIFE);
        else if (m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_SHOOTABLE)))
            m_pASC->AbilityLocalInputPressed(ACTION_SHOOT);
    }
    else if (m_pGameInstance->Mouse_Pressing(MOUSEKEYSTATE::DIM_LB))
    {
        if (WEAPON_SMG == m_eWeaponType)
            m_pASC->AbilityLocalInputPressed(ACTION_SHOOT);
    }
    else if (m_pGameInstance->Mouse_Up(MOUSEKEYSTATE::DIM_LB))
    {
        m_pASC->AbilityLocalInputReleased(ACTION_SHOOT);
    }


    if (m_pGameInstance->Key_Down(DIK_SPACE))
    {
        m_pASC->AbilityLocalInputPressed(ACTION_HOLDING_KNIFE);
    }
    else if (m_pGameInstance->Key_Up(DIK_SPACE))
    {
        m_pASC->AbilityLocalInputReleased(ACTION_HOLDING_KNIFE);
    }
}

void CReon::DebugKey_Input()
{
    if (m_pGameInstance->Key_Pressing(DIK_0) && m_pGameInstance->Key_Down(DIK_T))
    {
        m_pASC->SetNumericAttributeBase("CurrentHP", 100.f);
    }
    if (m_pGameInstance->Key_Pressing(DIK_0) && m_pGameInstance->Key_Down(DIK_Y))
    {
        m_pASC->SetNumericAttributeBase("CurrentHP", 2300.f);
    }

    if (m_pGameInstance->Key_Pressing(DIK_0) && m_pGameInstance->Key_Down(DIK_J))
    {
        m_pTarget = m_pGameInstance->Get_Object<CMonsterObject>(LEVEL_GAMEPLAY, L"Layer_Monster");
        if (m_pTarget)
        {
            m_pCompositeArmature->Select_Animation("", "chc0_jack_pl_0th_com_0070_hold_escape");
            m_pTarget->Get_Component<CCompositeArmature>()->Select_Animation("", "chc0_grapple_0th_com_0070_hold_escape");
        }
    }

    if (m_pGameInstance->Key_Pressing(DIK_0) && m_pGameInstance->Key_Down(DIK_L))
    {
        m_pTarget = m_pAshley;
        if (m_pTarget)
        {
            m_pCompositeArmature->Select_Animation("", "cha0_exception_0034_coop_over");
            m_pTarget->Get_Component<CCompositeArmature>()->Select_Animation("", "cha1_exception_0034_coop_over");
        }
    }

    if (m_pGameInstance->Key_Pressing(DIK_0) && m_pGameInstance->Key_Down(DIK_K))
    {
        m_pCompositeArmature->Select_AnimLayer_Animation("", "Add_Breath", "cha1_general2_1002_dazzling_loop");
    }

    if (m_pGameInstance->Key_Pressing(DIK_0) && m_pGameInstance->Key_Down(DIK_M))
    {
        m_pCompositeArmature->Select_AnimLayer_Animation("", "Add_Breath", "cha1_general2_1003_dazzling_loop_add");
    }
}

float CReon::Take_Damage(_float fDamageAmount, DamageEvent* pDamageEvent, CGameObject* pDamageInstigator, CGameObject* pDamageCauser)
{
    if (false == m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_HIT)))
    {
        m_pTarget = pDamageInstigator;

        GameplayEventData EventData;
        EventData.pInstigator = pDamageInstigator;
        EventData.pTarget = pDamageCauser;
        
        DamageEventRE DmgEventRE = *static_cast<DamageEventRE*>(pDamageEvent);

        // Ensure DamageAmount(수정이 필요할 수 있음)
        DmgEventRE.fDamage = fDamageAmount;
        DmgEventRE.pInstigator = pDamageInstigator;

        EventData.EventTag = GameplayTag(KEY_EVENT_ON_HIT);
        EventData.InstigatorTags.AddTag(GameplayTag(DmgEventRE.strInstigatorAnimKey));
        EventData.pContext = const_cast<COLLISION_INFO*>(&pDamageEvent->ColInfo);
        if (pDamageEvent->ColInfo.point.size())
            EventData.vTargetPoint = pDamageEvent->ColInfo.point[0].vContactPoint;
        EventData.pUserData = &DmgEventRE;

        m_pASC->TriggerAbility_FromGameplayEvent("Hit", nullptr, EventData.EventTag, &EventData);
    }

    if (m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_HIT_SHOULD_TRIGGER_CUTSCENE)))
    {
        m_pASC->RemoveLooseGameplayTag(GameplayTag(KEY_STATE_HIT_SHOULD_TRIGGER_CUTSCENE));

        // 컷씬 재생용 함수 호출
    }

    return 0.0f;
}

void CReon::Receive_Event(const GameplayEventData* pEventData)
{
    if (nullptr == pEventData)
        return;

    __super::Receive_Event(pEventData);

    if (pEventData->EventTag == GameplayTag(KEY_EVENT_ST1_RECEIVE_CALLING))
    {
        m_pASC->TryActivate_Ability("Calling");
    }
}

void CReon::OnASCTagAddReceived(const GameplayTagContainer& TagContainer)
{
    if (TagContainer.HasTag(GameplayTag(KEY_STATE_CROUCHING)))
    {
        m_bCrouching = true;
    }

    if (TagContainer.HasTag(GameplayTag(KEY_STATE_DASH)))
        m_bDashActivate = true;

    if (TagContainer.HasTag(GameplayTag(KEY_STATE_AIMING)))
    {
        m_bAiming = true;
        m_pAnimInstance->Re_Enter_CurrentState();

        if (m_pAshley)
            m_pAshley->Get_BehavioreTree()->Set_BlackboardKey(KEY_BB_AIMING, true);
    }

    if (TagContainer.HasTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_HOLDING)))
    {
        m_bHoldingKnife = true;
        m_pAnimInstance->Re_Enter_CurrentState();

        if (m_pAshley)
            m_pAshley->Get_BehavioreTree()->Set_BlackboardKey(KEY_BB_AIMING, true);
    }
}

void CReon::OnASCTagRemoveReceived(const GameplayTagContainer& TagContainer)
{
    if (TagContainer.HasTag(GameplayTag(KEY_STATE_CROUCHING)))
    {
        m_bCrouching = false;
    }

    if (TagContainer.HasTag(GameplayTag(KEY_STATE_DASH)))
        m_bDashActivate = false;

    if (TagContainer.HasTag(GameplayTag(KEY_STATE_AIMING)))
    {
        m_bAiming = false;
        m_pAnimInstance->Re_Enter_CurrentState();

        if (m_pAshley)
            m_pAshley->Get_BehavioreTree()->Set_BlackboardKey(KEY_BB_AIMING, false);
    }

    if (TagContainer.HasTag(GameplayTag(KEY_ABILITY_COMBAT_WEAPON_KNIFE_HOLDING)))
    {
        m_bHoldingKnife = false;
        m_pAnimInstance->Re_Enter_CurrentState();

        if (m_pAshley)
            m_pAshley->Get_BehavioreTree()->Set_BlackboardKey(KEY_BB_AIMING, false);
    }

    if (TagContainer.HasTag(GameplayTag(KEY_STATE_GUN)))
        Hold_Weapon(m_pASC, "", false);

    if (TagContainer.HasTag(GameplayTag(KEY_STATE_OCCUPIED)))
        m_pASC->TryActivate_Ability("Idle");
}

void CReon::Invoke_AttackBehavior(_bool bActive, _uint iAnimIndex, CAnimator* pAnimator, _bool bInstance)
{
    __super::Invoke_AttackBehavior(bActive, iAnimIndex, pAnimator, bInstance);
}

void CReon::OnLevelStarted(_uint iLevelIndex)
{
    CCamera_Manager::GetInstance()->Set_Target("ThirdPerson_Cam", this);
    CCamera_Manager::GetInstance()->Set_Target("FirstPerson_Cam", this);

    _bool bFound = false;
    
    m_pASC->SetNumericAttributeBase("CurrentHP", 5000);

    if (iLevelIndex == LEVEL_GAMEPLAY)
    {
        m_pTransformCom->Set_State_Notify(CTransform::STATE_POSITION, XMVectorSet(9.165f, -5.542f, 3.383f, 1.f));
        XMVECTOR quat = XMQuaternionRotationRollPitchYaw(
            XMConvertToRadians(0.f),     // pitch (X축 회전)
            XMConvertToRadians(-90.f),   // yaw (Y축 회전)
            XMConvertToRadians(0.f)      // roll (Z축 회전)
        );
        m_pTransformCom->Rotation(quat);

        m_pCCT->Renew_Controller();
    }

    if (iLevelIndex == LEVEL_HARBOR)
    {
        CGameManager::GetInstance()->Change_CurrentCharacter(this);

        m_pCompositeArmature->Select_Animation("", CGameManager::Parsing_Animation(m_iCharacterNumber, "general_0160_stand_loop"));

        XMVECTOR quat = XMQuaternionRotationRollPitchYaw(
            XMConvertToRadians(0.f),     // pitch (X축 회전)
            XMConvertToRadians(-82.f),   // yaw (Y축 회전)
            XMConvertToRadians(0.f)      // roll (Z축 회전)
        );

        m_pTransformCom->Rotation(quat);

        Get_TransformCom()->Set_State_Notify(CTransform::STATE_POSITION, XMVectorSet(29.140f, 5.731f, 12.880f, 1.f));
        Get_CharacterController()->Apply_GhostMode(false);
        Get_CharacterController()->Apply_Gravity(true);
        m_pCCT->Renew_Controller();

        m_pCompositeArmature->Set_UseHairPhysics(false);
        m_pGameInstance->Add_Timer("CReon::OnLevelStarted", 3.f, [=]() {
            m_pCompositeArmature->Set_UseHairPhysics(true);
            });

        m_eBaseFootStepType = FOOTSTEP_IRON;
        m_eFootStepType = FOOTSTEP_IRON;
    }

    else if (iLevelIndex == LEVEL_VILLAGE)
    {
        m_bCanShowScript = true;

        m_eBaseFootStepType = FOOTSTEP_STONE;

        CGameManager::GetInstance()->Change_CurrentCharacter(this);

        m_pCompositeArmature->Select_Animation("", CGameManager::Parsing_Animation(m_iCharacterNumber, "general_0160_stand_loop"));

        XMVECTOR quat = XMQuaternionRotationRollPitchYaw(
            XMConvertToRadians(0.f),     // pitch (X축 회전)
            XMConvertToRadians(81.f),   // yaw (Y축 회전)
            XMConvertToRadians(0.f)      // roll (Z축 회전)
        );

        m_pTransformCom->Rotation(quat);

        Get_TransformCom()->Set_State_Notify(CTransform::STATE_POSITION, XMVectorSet(-36.f, 0.434f, -10.07f, 1.f));
        Get_CharacterController()->Apply_GhostMode(false);
        Get_CharacterController()->Apply_Gravity(true);
        m_pCCT->Renew_Controller();

        m_pCompositeArmature->Set_UseHairPhysics(false);
        m_pGameInstance->Add_Timer("CReon::OnLevelStarted", 3.f, [=]() {
            m_pCompositeArmature->Set_UseHairPhysics(true);
            });

        m_eBaseFootStepType = FOOTSTEP_STONE;
        m_eFootStepType = FOOTSTEP_STONE;
    }
}

void CReon::OnLevelEnded(_uint iLevelIndex)
{
    if (iLevelIndex != LEVEL_LOADING)
    {
        m_pASC->Cancel_AllAbilities();
        Get_CharacterController()->Apply_GhostMode(true);
        Get_CharacterController()->Apply_Gravity(false);
        m_bInputEnabled = false;
    }
}

void CReon::Play_CutSceneAnimation(string strPartKey, string strAnimationKey, _fvector vPosition, _fvector vLook, _bool bLoop, string strAnimLayerKey)
{
    __super::Play_CutSceneAnimation(strPartKey, strAnimationKey, vPosition, vLook, bLoop);
}

CReon* CReon::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CReon* pInstance = new CReon(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CReon");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CReon::Clone(void* pArg)
{
    CReon* pInstance = new CReon(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Cloned : CReon");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CReon::Free()
{
    m_pCompositeArmature->Remove_HairSystem("ReonHair");

    __super::Free();

    Safe_Release(m_pModelShader);
    Safe_Release(m_pTriggerKnife);

    if (false == m_MovePointLights.empty())
    {
        for (int i = 0; i < POINTLIGHT_END; i++)
            Safe_Release(m_MovePointLights[i]);
    }
}

