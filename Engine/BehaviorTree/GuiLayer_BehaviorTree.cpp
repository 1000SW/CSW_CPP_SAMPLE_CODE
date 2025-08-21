#include "GuiLayer_BehaviorTree.h"
#include "GameObject.h"
#include "ImGui_Manager.h"
#include "CompositeArmatrue.h"
#include "PointerWrapper.h"
#include "BehaviorTree.h"
#include "GameInstance.h"
#include "Blackboard.h"
#include <sstream> 
#include "BT_Task.h"

CGuiLayer_BehaviorTree::CGuiLayer_BehaviorTree()
    :CImGuiLayer()
{
}

HRESULT CGuiLayer_BehaviorTree::Initialize()
{
	///////////////////////////
	//BehaivorTree
	m_NodeClasses.push_back("Composite");
	m_NodeClasses.push_back("Decorator");
	m_NodeClasses.push_back("Task");
	m_NodeClasses.push_back("Service");

    /* For Composite */
	m_CompositeNames.push_back("Sequence");
	m_CompositeNames.push_back("Selector");

    /* For Task */
	m_NodeNames.push_back("CBT_Task_Wait");
	m_NodeNames.push_back("CBT_Task_PlayAnimation");
	m_NodeNames.push_back("CBT_Task_MoveTo");
	m_NodeNames.push_back("CBT_Task_FinishWithResult");
	m_NodeNames.push_back("CBT_Task_Activate_Ability");
	m_NodeNames.push_back("CBT_Task_SetBlackboardKey");
	m_NodeNames.push_back("CBT_Task_WaitUntilAnimFinished");

    /* Ashley Task */
    m_NodeNames.push_back("CBTTask_TurnToTarget");
    m_NodeNames.push_back("CBTTask_MoveToTarget");
	m_NodeNames.push_back("CBTTask_EnableTurn");
	m_NodeNames.push_back("CBTTask_Ashley_MoveClose");
	m_NodeNames.push_back("CBTTask_Ashley_MoveKeepDistance");
	m_NodeNames.push_back("CBTTask_SendGameplayEvent");
	m_NodeNames.push_back("CBTTask_MoveToGoalLocationByCrouching");
	m_NodeNames.push_back("CBTTask_ActivateAbilityLocalInputPressed");
	m_NodeNames.push_back("CBTTask_ImitatePlayerMove");
	m_NodeNames.push_back("CBTTask_WaitUntilReceiveGameplayEvent");
	m_NodeNames.push_back("CBTTask_ActivateScript");

    /* For Decorator */
    m_DecoratorNames.push_back("CDecorator_Blackboard");
    m_DecoratorNames.push_back("CDecorator_SetBlackboardRandomValue");
    m_DecoratorNames.push_back("CDecorator_CloseToTarget");
    m_DecoratorNames.push_back("CDecorator_TimeLimit");
    m_DecoratorNames.push_back("CDecorator_FarFromTarget");
    m_DecoratorNames.push_back("CDecorator_CoolDown");

	m_BlackboardQueryNames.push_back("IS_SET");
	m_BlackboardQueryNames.push_back("IS_NOT_SET");
	m_BlackboardQueryNames.push_back("EQUAL");
	m_BlackboardQueryNames.push_back("NOTEQUAL");
	m_BlackboardQueryNames.push_back("LESSEQUAL");
	m_BlackboardQueryNames.push_back("LESS");
	m_BlackboardQueryNames.push_back("GREATEREQUAL");
	m_BlackboardQueryNames.push_back("GREATER");

	m_TypeNames.push_back("TYPE_INT");
	m_TypeNames.push_back("TYPE_UINT");
	m_TypeNames.push_back("TYPE_FLOAT");
	m_TypeNames.push_back("TYPE_DOUBLE");
	m_TypeNames.push_back("TYPE_BOOL");
	m_TypeNames.push_back("TYPE_STRING");
	m_TypeNames.push_back("TYPE_WSTRING");
	m_TypeNames.push_back("TYPE_VOID_PTR");
	m_TypeNames.push_back("TYPE_GAMEOBJECT_PTR");
	m_TypeNames.push_back("TYPE_VECTOR");

    /* For Service */
    m_ServiceNames.push_back("CBTService_UpdateGoalLocation");
    m_ServiceNames.push_back("CBTService_CheckingConditionInfrontOfTarget");
    m_ServiceNames.push_back("CBTService_UpdateCrouchCondition");

    return S_OK;
}

