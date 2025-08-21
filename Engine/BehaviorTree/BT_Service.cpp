#include "BT_Service.h"
#include "BehaviorTree.h"

IBT_Service::IBT_Service()
{
    m_strClassName;

    m_strNodeName = typeid(*this).name();
}

HRESULT IBT_Service::Initialize(void* pArg)
{
    return S_OK;
}

void IBT_Service::PreExecuteService()
{
    m_pBehaviorTree->Add_Service(this);
    ExecuteService();
}

void IBT_Service::FinishService()
{
    m_pBehaviorTree->Remove_Service(this);
    OnServiceFinished();
}

void IBT_Service::ExecuteService()
{
}

void IBT_Service::PreTickService(_float DeltaSeconds)
{
    m_fAccTime += DeltaSeconds;

    if (m_fAccTime >= m_fTickInterval)
    {
        m_fAccTime = 0.f;
        TickService(DeltaSeconds);
    }
}

void IBT_Service::TickService(_float DeltaSeconds)
{
}

void IBT_Service::OnServiceFinished()
{
}

rapidjson::Value IBT_Service::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    rapidjson::Value ObjValue = __super::Save_JsonFile(Alloc);

    ObjValue.AddMember("m_fTickInterval", m_fTickInterval, Alloc);

    return ObjValue;
}

HRESULT IBT_Service::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    __super::Load_JsonFile(pJsonValue, pTree);

    if (pJsonValue)
    {
        if (nullptr == m_pBlackboard)
            return E_FAIL;

        if ((*pJsonValue).HasMember("m_fTickInterval"))
            m_fTickInterval = (*pJsonValue)["m_fTickInterval"].GetFloat();
    }

    return S_OK;
}

void IBT_Service::Free()
{
    __super::Free();
}


