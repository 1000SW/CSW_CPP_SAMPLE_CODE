#include "WorldItem.h"
#include "GameInstance.h"
#include "Trigger.h"
#include "EventFunc.h"
#include "CompositeArmatrue.h"
#include "ParticleEmitter.h"
#include "GameplayAbilitySystemHeader.h"
#include "Ashley.h"
#include "Trigger_Socket.h"

CWorldItem::CWorldItem(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CBRDFObject{ pDevice, pContext }
{
}

CWorldItem::CWorldItem(const CWorldItem& Prototype)
    : CBRDFObject(Prototype)
{
}

HRESULT CWorldItem::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CWorldItem::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (nullptr != pArg)
    {
        WorldItemDesc* pDesc = reinterpret_cast<WorldItemDesc*>(pArg);

        m_pInstigator = pDesc->pInstigator;
        m_pTransformCom->Set_State_Notify(CTransform::STATE_LOOK, XMLoadFloat3(&pDesc->vLook));
    }

    if (FAILED(Ready_Components()))
        return E_FAIL;

    return S_OK;
}

HRESULT CWorldItem::Initialize_PoolingObject(void* pArg)
{
    if (nullptr != pArg)
    {
        WorldItemDesc* pDesc = reinterpret_cast<WorldItemDesc*>(pArg);

        m_pInstigator = pDesc->pInstigator;
        m_pTransformCom->Set_State_Notify(CTransform::STATE_POSITION, XMVectorSetW(XMLoadFloat3(&pDesc->vPosition), 1.f));
        //m_pTransformCom->Set_State_Notify(CTransform::STATE_LOOK, XMLoadFloat3(&pDesc->vLook));
        m_pTransformCom->LookAt(XMLoadFloat3(&pDesc->vLook));

        m_pRigid->Update_AllTransform(m_pTransformCom->Get_WorldMatrix());
    }

    return S_OK;
}

void CWorldItem::Update(_float fTimeDelta)
{
    __super::Update(fTimeDelta);
}

void CWorldItem::Late_Update(_float fTimeDelta)
{
    m_pGameInstance->Add_RenderObject(CRenderer::RG_NONBLEND, this);
    m_pGameInstance->Add_RenderObject(CRenderer::RG_SHADOW, this);

    __super::Late_Update(fTimeDelta);
}

HRESULT CWorldItem::Render()
{
    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    if (m_pModelCom)
    {
        if (FAILED(CBRDFObject::Render(m_pModelCom, m_pModelShaderCom, 3)))
            return E_FAIL;
    }
    else if (m_pCompositeArmature)
    {
        Render_CompositeArmatrue(m_pCompositeArmature, m_pArmatureShaderCom, 3);
    }

    return S_OK;
}

HRESULT CWorldItem::Render_Shadow()
{
    if (m_pModelCom)
    {
        if (FAILED(m_pTransformCom->Bind_ShaderResource(m_pModelShaderCom, "g_WorldMatrix")))
            return E_FAIL;

        if (FAILED(m_pGameInstance->Bind_ShadowMatrix_ShaderResource(m_pModelShaderCom, "g_ViewMatrix", CShadow::D3DTS_VIEW)))
            return E_FAIL;

        if (FAILED(m_pGameInstance->Bind_ShadowMatrix_ShaderResource(m_pModelShaderCom, "g_ProjMatrix", CShadow::D3DTS_PROJ)))
            return E_FAIL;
    }

    else if (m_pCompositeArmature)
    {
        if (FAILED(m_pTransformCom->Bind_ShaderResource(m_pArmatureShaderCom, "g_WorldMatrix")))
            return E_FAIL;

        if (FAILED(m_pGameInstance->Bind_ShadowMatrix_ShaderResource(m_pArmatureShaderCom, "g_ViewMatrix", CShadow::D3DTS_VIEW)))
            return E_FAIL;

        if (FAILED(m_pGameInstance->Bind_ShadowMatrix_ShaderResource(m_pArmatureShaderCom, "g_ProjMatrix", CShadow::D3DTS_PROJ)))
            return E_FAIL;

        return S_OK;
    }

    return S_OK;
}

