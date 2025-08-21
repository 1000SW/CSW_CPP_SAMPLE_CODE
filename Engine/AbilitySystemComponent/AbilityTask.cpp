#include "AbilityTask.h"
#include "GameplayAbility.h"
#include "GameInstance.h"

IAbilityTask::IAbilityTask()
	: IGameplayTask()
{
}

IAbilityTask::IAbilityTask(CAbilitySystemComponent* pGAS)
	:m_pAbilitySystemComponent(pGAS)
{
}

void IAbilityTask::Set_TickingTask(_bool bState)
{
	if (bState)
	{
		m_bTickingTask = bState; 
	}
}

HRESULT IAbilityTask::Initialize(IGameplayAbility* pThisAbility)
{
	Init_Task(*pThisAbility);
	return S_OK;
}

void IAbilityTask::Tick_Task(_float fTimeDelta)
{
}

_bool IAbilityTask::ReadyForActivation()
{
    __super::ReadyForActivation();

    m_pOnwerAbility->Add_ActivatingTask(this);

    return true;
}

void IAbilityTask::Init_Task(IGameplayAbility& ThisAbility)
{
	m_pOnwerAbility = &ThisAbility;

	ThisAbility.Assign_Task(this);

	m_pAbilitySystemComponent = m_pOnwerAbility->Get_GAS();
    m_ActorInfo = ThisAbility.Get_ActorInfo();
}

void IAbilityTask::End_Task()
{
    if (m_eTaskState == TYPE_ACTIVE)
    {
        __super::End_Task();

        m_OnEventDelegate.Broadcast(this, TYPE_END_TASK);

        m_pOnwerAbility->Delete_ActivatingTask(this);

        OnDestroy(true);
    }
}

IAbilityTask* IAbilityTask::Create()
{
	return nullptr;
}

void IAbilityTask::Free()
{
	__super::Free();
}