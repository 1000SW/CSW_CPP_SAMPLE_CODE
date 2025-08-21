#include "Blackboard.h"

CBlackboard::CBlackboard(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice{ pDevice }
	, m_pContext{ pContext }
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
 }

CBlackboard::CBlackboard(const CBlackboard& Prototype)
	: m_pDevice{ Prototype.m_pDevice }
	, m_pContext{ Prototype.m_pContext }
	, m_Variables{ Prototype. m_Variables }
	, m_VariableTypes { Prototype.m_VariableTypes }
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

HRESULT CBlackboard::Add_Variable(const _string& strKey, pair<const VariantTypes&, TYPES> Value)
{
	if (m_Variables.find(strKey) != m_Variables.end())
		return E_FAIL;
	
	m_Variables[strKey] = Value.first;
	m_VariableTypes[strKey] = Value.second;

	return S_OK;
}

HRESULT CBlackboard::Bind_Delegate(const _string& strKey, function<void()> func)
{
    m_Delegates[strKey].Add(strKey, func);
	return S_OK;
}

HRESULT CBlackboard::Remove_Delegate(const _string& strKey)
{
    m_Delegates[strKey].Remove_Function(strKey);
	return S_OK;
}

_bool CBlackboard::Has_Variable(const _string& strKey)
{
    if (m_Variables.find(strKey) == m_Variables.end())
        return false;

    return true;
}

rapidjson::Value CBlackboard::Save_JsonFile(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& Alloc)
{
	using namespace rapidjson;

	rapidjson::Value ObjValue(kObjectType);

	rapidjson::Value ArrayValue(kArrayType);

	return ObjValue;
}

CBlackboard* CBlackboard::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	return new CBlackboard(pDevice, pContext);
}

CBlackboard* CBlackboard::Clone(void* pArg)
{
	return new CBlackboard(*this);
}

void CBlackboard::Free()
{
	__super::Free();

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);

	m_Delegates.clear();
}
