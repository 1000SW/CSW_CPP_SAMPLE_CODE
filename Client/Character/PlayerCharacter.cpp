#include "PlayerCharacter.h"
#include "CompositeArmatrue.h"
#include "Camera_Follow.h"
#include "Camera_Manager.h"
#include "GameInstance.h"
#include "Navigation.h"
#include "AnimInstance.h"
#include "AbilitySystemComponent.h"
#include "Context.h"
#include "GameplayAbilitySystemHeader.h"
#include "MonsterObject.h"
#include "Character_AttributeSet.h"
#include "CharacterController.h"
#include "DistortEffect.h"

CMovePointLight* CPlayerCharacter::Get_PointLight(MovePointLightType eType)
{
    if (eType < POINTLIGHT_ATTACHED || eType >= POINTLIGHT_END)
        return nullptr;

    return m_MovePointLights[eType];
}

void CPlayerCharacter::Set_PickupItemInfo(_int iItemIndex, _int iItemCount)
{
    m_iItemIndex = iItemIndex;
    m_iItemCount = iItemCount;
}

CPlayerCharacter::CPlayerCharacter(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CCharacter{ pDevice, pContext }
{
    
}

CPlayerCharacter::CPlayerCharacter(const CPlayerCharacter& Prototype)
    : CCharacter{ Prototype }
{
}

HRESULT CPlayerCharacter::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CPlayerCharacter::Initialize(void* pArg)
{
    CGameObject::GAMEOBJECT_DESC* pDesc = reinterpret_cast<CGameObject::GAMEOBJECT_DESC*>(pArg);

    pDesc->BoundBox = BoundingBox({ 0.0f, 1.f, 0.f }, { 0.35f, 1.f, 0.35f });
    pDesc->boundType = BOUNDTYPE::AABB;

    if (m_bCreateController)
        pDesc->eRigidType = RIGIDSHAPE::NONE;
    else
        pDesc->eRigidType = RIGIDSHAPE::CAPSULE;

    pDesc->Physics_Info.bDynamic = false;
    pDesc->Physics_Info.bSimulate = true;
    pDesc->Physics_Info.bRotate = false;
    pDesc->Physics_Info.bTrigger = false;

    pDesc->fSpeedPerSec = 0.9f;

    pDesc->fRadius = 1.f;
    pDesc->vOffset = _float3(0.f, 1.f, 0.f);

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    if (m_pCCT)
    {
        m_pCCT->Apply_GhostMode(false);
        m_pCCT->Apply_Gravity(WORLD_GRAVITY);
    }

    m_pBound->Set_Mask(OT_PLAYER);

    m_pBound->Set_BoundType(RIGIDSHAPE::CAPSULE);

    m_pBound->Set_Active(false);
    m_pGameInstance->Add_Timer("CPlayerCharacter", 0.1f, [&]() {m_pBound->Set_Active(true); });

    return S_OK;
}

void CPlayerCharacter::Update(_float fTimeDelta)
{
    if (m_pGameInstance->Key_Down(DIK_F2))
        m_bInputEnabled ^= 1;

    if (m_bInputEnabled)
    {
        Key_Input_Common();
        Key_Input_Debug();
        Key_Input();
    }

    m_pASC->Update_AbilitySystem(fTimeDelta);
    m_pAnimInstance->Update_AnimInstance(fTimeDelta);
    if (m_pCCT)
        m_pCCT->Update_Controller(fTimeDelta);

    //m_pNavigationCom->Update(fTimeDelta);

    __super::Update(fTimeDelta);

    // 헤어에 대한 업데이트를 수행
    m_pCompositeArmature->Update_HairSystems(fTimeDelta);
}

void CPlayerCharacter::Late_Update(_float fTimeDelta)
{
    if (m_bVisible)
    {
        m_pGameInstance->Add_RenderObject(CRenderer::RG_NONBLEND, this);
        m_pGameInstance->Add_RenderObject(CRenderer::RG_SHADOW, this);
    }

    __super::Late_Update(fTimeDelta);
}

HRESULT CPlayerCharacter::Render()
{
    // 헤어를 적용한다
    m_pCompositeArmature->Apply_HairPhysics();

    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    //m_pCompositeArmature->Render(m_pShaderCom, 0);
    Render_CompositeArmatrue(m_pCompositeArmature, m_pShaderCom, 3);

    return S_OK;
}

HRESULT CPlayerCharacter::Render_Shadow()
{
    if (FAILED(m_pTransformCom->Bind_ShaderResource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_ShadowMatrix_ShaderResource(m_pShaderCom, "g_ViewMatrix", CShadow::D3DTS_VIEW)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_ShadowMatrix_ShaderResource(m_pShaderCom, "g_ProjMatrix", CShadow::D3DTS_PROJ)))
        return E_FAIL;

    m_pCompositeArmature->Render_Shadow(m_pShaderCom, 2);

    return S_OK;
}

float CPlayerCharacter::Take_Damage(_float fDamageAmount, DamageEvent* pDamageEvent, CGameObject* pDamageInstigator, CGameObject* pDamageCauser)
{
//#ifdef  LOAD_UI
//    GameplayAttribute* pAttribute = m_pAttributeSet->Get_Attribute("CurrentHP");
//    if (pAttribute)
//    {
//        _float fCurrentHP = pAttribute->GetBaseValue();
//        m_pASC->SetNumericAttributeBase("CurrentHP", fCurrentHP - fDamageAmount);
//    }
//#endif //  LOAD_UI

    return fDamageAmount;
}

void CPlayerCharacter::Change_Weapon(WeaponType eType, _bool bRevertHideWeapon)
{
    m_pCompositeArmature->Activate_SubPart("Back_" + Parsing_WeaponParts(m_eBackWeaponType), false);

    if (eType == WEAPON_KNIFE || eType == WEAPON_NONE)
    {
        m_pCompositeArmature->Deactivate_AnimLayer("", "BothArms");

        // 현재 들고있는 무기 Hide시킴, 무기형태 Knife였다면 무시
        if (m_eWeaponType != WEAPON_KNIFE)
        {
           m_eHideWeaponType = m_eWeaponType;

            if (m_eHideWeaponType != WEAPON_GRENADE && m_eHideWeaponType != WEAPON_FLASHBANG)
                m_pCompositeArmature->Activate_Part(Parsing_WeaponParts(m_eHideWeaponType), false);
            else
                m_pCompositeArmature->Activate_SubPart(Parsing_WeaponParts(m_eHideWeaponType), false);
        }

        m_eWeaponType = eType;
        if (m_eWeaponType == WEAPON_KNIFE)
            m_pCompositeArmature->Activate_SubPart("Knife", true);
        else
            m_pCompositeArmature->Activate_SubPart("Knife", false);
    }
    else
    {
        m_pCompositeArmature->Activate_SubPart("Knife", false);
        if (bRevertHideWeapon)
        {
            m_eWeaponType = m_eHideWeaponType;
            if (m_eWeaponType != WEAPON_GRENADE && m_eWeaponType != WEAPON_FLASHBANG)
                m_pCompositeArmature->Activate_Part(Parsing_WeaponParts(m_eWeaponType), true);
            else
                m_pCompositeArmature->Activate_SubPart(Parsing_WeaponParts(m_eWeaponType), true);
        }
        else
        {
            // 현재 무기를 등에 붙임
            m_eBackWeaponType = m_eWeaponType;
            if (m_eBackWeaponType != WEAPON_GRENADE && m_eBackWeaponType != WEAPON_FLASHBANG)
                m_pCompositeArmature->Activate_Part(Parsing_WeaponParts(m_eBackWeaponType), false);
            else
                m_pCompositeArmature->Activate_SubPart(Parsing_WeaponParts(m_eBackWeaponType), false);

            // 새로 들어올 무기를 활성화시킨다
            m_eWeaponType = eType;
            if (m_eWeaponType != WEAPON_GRENADE && m_eWeaponType != WEAPON_FLASHBANG)
                m_pCompositeArmature->Activate_Part(Parsing_WeaponParts(m_eWeaponType), true);
            else
                m_pCompositeArmature->Activate_SubPart(Parsing_WeaponParts(m_eWeaponType), true);
        }
    }

    if (m_eWeaponType != m_eBackWeaponType)
        m_pCompositeArmature->Activate_SubPart("Back_" + Parsing_WeaponParts(m_eBackWeaponType));
}

void CPlayerCharacter::Add_TargetMonster(CMonsterObject* pMonster)
{
    m_TargetMonsters.insert(pMonster);
}

void CPlayerCharacter::Add_StealthKillTargetMonster(CMonsterObject* pMonster)
{
    m_StealthKillTargetMonsters.insert(pMonster);
}

void CPlayerCharacter::Remove_TargetMonster(CMonsterObject* pMonster)
{
    auto iter = m_TargetMonsters.find(pMonster);

    if (iter != m_TargetMonsters.end())
    {
        m_TargetMonsters.erase(pMonster);
    }
}

void CPlayerCharacter::Remove_StealthKillTargetMonster(CMonsterObject* pMonster)
{
    auto iter = m_StealthKillTargetMonsters.find(pMonster);

    if (iter != m_StealthKillTargetMonsters.end())
    {
        m_StealthKillTargetMonsters.erase(pMonster);
    }
}

_bool CPlayerCharacter::Is_HidingWeapon() const
{
    if (Get_WeaponType() != WEAPON_KNIFE && Get_WeaponType() != WEAPON_NONE)
        return false;

    _bool bHideWeaponFlag;

    WeaponType eType = Get_HideWeaponType();
    switch (eType)
    {
    case Client::WEAPON_NONE:
    case Client::WEAPON_KNIFE:
    case Client::WEAPON_END:
    default:
        bHideWeaponFlag = false;
    case Client::WEAPON_PISTOL:
    case Client::WEAPON_SHOTGUN:
    case Client::WEAPON_RIFLE:
    case Client::WEAPON_GRENADE:
    case Client::WEAPON_FLASHBANG:
    case Client::WEAPON_MAGNUM:
    case Client::WEAPON_ROCKET_LAUNCHER:
    case Client::WEAPON_SMG:
        bHideWeaponFlag = true;
    }

    return bHideWeaponFlag;
}

void CPlayerCharacter::Receive_Event(const GameplayEventData* pEventData)
{
    if (nullptr == pEventData)
        return;

    if (pEventData->EventTag == GameplayTag("ConSume_Item"))
    {
        _float fMaxHpValue = max(0, pEventData->iUserData);    // 실 수치
        _float fCurHpValue = max(0, pEventData->iUserData1);   // 실 수치

        _bool bMaxFound, bCurFound, bLimitFound;

        _float fMaxValue = m_pASC->GetNumericAttributeBase("MaxHP", bMaxFound);
        _float fCurValue = m_pASC->GetNumericAttributeBase("CurrentHP", bCurFound);
        _float fLimitHP = m_pASC->GetNumericAttributeBase("fLimitHP", bLimitFound);

        if (!bMaxFound || !bCurFound || !bLimitFound)
            return;

        _float fNewMaxValue = fMaxValue + fMaxHpValue;
        fNewMaxValue = min(fNewMaxValue, fLimitHP);

        _float fNewCurValue = fCurValue + fCurHpValue;
        fNewCurValue = min(fNewCurValue, fNewMaxValue); // 현재 체력은 최대 체력 넘지 않도록

        m_pASC->SetNumericAttributeBase("MaxHP", fNewMaxValue);
        m_pASC->SetNumericAttributeBase("CurrentHP", fNewCurValue);
    }
    else if (pEventData->EventTag == GameplayTag(KEY_EVENT_CHANGE_LEVEL))
    {
        const CTrigger::TRIGGER_DATA& TriggerData = m_pTrigger->Get_TriggerData();
        if (TriggerData.vecIntValues.empty())
            return;

        CGameManager::GetInstance()->Change_Level(static_cast<LEVEL>(TriggerData.vecIntValues[0]));
    }
    else if (pEventData->EventTag == GameplayTag(KEY_EVENT_CHANGE_FOOTSTEP_TYPE))
    {
        if (EVENT_START == pEventData->eEventType)
        {
            if (nullptr == m_pTrigger)
                return;
            const CTrigger::TRIGGER_DATA& TriggerData = m_pTrigger->Get_TriggerData();
            if (TriggerData.vecIntValues.empty())
                return;

            m_eFootStepType = static_cast<FootStepType>(TriggerData.vecIntValues[0]);
        }
        else if (EVENT_FINISH == pEventData->eEventType)
        {
            m_eFootStepType = m_eBaseFootStepType;
        }
    }

}

void CPlayerCharacter::Invoke_AttackBehavior(_bool bActive, _uint iAnimIndex, CAnimator* pAnimator, _bool bInstance)
{
    __super::Invoke_AttackBehavior(bActive, iAnimIndex, pAnimator, bInstance);

    if (m_KickAmimSet.contains(iAnimIndex))
    {
        if (false == bActive)
            m_bKickable = false;
        else
        {
            m_bKickable = true;

            m_DamegeInfo.eReactionType = REACTION_KICK;
            m_DamegeInfo.fDamage = 1000.f;
        }
    }
    else if (m_KnifeAnimSet.contains(iAnimIndex))
    {
        if (false == bActive)
            m_bKnifeOn = false;
        else
        {
            m_bKnifeOn = true;

            m_DamegeInfo.eReactionType = REACTION_SMALL;
            m_DamegeInfo.fDamage = 200.f;
        }
    }
}

void CPlayerCharacter::Turn_ON_Muzzle()
{
   if (nullptr == m_pDistortMuzzle)
        return;

   m_pDistortMuzzle->Turn_On(true); 
}

_bool CPlayerCharacter::Can_Hit()
{
    _bool IsOccupied = m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_OCCUPIED));

    _bool IsUnoccupied = m_pASC->Has_MatchingGameplayTag(GameplayTag(KEY_STATE_UNOCCUPIED));

    return (false == IsOccupied && true == IsUnoccupied);
}

