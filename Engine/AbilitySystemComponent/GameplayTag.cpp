#include "GameplayTag.h"
#include <sstream>

// ======================
// 게임플레이 태그 컨테이너 관련 함수
// 내부적으로 게임 플레이 태그라는 구조체를 모아놓는 구조체
// m_GameplayTags는 std::set으로 구현되어 중복된 tag를 추가하는 것을 방지
// 명시적 태그외에 부모 태그도 같이 추가
// ======================
_bool GameplayTagContainer::AddTag(GameplayTag Tag)
{
    m_GameplayTags.insert(Tag);

    Add_ParentTags(Tag);

    return true;
}

void GameplayTagContainer::AddTags(const GameplayTagContainer& TagsToAppend)
{
    for (auto& Tag : TagsToAppend.m_GameplayTags)
        AddTag(Tag);
}

bool GameplayTagContainer::RemoveTag(const GameplayTag& TagToRemove)
{
    auto it = m_GameplayTags.find(TagToRemove);
    if (it != m_GameplayTags.end())
    {
        m_GameplayTags.erase(it);
        FillParentTags();
        return true;
    }

    return false;
}


void GameplayTagContainer::RemoveTags(const GameplayTagContainer& TagsToRemove)
{
    for (auto Tag : TagsToRemove.m_GameplayTags)
    {
        RemoveTag(Tag);
    }
}

// ============================
// 특정 태그가 삭제된 뒤 불려지는 함수, 부모 태그를 다시 만듦
// ============================
void GameplayTagContainer::FillParentTags()
{
    m_ParentTags.clear();

    if (m_GameplayTags.size())
    {
        for (auto& Tag : m_GameplayTags)
        {
            Add_ParentTags(Tag);
        }
    }
}

// ============================
// 주어진 명시적 태그에 기반해서 부모 태그를 만들고 부모 태그를 모아놓은 자료구조에 담는 함수
// ============================
void GameplayTagContainer::Add_ParentTags(const GameplayTag& Tag)
{
    const string& FullTag = Tag.m_strTag;

    stringstream ss(FullTag);
    string item;
    string currentPath;

    while (getline(ss, item, '.'))
    {
        if (!item.empty())
        {
            if (!currentPath.empty())
                currentPath += ".";
            currentPath += item;

            m_ParentTags.insert(GameplayTag(currentPath));
        }
    }
}

// ============================
// IGameplayTagAssetInterface 인터페이스 함수 관련
// 현재 가지고있는 GameplayTagContainer에서 주어진 인자와 매칭되는 GameplayTag가 있는지 검사
// ============================
bool IGameplayTagAssetInterface::Has_MatchingGameplayTag(GameplayTag TagToCheck) const
{
    GameplayTagContainer OwnedTags;
    Get_OwnedGameplayTags(OwnedTags);

    return OwnedTags.HasTag(TagToCheck);
}

// ============================
// 인자로 주어진 게임플레이 컨테이너가 내부적인 컨테이너와 모두 매핑되는지 체킹
// ============================
bool IGameplayTagAssetInterface::Has_AllMatchingGameplayTags(const GameplayTagContainer& TagContainer) const
{
    GameplayTagContainer OwnedTags;
    Get_OwnedGameplayTags(OwnedTags);

    return OwnedTags.HasAll(TagContainer);
}

// ============================
// 인자로 주어진 게임플레이 컨테이너가 내부적인 컨테이너와 하나라도 매핑되는지 체킹
// ============================
bool IGameplayTagAssetInterface::Has_AnyMatchingGameplayTags(const GameplayTagContainer& TagContainer) const
{
    GameplayTagContainer OwnedTags;
    Get_OwnedGameplayTags(OwnedTags);

    return OwnedTags.HasAny(TagContainer);
}

