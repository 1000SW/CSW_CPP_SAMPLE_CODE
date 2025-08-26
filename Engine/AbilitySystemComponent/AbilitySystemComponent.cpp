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
// ������ / ���� ������
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
// �ʱ�ȭ ���� �޼���
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
// Ability Actor ���� �ʱ�ȭ
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
// Update ����: AbilityTask�� Tick �Լ� ȣ��
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
// �����Ƽ �ο�: ex)����, ���� ���
// ==================================================
_bool CAbilitySystemComponent::Give_Ability(_string strAbilityTag, IGameplayAbility* pAbility)
{
    if (Find_Ability(strAbilityTag))
        return false;

    pAbility->m_pASC = this;
    // �����Ƽ�� �̺�Ʈ(����, ���� ���) �߻����� �� �����ϱ� ���� ��������Ʈ ���ε�
    pAbility->m_AbilityDelegate.AddDynamic("ASC", this, &CAbilitySystemComponent::OnAbilityEventReceived);
    pAbility->m_strAbilityName = strAbilityTag;
    pAbility->m_ActorInfo = m_ActorInfo;

    pAbility->Late_Initialize();
    // ���ε��� ��ǲ �׼� ID�� ���� �� ��ǲ ���ε� �����Ƽ �ڷᱸ���� �߰��ؼ� Ű �Է����� Ʈ���ŵǴ� �����Ƽ ���� ����
    if (pAbility->m_iInputActionID != -1 && !Find_InputBindingAbilites(pAbility->m_iInputActionID))
    {
        m_InputBindingAbilites.emplace(pAbility->m_iInputActionID, pAbility);
    }

    m_ActivatableAbilities.emplace(strAbilityTag, pAbility);

    return true;
}

// ==================================================
// �����Ƽ ����ε� �� ���� ����
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
// Ȱ��ȭ�ǰ� �ִ� �����Ƽ�� �̺�Ʈ �Լ��� ȣ���ϱ� ���� �Լ�. (���� ������ ���� �ִϸ��̼� ���� ���̸��� �������� �����ؾ��� �� �����)
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
// �±׸� ���� �����Ƽ Ʈ����
// ==================================================
_bool CAbilitySystemComponent::TryActivate_Ability(string strAbilityTag)
{
    IGameplayAbility* pAbility = Find_Ability(strAbilityTag);
    if (nullptr == pAbility)
        return false;

    return TryActivate_Ability(pAbility);
}

// ==================================================
// �����͸� ���� �����Ƽ Ʈ����
// ==================================================
_bool CAbilitySystemComponent::TryActivate_Ability(IGameplayAbility* pAbility)
{
    if (false == CheckingActivateCondition(pAbility))
        return false;

    pAbility->PreActivate_Ability(&m_ActorInfo);

    return true;
}

// ==================================================
// �����÷��� �̺�Ʈ �����̳ʸ� ���� �����Ƽ Ʈ���� �Լ�
// �����Ƽ�� �ߵ��� �� �ֺ� ȯ�濡 ���� ������ �ʿ��� �� ���
// ex) ��ٸ� Ÿ��, ���̾� �߻� ���
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
// ��ǲ Ű�� ���� �����Ƽ Ʈ����
// �����Ƽ�� �ߵ���Ű�ų� �̹� �ߵ����̶�� InputPressed�Լ� ȣ��
// ��� ����) �ѱ� �߻�, QTE �̺�Ʈ ���
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
// ��ǲ Ű�� ���� �����Ƽ �̺�Ʈ ���� 
// �����Ƽ�� InputReleased �̺�Ʈ ����
// ��� ����) ���� �����Ƽ�� ���콺 ��Ŭ�� ���� �� ���� �����Ƽ ���� ����, ������ Ȧ�� ���� ���
// ==================================================
void CAbilitySystemComponent::AbilityLocalInputReleased(_int iInputID)
{
    IGameplayAbility* pAbility = Find_InputBindingAbilites(iInputID);
    if (nullptr == pAbility || !pAbility->IsActivate())
        return;

    pAbility->InputReleased(&m_ActorInfo);
}

// ==================================================
// Ȱ��ȭ�� �����Ƽ ���
// ==================================================
void CAbilitySystemComponent::Cancel_Ability(_string strAbilityTag)
{
    IGameplayAbility* pAbility = Find_ActivatingAbility(strAbilityTag);
    if (nullptr == pAbility || !pAbility->IsActivate())
        return;

    pAbility->Cancel_Ability(&m_ActorInfo);
}