HRESULT CWorldItem::Bind_ShaderResources()
{
    if (m_pModelCom)
    {
        if (FAILED(m_pTransformCom->Bind_ShaderResource(m_pModelShaderCom, "g_WorldMatrix")))
            return E_FAIL;
        if (FAILED(m_pGameInstance->Bind_Matrix_ShaderResource(m_pModelShaderCom, "g_ViewMatrix", CPipeLine::D3DTS_VIEW)))
            return E_FAIL;
        if (FAILED(m_pGameInstance->Bind_Matrix_ShaderResource(m_pModelShaderCom, "g_ProjMatrix", CPipeLine::D3DTS_PROJ)))
            return E_FAIL;
    }
    else if (m_pCompositeArmature)
    {
        _uint frameCount = m_pGameInstance->Get_FrameCount();
        if (FAILED(m_pArmatureShaderCom->Bind_RawValue("g_FrameCount", &frameCount, sizeof(_uint))))
            return E_FAIL;

        if (FAILED(m_pTransformCom->Bind_ShaderResource(m_pArmatureShaderCom, "g_WorldMatrix")))
            return E_FAIL;

        if (FAILED(m_pGameInstance->Bind_Matrix_ShaderResource(m_pArmatureShaderCom, "g_ViewMatrix", CPipeLine::D3DTS_VIEW)))
            return E_FAIL;

        if (FAILED(m_pGameInstance->Bind_Matrix_ShaderResource(m_pArmatureShaderCom, "g_ProjMatrix", CPipeLine::D3DTS_PROJ)))
            return E_FAIL;

        if (FAILED(m_pTransformCom->Bind_OldShaderResource(m_pArmatureShaderCom, "g_PrevWorldMatrix")))
            return E_FAIL;

        if (FAILED(m_pGameInstance->Bind_Matrix_OldShaderResource(m_pArmatureShaderCom, "g_PrevViewMatrix", CPipeLine::D3DTS_VIEW)))
            return E_FAIL;

        if (FAILED(m_pGameInstance->Bind_Matrix_OldShaderResource(m_pArmatureShaderCom, "g_PrevProjMatrix", CPipeLine::D3DTS_PROJ)))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CWorldItem::Ready_Components()
{
    /* Com_Shader For SubModel */
    if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_Component_Common_VtxMesh"),
        reinterpret_cast<CComponent**>(&m_pModelShaderCom), TEXT("Com_ModelShader"))))
        return E_FAIL;

    /* Com_Shader For CompositeArmature */
    if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxAnimMesh"),
        reinterpret_cast<CComponent**>(&m_pArmatureShaderCom), TEXT("Com_ArmatureShader"))))
        return E_FAIL;

    return S_OK;
}

HRESULT CWorldItem::Ready_Materials()
{
    // 메터리얼을 추가시킨다.
    if (m_pModelCom)
    {
        if (FAILED(Check_Materials(m_pModelCom)))
            return E_FAIL;
    }
    else if (m_pCompositeArmature)
    {
        queue<pair<wstring, wstring>> detailMapQueue;
        if (FAILED(Check_Materials(m_pCompositeArmature, &detailMapQueue)))
            return E_FAIL;
    }
    else
        return E_FAIL;

    return S_OK;
}

void CWorldItem::Free()
{
    __super::Free();
}

