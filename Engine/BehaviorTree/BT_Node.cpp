#include "BT_Node.h"
#include "BT_Decorator.h"
#include "GameInstance.h"
#include "BehaviorTree.h"
#include "GameObject.h"
#include "CompositeArmatrue.h"

vector<_uint> CBT_Node::Get_SubTreeindices()
{
    CBT_Node* pParentNode = m_pParentNode;
    vector<_uint> ret;
    ret.push_back(m_iNodeIndex);

    while (pParentNode)
    {
        ret.push_back(pParentNode->Get_NodeIndex());
        pParentNode = pParentNode->Get_ParentNode();
    }

    return ret;
}

vector<CBT_Node*> CBT_Node::GetAllAncestors()
{
    /* 루트 노드까지 재귀적으로 부모 노드들을 자료구조에 담음 */
    vector<CBT_Node*> ret;

    CBT_Node* pParentNode = m_pParentNode;
    while (pParentNode)
    {
        ret.push_back(pParentNode->Get_ParentNode());
        pParentNode = pParentNode->Get_ParentNode();
    }

    return ret;
}

CBT_Node::CBT_Node()
    : m_pGameInstance{ CGameInstance::GetInstance() }
{
    Safe_AddRef(m_pGameInstance);
}

CBT_Node::CBT_Node(const CBT_Node& Prototype)
    : m_pGameInstance{ Prototype.m_pGameInstance }
    , m_strNodeName{ Prototype.m_strNodeName }
    , m_strClassName{ Prototype.m_strClassName }
{
    Safe_AddRef(m_pGameInstance);
}

HRESULT CBT_Node::Initialize_Prototype(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    return S_OK;
}

HRESULT CBT_Node::Initialize(void* pArg)
{
    if (pArg)
    {
        BT_NODE_DESC* pDesc = static_cast<BT_NODE_DESC*>(pArg);

        m_pBehaviorTree = pDesc->pTree;
        m_pBlackboard = pDesc->pBloackBoard;

        for (auto& pDecorator : m_Decorators)
        {
            pDecorator->Initialize(pDesc);
            pDecorator->Set_ParentNode(this);
            pDecorator->Set_ParentNodeIndex(m_iParentNodeIndex);
        }
        m_iNodeIndex = pDesc->iNodeIndex++;
    }
    return S_OK;
}

HRESULT CBT_Node::Late_Initialize(void* pArg)
{
    return S_OK;
}

HRESULT CBT_Node::Add_Decorator(CBT_Decorator* pDecorator)
{
    if (pDecorator == nullptr)
        return E_FAIL;

    m_Decorators.push_back(pDecorator);
    pDecorator->Set_ParentNode(this);
    pDecorator->Set_ParentNodeIndex(m_iNodeIndex);

    return S_OK;
}

HRESULT CBT_Node::Insert_Decorator(_int iTargetIndex, CBT_Decorator* pDecorator)
{
    if (pDecorator == nullptr)
        return E_FAIL;

    if (iTargetIndex < 0 || iTargetIndex >(m_Decorators.size()))
        return E_FAIL;

    m_Decorators.insert(m_Decorators.begin() + iTargetIndex, pDecorator);

    m_pBehaviorTree->Reorder_Nodes();

    return S_OK;
}

HRESULT CBT_Node::Delete_Decorator(_uint iIndex)
{
    if (m_Decorators.size() <= iIndex || iIndex < 0)
        return E_FAIL;

    m_Decorators.erase(m_Decorators.begin() + iIndex);

    return S_OK;
}

HRESULT CBT_Node::Add_Service(IBT_Service* pService)
{
    if (pService == nullptr)
        return E_FAIL;

    m_Services.push_back(pService);

    return S_OK;
}