HRESULT CPlayerCharacter::Ready_Components()
{
    /* Com_Shader */
    if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxAnimMesh"),
        reinterpret_cast<CComponent**>(&m_pShaderCom), TEXT("Com_Shader"))))
        return E_FAIL;

    /* Navigation */
    if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_Navigation"),
        reinterpret_cast<CComponent**>(&m_pNavigationCom), TEXT("Com_Navigation"))))
        return E_FAIL;
    m_pNavigationCom->Ready_PathFinder();

    /* composite armature setting */
    if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_CompositeArmature"),
        reinterpret_cast<CComponent**>(&m_pCompositeArmature), TEXT("Com_CompositeArmature"))))
        return E_FAIL;

    /* VariableContext */
    if (FAILED(__super::Add_Component(LEVEL_STATIC, KEY_PROTO_COM_CONTEXT,
        reinterpret_cast<CComponent**>(&m_pVariableContext), TEXT("Com_VariableContext"))))
        return E_FAIL;

    /* Anim Instance */
    if (FAILED(__super::Add_Component(LEVEL_STATIC, KEY_PROTO_COM_ANIM_INSTANCE,
        reinterpret_cast<CComponent**>(&m_pAnimInstance), TEXT("Com_AnimInstance"))))
        return E_FAIL;
    m_pAnimInstance->Add_CompositeArmature(m_pCompositeArmature);
    m_pAnimInstance->Add_Context(m_pVariableContext);

    /* CharacterController */
    if (m_bCreateController)
    {
        CCharacterController::CharacterController_DESC Desc;
        Desc.pOwnerTransform = m_pTransformCom;
        Desc.pBound = m_pBound;
        Desc.eRigidType = RIGIDSHAPE::CAPSULE;
        PHYSICS_INFO Info;
        Info.bDynamic = true;
        Info.bGhost = false;
        Info.bGravity = false;
        Info.iObjectType = OT_PLAYER;
        Info.iGroupMask = OT_MONSTER | OT_TRIGGER;
        Desc.Physics_Info = Info;
        Desc.fRadiusScale = 0.5f;
        Desc.fHeightScale = 0.7f;

        if (FAILED(__super::Add_Component(LEVEL_STATIC, KEY_PROTO_COM_CHARACTER_CONTROLLER,
            reinterpret_cast<CComponent**>(&m_pCCT), TEXT("Com_CharacterController"), &Desc)))
            return E_FAIL;
    }

    /* GameplayAbilitySystem */
    if (FAILED(__super::Add_Component(LEVEL_STATIC, KEY_PROTO_COM_ABILITY_SYSTEM,
        reinterpret_cast<CComponent**>(&m_pASC), TEXT("Com_GAS"))))
        return E_FAIL;
    m_pAttributeSet = CCharacter_AttributeSet::Create();
    m_pASC->Init_Ability_ActorInfo(this, this);

    m_pASC->Add_AttributeSet(m_pAttributeSet);

    return S_OK;
}