CGrenade::CGrenade(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CWorldItem{ pDevice, pContext }
{
}

CGrenade::CGrenade(const CGrenade& Prototype)
    : CWorldItem(Prototype)
{
}

HRESULT CGrenade::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CGrenade::Initialize(void* pArg)
{
    GRENADE_DESC GrenadeDesc;

    // 외부에서 세팅한 값으로 세팅하기 위한 캐스팅
    if (nullptr != pArg)
    {
        GrenadeDesc = *reinterpret_cast<CGrenade::GRENADE_DESC*>(pArg);
        m_eGrandeType = GrenadeDesc.eGrandeType;
    }

    GrenadeDesc.BoundBox = BoundingBox({ 0.0f, 0.05f, 0.f }, { 0.05f, 0.05f, 0.05f });
    GrenadeDesc.boundType = BOUNDTYPE::AABB;

    GrenadeDesc.eRigidType = RIGIDSHAPE::SPHERE;
    GrenadeDesc.Physics_Info.bRotate = true;

    GrenadeDesc.fSpeedPerSec = 10.f;
    GrenadeDesc.fRotationPerSec = XM_PI * 0.4f;

    lstrcpy(GrenadeDesc.szGameObjectTag, L"Projectile_Grenade");

    GrenadeDesc.Physics_Info.bGravity = true;
    GrenadeDesc.Physics_Info.mass = 1000000000.f;

    if (FAILED(__super::Initialize(&GrenadeDesc)))
        return E_FAIL;

    if (E_FAIL == Ready_Components())
        return E_FAIL;

    if (E_FAIL == Ready_Materials())
        return E_FAIL;

    if (E_FAIL == Ready_Collision())
        return E_FAIL;

    m_pBound->Set_BoundType(RIGIDSHAPE::SPHERE);

    m_pBound->Set_Mask(OT_PROJECTILE_ALL);

    m_pRigid->Set_Gravity(true);

    Ready_GrenadeAttribute(GrenadeDesc);

    return S_OK;
}

HRESULT CGrenade::Initialize_PoolingObject(void* pArg)
{
    GRENADE_DESC GrenadeDesc;

    // 외부에서 세팅한 값으로 세팅하기 위한 캐스팅
    if (nullptr != pArg)
    {
        GrenadeDesc = *reinterpret_cast<CGrenade::GRENADE_DESC*>(pArg);
        m_eGrandeType = GrenadeDesc.eGrandeType;
    }
    GrenadeDesc.vLook = GrenadeDesc.vLook;
    __super::Initialize_PoolingObject(pArg);

    Ready_GrenadeAttribute(GrenadeDesc);

    return S_OK;
}

void CGrenade::Priority_Update(_float fTimeDelta)
{
    if (m_pBombTrigger)
        m_pBombTrigger->Priority_Update(fTimeDelta);

    __super::Priority_Update(fTimeDelta);
}

void CGrenade::Update(_float fTimeDelta)
{
    __super::Update(fTimeDelta);

    m_fAccTime += fTimeDelta;

    m_pExplosionPoint->Set_Position(m_pTransformCom->Get_Position());

    if (m_fAccTime >= m_fExploisionTime && false == m_bExplosionOn)
    {
        m_bExplosionOn = true;
        
        // 폭발하고 한 틱 동안은 업데이트 흐름을 돌게 함 => 충돌검사에 대한 한 틱을 보장하기 위함 
        m_pGameInstance->Add_Timer("CGrenade", 0.f, [&]() {
            m_bDone = true; 
            m_pGameInstance->Return_PoolingObject(this);
            });


        /* Spawn Particle And Sound */
        if (GRENADE_FLASHBANG == m_eGrandeType)
        {
#ifdef LOAD_EFFECT
            Spawn_FlashBangParticle();
#endif

            // 2차원 사운드 재생
            PlaySoundEvent("event:/Weapon/FlashBang/Tinnitus", true, false, false, _float4x4{});

            m_pExplosionPoint->PlaySoundEvent("event:/Weapon/FlashBang/Explosion", false, false, false, _float4x4{});

            m_pExplosionPoint->PlaySoundEvent("event:/Weapon/FlashBang/FlsahbangExplosion", false, false, false, _float4x4{});

           /* _float4x4 matTransform;
            XMStoreFloat4x4(&matTransform, m_pTransformCom->Get_WorldMatrix());
            PlaySoundEvent("event:/Weapon/FlashBang/Explosion", false, false, false, _float4x4{});
            PlaySoundEvent("event:/Weapon/FlashBang/FlsahbangExplosion", false, false, true, matTransform);*/
        }
        else if (GRENADE == m_eGrandeType)
        {
#ifdef LOAD_EFFECT
            Spawn_GranadeParticle();
#endif
            m_pExplosionPoint->PlaySoundEvent("event:/Weapon/Granade/Boom", false, false, true, _float4x4{});

            m_pGameInstance->Add_Timer("CGrenade", 1.3f, [&]() 
                {
                    m_pExplosionPoint->PlaySoundEvent("event:/Weapon/Granade/SideEffect", false, false, false, _float4x4{});
                   
                });
        }
    }
}

HRESULT CGrenade::Ready_Components()
{
    /* Com_Model */
    if (GRENADE == m_eGrandeType)
    {
        if (FAILED(__super::Add_Component(LEVEL_STATIC, KEY_PROTO_MODEL_GRENADE,
            reinterpret_cast<CComponent**>(&m_pModelCom), TEXT("Com_Model"))))
            return E_FAIL;
    }
    else if (GRENADE_FLASHBANG == m_eGrandeType)
    {
        if (FAILED(__super::Add_Component(LEVEL_STATIC, KEY_PROTO_MODEL_FLASHBANG,
            reinterpret_cast<CComponent**>(&m_pModelCom), TEXT("Com_Model"))))
            return E_FAIL;
    }

    GAMEOBJECT_DESC ObjectDesc{};
    ObjectDesc.vPosition = { 0, 0, 0 };
    ObjectDesc.vOffset = { 0, 0, 0 };
    ObjectDesc.fSpeedPerSec = 0.9f;
    ObjectDesc.fRotationPerSec = XM_PI * 0.4f;
    ObjectDesc.fRadius = 0.15f;
    ObjectDesc.iMask = OT_TRIGGER;
    ObjectDesc.boundType = BOUNDTYPE::AABB;
    ObjectDesc.BoundBox = BoundingBox({ 0.0f, 0.f, 0.f }, { m_fExplosionRadius, m_fExplosionRadius, m_fExplosionRadius });

    // 리지드 박스의 형태는 동일
    ObjectDesc.eRigidType = RIGIDSHAPE::SPHERE;
    ObjectDesc.Physics_Info.bRotate = false;
    ObjectDesc.Physics_Info.bDynamic = false;
    ObjectDesc.Physics_Info.bSimulate = false;
    ObjectDesc.Physics_Info.bUseRigid = false;
    ObjectDesc.Physics_Info.bTrigger = true;

    lstrcpy(ObjectDesc.szGameObjectTag, L"Grenade_ExplosionPoint");
    m_pExplosionPoint = dynamic_cast<CTargetPoint*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::TYPE_GAMEOBJECT, LEVEL_STATIC, TEXT("Prototype_GameObject_TargetPoint"), &ObjectDesc));
    if (nullptr == m_pExplosionPoint)
        return E_FAIL;
    m_pExplosionPoint->Get_Bound()->Set_Mask(OT_PROJECTILE_ALL);
    m_pExplosionPoint->Get_Bound()->Set_BoundType(RIGIDSHAPE::SPHERE);

    m_pGameInstance->Add_GameObject(m_pExplosionPoint, LEVEL_STATIC, L"Layer_TargetPoint");

    return S_OK;
}

