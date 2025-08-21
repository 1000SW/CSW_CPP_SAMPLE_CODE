#include "GameplayAbility.h"
#include "GameInstance.h"
#include "AbilityTask.h"

IGameplayAbility::IGameplayAbility()
    :m_pGameInstance(CGameInstance::GetInstance())
{
    Safe_AddRef(m_pGameInstance);
}

HRESULT IGameplayAbility::Initialize_Prototype()
{
	return S_OK;
}

HRESULT IGameplayAbility::Initialize(void* pArg)
{
	return S_OK;
}

HRESULT IGameplayAbility::Late_Initialize()
{
    return S_OK;
}

void IGameplayAbility::Assign_Task(IAbilityTask* pTask)
{
	m_OwningTasks.push_back(pTask);
    pTask->m_OnEventDelegate.AddDynamic("GameplayAbility", this, &IGameplayAbility::OnTaskEventReceived);
}

void IGameplayAbility::Add_ActivatingTask(IAbilityTask* pTask)
{
    auto it = find(m_ActivatingTasks.begin(), m_ActivatingTasks.end(), pTask);
    if (it != m_ActivatingTasks.end())
        return;
    m_ActivatingTasks.push_back(pTask);
}

void IGameplayAbility::Delete_ActivatingTask(IAbilityTask* pTask)
{
    auto it = find(m_ActivatingTasks.begin(), m_ActivatingTasks.end(), pTask);
    if (it == m_ActivatingTasks.end())
        return;

    m_ActivatingTasks.erase(it);
}

_bool IGameplayAbility::CanActivate_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
    return true;
}

void IGameplayAbility::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{

}

void IGameplayAbility::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    m_bActivate = false;

    // 컨테이너 순회중 삭제를 진행하면 현재 컨테이너가 영향을 받기때문에 복제해서 삭제를 수행
    vector<IAbilityTask*> ActivatingTasks = m_ActivatingTasks;
    for (auto& pTask : ActivatingTasks)
    {
        pTask->End_Task();
    }
    m_ActivatingTasks.clear();

    m_AbilityDelegate.Broadcast(this, TYPE_END_ABILITY);
}

bool IGameplayAbility::Commit_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
    return true;
}

void IGameplayAbility::Cancel_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
    End_Ability(pActorInfo, true);
}

void IGameplayAbility::OnTaskEventReceived(IAbilityTask* pTask, _uint iEventType)
{

}

_bool IGameplayAbility::CanBeCanceled() const
{
	// 인스턴싱 정책에 따른 취소 여부 판단?

	return true;
}

void IGameplayAbility::CleanUp()
{
}

void IGameplayAbility::OnGameplayEvent(GameplayTag EventTag, const GameplayEventData* pTriggerEventData)
{
}

void IGameplayAbility::PreActivate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{
    m_bActivate = true;

    m_AbilityDelegate.Broadcast(this, TYPE_ACTIVATE);

    Activate_Ability(pActorInfo, pTriggerEventData);
}

void IGameplayAbility::PreCancel_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
    if (CanBeCanceled())
    {
        if (m_pASC)
            m_AbilityDelegate.Broadcast(this, TYPE_CANCEL);

        Cancel_Ability(pActorInfo);
    }
}

void IGameplayAbility::PreOnGranted(CAbilitySystemComponent* pGAS)
{
    m_pASC = pGAS;
}

IGameplayAbility* IGameplayAbility::Clone(void* pArg)
{
	return nullptr;
}

void IGameplayAbility::Free()
{
    Safe_Release(m_pGameInstance);

    for (auto& pTask : m_OwningTasks)
        Safe_Release(pTask);

    m_OwningTasks.clear();

    __super::Free();
}
