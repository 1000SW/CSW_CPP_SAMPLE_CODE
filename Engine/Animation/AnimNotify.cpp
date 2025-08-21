#include "AnimNotify.h"
#include "GameInstance.h"
#include "Animator.h"
#include "HealthObject.h"
#include "CharacterController.h"
#include "ParticleEmitter.h"

using namespace rapidjson;

CAnimNotify::CAnimNotify()
    : m_pGameInstance{ CGameInstance::GetInstance() }
{
    Safe_AddRef(m_pGameInstance);
}

CAnimNotify::CAnimNotify(const CAnimNotify& Prototype)
    : m_fTriggerTime{ Prototype.m_fTriggerTime }
    , m_strNotifyName{ Prototype.m_strNotifyName }
    , m_pGameInstance{ Prototype.m_pGameInstance }
    , m_pOwner{ Prototype.m_pOwner }
    , m_pArmature { Prototype.m_pArmature }
    /*, m_fnTriggered { Prototype.m_fnTriggered }*/
{
    Safe_AddRef(m_pGameInstance);
}

HRESULT CAnimNotify::Initialize_Prototype(const _string& strNotifyName, rapidjson::Value* pJsonValue)
{
    m_strNotifyName = strNotifyName;
    if (pJsonValue)
    {
        m_fTriggerTime = (*pJsonValue)["m_fTriggerTime"].GetFloat();
    }

    return S_OK;
}

HRESULT CAnimNotify::Late_Initialize(CGameObject* pOnwer)
{
    if (pOnwer)
        m_pOwner = pOnwer;

    return S_OK;
}

void CAnimNotify::Update(_float fTrackPos)
{
    if (!m_bTriggered && m_fTriggerTime <= fTrackPos)
    {
        m_bTriggered = true;
        Received_Notify(fTrackPos);
    }
}

void CAnimNotify::Received_Notify(_float fTrackPos)
{
    if (m_fnTriggered)
        m_fnTriggered(fTrackPos);
}

void CAnimNotify::Reset(_float fCurrentTime)
{
    m_bTriggered = false;
}

rapidjson::Value CAnimNotify::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    Value NotifyValue(kObjectType);

    NotifyValue.AddMember("m_strNotifyName", StringRef(m_strNotifyName.c_str()), Alloc);
    NotifyValue.AddMember("m_fTriggerTime", m_fTriggerTime, Alloc);

    return NotifyValue;
}

void CAnimNotify::Free()
{
    __super::Free();

    Safe_Release(m_pGameInstance);
}

CAnimNotify_Generic::CAnimNotify_Generic()
    :CAnimNotify()
{

}

CAnimNotify_Generic::CAnimNotify_Generic(const CAnimNotify_Generic& Prototype)
    : CAnimNotify(Prototype)
    , m_strSoundTag{ Prototype.m_strSoundTag }
    , m_fActivationRate{ Prototype.m_fActivationRate }
    , m_bStopPrevSound{ Prototype.m_bStopPrevSound }
    , m_strSocketKey{ Prototype.m_strSocketKey }
    , m_strParticleKey{ Prototype.m_strParticleKey }
    , m_bApplyScaleHundred{ Prototype.m_bApplyScaleHundred }
    , m_ScriptIndices{ Prototype.m_ScriptIndices }
{
}

HRESULT CAnimNotify_Generic::Initialize_Prototype(const _string& strNotifyName, rapidjson::Value* pJsonValue)
{
    __super::Initialize_Prototype(strNotifyName, pJsonValue);

    if (pJsonValue)
    {
        if (m_strNotifyName == "PlaySound" || m_strNotifyName == "StopSound" || m_strNotifyName == "FadeOutSound")
        {
            if (pJsonValue->HasMember("m_strSoundTag"))
                m_strSoundTag = (*pJsonValue)["m_strSoundTag"].GetString();
            if (pJsonValue->HasMember("m_fActivationRate"))
                m_fActivationRate = (*pJsonValue)["m_fActivationRate"].GetFloat();
            if (pJsonValue->HasMember("m_bStopPrevSound"))
                m_bStopPrevSound = (*pJsonValue)["m_bStopPrevSound"].GetBool();
        }

        else if (m_strNotifyName == "AN_SpawnParticle")
        {
            if (pJsonValue->HasMember("m_strSocketKey"))
                m_strSocketKey = (*pJsonValue)["m_strSocketKey"].GetString();
            if (pJsonValue->HasMember("m_strParticleKey"))
                m_strParticleKey = (*pJsonValue)["m_strParticleKey"].GetString();
            if (pJsonValue->HasMember("m_bApplyScaleHundred"))
                m_bApplyScaleHundred = (*pJsonValue)["m_bApplyScaleHundred"].GetBool();
        }

        else if (m_strNotifyName == "AN_ActivateScript")
        {
            if (pJsonValue->HasMember("m_ScriptIndices"))
            {
                Value& ArrValue = (*pJsonValue)["m_ScriptIndices"];

                for (Value::ValueIterator itr = ArrValue.Begin(); itr != ArrValue.End(); ++itr)
                {
                    const Value& element = *itr; 
                    if (element.IsInt())
                    {
                        _int scriptIndex = element.GetInt();
                        m_ScriptIndices.push_back(scriptIndex);
                    }
                }
            }

            if (pJsonValue->HasMember("m_fActivationRate"))
                m_fActivationRate = (*pJsonValue)["m_fActivationRate"].GetFloat();
        }
    }

    return S_OK;
}