HRESULT CBT_Node::Insert_Service(_int iTargetIndex, IBT_Service* pService)
{
    if (pService == nullptr)
        return E_FAIL;

    if (iTargetIndex < 0 || iTargetIndex >(m_Services.size()))
        return E_FAIL;

    m_Services.insert(m_Services.begin() + iTargetIndex, pService);

    m_pBehaviorTree->Reorder_Nodes();

    return S_OK;
}

HRESULT CBT_Node::Delete_Service(_uint iIndex)
{
    if (iIndex < 0 || iIndex >= m_Services.size())
        return E_FAIL;

    Safe_Release(m_Services[iIndex]);
    m_Services.erase(m_Services.begin() + iIndex);

    m_pBehaviorTree->Reorder_Nodes();

    return S_OK;
}

BT_RESULT_TYPE CBT_Node::Receive_Excute(CGameObject* pOwner)
{
    return BT_RESULT_SUCCESS;

    ////Continue 상태였다면 데코레이터 조건 체킹을 하지 않음
    //if (m_eResultType == BT_RESULT_NONE)
    //{
    //    for (auto& pDecorator : m_Decorators)
    //    {
    //        if (BT_RESULT_TYPE::BT_RESULT_FAIL == pDecorator->Run(pOwner))
    //            return m_eResultType = BT_RESULT_TYPE::BT_RESULT_FAIL;
    //    }
    //}

    //m_eResultType = Run(pOwner);

    //if (m_eResultType == BT_RESULT_FAIL || m_eResultType == BT_RESULT_ABORT)
    //    return m_eResultType;

    //for (auto& pDecorator : m_Decorators)
    //{
    //    //ResultType이 Continue일 경우에 한해서만 조기 리턴을 하지 않겠음
    //    BT_RESULT_TYPE eType = pDecorator->Late_Run(pOwner);
    //    if (BT_RESULT_SUCCESS == eType)
    //        return m_eResultType = BT_RESULT_TYPE::BT_RESULT_SUCCESS;
    //    else if (BT_RESULT_FAIL == eType)
    //        return m_eResultType = BT_RESULT_TYPE::BT_RESULT_FAIL;
    //    else if (BT_RESULT_CONTINUE == eType)
    //        m_eResultType = BT_RESULT_TYPE::BT_RESULT_CONTINUE;
    //}

    //if (m_eResultType == BT_RESULT_CONTINUE)
    //    m_pBehaviorTree->Set_ContinueTask(this);

    //return m_eResultType;
}

BT_RESULT_TYPE CBT_Node::Run(CGameObject* pOwner)
{
    return BT_RESULT_TYPE::BT_RESULT_SUCCESS;
}

void CBT_Node::Reset()
{
    m_eResultType = BT_RESULT_NONE;
    m_eTaskStatus = EBTTaskStatus::Inactive;
}

void CBT_Node::Receive_Abort(class CGameObject* pOwner)
{
}

void CBT_Node::Request_Abort()
{

}

vector<CBT_Node*> CBT_Node::Reorder_Nodes(CBehaviorTree* pTree)
{
    vector<CBT_Node*> ret;

    m_pBehaviorTree = pTree;
    m_pOwner = m_pBehaviorTree->Get_Owner();

    for (auto& pDeco : m_Decorators)
    {
        pDeco->Set_NodeIndex(pTree->Get_UniqueNodeIndex());
        pDeco->Set_BehaviorTree(pTree);
        pDeco->Set_Owner(m_pOwner);

        pDeco->Set_ParentNode(this);
        pDeco->Set_ParentNodeIndex(m_iParentNodeIndex);
        ret.push_back(pDeco);
    }

    for (auto& pService : m_Services)
    {
        pService->Set_NodeIndex(pTree->Get_UniqueNodeIndex());
        ret.push_back(pService);
        pService->Set_BehaviorTree(pTree);
        pService->Set_Owner(m_pOwner);
    }

    m_iNodeIndex = pTree->Get_UniqueNodeIndex();

    ret.push_back(this);

    return ret;
}

