#include "GuiLayer_GameplayAbilitySystem.h"
#include "GuiLayer_Armature.h"
#include "GameInstance.h"
#include "ImGui_Manager.h"
#include "GameObject.h"
#include "Armature.h"
#include "Animator.h"
#include "AnimationClip.h"
#include "CompositeArmatrue.h"
#include "Layer.h"
#include "ImGui/implot.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbility.h"
#include "AbilityTask.h"

CGuiLayer_GameplayAbilitySystem::CGuiLayer_GameplayAbilitySystem()
{
}

HRESULT CGuiLayer_GameplayAbilitySystem::Initialize()
{
    return S_OK;
}

void CGuiLayer_GameplayAbilitySystem::On_GuiRender(CImGui_Manager* pManager)
{
    if (nullptr == m_pASC)
    {
        CGameObject* pObj = pObj = pManager->Get_Selected();
        if (nullptr == pObj)
            return;

        CAbilitySystemComponent* pASC = pObj->Get_Component<CAbilitySystemComponent>();
        if (nullptr == pASC)
            return;

        m_pASC = pASC;
    }
    else
    {
        CGameObject* pObj = pObj = pManager->Get_Selected();
        if (pObj)
        {
            CAbilitySystemComponent* pASC = pObj->Get_Component<CAbilitySystemComponent>();
            if (pASC)
            {
                m_pASC = pASC;
            }
        }
    }
    

    ImGui::Begin("GAS Tool");

    ImGui::SeparatorText("OwnedTags");

    Show_GameplayTagContainer(pManager, m_pASC, m_pASC->m_OwnedTags);

    ImGui::SeparatorText("Activating Abilities");


    int i = 0;
    for (auto& Pair : m_pASC->m_ActivatingAbilites)
    {
        i++;
        ImGui::PushID(i);
        if (ImGui::TreeNode(Pair.first.c_str()))
        {
            DealWithAbility(pManager, m_pASC, Pair.second);

            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    ImGui::SeparatorText("Activatable Abilities");

    _uint iUniqueID = 0;
    for (auto& Pair : m_pASC->m_ActivatableAbilities)
    {
        i++;
        ImGui::PushID(i);
        if (ImGui::TreeNode(Pair.first.c_str()))
        {
            DealWithAbility(pManager, m_pASC, Pair.second);

            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    ImGui::SeparatorText("Attribute Sets");

    for (auto& Pair : m_pASC->m_pAttributeSet->m_Attributes)
    {
        ImGui::Text("%s : BaseValue : %f", Pair.first.c_str(), Pair.second.fBaseValue);
    }

	ImGui::End();
}

void CGuiLayer_GameplayAbilitySystem::Show_GameplayTagContainer(CImGui_Manager* pManager, CAbilitySystemComponent* pASC, GameplayTagContainer& Container)
{
    for (auto& Tag : Container.m_GameplayTags)
    {
        if (Tag.m_strTag.size()/* && ImGui::TreeNode(Tag.m_strTag.c_str())*/)
        {
            ImGui::Text(Tag.m_strTag.c_str());
            /*for (auto& ParentTag : Container.m_ParentTags)
            {
                ImGui::Text(ParentTag.m_strTag.c_str());
            }

            ImGui::TreePop();*/
        }
    }
}

void CGuiLayer_GameplayAbilitySystem::DealWithAbility(CImGui_Manager* pManager, CAbilitySystemComponent* pASC, IGameplayAbility* pAbility)
{
    // Todo
    // 이름띄우고, 태그 목록들 띄워주기
    // 활성화된 어빌리티들 클래스 이름 정도 띄워주기.

    ImGui::Text(pAbility->m_strAbilityName.c_str());

    ImGui::SeparatorText("AbilityTags");
    Show_GameplayTagContainer(pManager, pASC, pAbility->m_AbilityTags);
    ImGui::SeparatorText("ActivationOwnedTags");
    Show_GameplayTagContainer(pManager, pASC, pAbility->m_ActivationOwnedTags);
    ImGui::SeparatorText("CancelAbilitiesTags");
    Show_GameplayTagContainer(pManager, pASC, pAbility->m_CancelAbilitiesWithTags);
    ImGui::SeparatorText("BlockAbilitiesTags");
    Show_GameplayTagContainer(pManager, pASC, pAbility->m_BlockAbilitiesWithTag);
    ImGui::SeparatorText("ActivationRequiredTags");
    Show_GameplayTagContainer(pManager, pASC, pAbility->m_ActivationRequiredTags);
    ImGui::SeparatorText("ActivationBlockedTags");
    Show_GameplayTagContainer(pManager, pASC, pAbility->m_ActivationBlockedTags);

    ImGui::SeparatorText("Activating Tasks");
    for (auto& pTask : pAbility->m_ActivatingTasks)
        ImGui::Text("Class Name : %s", typeid(*pTask).name());

    ImGui::SeparatorText("Owning Tasks");
    for (auto& pTask : pAbility->m_OwningTasks)
        ImGui::Text("Class Name : %s", typeid(*pTask).name());
}

CGuiLayer_GameplayAbilitySystem* CGuiLayer_GameplayAbilitySystem::Create()
{
	CGuiLayer_GameplayAbilitySystem* pInstance = new CGuiLayer_GameplayAbilitySystem();

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CGuiLayer_GameplayAbilitySystem");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CGuiLayer_GameplayAbilitySystem::Free()
{
    __super::Free();
}