HRESULT CAnimNotify_Generic::Late_Initialize(CGameObject* pOwner)
{
    __super::Late_Initialize(pOwner);

    if (nullptr == m_pOwner)
        return E_FAIL;

    Bind_Function();

    return S_OK;
}

rapidjson::Value CAnimNotify_Generic::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    Value NotifyValue(kObjectType);

    NotifyValue = __super::Save_JsonFile(Alloc);

    NotifyValue.AddMember("m_strNotifyClassName", "CAnimNotify_Generic", Alloc);
    if (m_strNotifyName == "PlaySound" || m_strNotifyName == "StopSound" || m_strNotifyName == "FadeOutSound")
    {
        NotifyValue.AddMember("m_strSoundTag", StringRef(m_strSoundTag.c_str()), Alloc);
        NotifyValue.AddMember("m_fActivationRate", m_fActivationRate, Alloc);
        NotifyValue.AddMember("m_bStopPrevSound", m_bStopPrevSound, Alloc);
    }
    
    else if (m_strNotifyName == "AN_SpawnParticle")
    {
        NotifyValue.AddMember("m_strSocketKey", StringRef(m_strSocketKey.c_str()), Alloc);
        NotifyValue.AddMember("m_strParticleKey", StringRef(m_strParticleKey.c_str()), Alloc);
        NotifyValue.AddMember("m_bApplyScaleHundred", m_bApplyScaleHundred, Alloc);
    }

    else if (m_strNotifyName == "AN_ActivateScript")
    {
        Value ArrayValue(kArrayType);

        for (auto& iIndex : m_ScriptIndices)
            ArrayValue.PushBack(iIndex, Alloc);

        NotifyValue.AddMember("m_ScriptIndices", ArrayValue, Alloc);
        NotifyValue.AddMember("m_fActivationRate", m_fActivationRate, Alloc);

    }

    return NotifyValue;
}

HRESULT CAnimNotify_Generic::Bind_Function()
{
    if (m_strNotifyName == "PlaySound")
    {
        m_fnTriggered = [&](_float fTrackPos)
            {
                if (CMyUtils::Get_RandomFloat(0.f, 0.999999f) < m_fActivationRate)
                {
                    if (true == m_bStopPrevSound)
                        m_pOwner->StopSoundEvent(m_strSoundTag, false);

                    m_pOwner->PlaySoundEvent(m_strSoundTag, false, false, false, _float4x4{});
                }
            };
    }

    else if (m_strNotifyName == "AN_GravityOff")
    {
        m_fnTriggered = [&](_float fTrackPos)
            {
                if (m_pOwner)
                    m_pOwner->Get_RigidBody()->Set_Gravity(false);
            };
    }
    else if (m_strNotifyName == "AN_GravityOn")
    {
        m_fnTriggered = [&](_float fTrackPos)
            {
                if (m_pOwner->Get_RigidBody())
                    m_pOwner->Get_RigidBody()->Set_Gravity(true);
            };
    }
    else if (m_strNotifyName == "AN_FixRootBoneTranslation")
    {
        m_fnTriggered = [&](_float fTrackPos)
            {
                m_pArmature->Set_FixRootBoneTranslation(true);
            };
    }
    else if (m_strNotifyName == "AN_ApplyRootBoneTranslation")
    {
        m_fnTriggered = [&](_float fTrackPos)
            {
                m_pArmature->Set_FixRootBoneTranslation(false);
            };
    }

    else if (m_strNotifyName == "StopSound")
    {
        m_fnTriggered = [&](_float fTrackPos)
            {
                m_pOwner->StopSoundEvent(m_strSoundTag, false);
                //cout << fTrackPos << endl;
            };
    }

    else if (m_strNotifyName == "FadeOutSound")
    {
        m_fnTriggered = [&](_float fTrackPos)
            {
                m_pOwner->StopSoundEvent(m_strSoundTag, true);
                //cout << fTrackPos << endl;
            };
    }

    else if (m_strNotifyName == "AN_SpawnParticle")
    {
        m_fnTriggered = [&](_float fTrackPos)
            {
                _matrix matSocket;
                if (true == m_pArmature->Get_SocketTrasnform(m_strSocketKey, matSocket))
                {
                    _matrix matCombined = matSocket * m_pOwner->Get_TransformCom()->Get_WorldMatrix();

                    CParticleEmitter* pEmitter = { nullptr };
                    m_pGameInstance->Spawn_Particle(CMyUtils::Convert_WideString(m_strParticleKey), pEmitter);
                    if (nullptr == pEmitter)
                        return;

                    pEmitter->Set_WorldMatrix(true == m_bApplyScaleHundred ? XMMatrixScaling(100.f, 100.f, 100.f) * matCombined : matCombined);
                }
            };
    }

    else if (m_strNotifyName == "AN_ActivateScript")
    {
        m_fnTriggered = [&](_float fTrackPos)
            {
                if (CMyUtils::Get_RandomFloat(0.f, 0.999999f) < m_fActivationRate)
                {
                    if (false == m_ScriptIndices.empty())
                    {
                        _int iRandomValueIndex = CMyUtils::Get_RandomInt(0, m_ScriptIndices.size() - 1);
                        m_pGameInstance->Start_Script(m_ScriptIndices[iRandomValueIndex]);
                    }
                }
            };
    }


    return S_OK;
}