void CGuiLayer_BehaviorTree::On_GuiRender(CImGui_Manager* pManager)
{
	if (CGameObject* pObj = pManager->Get_Selected())
	{
        CBehaviorTree* pTree = pObj->Get_Component<CBehaviorTree>();
		if (nullptr == pTree)
			return;

		ImGui::Begin("BehaviorTree Tool");
		ImGui::PushID(pTree);
		ImGui::Checkbox("Tree Active", &pTree->m_bActive);

        if (ImGui::Button("Reorder Tree"))
        {
            pTree->Reorder_Nodes();
        }

		ImGui::Text("Mouse MB(Click Node) : Open Edit Node Tool");

		if (ImGui::TreeNodeEx("RootNode", ImGuiTreeNodeFlags_DefaultOpen))
		{
            if (pTree->m_pRootNode == nullptr)
            {
                pTree->m_pRootNode = CBT_Composite::Create(nullptr, pTree);
            }

			CBT_Composite* pCompositeNode = dynamic_cast<CBT_Composite*>(pTree->m_pRootNode);
			if (pCompositeNode)
				DealWithCompositeNode(pCompositeNode);

			ImGui::TreePop();
		}

		ImGui::SeparatorText("Blackboard Keys");
		for (auto& Pair : pTree->m_pBlackBoard->m_Variables)
		{
			TYPES eType = pTree->m_pBlackBoard->m_VariableTypes[Pair.first];

			string strValue = Get_VariantTypeValue(eType, Pair.second);
            if (strValue.size())
    			ImGui::Text("%s : %s", Pair.first.c_str(), strValue.c_str());
		}

		pManager->Open_FileDialog("SaveBehviorTree", "BehaviorTree",
			[&](auto& filepath)
			{
				rapidjson::Document& dom = m_pGameInstance->Get_EmptyDocument("Com_BehaviorTree");
				pTree->Save_JsonFile(dom, dom.GetAllocator());

				string strPath = filepath;

                pTree->Reorder_Nodes();
				m_pGameInstance->Save_JsonFileWithBackup("Com_BehaviorTree", strPath.c_str());
			}
		);

        pManager->Open_FileDialog("LoadBehviorTree", "BehaviorTree",
            [&](auto& filepath)
            {
                pTree->Load_JsonFile(filepath, pObj->Get_ObjName());

                string strPath = filepath;
            }
        );

		ImGui::PopID();
		ImGui::End();
	}
}

HRESULT CGuiLayer_BehaviorTree::Set_FactoryMethod(IBT_Task::fnTaskFactoryMethod Method1, CBT_Decorator::fnDecoratorFactoryMethod Method2, CBT_Composite::fnCompositeFactoryMethod Method3,
    IBT_Service::fnServiceFactoryMethod ServiceMethod)
{
	if (!Method1 || !Method2 || !Method3 || !ServiceMethod)
		return E_FAIL;

	m_fnTaskFactoryMethod = Method1;
	m_fnDecoratorFactoryMethod = Method2;
	m_fnCompositeFactoryMethod = Method3;
    m_fnServiceFactoryMethod = ServiceMethod;

	return S_OK;
}