HRESULT CGrenade::Ready_Collision()
{
    function<void(CBound*, COLLISION_INFO*)> fnLambda =
        [&](CBound* pBound, COLLISION_INFO* pColInfo)
        {
            if (m_bExplosionOn && false == m_bDone)
            {
                _float fDamage = 0.f;

                DamageEventRE DamageEvent;
                DamageEvent.eType = DAMAGE_RADIAL;
                if (GRENADE == m_eGrandeType)
                {
                    _float fDist = XMVectorGetX(XMVector3Length(m_pTransformCom->Get_Position() - pBound->Get_Owner()->Get_TransformCom()->Get_Position()));

                    // 게산된 데미지
                    fDamage = Calculate_GrenadeDamage(fDist, 1000.f, m_fExplosionRadius);
                    // 원래 데미지
                    DamageEvent.fDamage = 1000.f;
                    DamageEvent.eReactionType = REACTION_EXPLOSION;
                }
                else if (GRENADE_FLASHBANG == m_eGrandeType)
                {
                    fDamage = 1.f;
                    DamageEvent.fDamage = 1.f;
                    DamageEvent.eReactionType = REACTION_FLASHBANG;
                }

                DamageEvent.vRadialCenter = m_pTransformCom->Get_Position();
                //DamageEvent.pOverlappedComp = m_pBombTrigger->Get_Bound();
                DamageEvent.pOverlappedComp = pBound;
                DamageEvent.ColInfo = *pColInfo;

                if (false == pBound->Is_Subscribe())
                    pBound->Get_Owner()->Take_Damage(fDamage, &DamageEvent, m_pOwner, this);
            }
        };

    m_pExplosionPoint->Get_Bound()->Add_CollisionCallBack(fnLambda);

    /*CTriggerSocket::TriggerSoket_DESC TriggerDesc{};
    TriggerDesc.pOwner = this;
    TriggerDesc.vPosition = { 0, 0, 0 };
    TriggerDesc.vScale = { m_fExplosionRadius, m_fExplosionRadius, m_fExplosionRadius };
    TriggerDesc.OnCallback = fnLambda;
    TriggerDesc.iMask = OT_PROJECTILE_ALL;
    TriggerDesc.eRigidType = RIGIDSHAPE::SPHERE;
    TriggerDesc.pOwner = this;
    TriggerDesc.pRealOwner = this;
    TriggerDesc.pObject = this;

    XMStoreFloat4x4(&m_IdentityMatrix, XMMatrixIdentity());
    TriggerDesc.pSocketMatrix = &m_IdentityMatrix;

    lstrcpy(TriggerDesc.szGameObjectTag, L"Grenade_Trigger");
    CTriggerSocket* pSocket = dynamic_cast<CTriggerSocket*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::TYPE_GAMEOBJECT, LEVEL_STATIC, TEXT("Prototype_GameObject_TriggerSocket"), &TriggerDesc));
    if (pSocket)
        m_pBombTrigger = pSocket;

    if (nullptr == m_pBombTrigger)
        return E_FAIL;*/

    //m_pBombTrigger->Get_Bound()->Set_Mask(OT_PROJECTILE_ALL);
    //m_pBombTrigger->Get_Bound()->Set_BoundType(RIGIDSHAPE::SPHERE);

    return S_OK;
}