HRESULT CPlayerCharacter::Ready_AnimSet()
{
    m_KnifeAnimSet = {
    m_pCompositeArmature->Get_AnimationIndexByName("", CGameManager::Parsing_Animation(m_iCharacterNumber, "wp5000H_0660_attack_jog")),
    m_pCompositeArmature->Get_AnimationIndexByName("", CGameManager::Parsing_Animation(m_iCharacterNumber, "wp5000H_0630_attack_1st")),
    m_pCompositeArmature->Get_AnimationIndexByName("", CGameManager::Parsing_Animation(m_iCharacterNumber, "wp5000H_0640_attack_2nd")),
    m_pCompositeArmature->Get_AnimationIndexByName("", CGameManager::Parsing_Animation(m_iCharacterNumber, "wp5000H_0650_attack_3rd")),
     m_pCompositeArmature->Get_AnimationIndexByName("", CGameManager::Parsing_Animation(m_iCharacterNumber, "wp5000H_0600_attack")),
    m_pCompositeArmature->Get_AnimationIndexByName("", CGameManager::Parsing_Animation(m_iCharacterNumber, "wp5000H_0610_attack")),
    m_pCompositeArmature->Get_AnimationIndexByName("", CGameManager::Parsing_Animation(m_iCharacterNumber, "wp5000H_0146_parry")),
    };

    m_KickAmimSet = {
       m_pCompositeArmature->Get_AnimationIndexByName("", CGameManager::Parsing_Animation(m_iCharacterNumber, "general_0900_fatal_VerA")),
       m_pCompositeArmature->Get_AnimationIndexByName("", CGameManager::Parsing_Animation(m_iCharacterNumber, "general_0902_fatal_VerB")),
       m_pCompositeArmature->Get_AnimationIndexByName("", CGameManager::Parsing_Animation(m_iCharacterNumber, "general_0908_fatal_VerE")),
       m_pCompositeArmature->Get_AnimationIndexByName("", CGameManager::Parsing_Animation(m_iCharacterNumber, "Hookshot_0905_shoot_jump_fatal_VerA")),
    };

    return S_OK;
}