CGuiLayer_BehaviorTree* CGuiLayer_BehaviorTree::Create()
{
	CGuiLayer_BehaviorTree* pInstance = new CGuiLayer_BehaviorTree();

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CGuiLayer_BehaviorTree");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CGuiLayer_BehaviorTree::Free()
{
    __super::Free();
}

void CGuiLayer_BehaviorTree::Show_CompositeEditTool(CBT_Composite* pNode)
{
	if (ImGui::IsItemClicked(ImGuiMouseButton_Middle))
		ImGui::OpenPopup("Edit Composite");

	if (ImGui::BeginPopupModal("Edit Composite", nullptr))
	{
		ImGui::Text("Composite Name : %s", pNode->m_strNodeName.c_str());

        /* Showing Decorator List */
		vector<const char*> ListBoxDecorators;
		vector<string> DecoratorNames;
		for (auto& b : pNode->m_Decorators)
		{
			string s = typeid(*b).name();
			DecoratorNames.push_back(s.c_str());
			ListBoxDecorators.push_back(DecoratorNames.back().c_str());
		}

		ImGui::ListBox("Decorators", &m_iListBoxDecoratorsIndex, ListBoxDecorators.data(), ListBoxDecorators.size());

		if (m_iListBoxDecoratorsIndex != -1 && ImGui::Button("Delete Decorator"))
		{
			pNode->Delete_Decorator(m_iListBoxDecoratorsIndex);
			m_iListBoxDecoratorsIndex = -1;
		}

        /* Showing ChildNode Lists */
		vector<const char*> ListBoxChilds;
		vector<string> ChildNodes;
		for (auto& b : pNode->m_ChildNodes)
		{
			string s = typeid(*b).name() + to_string(b->m_iNodeIndex);
			ChildNodes.push_back(s);
			ListBoxChilds.push_back(ChildNodes.back().c_str());
		}

		ImGui::ListBox("ChildNodes", &m_iListBoxChildNodeIndex, ListBoxChilds.data(), ListBoxChilds.size());

		if (m_iListBoxChildNodeIndex != -1 && ImGui::Button("Delete Child"))
		{
			pNode->Delete_Child(m_iListBoxChildNodeIndex);
			m_iListBoxChildNodeIndex = -1;
		}

        /* Showing Service Lists */
        vector<const char*> ListBoxService;
        vector<string> ServiceNodes;
        for (auto& b : pNode->m_Services)
        {
            string s = typeid(*b).name() + to_string(b->m_iNodeIndex);
            ServiceNodes.push_back(s);
            ListBoxService.push_back(ServiceNodes.back().c_str());
        }

        ImGui::ListBox("ServiceNodes", &m_iListBoxServiceNodeIndex, ListBoxService.data(), ListBoxService.size());

        if (m_iListBoxServiceNodeIndex != -1 && ImGui::Button("Delete Service"))
        {
            pNode->Delete_Service(m_iListBoxServiceNodeIndex);
            m_iListBoxServiceNodeIndex = -1;
        }

		////////////////////////////////
		// 생성을 위한 영역
		ImGui::SeparatorText("Node Add Tool");

        // 0 컴포짓 1 데코레이터 2 태스크 3 서비스
		ImGui::Combo("NodeClassList", &m_iComboNodeClassIndex, m_NodeClasses.data(), m_NodeClasses.size());

        if (m_iComboNodeClassIndex == 0)
        {
            ImGui::Combo("Composite List", &m_iComboCompositeIndex, m_CompositeNames.data(), m_CompositeNames.size());
            ImGui::InputText("Composite Name", m_szCompositeName, 256);
        }
        else if (m_iComboNodeClassIndex == 1)
        {
            ImGui::Combo("Decorator List", &m_iComboDecoratorIndex, m_DecoratorNames.data(), m_DecoratorNames.size());
        }
        else if (m_iComboNodeClassIndex == 2)
        {
            ImGui::Combo("Task List", &m_iComboNodeIndex, m_NodeNames.data(), m_NodeNames.size());
        }
        else if (m_iComboNodeClassIndex == 3)
        {
            ImGui::Combo("Service List", &m_iComboServiceIndex, m_ServiceNames.data(), m_ServiceNames.size());
        }

        ImGui::Text("Node Insertion Mode:");
        ImGui::SameLine(); // 다음 요소를 같은 줄에 배치
        ImGui::RadioButton("Pushback", &m_iNodeInsertMode, 1);
        ImGui::SameLine(); // 다음 요소를 같은 줄에 배치
        ImGui::RadioButton("Insert", &m_iNodeInsertMode, 0);

        if (ImGui::Button("Create"))
        {
            // 시퀀스 추가
            if (m_iComboNodeClassIndex == 0)
            {
                if (m_iNodeInsertMode == 0)
                {
                    pNode->Insert_Node(m_iListBoxChildNodeIndex, CBT_Composite::Create(m_iComboCompositeIndex == 0 ? CBT_Composite::TYPE_SEQUENCE : CBT_Composite::TYPE_SELECTOR, m_szCompositeName));
                }
                else
                {
                    pNode->Add_Node(CBT_Composite::Create(m_iComboCompositeIndex == 0 ? CBT_Composite::TYPE_SEQUENCE : CBT_Composite::TYPE_SELECTOR, m_szCompositeName));
                }
            }
            else if (m_iComboNodeClassIndex == 1)
            {
                if (m_iNodeInsertMode == 0)
                    pNode->Insert_Decorator(m_iListBoxDecoratorsIndex, m_fnDecoratorFactoryMethod(m_DecoratorNames[m_iComboDecoratorIndex], nullptr, pNode->m_pBehaviorTree));
                else
                    pNode->Add_Decorator(m_fnDecoratorFactoryMethod(m_DecoratorNames[m_iComboDecoratorIndex], nullptr, pNode->m_pBehaviorTree));
            }
            else if (m_iComboNodeClassIndex == 2)
            {
                if (m_iNodeInsertMode == 0)
                    pNode->Insert_Node(m_iListBoxChildNodeIndex, m_fnTaskFactoryMethod(m_NodeNames[m_iComboNodeIndex], nullptr, pNode->m_pBehaviorTree));
                else
                    pNode->Add_Node(m_fnTaskFactoryMethod(m_NodeNames[m_iComboNodeIndex], nullptr, pNode->m_pBehaviorTree));
            }
            else if (m_iComboNodeClassIndex == 3)
            {
                if (m_iNodeInsertMode == 0)
                    pNode->Insert_Service(m_iListBoxServiceNodeIndex, m_fnServiceFactoryMethod(m_ServiceNames[m_iComboServiceIndex], nullptr, pNode->m_pBehaviorTree));
                else
                    pNode->Add_Service(m_fnServiceFactoryMethod(m_ServiceNames[m_iComboServiceIndex], nullptr, pNode->m_pBehaviorTree));
            }
        }

		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		ImGui::EndPopup();
	}
}

void CGuiLayer_BehaviorTree::Show_TaskEditTool(CBT_Node* pNode)
{
	if (ImGui::IsItemClicked(ImGuiMouseButton_Middle))
	{
		ImGui::OpenPopup("Edit Task");
	}

	if (ImGui::BeginPopupModal("Edit Task", nullptr))
	{
		ImGui::Text("Task Name : %s", pNode->m_strNodeName.c_str());

		vector<const char*> ListBoxDecorators;
		vector<string> Temps;
		for (auto& b : pNode->m_Decorators)
		{
			string s = typeid(*b).name() + to_string(b->m_iNodeIndex);
			Temps.push_back(s.c_str());
			ListBoxDecorators.push_back(Temps.back().c_str());
		}

		ImGui::ListBox("Decorators", &m_iListBoxDecoratorsIndex, ListBoxDecorators.data(), ListBoxDecorators.size());

		if (m_iListBoxDecoratorsIndex != -1 && ImGui::Button("Delete Decorator"))
		{
			pNode->Delete_Decorator(m_iListBoxDecoratorsIndex);
			m_iListBoxDecoratorsIndex = -1;
		}

		////////////////////////////////
		// 생성 및 삭제를 위한 영역
		ImGui::SeparatorText("Node Add Delete Tool");

		ImGui::Combo("NodeList", &m_iComboNodeClassIndex, m_NodeClasses.data(), m_NodeClasses.size());

		// 데코레이터
		if (m_iComboNodeClassIndex == 1)
		{
			ImGui::Combo("Decorator List", &m_iComboDecoratorIndex, m_DecoratorNames.data(), m_DecoratorNames.size());
			ImGui::InputText("Decorator Name", m_szDecoratorName, 256);

			if (ImGui::Button("Create"))
			{
				pNode->Add_Decorator(m_fnDecoratorFactoryMethod(m_DecoratorNames[m_iComboDecoratorIndex], nullptr, pNode->m_pBehaviorTree));
				pNode->m_pBehaviorTree->Initialize(nullptr);

			}
		}

		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

}


string CGuiLayer_BehaviorTree::Get_VariantTypeValue(TYPES eType, const VariantTypes& Value)
{
   string strValue;
   std::stringstream ss;

   switch (eType)
   {
   case Engine::TYPE_INT:
       ss << get<_int>(Value);
       break;
   case Engine::TYPE_UINT:
       ss << get<_uint>(Value);
       break;
   case Engine::TYPE_FLOAT:
       ss << get<_float>(Value);
       break;
   case Engine::TYPE_DOUBLE:
       ss << get<_double>(Value);
       break;
   case Engine::TYPE_BOOL:
       ss << get<_bool>(Value);
       break;
   case Engine::TYPE_VOID_PTR:
       ss << get<void*>(Value);
       break;
   case Engine::TYPE_GAMEOBJECT_PTR:
       ss << get<CGameObject*>(Value);
       break;
   case Engine::TYPE_STRING:
       ss << get<string>(Value);
       break;
   case Engine::TYPE_VECTOR:
       ss << CMyUtils::String_Vector(get<_vector>(Value));
       break;
   case Engine::TYPE_END:
       break;
   default:
       break;
   }

   return strValue = ss.str();
}

void CGuiLayer_BehaviorTree::DealWithCompositeNode(CBT_Composite* pNode)
{
	if (!pNode)
		return;

	PushStyleColor(pNode);

	string s = pNode->m_strNodeName + to_string(pNode->Get_NodeIndex());

	if (ImGui::TreeNodeEx(s.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PopStyleColor();

		Show_CompositeEditTool(pNode);

		for (auto& pDecorator : pNode->m_Decorators)
		{
			DealWithDecorator(pDecorator);
		}

        for (auto& pService : pNode->m_Services)
        {
            DealWithService(pService);
        }

		for (auto& pNode : pNode->m_ChildNodes)
		{
			CBT_Composite* pCompositeNode = dynamic_cast<CBT_Composite*>(pNode);
			IBT_Task* pTaskNode = dynamic_cast<IBT_Task*>(pNode);
			if (pCompositeNode)
				DealWithCompositeNode(pCompositeNode);
			else if (pTaskNode)
				DealWithTaskNode(pTaskNode);
		}

		ImGui::TreePop();
	}
	else
		ImGui::PopStyleColor();

}

void CGuiLayer_BehaviorTree::DealWithTaskNode(IBT_Task* pTask)
{
	if (!pTask)
		return;

	string s = pTask->m_strClassName + to_string(pTask->Get_NodeIndex());

    PushStyleColor(pTask);
    if (ImGui::TreeNodeEx(s.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PopStyleColor();

        Show_TaskEditTool(pTask);

        for (auto& pDecorator : pTask->m_Decorators)
            DealWithDecorator(pDecorator);

        for (auto& pService : pTask->m_Services)
        {
            DealWithService(pService);
        }

		ImGui::TreePop();
	}
	else
		ImGui::PopStyleColor();
}

void CGuiLayer_BehaviorTree::DealWithDecorator(CBT_Decorator* pNode)
{
	enum Observer_Abort
	{
		BT_RESULT_NONE,
		TYPE_SELF,
		TYPE_LOWER_PRIORITY,
		TYPE_BOTH,
        TYPE_END
	};

	if (CDecorator_Blackboard* pObj = dynamic_cast<CDecorator_Blackboard*>(pNode))
	{
		string s = pObj->m_pairBlackboardValue.first;
		if (!s.empty())
		{
			string strValue = Get_VariantTypeValue(pObj->m_pairBlackboardValue.second, pObj->m_Value);
			ImGui::Text("%s %s", s.c_str(), strValue.c_str());
		}
		else
		{
			ImGui::Text("%s", pObj->m_strNodeName.c_str());
		}

		string strAbortType;

		switch (pObj->m_eAbortType)
		{
		case BT_RESULT_NONE:
			strAbortType = "NONE";
			break;
		case TYPE_SELF:
			strAbortType = "SELF";
			break;
		case TYPE_LOWER_PRIORITY:
			strAbortType = "LOWER_PRIORITY";
			break;
		case TYPE_BOTH:
			strAbortType = "BOTH";
			break;
		default:
			break;
		}

		ImGui::Text("Key QueryType:%s, Abort Type:%s  %d",
			m_BlackboardQueryNames[pObj->m_eQueryType],
			strAbortType.c_str(),
			pNode->Get_NodeIndex());
	}
	else
	{
		string s = typeid(*pNode).name();

		ImGui::Text("%s", s.c_str());
	}
}

void CGuiLayer_BehaviorTree::DealWithService(IBT_Service* pService)
{
    ImGui::Text("%s", pService->m_strClassName.c_str());
}

void CGuiLayer_BehaviorTree::PushStyleColor(CBT_Node* pNode)
{
	if (pNode->m_eResultType == BT_RESULT_SUCCESS)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
	else if (pNode->m_eResultType == BT_RESULT_CONTINUE)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
	else if (pNode->m_eResultType == BT_RESULT_FAIL)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
	else if (pNode->m_eResultType == BT_RESULT_ABORT)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 1.f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.f, 1.f, 1.0f));
}