HRESULT CGrenade::Ready_GrenadeAttribute(const GRENADE_DESC& GrenadeDesc)
{
    _matrix matView = m_pGameInstance->Get_Transform_Matrix(CPipeLine::D3DTS_VIEW);
    m_bDownwardThrow = GrenadeDesc.bIsDownwardThrow;
    _vector vNormalizedCamLook = XMVector3Normalize(XMLoadFloat4(m_pGameInstance->Get_CamDirection()));

    if (false == m_bDownwardThrow)
    {
        PxRigidDynamic* pRigidDynamic = m_pRigid->Get_PxActor()->is<PxRigidDynamic>();
        if (pRigidDynamic)
            pRigidDynamic->setLinearDamping(0.f);

        // 시작 위치와 목표 위치를 설정
        _vector start = m_pTransformCom->Get_Position();

        _vector target = start + vNormalizedCamLook * m_fThrowDistance;

        _float T = 1.2f;               // 목표 위치까지 도달하는 시간 (초)

        _vector angularVel = XMVectorSet(7.5f, 0.f, 0.f, 0.f);
        m_pRigid->Set_AngularVelocity(angularVel);

        m_pRigid->Set_Velocity(CMyUtils::Get_InitialVelocityFromTargetPoint(start, target, 1.2f));
    }
    else
    {
        // 아래로 던지기 (위로 올라갔다가 내려오는 궤적)
        // 카메라 방향의 수평 성분만 추출
        _vector vCamLookHorizontal = XMVectorSetY(vNormalizedCamLook, 0.f);
        vCamLookHorizontal = XMVector3Normalize(vCamLookHorizontal);

        // 초기 속도 설정
        _float fHorizontalSpeed = 15.f;  // 수평 속도 (더 강하게)
        _float fVerticalSpeed = -5.f;    // 수직 속도 (아래로, 약하게)

        // 수평 속도 + 수직 속도 조합
        _vector vInitialVelocity = vCamLookHorizontal * fHorizontalSpeed;
        vInitialVelocity = XMVectorSetY(vInitialVelocity, fVerticalSpeed);

        //_vector vVel = XMVectorSetY(vNormalizedCamLook * m_fDownThrowDistance, 0.f); // 수평 속도만

        m_pRigid->Set_Velocity(vInitialVelocity);
        m_pRigid->Set_AngularVelocity(XMVectorSet(0.f, 0.f, 0.f, 0.f));

        PxRigidDynamic* pRigidDynamic = m_pRigid->Get_PxActor()->is<PxRigidDynamic>();
        if (pRigidDynamic)
            pRigidDynamic->setLinearDamping(1.5f); 
    }

    m_bDone = false;
    m_fAccTime = 0.f;
    m_bExplosionOn = false;

    return S_OK;
}