HRESULT CPlayerCharacter::Bind_ShaderResources()
{
    _uint frameCount = m_pGameInstance->Get_FrameCount();
    if(FAILED(m_pShaderCom->Bind_RawValue("g_FrameCount", &frameCount, sizeof(_uint))))
        return E_FAIL;

    // NEW DATA
    if (FAILED(m_pTransformCom->Bind_ShaderResource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_Matrix_ShaderResource(m_pShaderCom, "g_ViewMatrix", CPipeLine::D3DTS_VIEW)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_Matrix_ShaderResource(m_pShaderCom, "g_ProjMatrix", CPipeLine::D3DTS_PROJ)))
        return E_FAIL;

    // OLD DATA
    if (FAILED(m_pTransformCom->Bind_OldShaderResource(m_pShaderCom, "g_PrevWorldMatrix")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_Matrix_OldShaderResource(m_pShaderCom, "g_PrevViewMatrix", CPipeLine::D3DTS_VIEW)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_Matrix_OldShaderResource(m_pShaderCom, "g_PrevProjMatrix", CPipeLine::D3DTS_PROJ)))
        return E_FAIL;

    return S_OK;
}

void CPlayerCharacter::Key_Input()
{
}

void CPlayerCharacter::Change_Weapon_InteractWithUI(_ubyte KeyState)
{
    CItem* pItem = CGameManager::GetInstance()->Swap_Equip_Item(KeyState);
    CItem_Weapon* pWeaponItem = dynamic_cast<CItem_Weapon*>(pItem);
    if (pWeaponItem)
    {
        m_pHoldWeapon = pWeaponItem;
        m_eChangeWeaponType = static_cast<WeaponType>(pWeaponItem->Get_WeaponType());
    }
}

