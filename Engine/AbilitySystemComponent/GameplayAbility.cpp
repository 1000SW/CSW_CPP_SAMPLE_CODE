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
// AbilityTask를 생성할 때 불려지는 함수 Task->InitTask => IGameplayAbility->Assign_Task 
// Task에 대해서 자료구조에 추가 및 델리게이트 이벤트 등록
// ======================
void IGameplayAbility::Assign_Task(IAbilityTask* pTask)
{
	m_OwningTasks.push_back(pTask);
    pTask->m_OnEventDelegate.AddDynamic("GameplayAbility", this, &IGameplayAbility::OnTaskEventReceived);
}

// ======================
// Task의 ReadyForActivation 호출시 불려지는 함수. Task->ReadyForActivation => IGameplayAbility->Add_ActivatingTask
// 실행되고 있는 Task에 대한 관리 수행
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
// ASC가 어빌리티를 실행하기 위해 태그 검사 하기 전에, 간단한 조건 체킹으로 실행 가능한지 체킹하기 위한 함수
// ======================
_bool IGameplayAbility::CanActivate_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
    return true;
}

// ======================
// 어빌리티가 트리거될 때 불려지는 핵심 함수 
// 트리거 된 그 순간에만 이 함수가 불리고 추가적인 업데이트가 필요하다면 어빌리티 태스크를 해당 함수에서 트리거하는 식으로 추가 업데이트 로직을 구성하게 됨.
// GameplayEventData로 트리거될 수도 있고, 아니라면 nullptr를 전달받게 됨.
// ======================
void IGameplayAbility::Activate_Ability(const GameplayAbility_ActorInfo* pActorInfo, const GameplayEventData* pTriggerEventData)
{

}

// 내부적으로 어빌리티를 종료할 때 부르는 함수
// 실행중인 Task종료 및 ASC에게 종료됐다는 이벤트 송신
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

// 어빌리티가 발동된 후, 어빌리티에 대한 비용을 지불하기 위한 함수 ex)총알 발사, 아이템 회복 등등
bool IGameplayAbility::Commit_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
    return true;
}

// 다른 어빌리티에 의해 블로킹되거나 취소될 때 수동적으로 불려지는 함수
void IGameplayAbility::Cancel_Ability(const GameplayAbility_ActorInfo* pActorInfo)
{
    End_Ability(pActorInfo, true);
}

// 어빌리티 테스크의 델리게이트에 의해 트리거되는 함수
// 클라이언트에서 오버라이딩해서 테스크 종료같은 이벤트를 수신하기 위해서 사용했음
void IGameplayAbility::OnTaskEventReceived(IAbilityTask* pTask, _uint iEventType)
{

}

// 어빌리리가 취소할 때 해당 어빌리티가 취소가능한지 체킹하기 위한 함수
// 실질적으로 사용하지는 않았음
_bool IGameplayAbility::CanBeCanceled() const
{
	return true;
}

// 어빌리티가 실행되고 있을 때 외부적 요인에 의해 트리거 되는 함수
// 어빌리티를 초기화할 때 AbilityTriggerData를 보관하는 자료구조에 AbilityTriggerData가 존재한다면 트리거 될 수 있음
// 어빌리티가 실행되고 있고, 내부적으로 가지고 있는 AbilityTriggerData구조체의 EventTag가 매개변수로 들어오는 EventTag와 일치하면 트리거 된다.
// 사용예시) 재장전 애니메이션 수행 중 탄약 장전 이벤트 수행 등등
void IGameplayAbility::OnGameplayEvent(GameplayTag EventTag, const GameplayEventData* pTriggerEventData)
{
}

// 어빌리티가 실행되기 전 불려지는 오버라이딩 불가 함수
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
