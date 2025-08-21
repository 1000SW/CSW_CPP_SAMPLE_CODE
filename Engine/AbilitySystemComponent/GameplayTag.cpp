#include "GameplayTag.h"
#include <sstream>

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

bool IGameplayTagAssetInterface::Has_MatchingGameplayTag(GameplayTag TagToCheck) const
{
    GameplayTagContainer OwnedTags;
    Get_OwnedGameplayTags(OwnedTags);

    return OwnedTags.HasTag(TagToCheck);
}

bool IGameplayTagAssetInterface::Has_AllMatchingGameplayTags(const GameplayTagContainer& TagContainer) const
{
    GameplayTagContainer OwnedTags;
    Get_OwnedGameplayTags(OwnedTags);

    return OwnedTags.HasAll(TagContainer);
}

bool IGameplayTagAssetInterface::Has_AnyMatchingGameplayTags(const GameplayTagContainer& TagContainer) const
{
    GameplayTagContainer OwnedTags;
    Get_OwnedGameplayTags(OwnedTags);

    return OwnedTags.HasAny(TagContainer);
}

