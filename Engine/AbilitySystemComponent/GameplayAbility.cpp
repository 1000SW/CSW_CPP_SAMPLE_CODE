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
// ======================
// AbilityTask�� ������ �� �ҷ����� �Լ� Task->InitTask => IGameplayAbility->Assign_Task 
// Task�� ���ؼ� �ڷᱸ���� �߰� �� ��������Ʈ �̺�Ʈ ���
// ======================
void IGameplayAbility::Assign_Task(IAbilityTask* pTask)
{
	m_OwningTasks.push_back(pTask);
    pTask->m_OnEventDelegate.AddDynamic("GameplayAbility", this, &IGameplayAbility::OnTaskEventReceived);
}

// ======================
// Task�� ReadyForActivation ȣ��� �ҷ����� �Լ�. Task->ReadyForActivation => IGameplayAbility->Add_ActivatingTask
// ����ǰ� �ִ� Task�� ���� ���� ����
// ======================
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

// ======================
// ASC�� �����Ƽ�� �����ϱ� ���� �±� �˻� �ϱ� ����, ������ ���� üŷ���� ���� �������� üŷ�ϱ� ���� �Լ�
// ======================
_bool IGameplayAbility::CanActivate_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
    return true;
}

// ======================
// �����Ƽ�� Ʈ���ŵ� �� �ҷ����� �ٽ� �Լ� 
// Ʈ���� �� �� �������� �� �Լ��� �Ҹ��� �߰����� ������Ʈ�� �ʿ��ϴٸ� �����Ƽ �½�ũ�� �ش� �Լ����� Ʈ�����ϴ� ������ �߰� ������Ʈ ������ �����ϰ� ��.
// GameplayEventData�� Ʈ���ŵ� ���� �ְ�, �ƴ϶�� nullptr�� ���޹ް� ��.
// ======================
void IGameplayAbility::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{

}

// ���������� �����Ƽ�� ������ �� �θ��� �Լ�
// �������� Task���� �� ASC���� ����ƴٴ� �̺�Ʈ �۽�
void IGameplayAbility::End_Ability(const GameplayAbility_ActorInfo* pActorInfo, bool bWasCancelled)
{
    m_bActivate = false;

    // �����̳� ��ȸ�� ������ �����ϸ� ���� �����̳ʰ� ������ �ޱ⶧���� �����ؼ� ������ ����
    vector<IAbilityTask*> ActivatingTasks = m_ActivatingTasks;
    for (auto& pTask : ActivatingTasks)
    {
        pTask->End_Task();
    }
    m_ActivatingTasks.clear();

    m_AbilityDelegate.Broadcast(this, TYPE_END_ABILITY);
}

// �����Ƽ�� �ߵ��� ��, �����Ƽ�� ���� ����� �����ϱ� ���� �Լ� ex)�Ѿ� �߻�, ������ ȸ�� ���
bool IGameplayAbility::Commit_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
    return true;
}

// �ٸ� �����Ƽ�� ���� ���ŷ�ǰų� ��ҵ� �� ���������� �ҷ����� �Լ�
void IGameplayAbility::Cancel_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
    End_Ability(pActorInfo, true);
}

// �����Ƽ �׽�ũ�� ��������Ʈ�� ���� Ʈ���ŵǴ� �Լ�
// Ŭ���̾�Ʈ���� �������̵��ؼ� �׽�ũ ���ᰰ�� �̺�Ʈ�� �����ϱ� ���ؼ� �������
void IGameplayAbility::OnTaskEventReceived(IAbilityTask* pTask, _uint iEventType)
{

}

// ��������� ����� �� �ش� �����Ƽ�� ��Ұ������� üŷ�ϱ� ���� �Լ�
// ���������� ��������� �ʾ���
_bool IGameplayAbility::CanBeCanceled() const
{
	return true;
}

// �����Ƽ�� ����ǰ� ���� �� �ܺ��� ���ο� ���� Ʈ���� �Ǵ� �Լ�
// �����Ƽ�� �ʱ�ȭ�� �� AbilityTriggerData�� �����ϴ� �ڷᱸ���� AbilityTriggerData�� �����Ѵٸ� Ʈ���� �� �� ����
// �����Ƽ�� ����ǰ� �ְ�, ���������� ������ �ִ� AbilityTriggerData����ü�� EventTag�� �Ű������� ������ EventTag�� ��ġ�ϸ� Ʈ���� �ȴ�.
// ��뿹��) ������ �ִϸ��̼� ���� �� ź�� ���� �̺�Ʈ ���� ���
void IGameplayAbility::OnGameplayEvent(GameplayTag EventTag, const GameplayEventData* pTriggerEventData)
{
}

// �����Ƽ�� ����Ǳ� �� �ҷ����� �������̵� �Ұ� �Լ�
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