_float CGrenade::Calculate_GrenadeDamage(_float fDistance, _float fMaxDamage, _float fExplosionRadius)
{
    if (fDistance > fExplosionRadius) 
        return 0.f;

    _float fNormalizedDistance = fDistance / fExplosionRadius;
    return fMaxDamage * (1.0f - (fNormalizedDistance * fNormalizedDistance));
}

void CGrenade::Spawn_FlashBangParticle()
{
    _vector vBombCenter = m_pTransformCom->Get_Position();

    _float posX = vBombCenter.m128_f32[0];
    _float posY = vBombCenter.m128_f32[1];
    _float posZ = vBombCenter.m128_f32[2];

    _vector pos = vBombCenter;

    CParticleEmitter* light_0;
    CParticleEmitter* light_1;
    CParticleEmitter* light_2;
    CParticleEmitter* light_3;
    CParticleEmitter* light_4;
    m_pGameInstance->Spawn_Particle(L"Flash_Bang", light_0);
    m_pGameInstance->Spawn_Particle(L"Flash_Bust", light_1);
    m_pGameInstance->Spawn_Particle(L"Flash_Smoke_Sub", light_2);
    m_pGameInstance->Spawn_Particle(L"Flash_Dust_Sub", light_3);
    m_pGameInstance->Spawn_Particle(L"Flash_Smoke", light_4);
    light_0->Set_WorldMatrix(XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixTranslation(posX, posY + 0.5f, posZ));
    light_1->Set_WorldMatrix(XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixTranslation(posX, posY, posZ));
    light_2->Set_WorldMatrix(XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixTranslation(posX, posY, posZ));
    light_3->Set_WorldMatrix(XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixTranslation(posX, posY, posZ));
    light_4->Set_WorldMatrix(XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixTranslation(posX, posY, posZ));

    // 디스토션 디스크 생성
    CGameManager::GetInstance()->Spawn_DistortDisk(pos);

    // 광원 틀기
    _vector vRadiance = XMVectorSet(20.f, 18.f, 17.f, 10.f);
    CGameManager::GetInstance()->Blink_PointLight(pos, vRadiance, 0.1f);

    // 샤프트 키기
    CGameManager::GetInstance()->Set_Shaft(true, pos, 2.f);
}