void CPlayerCharacter::Activate_Part(WeaponType eType, _bool bActive)
{
    m_eBackWeaponType = m_eWeaponType;
    if (m_eBackWeaponType != WEAPON_GRENADE && m_eBackWeaponType != WEAPON_FLASHBANG)
        m_pCompositeArmature->Activate_Part(Parsing_WeaponParts(m_eBackWeaponType), false);
    else
        m_pCompositeArmature->Activate_SubPart(Parsing_WeaponParts(m_eBackWeaponType), false);
}

void CPlayerCharacter::OnLevelStarted(_uint iLevelIndex)
{
}

void CPlayerCharacter::OnLevelEnded(_uint iLevelIndex)
{

}

void CPlayerCharacter::Key_Input_Common()
{

}

void CPlayerCharacter::Key_Input_Debug()
{
    if (m_pGameInstance->Key_Pressing(DIK_LALT) && m_pGameInstance->Key_Down(DIK_P))
    {
        if (m_pASC)
            m_pASC->Cancel_AllAbilities();
    }

    if (/*m_pGameInstance->Key_Pressing(DIK_LCONTROL) && */m_pGameInstance->Key_Down(DIK_G))
    {
        m_bApplyGhost ^= 1;

        if (m_pCCT)
            m_pCCT->Apply_GhostMode(m_bApplyGhost);
    }

    if (/*m_pGameInstance->Key_Pressing(DIK_LCONTROL) && */m_pGameInstance->Key_Pressing(DIK_Q))
    {
        m_pTransformCom->Go_Up(m_pGameInstance->Get_TimeDelta() * 5.f);
    }

    if (/*m_pGameInstance->Key_Pressing(DIK_LCONTROL) && */m_pGameInstance->Key_Pressing(DIK_E))
    {
        m_pTransformCom->Go_Down(m_pGameInstance->Get_TimeDelta() * 5.f);
    }

    if (/*m_pGameInstance->Key_Pressing(DIK_LCONTROL) && */m_pGameInstance->Key_Down(DIK_B))
    {
        if (m_pRigid)
        {
            m_pRigid->Set_Gravity(WORLD_GRAVITY);
            if (false == WORLD_GRAVITY)
                m_pRigid->Set_Velocity(XMVectorSet(0, 0, 0, 0));
        }
        else
        {
            m_pCCT->Apply_Gravity(WORLD_GRAVITY);
        }
    }

#ifdef _DEBUG
   

#endif
}

CPlayerCharacter* CPlayerCharacter::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CPlayerCharacter* pInstance = new CPlayerCharacter(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CPlayerCharacter");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CPlayerCharacter::Clone(void* pArg)
{
    CPlayerCharacter* pInstance = new CPlayerCharacter(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Cloned : CPlayerCharacter");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CPlayerCharacter::Free()
{
    // Safe_Release(m_pDistortMuzzle);
    // Safe_Release(m_pBullet);
    // Safe_Release(m_pLaser);

    __super::Free();
}
