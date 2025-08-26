#include "AbilitySystemComponent.h"
#include "GameObject.h"
#include "CompositeArmatrue.h"
#include "AnimInstance.h"
#include "AbilityTask.h"
#include "GameplayAbility.h"
#include "RigidBody.h"
#include "Context.h"
#include "GameplayEffect.h"
#include "Character.h"
#include "GameInstance.h"
#include "CharacterController.h"

// ======================
// 생성자 / 복사 생성자
// ======================
CAbilitySystemComponent::CAbilitySystemComponent(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CComponent(pDevice, pContext)
{
}

CAbilitySystemComponent::CAbilitySystemComponent(const CAbilitySystemComponent& Prototype)
    : CComponent(Prototype)
{
}

// ======================
// 초기화 관련 메서드
// ======================
HRESULT CAbilitySystemComponent::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CAbilitySystemComponent::Initialize(void* pArg)
{
    return S_OK;
}

// ======================
// Ability Actor 정보 초기화
// ======================
void CAbilitySystemComponent::Init_Ability_ActorInfo(CGameObject* pOwnerActor, CGameObject* pAvatarActor)
{
    m_ActorInfo.pOwner = pOwnerActor;
    m_ActorInfo.pCharacter = dynamic_cast<CCharacter*>(pAvatarActor);
    m_ActorInfo.pCompositeArmature = pAvatarActor->Get_Component<CCompositeArmature>();
    m_ActorInfo.pAbilitySystemComponent = this;
    m_ActorInfo.pAnimInstance = pAvatarActor->Get_Component<CAnimInstance>();
    m_ActorInfo.pRigidBody = pAvatarActor->Get_Component<CRigidBody>();
    m_ActorInfo.pContext = pAvatarActor->Get_Component<CContext>();
    m_ActorInfo.pBound = pAvatarActor->Get_Component<CBound>();
    m_ActorInfo.pCCT = pAvatarActor->Get_Component<CCharacterController>();
} 

// ==================================================
// Update 로직: AbilityTask의 Tick 함수 호출
// ==================================================
void CAbilitySystemComponent::Update_AbilitySystem(_float fTimeDelta)
{
    for (auto& Pair : m_ActivatingAbilites)
    {
        for (auto& pTask : Pair.second->m_ActivatingTasks)
        {
            if (pTask->m_bTickingTask && pTask->m_eTaskState == IGameplayTask::TYPE_ACTIVE)
                pTask->Tick_Task(fTimeDelta);
        }
    }
} 

// ==================================================
// 어빌리티 부여: ex)점프, 공격 등등
// ==================================================
_bool CAbilitySystemComponent::Give_Ability(_string strAbilityTag, IGameplayAbility* pAbility)
{
    if (Find_Ability(strAbilityTag))
        return false;

    pAbility->m_pASC = this;
    // 어빌리티에 이벤트(종료, 시작 등등) 발생했을 때 수신하기 위한 델리게이트 바인딩
    pAbility->m_AbilityDelegate.AddDynamic("ASC", this, &CAbilitySystemComponent::OnAbilityEventReceived);
    pAbility->m_strAbilityName = strAbilityTag;
    pAbility->m_ActorInfo = m_ActorInfo;

    pAbility->Late_Initialize();
    // 바인딩된 인풋 액션 ID가 있을 시 인풋 바인딩 어빌리티 자료구조에 추가해서 키 입력으로 트리거되는 어빌리티 관리 수행
    if (pAbility->m_iInputActionID != -1 && !Find_InputBindingAbilites(pAbility->m_iInputActionID))
    {
        m_InputBindingAbilites.emplace(pAbility->m_iInputActionID, pAbility);
    }

    m_ActivatableAbilities.emplace(strAbilityTag, pAbility);

    return true;
}

// ==================================================
// 어빌리티 언바인딩 및 해제 수행
// ==================================================
void CAbilitySystemComponent::Clear_Ability(string strAbilityKey)
{
    if (IGameplayAbility* pAbility = Find_Ability(strAbilityKey))
    {
        if (pAbility)
            pAbility->End_Ability(&m_ActorInfo, true);
        m_ActivatableAbilities.erase(strAbilityKey);
        Safe_Release(pAbility);
    }
}

void CAbilitySystemComponent::Clear_AllAbilities()
{
    for (auto& [Key, pAbility] : m_ActivatableAbilities)
        Safe_Release(pAbility);
    m_ActivatableAbilities.clear();
}

// ==================================================
// 활성화되고 있는 어빌리티에 이벤트 함수를 호출하기 위한 함수. (샷건 재장전 같이 애니메이션 사이 사이마다 재장전을 수행해야할 때 사용함)
// ==================================================
_uint CAbilitySystemComponent::HandleGameplayEvent(GameplayTag EventTag, const GameplayEventData* Payload)
{
    if (m_ActivatingAbilites.empty() || false == EventTag.IsValid())
        return 0;

    int iRet = 0;
    for (auto& [key, pAbility] : m_ActivatingAbilites)
    {
        for (auto& TriggerData : pAbility->m_AbilityTriggers)
        {
            if (TriggerData.TriggerTag == EventTag && TriggerData.TriggerSource == IGameplayAbility::GameplayEvent)
            {
                GameplayEventData TempPayLoad = *Payload;
                m_pGameInstance->Add_Timer("CAbilitySystemComponent", 0.f, [EventTag, TempPayLoad, pAbility]() {pAbility->OnGameplayEvent(EventTag, &TempPayLoad); });
                iRet++;
                break;
            }
        }
    }

    return iRet;
}

// ==================================================
// 태그를 통한 어빌리티 트리거
// ==================================================
_bool CAbilitySystemComponent::TryActivate_Ability(string strAbilityTag)
{
    IGameplayAbility* pAbility = Find_Ability(strAbilityTag);
    if (nullptr == pAbility)
        return false;

    return TryActivate_Ability(pAbility);
}

// ==================================================
// 포인터를 통한 어빌리티 트리거
// ==================================================
_bool CAbilitySystemComponent::TryActivate_Ability(IGameplayAbility* pAbility)
{
    if (false == CheckingActivateCondition(pAbility))
        return false;

    pAbility->PreActivate_Ability(&m_ActorInfo);

    return true;
}

// ==================================================
// 게임플레이 이벤트 컨테이너를 통한 어빌리티 트리거 함수
// 어빌리티를 발동할 때 주변 환경에 대한 문맥이 필요할 때 사용
// ex) 사다리 타기, 와이어 발사 등등
// ==================================================
bool CAbilitySystemComponent::TriggerAbility_FromGameplayEvent(const string& strAbilityTag, GameplayAbility_ActorInfo* pActorInfo, GameplayTag EventTag, const GameplayEventData* pPayload)
{
    IGameplayAbility* pAbility = Find_Ability(strAbilityTag);
    if (nullptr == pAbility)
        return false;

    if (false == CheckingActivateCondition(pAbility))
        return false;

    GameplayEventData TempEventData = *pPayload;
    TempEventData.EventTag = EventTag;

    pAbility->PreActivate_Ability(pActorInfo, &TempEventData);

    return true;
}

// ==================================================
// 인풋 키를 통한 어빌리티 트리거
// 어빌리티를 발동시키거나 이미 발동중이라면 InputPressed함수 호출
// 사용 예시) 총기 발사, QTE 이벤트 등등
// ==================================================
void CAbilitySystemComponent::AbilityLocalInputPressed(_int iInputID)
{
    IGameplayAbility* pAbility = Find_InputBindingAbilites(iInputID);
    if (nullptr == pAbility)
        return;

    if (!pAbility->IsActivate())
        TryActivate_Ability(pAbility);
    else
        pAbility->InputPressed(&m_ActorInfo);
}

// ==================================================
// 인풋 키를 통한 어빌리티 이벤트 전달 
// 어빌리티에 InputReleased 이벤트 전달
// 사용 예시) 에임 어빌리티에 마우스 우클릭 뗐을 시 에임 어빌리티 종료 수행, 나이프 홀딩 제어 등등
// ==================================================
void CAbilitySystemComponent::AbilityLocalInputReleased(_int iInputID)
{
    IGameplayAbility* pAbility = Find_InputBindingAbilites(iInputID);
    if (nullptr == pAbility || !pAbility->IsActivate())
        return;

    pAbility->InputReleased(&m_ActorInfo);
}

// ==================================================
// 활성화된 어빌리티 취소
// ==================================================
void CAbilitySystemComponent::Cancel_Ability(_string strAbilityTag)
{
    IGameplayAbility* pAbility = Find_ActivatingAbility(strAbilityTag);
    if (nullptr == pAbility || !pAbility->IsActivate())
        return;

    pAbility->Cancel_Ability(&m_ActorInfo);
}

// ==================================================
// 어트리뷰트 세트 관련 함수
// 키를 통해 어트리뷰트를 찾고 어트리뷰트의 Base값 리턴
// ==================================================
float CAbilitySystemComponent::GetNumericAttributeBase(const string& strAttributeKey, bool& bFound) const
{
    if (nullptr == m_pAttributeSet)
    {
        bFound = false;
        return 0.f;
    }

    GameplayAttribute* pAttribute = m_pAttributeSet->Get_Attribute(strAttributeKey);
    if (nullptr == pAttribute)
    {
        bFound = false;
        return 0.f;
    }

    bFound = true;
    return m_pAttributeSet->Get_Attribute(strAttributeKey)->GetBaseValue();
}

// ==================================================
// 어트리뷰트의 Base데이터를 Set해주고 영향을 받기 전(PreAttributeBaseChange), 영향을 받은 후(PostAttributeBaseChange)에 대한 이벤트 함수 호출
// 클라이언트에서 위의 두 함수 오바라이딩을 통해 UI 이벤트나 이펙트 등을 바인딩해서 사용함
// ==================================================
_bool CAbilitySystemComponent::SetNumericAttributeBase(const string& strAttributeKey, float NewBaseValue)
{
    if (nullptr == m_pAttributeSet)
        return false;

    GameplayAttribute* pAttribute = m_pAttributeSet->Get_Attribute(strAttributeKey);
    if (nullptr == pAttribute)
        return false;

    _float fOldValue, fNewValue;

    // 변경전 값 저장
    fOldValue = pAttribute->GetBaseValue();

    m_pAttributeSet->PreAttributeBaseChange(strAttributeKey, NewBaseValue);
    pAttribute->SetBaseValue(NewBaseValue);

    // 변경 후 값 가져오기
    fNewValue = pAttribute->GetBaseValue();

    m_pAttributeSet->PostAttributeBaseChange(strAttributeKey, fOldValue, fNewValue);

    return true;
}

// ==================================================
// 어트리뷰트 CurrentValue 리턴
// ==================================================
float CAbilitySystemComponent::GetGameplayAttributeValue(const string& strAttributeKey, bool& bFound) const
{
    if (nullptr == m_pAttributeSet)
    {
        bFound = false;
        return 0.f;
    }

    GameplayAttribute* pAttribute = m_pAttributeSet->Get_Attribute(strAttributeKey);
    if (nullptr == pAttribute)
    {
        bFound = false;
        return 0.f;
    }

    bFound = true;

    // 원래는 CurrentValue를 리턴해야 하지만 시간적 한계로 인해 시스템을 구축하기 힘들어 BaseValue만을 사용했음
    return pAttribute->GetBaseValue();
}

// ==================================================
// IGameplayTagAssetInterface 인터페이스 오버라이드
// 이 함수와 인터페이스에 구현되어있는 Has_MatchingGameplayTag같은 함수를 통해 현재 ASC가 들고있는 상태와 태그를 체킹 및 쿼리가 가능
// ex) 플레이어가 웅크리고 있는 상태인지 체킹, 플레이어가 현재 상호작용 가능한지 쿼리 등등
// ==================================================
void CAbilitySystemComponent::Get_OwnedGameplayTags(GameplayTagContainer& TagContainer) const
{
    TagContainer = m_OwnedTags;
}

// ==================================================
// 명시적으로 태그 추가/삭제 지원
// 태그 변경/삭제 델리게이트를 발동시켜 클라이언트에서 태그 변경에 대한 이벤트를 수신받을 수 있게 만듦
// 사용 예시) 트리거와의 접촉을 통해 플레이어에게 사다리 타기나 지형 뛰어넘을 수 있는 상태 부여 및 상태 제거
// ==================================================
void CAbilitySystemComponent::AddLooseGameplayTag(const GameplayTag& tag)
{
    GameplayTagContainer Container;
    Container.AddTag(tag);

    m_OwnedTags.AddTag(tag);

    m_OnTagAddedDelegate.Broadcast(Container);
}

void CAbilitySystemComponent::RemoveLooseGameplayTag(const GameplayTag& tag)
{
    GameplayTagContainer Container;
    Container.AddTag(tag);

    m_OwnedTags.RemoveTag(tag);

    m_OnTagRemovedDelegate.Broadcast(Container);
}

void CAbilitySystemComponent::RemoveOwnedTags(const GameplayTagContainer& Tags)
{
    m_OwnedTags.RemoveTags(Tags);
    m_OnTagRemovedDelegate.Broadcast(Tags);
}

// ==================================================
// 어트리뷰트 세트(어트리뷰트를 관리하는 클래스) 추가하기 위한 메소드
// ==================================================
void CAbilitySystemComponent::Add_AttributeSet(IAttributeSet* pAttributeSet)
{
    if (nullptr == m_pAttributeSet)
    {
        m_pAttributeSet = pAttributeSet;
        m_pAttributeSet->Init_AttributeSet(this);
    }
}

IGameplayAbility* CAbilitySystemComponent::Find_Ability(string strAbilityKey)
{
    auto it = m_ActivatableAbilities.find(strAbilityKey);
    if (it == m_ActivatableAbilities.end())
        return nullptr;
    return it->second;
}

IGameplayAbility* CAbilitySystemComponent::Find_ActivatingAbility(string strAbilityKey)
{
    auto it = m_ActivatingAbilites.find(strAbilityKey);
    if (it == m_ActivatingAbilites.end())
        return nullptr;
    return it->second;
}

IGameplayAbility* CAbilitySystemComponent::Find_InputBindingAbilites(_int iInputID)
{
    auto it = m_InputBindingAbilites.find(iInputID);

    if (it != m_InputBindingAbilites.end())
        return ((*it).second);
    return nullptr;
}

// ==================================================
// Give_Ability에서 어빌리티의 델리게이트에 이 함수를 매핑시켰음
// 어빌리티의 특정 이벤트시 처리 로직 수행
// ==================================================
void CAbilitySystemComponent::OnAbilityEventReceived(IGameplayAbility* pAbility, _uint iEventType)
{
    switch (static_cast<IGameplayAbility::Delegate_EventType>(iEventType))
    {
    case Engine::IGameplayAbility::TYPE_ACTIVATE:
        break;
    case Engine::IGameplayAbility::TYPE_CREATE_TASK:
        break;
    case Engine::IGameplayAbility::TYPE_CANCEL:
        break;
    case Engine::IGameplayAbility::TYPE_END_ABILITY:
        Deactivate_Ability(pAbility);
        break;
    case Engine::IGameplayAbility::TYPE_END:
        break;
    default:
        break;
    }
}

// ==================================================
// 어빌리티 종료시 호출
// ==================================================
void CAbilitySystemComponent::Deactivate_Ability(IGameplayAbility* pAbility)
{
    if (pAbility && Find_ActivatingAbility(pAbility->m_strAbilityName))
    {
        m_OwnedTags.RemoveTags(pAbility->m_ActivationOwnedTags);
        m_ActivatingAbilites.erase(pAbility->m_strAbilityName);
        m_OnTagRemovedDelegate.Broadcast(pAbility->m_ActivationOwnedTags);
    }
}

// ==================================================
// 어빌리티가 발동을 시작할 때 이 어빌리티가 발동가능한지 체킹을 수행하는 핵심 함수
// GameplayTagContainer의 함수를 통해 어빌리티의 취소, 차단, 요구 태그들을 검출해 최종적으로 수행가능한지 체킹 
// 어빌리티가 수행가능하면 어빌리티가 가지고 시작할 태그, 취소시킬 다른 어빌리티 등을 검출해서 취소시키는 기능도 포함
// ==================================================
_bool CAbilitySystemComponent::CheckingActivateCondition(IGameplayAbility* pAbility)
{
    if (nullptr == pAbility)
        return false;

    if (pAbility->IsActivate())
        return false;

    if (!pAbility->CanActivate_Ability(&m_ActorInfo))
        return false;

    GameplayTagContainer AbilityTags = pAbility->m_AbilityTags;

    // 예외처리 : 추가하려는 어빌리티가 자기 자신을 블로킹하는 조건에 있다면
    if (AbilityTags.HasAny(pAbility->m_BlockAbilitiesWithTag))
        return false;

    AbilityTags.AddTags(pAbility->m_ActivationOwnedTags);

    // 발동에 필요한 태그 조건을 검사한다
    if (!pAbility->m_ActivationRequiredTags.IsEmpty())
    {
        if (!m_OwnedTags.HasAll(pAbility->m_ActivationRequiredTags))
            return false;
    }

    // 발동을 막는 태그 조건을 검사한다
    if (!pAbility->m_ActivationBlockedTags.IsEmpty())
    {
        if (m_OwnedTags.HasAny(pAbility->m_ActivationBlockedTags))
            return false;
    }

    // 발동을 차단하는 태그 조건을 검사한다
    for (auto& Pair : m_ActivatingAbilites)
    {
        if (!Pair.second->m_BlockAbilitiesWithTag.IsEmpty() && AbilityTags.HasAny(Pair.second->m_BlockAbilitiesWithTag))
            return false;
    }

    // 발동할 때 소유할 태그를 추가
    m_OwnedTags.AddTags(pAbility->m_ActivationOwnedTags);
    m_OnTagAddedDelegate.Broadcast(pAbility->m_ActivationOwnedTags);

    // 추가하려는 어빌리티가 기존에 추가된 어빌리티중에서 Cancel 태그와 일치하면 제거한다
    vector<string> CancelAbilityTagNames;
    for (auto& Pair : m_ActivatingAbilites)
    {
        if (Pair.second->m_AbilityTags.HasAny(pAbility->m_CancelAbilitiesWithTags))
            CancelAbilityTagNames.push_back(Pair.first);
    }

    for (auto& str : CancelAbilityTagNames)
        m_ActivatableAbilities[str]->PreCancel_Ability(&m_ActorInfo);

    m_ActivatingAbilites.emplace(pAbility->m_strAbilityName, pAbility);

    return true;
}

// ==================================================
// ASC 생성 인터페이스
// ==================================================
CAbilitySystemComponent* CAbilitySystemComponent::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CAbilitySystemComponent* pInstance = new CAbilitySystemComponent(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CAbilitySystemComponent");
        Safe_Release(pInstance);
    }

    return pInstance;
}

// ==================================================
// ASC 복제 인터페이스
// ==================================================
CComponent* CAbilitySystemComponent::Clone(void* pArg)
{
    CAbilitySystemComponent* pInstance = new CAbilitySystemComponent(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Cloned : CAbilitySystemComponent");
        Safe_Release(pInstance);
    }

    return pInstance;
}

// ==================================================
// 객체의 delete전에 호출되서 소멸관련 작업 수행
// ==================================================
void CAbilitySystemComponent::Free()
{
    __super::Free();

    for (auto& [Key, pAbility] : m_ActivatableAbilities)
        Safe_Release(pAbility);
    m_ActivatableAbilities.clear();
}