void CGrenade::Spawn_GranadeParticle()
{
    _vector vBombCenter = m_pTransformCom->Get_Position();

    CParticleEmitter* light_0;
    CParticleEmitter* light_1;
    CParticleEmitter* light_2;
    CParticleEmitter* light_3;
    CParticleEmitter* light_4;
    CParticleEmitter* light_5;
    CParticleEmitter* light_6;
    m_pGameInstance->Spawn_Particle(L"Granade_Bust", light_0);
    m_pGameInstance->Spawn_Particle(L"Granade_Dusts", light_1);
    m_pGameInstance->Spawn_Particle(L"Granade_Smoke_Sub", light_2);
    m_pGameInstance->Spawn_Particle(L"Granade_Spread", light_3);
    m_pGameInstance->Spawn_Particle(L"Granade_UpSmoke", light_4);
    m_pGameInstance->Spawn_Particle(L"Granade_Dust_Sub", light_5);
    m_pGameInstance->Spawn_Particle(L"Grenade_Smoke", light_6);
    _vector vOffsetPos = XMVectorSet(0.f, 0.5f, 0.f, 0.f);
    light_0->Set_WorldMatrix(XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixTranslationFromVector(vBombCenter + vOffsetPos));
    light_1->Set_WorldMatrix(XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixTranslationFromVector(vBombCenter));
    light_2->Set_WorldMatrix(XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixTranslationFromVector(vBombCenter));
    light_3->Set_WorldMatrix(XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixTranslationFromVector(vBombCenter));
    light_4->Set_WorldMatrix(XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixTranslationFromVector(vBombCenter));
    light_5->Set_WorldMatrix(XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixTranslationFromVector(vBombCenter));
    light_6->Set_WorldMatrix(XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixTranslationFromVector(vBombCenter));

    CGameManager::GetInstance()->Spawn_DistortDisk(vBombCenter);

    // 광원 틀기
    _vector vRadiance = XMVectorSet(20.f, 10.f, 3.f, 1.f);
    CGameManager::GetInstance()->Blink_PointLight(vBombCenter, vRadiance, 0.1f);
}

