#include "GameplayTask.h"
#include "GameInstance.h"

IGameplayTask::IGameplayTask()
	:m_pGameInstance{ CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
    m_ullUniqueID = m_pGameInstance->Get_UniqueID();
}

HRESULT IGameplayTask::Initialize()
{
	return S_OK;
}

_bool IGameplayTask::ReadyForActivation()
{
    if (m_eTaskState == TYPE_ACTIVE)
        return false;

    m_eTaskState = TYPE_ACTIVE;

    Activate();
    
    return true;
}

void IGameplayTask::Activate()
{
}

void IGameplayTask::OnDestroy(bool bInOwnerFinished)
{
   
}

void IGameplayTask::End_Task()
{
    m_eTaskState = TYPE_FINISHED;
}

IGameplayTask* IGameplayTask::Clone(void* pArg)
{
	return nullptr;
}

void IGameplayTask::Free()
{
	Safe_Release(m_pGameInstance);
}