// ==================================================
// ��Ʈ����Ʈ ��Ʈ ���� �Լ�
// Ű�� ���� ��Ʈ����Ʈ�� ã�� ��Ʈ����Ʈ�� Base�� ����
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
// ��Ʈ����Ʈ�� Base�����͸� Set���ְ� ������ �ޱ� ��(PreAttributeBaseChange), ������ ���� ��(PostAttributeBaseChange)�� ���� �̺�Ʈ �Լ� ȣ��
// Ŭ���̾�Ʈ���� ���� �� �Լ� ���ٶ��̵��� ���� UI �̺�Ʈ�� ����Ʈ ���� ���ε��ؼ� �����
// ==================================================
_bool CAbilitySystemComponent::SetNumericAttributeBase(const string& strAttributeKey, float NewBaseValue)
{
    if (nullptr == m_pAttributeSet)
        return false;

    GameplayAttribute* pAttribute = m_pAttributeSet->Get_Attribute(strAttributeKey);
    if (nullptr == pAttribute)
        return false;

    _float fOldValue, fNewValue;

    // ������ �� ����
    fOldValue = pAttribute->GetBaseValue();

    m_pAttributeSet->PreAttributeBaseChange(strAttributeKey, NewBaseValue);
    pAttribute->SetBaseValue(NewBaseValue);

    // ���� �� �� ��������
    fNewValue = pAttribute->GetBaseValue();

    m_pAttributeSet->PostAttributeBaseChange(strAttributeKey, fOldValue, fNewValue);

    return true;
}

// ==================================================
// ��Ʈ����Ʈ CurrentValue ����
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

    // ������ CurrentValue�� �����ؾ� ������ �ð��� �Ѱ�� ���� �ý����� �����ϱ� ����� BaseValue���� �������
    return pAttribute->GetBaseValue();
}

// ==================================================
// IGameplayTagAssetInterface �������̽� �������̵�
// �� �Լ��� �������̽��� �����Ǿ��ִ� Has_MatchingGameplayTag���� �Լ��� ���� ���� ASC�� ����ִ� ���¿� �±׸� üŷ �� ������ ����
// ex) �÷��̾ ��ũ���� �ִ� �������� üŷ, �÷��̾ ���� ��ȣ�ۿ� �������� ���� ���
// ==================================================
void CAbilitySystemComponent::Get_OwnedGameplayTags(GameplayTagContainer& TagContainer) const
{
    TagContainer = m_OwnedTags;
}

// ==================================================
// ��������� �±� �߰�/���� ����
// �±� ����/���� ��������Ʈ�� �ߵ����� Ŭ���̾�Ʈ���� �±� ���濡 ���� �̺�Ʈ�� ���Ź��� �� �ְ� ����
// ��� ����) Ʈ���ſ��� ������ ���� �÷��̾�� ��ٸ� Ÿ�⳪ ���� �پ���� �� �ִ� ���� �ο� �� ���� ����
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
// ��Ʈ����Ʈ ��Ʈ(��Ʈ����Ʈ�� �����ϴ� Ŭ����) �߰��ϱ� ���� �޼ҵ�
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
// Give_Ability���� �����Ƽ�� ��������Ʈ�� �� �Լ��� ���ν�����
// �����Ƽ�� Ư�� �̺�Ʈ�� ó�� ���� ����
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
// �����Ƽ ����� ȣ��
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
// �����Ƽ�� �ߵ��� ������ �� �� �����Ƽ�� �ߵ��������� üŷ�� �����ϴ� �ٽ� �Լ�
// GameplayTagContainer�� �Լ��� ���� �����Ƽ�� ���, ����, �䱸 �±׵��� ������ ���������� ���డ������ üŷ 
// �����Ƽ�� ���డ���ϸ� �����Ƽ�� ������ ������ �±�, ��ҽ�ų �ٸ� �����Ƽ ���� �����ؼ� ��ҽ�Ű�� ��ɵ� ����
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

    // ����ó�� : �߰��Ϸ��� �����Ƽ�� �ڱ� �ڽ��� ���ŷ�ϴ� ���ǿ� �ִٸ�
    if (AbilityTags.HasAny(pAbility->m_BlockAbilitiesWithTag))
        return false;

    AbilityTags.AddTags(pAbility->m_ActivationOwnedTags);

    // �ߵ��� �ʿ��� �±� ������ �˻��Ѵ�
    if (!pAbility->m_ActivationRequiredTags.IsEmpty())
    {
        if (!m_OwnedTags.HasAll(pAbility->m_ActivationRequiredTags))
            return false;
    }

    // �ߵ��� ���� �±� ������ �˻��Ѵ�
    if (!pAbility->m_ActivationBlockedTags.IsEmpty())
    {
        if (m_OwnedTags.HasAny(pAbility->m_ActivationBlockedTags))
            return false;
    }

    // �ߵ��� �����ϴ� �±� ������ �˻��Ѵ�
    for (auto& Pair : m_ActivatingAbilites)
    {
        if (!Pair.second->m_BlockAbilitiesWithTag.IsEmpty() && AbilityTags.HasAny(Pair.second->m_BlockAbilitiesWithTag))
            return false;
    }

    // �ߵ��� �� ������ �±׸� �߰�
    m_OwnedTags.AddTags(pAbility->m_ActivationOwnedTags);
    m_OnTagAddedDelegate.Broadcast(pAbility->m_ActivationOwnedTags);

    // �߰��Ϸ��� �����Ƽ�� ������ �߰��� �����Ƽ�߿��� Cancel �±׿� ��ġ�ϸ� �����Ѵ�
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
// ASC ���� �������̽�
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
// ASC ���� �������̽�
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
// ��ü�� delete���� ȣ��Ǽ� �Ҹ���� �۾� ����
// ==================================================
void CAbilitySystemComponent::Free()
{
    __super::Free();

    for (auto& [Key, pAbility] : m_ActivatableAbilities)
        Safe_Release(pAbility);
    m_ActivatableAbilities.clear();
}