CGrenade* CGrenade::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CGrenade* pInstance = new CGrenade(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CGrenade");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CGrenade::Clone(void* pArg)
{
    CGrenade* pInstance = new CGrenade(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Cloned : CGrenade");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CGrenade::Free()
{
    __super::Free();

    Safe_Release(m_pBombTrigger);
}

CRPG::CRPG(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CWorldItem(pDevice, pContext)
{
}

CRPG::CRPG(const CRPG& Prototype)
    : CWorldItem(Prototype)
{
}

HRESULT CRPG::Initialize(void* pArg)
{
    WorldItemDesc WorldItemDesc;

    if (nullptr != pArg)
        WorldItemDesc = *reinterpret_cast<CWorldItem::WorldItemDesc*>(pArg);

    WorldItemDesc.BoundBox = BoundingBox({ 0.0f, 1.f, 0.f }, { 1.f, 1.f, 1.f });
    WorldItemDesc.boundType = BOUNDTYPE::AABB;
    WorldItemDesc.eRigidType = RIGIDSHAPE::NONE;

    WorldItemDesc.fSpeedPerSec = 10.f;
    WorldItemDesc.fRotationPerSec = XM_PI * 0.4f;

    lstrcpy(WorldItemDesc.szGameObjectTag, L"Projectile_RPG");

    if (FAILED(__super::Initialize(&WorldItemDesc)))
        return E_FAIL;

    m_pBound->Set_BoundType(RIGIDSHAPE::SPHERE);
    m_pBound->Set_Mask(OT_PLAYER);

    if (E_FAIL == Ready_Components())
        return E_FAIL;

    if (E_FAIL == Ready_Materials())
        return E_FAIL;

    if (E_FAIL == Ready_Collision())
        return E_FAIL;

    m_pGameInstance->Spawn_Particle(L"RPG_Fire", m_pFireEmitter);
    m_pGameInstance->Spawn_Particle(L"RPG_Smoke", m_pSmokeEmitter);

    PlaySoundEvent("event:/Weapon/RPG/OnStartFlyEffet", false, false, false, {});

    return S_OK;
}

void CRPG::Update(_float fTimeDelta)
{
    m_pTransformCom->Go_Straight(fTimeDelta);

    Update_Particle(fTimeDelta);

    __super::Update(fTimeDelta);
}

HRESULT CRPG::Ready_Components()
{
    /* Com_Model */
    if (FAILED(__super::Add_Component(LEVEL_STATIC, L"Prototype_Component_Model_RPGShell",
        reinterpret_cast<CComponent**>(&m_pModelCom), TEXT("Com_Model"))))
        return E_FAIL;

    /* Com_CompositeArmature */
    //if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_CompositeArmature"),
    //    reinterpret_cast<CComponent**>(&m_pCompositeArmature), TEXT("Com_CompositeArmature"))))
    //    return E_FAIL;

    /*m_pCompositeArmature->Set_BaseArmature(
        TEXT("Prototype_Component_Model_RPGShell"),
        TEXT("Prototype_Component_Armature_RPGShell"),
        LEVEL_GAMEPLAY);*/

    return S_OK;
}

HRESULT CRPG::Ready_Collision()
{
    m_pBound->Add_CollisionEnterCallBack(
        [&](CBound* pBound, COLLISION_INFO* pInfo)
        {
            DamageEventRE DamageEvent;
            DamageEvent.ColInfo = *pInfo;

            DamageEvent.eType = DAMAGE_FINISH;
            DamageEvent.fDamage = 10000.f;
            DamageEvent.pOverlappedComp = pBound;

            DamageEvent.eReactionType = REACTION_BLOWNAWAY;
            
            pBound->Get_Owner()->Take_Damage(10000.f, &DamageEvent, m_pInstigator, this);

            PlaySoundEvent("event:/Weapon/RPG/HitSomething", false, false, false, {});

            Delete_Object(this);
        });

    return S_OK;
}

void CRPG::OnDebug()
{
    /*if (m_pGameInstance->Key_Pressing(DIK_1))
        m_fPitch -= 0.001f;
    else if (m_pGameInstance->Key_Pressing(DIK_2))
        m_fPitch += 0.001f;

    if (m_pGameInstance->Key_Pressing(DIK_3))
        m_fYaw -= 0.001f;
    else if (m_pGameInstance->Key_Pressing(DIK_4))
        m_fYaw += 0.001f;

    if (m_pGameInstance->Key_Pressing(DIK_5))
        m_fRoll -= 0.001f;
    else if (m_pGameInstance->Key_Pressing(DIK_6))
        m_fRoll += 0.001f;*/
}

void CRPG::Update_Particle(_float fTimeDelta)
{
    if (m_pFireEmitter)
        m_pFireEmitter->Set_WorldMatrix(XMMatrixTranslationFromVector(m_pTransformCom->Get_Position()));

    if (m_pSmokeEmitter)
        m_pSmokeEmitter->Set_WorldMatrix(XMMatrixTranslationFromVector(m_pTransformCom->Get_Position()));
}

CRPG* CRPG::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CRPG* pInstance = new CRPG(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CRPG");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CRPG::Clone(void* pArg)
{
    CRPG* pInstance = new CRPG(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Cloned : CRPG");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CRPG::Free()
{
    __super::Free();
}