CAnimNotify_Generic* CAnimNotify_Generic::Create(const _string& strNotifyName, rapidjson::Value* pJsonValue)
{
    CAnimNotify_Generic* pInstance = new CAnimNotify_Generic();

    if (FAILED(pInstance->Initialize_Prototype(strNotifyName, pJsonValue)))
    {
        MSG_BOX("Failed To Created : CAnimNotify_Generic");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CAnimNotify* CAnimNotify_Generic::Clone(void* pArg)
{
    return new CAnimNotify_Generic(*this);
}

void CAnimNotify_Generic::Free()
{
    __super::Free();
}

CAnimNotify_HealthObject::CAnimNotify_HealthObject()
{
}

CAnimNotify_HealthObject::CAnimNotify_HealthObject(const CAnimNotify_HealthObject& Prototype)
    : CAnimNotify(Prototype)
{
}

HRESULT CAnimNotify_HealthObject::Initialize_Prototype(const _string& strNotifyName, rapidjson::Value* pJsonValue)
{
    __super::Initialize_Prototype(strNotifyName, pJsonValue);

    if (m_strNotifyName == "AN_HealthObj_PlaySound_ReceiveParam")
    {
        if (pJsonValue->HasMember("m_strSoundTag"))
            m_strSoundTag = (*pJsonValue)["m_strSoundTag"].GetString();
        else if (pJsonValue->HasMember("m_strParamTag"))
            m_strParamTag = (*pJsonValue)["m_strParamTag"].GetString();
        else if (pJsonValue->HasMember("m_iIntParam"))
            m_iIntParam = (*pJsonValue)["m_iIntParam"].GetInt();
    }

    return S_OK;
}

HRESULT CAnimNotify_HealthObject::Late_Initialize(CGameObject* pOwner)
{
    __super::Late_Initialize(pOwner);

    m_pOwner = pOwner;
    if (m_pOwner == nullptr)
        return E_FAIL;

    m_pHealthObject = dynamic_cast<CHealthObject*>(m_pOwner);

    if (nullptr == m_pHealthObject)
        return E_FAIL;

    Bind_Function();

    return S_OK;
}

rapidjson::Value CAnimNotify_HealthObject::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    Value NotifyValue(kObjectType);

    NotifyValue = __super::Save_JsonFile(Alloc);

    NotifyValue.AddMember("m_strNotifyClassName", "CAnimNotify_HealthObject", Alloc);

    if (m_strNotifyName == "AN_HealthObj_PlaySound_ReceiveParam")
    {
        NotifyValue.AddMember("m_strSoundTag", StringRef(m_strSoundTag.c_str()), Alloc);
        NotifyValue.AddMember("m_strParamTag", StringRef(m_strParamTag.c_str()), Alloc);
        NotifyValue.AddMember("m_iIntParam", m_iIntParam, Alloc);
    }

    return NotifyValue;
}

HRESULT CAnimNotify_HealthObject::Bind_Function()
{
    if (m_strNotifyName == "AN_HealthObj_RotateToTarget")
    {
        m_fnTriggered = [&](_float fTrackPos)
            {
                if (nullptr == m_pHealthObject)
                    return;

                _vector vPoint = m_pHealthObject->Get_Target()->Get_TransformCom()->Get_Position() - m_pHealthObject->Get_TransformCom()->Get_Position();
                m_pHealthObject->Get_TransformCom()->Rotation_Dir(XMVectorSetY(vPoint, 0.f));
            };
    }
    else if (m_strNotifyName == "AN_HealthObj_InvokeAttackBehavior")
    {
        m_fnTriggered = [&](_float fTrackPos)
            {
                if (m_pHealthObject)
                    m_pHealthObject->Invoke_AttackBehavior(true, m_pAnimator->Get_CurrentAnimIndex(), m_pAnimator, true);
            };
    }
    else if (m_strNotifyName == "AN_HealthObj_PlaySound_ReceiveParam")
    {
        m_fnTriggered = [&](_float fTrackPos)
            {
                m_pOwner->PlaySoundEvent(m_strSoundTag, false, false, m_strParamTag, m_iIntParam);
            };
    }

    return S_OK;
}

CAnimNotify_HealthObject* CAnimNotify_HealthObject::Create(const _string& strNotifyName, rapidjson::Value* pJsonValue)
{
    CAnimNotify_HealthObject* pInstance = new CAnimNotify_HealthObject();

    if (FAILED(pInstance->Initialize_Prototype(strNotifyName, pJsonValue)))
    {
        MSG_BOX("Failed To Created : CAnimNotify_HealthObject");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CAnimNotify* CAnimNotify_HealthObject::Clone(void* pArg)
{
    return new CAnimNotify_HealthObject(*this);
}

void CAnimNotify_HealthObject::Free()
{
    __super::Free();
}

CAnimNotify_CharacterController::CAnimNotify_CharacterController()
{
}

CAnimNotify_CharacterController::CAnimNotify_CharacterController(const CAnimNotify_CharacterController& Prototype)
    : CAnimNotify(Prototype)
    , m_bFlag(Prototype.m_bFlag)
{
}

HRESULT CAnimNotify_CharacterController::Initialize_Prototype(const _string& strNotifyName, rapidjson::Value* pJsonValue)
{
    __super::Initialize_Prototype(strNotifyName, pJsonValue);

    if (pJsonValue)
    {
        if (m_strNotifyName == "AN_CharacterController_ApplyGravity")
        {
            if (pJsonValue->HasMember("m_bFlag"))
                m_bFlag = (*pJsonValue)["m_bFlag"].GetBool();
        }

        else if (m_strNotifyName == "AN_CharacterController_ApplyGhost")
        {
            if (pJsonValue->HasMember("m_bFlag"))
                m_bFlag = (*pJsonValue)["m_bFlag"].GetBool();
        }
    }

    return S_OK;
}

HRESULT CAnimNotify_CharacterController::Late_Initialize(CGameObject* pOwner)
{
    __super::Late_Initialize(pOwner);

    m_pCCT = (m_pOwner->Get_Component<CCharacterController>());
    if (nullptr == m_pCCT)
        return E_FAIL;

    Bind_Function();

    return S_OK;
}

rapidjson::Value CAnimNotify_CharacterController::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    Value NotifyValue(kObjectType);

    NotifyValue = __super::Save_JsonFile(Alloc);

    NotifyValue.AddMember("m_strNotifyClassName", "CAnimNotify_CharacterController", Alloc);

    if (m_strNotifyName == "AN_CharacterController_ApplyGravity" || m_strNotifyName == "AN_CharacterController_ApplyGhost")
    {
        NotifyValue.AddMember("m_bFlag", m_bFlag, Alloc);
    }

    return NotifyValue;
}

HRESULT CAnimNotify_CharacterController::Bind_Function()
{
    if (m_strNotifyName == "AN_CharacterController_ApplyGravity")
    {
        m_fnTriggered = [&](_float fTrackPos)
            {
                if (nullptr == m_pCCT)
                    return;

                m_pCCT->Apply_Gravity(true);
            };
    }
    else if (m_strNotifyName == "AN_CharacterController_ApplyGhost")
    {
        m_fnTriggered = [&](_float fTrackPos)
            {
                m_pCCT->Apply_GhostMode(true);
            };
    }

    return S_OK;
}

CAnimNotify_CharacterController* CAnimNotify_CharacterController::Create(const _string& strNotifyName, rapidjson::Value* pJsonValue)
{
    CAnimNotify_CharacterController* pInstance = new CAnimNotify_CharacterController();

    if (FAILED(pInstance->Initialize_Prototype(strNotifyName, pJsonValue)))
    {
        MSG_BOX("Failed To Created : CAnimNotify_CharacterController");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CAnimNotify* CAnimNotify_CharacterController::Clone(void* pArg)
{
    return new CAnimNotify_CharacterController(*this);
}

void CAnimNotify_CharacterController::Free()
{
    __super::Free();
}
