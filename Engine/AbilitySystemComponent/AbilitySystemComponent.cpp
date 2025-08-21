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

CAbilitySystemComponent::CAbilitySystemComponent(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CComponent(pDevice, pContext)
{
}

CAbilitySystemComponent::CAbilitySystemComponent(const CAbilitySystemComponent& Prototype)
    : CComponent(Prototype)
{
}

HRESULT CAbilitySystemComponent::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CAbilitySystemComponent::Initialize(void* pArg)
{
    return S_OK;
}

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

void CAbilitySystemComponent::Update_AbilitySystem(_float fTimeDelta)
{
    for (auto& Pair : m_ActivatingAbilites)
    {
        for (auto& pTask : Pair.second->m_ActivatingTasks)
        {
            if (pTask->m_bTickingTask && pTask->m_eTaskState == IGameplayTask::TYPE_ACTIVE)
                pTask->Tick_Task(fTimeDelta);
        }

        //auto& TaskList = Pair.second->m_ActivatingTasks;
        //for (size_t i = 0; i < TaskList.size(); ++i)
        //{
        //    auto pTask = TaskList[i];
        //    if (pTask == nullptr)
        //        continue;

        //    if (pTask->m_bTickingTask)
        //        pTask->Tick_Task(fTimeDelta);
        //}
    }
} 
_bool CAbilitySystemComponent::Give_Ability(_string strAbilityTag, IGameplayAbility* pAbility)
{
    if (Find_Ability(strAbilityTag))
        return false;

    pAbility->m_pASC = this;
    pAbility->m_AbilityDelegate.AddDynamic("ASC", this, &CAbilitySystemComponent::OnAbilityEventReceived);
    pAbility->m_strAbilityName = strAbilityTag;
    pAbility->m_ActorInfo = m_ActorInfo;

    pAbility->Late_Initialize();
    if (pAbility->m_iInputActionID != -1 && !Find_InputBindingAbilites(pAbility->m_iInputActionID))
    {
        m_InputBindingAbilites.emplace(pAbility->m_iInputActionID, pAbility);
    }

    m_ActivatableAbilities.emplace(strAbilityTag, pAbility);


    return true;
}

void CAbilitySystemComponent::Clear_Ability(string strAbilityKey)
{
    if (IGameplayAbility* pAbility = Find_Ability(strAbilityKey))
    {
        if (pAbility)
            pAbility->End_Ability(&m_ActorInfo, true);
        m_ActivatableAbilities.erase(strAbilityKey);
    }
}

void CAbilitySystemComponent::Clear_AllAbilities()
{
    m_ActivatableAbilities.clear();
}

_uint CAbilitySystemComponent::HandleGameplayEvent(GameplayTag EventTag, const GameplayEventData* Payload)
{
    if (m_ActivatingAbilites.empty() || false == EventTag.IsValid())
        return 0;

    _uint iRet = 0;

    for (auto& [key, pAbility] : m_ActivatingAbilites)
    //for (auto& Pair : m_ActivatingAbilites)
    {
        for (auto& TriggerData : pAbility->m_AbilityTriggers)
        {
            // 현재로서는 태그가 정확히 일치하는 케이스만 True로 봄 => 나중에 여유되면 ParentTag까지 지원하는 느낌으로 가면 좋을 것 같다
            if (TriggerData.TriggerTag == EventTag && TriggerData.TriggerSource == IGameplayAbility::GameplayEvent)
            {
                iRet++;
                GameplayEventData TempPayLoad = *Payload;
                m_pGameInstance->Add_Timer("CAbilitySystemComponent", 0.f, [EventTag, TempPayLoad, pAbility]() {pAbility->OnGameplayEvent(EventTag, &TempPayLoad); });
                break;
            }
        }
    }

    return iRet;
}

_bool CAbilitySystemComponent::TryActivate_Ability(_string strAbilityTag)
{
    IGameplayAbility* pAbility = Find_Ability(strAbilityTag);
    if (nullptr == pAbility)
        return false;

    return TryActivate_Ability(pAbility);
}

_bool CAbilitySystemComponent::TryActivate_Ability(IGameplayAbility* pAbility)
{
    if (false == CheckingActivateCondition(pAbility))
        return false;

    pAbility->PreActivate_Ability(&m_ActorInfo);

    return true;
}

_bool CAbilitySystemComponent::TryActivate_AbilitiesByTag(_string strAbilityTag)
{
    return false;
}

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

void CAbilitySystemComponent::AbilityLocalInputReleased(_int iInputID)
{
    IGameplayAbility* pAbility = Find_InputBindingAbilites(iInputID);
    if (nullptr == pAbility || !pAbility->IsActivate())
        return;

    pAbility->InputReleased(&m_ActorInfo);
}

void CAbilitySystemComponent::Cancel_Ability(_string strAbilityTag)
{
    IGameplayAbility* pAbility = Find_ActivatingAbility(strAbilityTag);
    if (nullptr == pAbility || !pAbility->IsActivate())
        return;

    pAbility->Cancel_Ability(&m_ActorInfo);
}

void CAbilitySystemComponent::Cancel_AllAbilities(_string strIgnoreTag)
{
    IGameplayAbility* pAbility = Find_Ability(strIgnoreTag);

    // 복사본 생성
    auto AbilitiesCopy = m_ActivatingAbilites;

    // 원본에서 제외
    m_ActivatingAbilites.erase(strIgnoreTag);

    // 복사본 순회
    for (auto& Pair : AbilitiesCopy)
    {
        if (Pair.first != strIgnoreTag && Pair.second)
            Pair.second->Cancel_Ability(&m_ActorInfo);
    }

    //m_ActivatingAbilites.clear();
    
    // 완전히 정리하고 예외 능력만 복원
    if (pAbility)
        m_ActivatingAbilites.emplace(strIgnoreTag, pAbility);
}

void CAbilitySystemComponent::Cancel_AllAbilities(const vector<string>& IgnoreTags)
{
    // 복사본 생성
    auto AbilitiesCopy = m_ActivatingAbilites;

    // 나중에 복원할 예외 능력 보관
    unordered_map<string, IGameplayAbility*> IgnoredAbilities;

    for (auto& Pair : AbilitiesCopy)
    {
        const string& Tag = Pair.first;
        IGameplayAbility* pAbility = Pair.second;

        // 무시할 태그 목록에 포함되어 있으면 건너뜀
        if (find(IgnoreTags.begin(), IgnoreTags.end(), Tag) != IgnoreTags.end())
        {
            IgnoredAbilities.emplace(Tag, pAbility);
            continue;
        }

        // 그렇지 않으면 취소
        if (pAbility)
            pAbility->Cancel_Ability(&m_ActorInfo);
    }

    m_ActivatingAbilites.clear();

    for (auto& Pair : IgnoredAbilities)
        m_ActivatingAbilites.emplace(Pair.first, Pair.second);
}

float CAbilitySystemComponent::GetNumericAttributeBase(const string& strKey, bool& bFound) const
{
    if (nullptr == m_pAttributeSet)
    {
        bFound = false;
        return 0.f;
    }

    GameplayAttribute* pAttribute = m_pAttributeSet->Get_Attribute(strKey);
    if (nullptr == pAttribute)
    {
        bFound = false;
        return 0.f;
    }

    bFound = true;
    return m_pAttributeSet->Get_Attribute(strKey)->GetBaseValue();
}

_bool CAbilitySystemComponent::SetNumericAttributeBase(const string& strKey, float NewBaseValue)
{
    if (nullptr == m_pAttributeSet)
        return false;

    GameplayAttribute* pAttribute = m_pAttributeSet->Get_Attribute(strKey);
    if (nullptr == pAttribute)
        return false;

    _float fOldValue, fNewValue;

    // 변경전 값 저장
    fOldValue = pAttribute->GetBaseValue();

    m_pAttributeSet->PreAttributeBaseChange(strKey, NewBaseValue);
    pAttribute->SetBaseValue(NewBaseValue);

    // 변경 후 값 가져오기
    fNewValue = pAttribute->GetBaseValue();

    m_pAttributeSet->PostAttributeBaseChange(strKey, fOldValue, fNewValue);

    return true;
}

float CAbilitySystemComponent::GetGameplayAttributeValue(const string& strKey, bool& bFound) const
{
    if (nullptr == m_pAttributeSet)
    {
        bFound = false;
        return 0.f;
    }

    GameplayAttribute* pAttribute = m_pAttributeSet->Get_Attribute(strKey);
    if (nullptr == pAttribute)
    {
        bFound = false;
        return 0.f;
    }

    bFound = true;

    // 원래는 CurrentValue이나 모디파이어 적용하기는 힘들어서 오로지 BaseValue로만 사용하겠음
    return pAttribute->GetBaseValue();
}

_bool CAbilitySystemComponent::ApplyGameplayEffectSpecToSelf(IGameplayEffect* pGameplayEffect)
{
    GameplayEffectDurationType eType = pGameplayEffect->m_eDurationType;
    switch (eType)
    {
        //Todo : 모티파이어 로직에 맞는 어트리뷰트 수정 필요함
    case Engine::GameplayEffectDurationType::Instant:
        for (auto& Modifer : pGameplayEffect->m_Modifers)
        {
        }
        break;
        //Todo : Period에 따른 이펙트 적용 로직 구현하기
    case Engine::GameplayEffectDurationType::Infinite:
        break;
        //위와 마찬가지임
    case Engine::GameplayEffectDurationType::HasDuration:
        break;
    default:
        break;
    }
  
    return false;
}

_bool CAbilitySystemComponent::RemoveActiveGameplayEffect(IGameplayEffect* pGameplayEffect)
{
    return _bool();
}

void CAbilitySystemComponent::Get_OwnedGameplayTags(GameplayTagContainer& TagContainer) const
{
    TagContainer = m_OwnedTags;
}

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

void CAbilitySystemComponent::Deactivate_Ability(IGameplayAbility* pAbility)
{
    if (pAbility && Find_ActivatingAbility(pAbility->m_strAbilityName))
    {
        m_OwnedTags.RemoveTags(pAbility->m_ActivationOwnedTags);
        m_ActivatingAbilites.erase(pAbility->m_strAbilityName);
        m_OnTagRemovedDelegate.Broadcast(pAbility->m_ActivationOwnedTags);

        //cout << "Deactivate_Ability" << endl;
    }
}

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

    // 추가하려는 어빌리티가 기존에 추가된 어빌리티중에서 Cancel/Block 태그가 일치하면 제거한다
    vector<string> CancelAbilityTagNames;
    for (auto& Pair : m_ActivatingAbilites)
    {
        if (Pair.second->m_AbilityTags.HasAny(pAbility->m_CancelAbilitiesWithTags))
            CancelAbilityTagNames.push_back(Pair.first);
        /*else if (Pair.second->m_AbilityTags.HasAny(pAbility->m_BlockAbilitiesWithTag))
            CancelAbilityTagNames.push_back(Pair.first);*/
    }

    for (auto& str : CancelAbilityTagNames)
        m_ActivatableAbilities[str]->PreCancel_Ability(&m_ActorInfo);

    m_ActivatingAbilites.emplace(pAbility->m_strAbilityName, pAbility);

    return true;
}

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

void CAbilitySystemComponent::Free()
{
    __super::Free();

    for (auto& [Key, pAbility] : m_ActivatableAbilities)
        Safe_Release(pAbility);
    m_ActivatableAbilities.clear();
}