rapidjson::Value CBT_Node::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
    Value ObjValue(kObjectType);

    ObjValue.AddMember("m_strClassName", StringRef(m_strClassName.c_str()), Alloc);

    if (m_strNodeName.size())
        ObjValue.AddMember("m_strNodeName", StringRef(m_strNodeName.c_str()), Alloc);
    else
        ObjValue.AddMember("m_strNodeName", StringRef(m_strClassName.c_str()), Alloc);

    ObjValue.AddMember("m_iNodeIndex", m_iNodeIndex, Alloc);

    rapidjson::Value ArrayValue(kArrayType);
    rapidjson::Value ServiceArrayValue(kArrayType);

    for (auto& pDecorator : m_Decorators)
        ArrayValue.PushBack(pDecorator->Save_JsonFile(Alloc), Alloc);

     for (auto& pService : m_Services)
         ServiceArrayValue.PushBack(pService->Save_JsonFile(Alloc), Alloc);

    if (false == ArrayValue.Empty())
        ObjValue.AddMember("m_Decorators", ArrayValue, Alloc);

    if (false == ServiceArrayValue.Empty())
        ObjValue.AddMember("m_Services", ServiceArrayValue, Alloc);

    return ObjValue;
}

HRESULT CBT_Node::Load_JsonFile(rapidjson::Value* pJsonValue, CBehaviorTree* pTree)
{
    m_pBehaviorTree = pTree;
    m_pBlackboard = m_pBehaviorTree->Get_Blackboard();

    if (nullptr == m_pBlackboard)
        return E_FAIL;

    if (pJsonValue)
    {
        if ((*pJsonValue).HasMember("m_strNodeName"))
            m_strNodeName = (*pJsonValue)["m_strNodeName"].GetString();

        if ((*pJsonValue).HasMember("m_iNodeIndex"))
            m_iNodeIndex = (*pJsonValue)["m_iNodeIndex"].GetInt();

        if (pJsonValue->HasMember("m_Decorators"))
        {
            Value& Decorators = (*pJsonValue)["m_Decorators"];
            for (Value::ValueIterator itr = Decorators.Begin(); itr != Decorators.End(); itr++)
            {
                Value& ObjValue = itr->GetObj();
                CBT_Decorator* pDecorator = m_pBehaviorTree->Get_DecoratorFactoryMethod()(ObjValue["m_strClassName"].GetString(), &ObjValue, m_pBehaviorTree);
                if (nullptr == pDecorator)
                    return E_FAIL;

                Add_Decorator(pDecorator);
                pDecorator->Load_JsonFile(&ObjValue, pTree);
            }
        }

        if (pJsonValue->HasMember("m_Services"))
        {
            Value& Services = (*pJsonValue)["m_Services"];
            for (Value::ValueIterator itr = Services.Begin(); itr != Services.End(); itr++)
            {
                Value& ObjValue = itr->GetObj();
                IBT_Service* pService = m_pBehaviorTree->Get_ServiceFactoryMethod() (ObjValue["m_strClassName"].GetString(), &ObjValue, m_pBehaviorTree);
                if (nullptr == pService)
                    return E_FAIL;

                Add_Service(pService);
                pService->Load_JsonFile(&ObjValue, pTree);
            }
        }
    }

    return S_OK;
}

_bool CBT_Node::Checking_Decorator()
{
    for (auto& pDecorator : m_Decorators)
    {
        if (BT_RESULT_TYPE::BT_RESULT_FAIL == pDecorator->Run(m_pBehaviorTree->Get_Owner()))
        {
            return m_eResultType = BT_RESULT_TYPE::BT_RESULT_FAIL;
        }
    }

    return true;
}

CBT_Node* CBT_Node::Clone(void* pArg)
{
    return nullptr;
}

void CBT_Node::Free()
{
    __super::Free();

    for (auto& pNode : m_Decorators)
        Safe_Release(pNode);

    Safe_Release(m_pGameInstance);
}
