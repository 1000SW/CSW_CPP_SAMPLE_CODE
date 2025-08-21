#include "Character_AttributeSet.h"
#include "GameManager.h"
#include "GameInstance.h"
#include "GameplayAbilitySystemHeader.h"

HRESULT CCharacter_AttributeSet::Initialize()
{
    GameplayAttribute Data;

    m_Attributes.emplace("MaxHP", Data);
    m_Attributes.emplace("CurrentHP", Data);
    m_Attributes.emplace("fLimitHP", Data);
    m_Attributes.emplace("fAimSpread", Data);

    return S_OK;
}

void CCharacter_AttributeSet::Init_AttributeSet(CAbilitySystemComponent* pASC)
{
    __super::Init_AttributeSet(pASC);

    CPlayerCharacter* pPlayerCharacter = dynamic_cast<CPlayerCharacter*> (pASC->Get_ActorInfo().pCharacter);
    if (nullptr == pPlayerCharacter)
        MSG_BOX("Invalid Player Character In CCharacter_AttributeSet::Init_AttributeSet");

    m_pPlayerCharacter = pPlayerCharacter;
}

CCharacter_AttributeSet::CCharacter_AttributeSet()
{
}

void CCharacter_AttributeSet::PreAttributeChange(const string& AttributeKey, float& NewValue)
{
}

void CCharacter_AttributeSet::PostAttributeChange(const string& AttributeKey, float OldValue, float NewValue)
{
}

void CCharacter_AttributeSet::PreAttributeBaseChange(const string& AttributeKey, float& NewValue) const
{
    if (AttributeKey == "CurrentHP")
    {
        NewValue = clamp(NewValue, 0.f, Get_Attribute("MaxHP")->GetBaseValue());
    }
    else if (AttributeKey == "MaxHP")
    {
        NewValue = clamp(NewValue, 2000.f, 2500.f);
    }
}

void CCharacter_AttributeSet::PostAttributeBaseChange(const string& AttributeKey, float OldValue, float NewValue) const
{
    if (AttributeKey == "CurrentHP")
    {


        /* Determine Condition Type*/
        _bool bFound;
        _float fMAXHP = m_pASC->GetNumericAttributeBase("MaxHP", bFound);
        if (bFound)
        {
            NewValue = clamp(NewValue, 0.f, fMAXHP);

            _float fHPRatio = NewValue / max(0.1f, fMAXHP);
            fHPRatio = clamp(fHPRatio, 0.f, 1.f);

            ConditionType ePrevConditionType = m_pPlayerCharacter->Get_ConditionType();

            if (fHPRatio < 0.3f)
            {
                CGameManager::GetInstance()->Set_Vinette(true);

                m_pPlayerCharacter->Set_ConditionType(CONDITION_DANGER);
            }
            else
            {
                CGameManager::GetInstance()->Set_Vinette(false);
                m_pPlayerCharacter->Set_ConditionType(CONDITION_GENERIC);
            }

            if (ePrevConditionType != m_pPlayerCharacter->Get_ConditionType())
            {
                m_pASC->Get_ActorInfo().pAnimInstance->Re_Enter_CurrentState();
            }
        }

       
#ifdef LOAD_UI
        GameplayAttribute* pAttribute = Get_Attribute(AttributeKey);
        pAttribute->GetBaseValue();

        auto it = m_Attributes.find("MaxHP");
        if (it != m_Attributes.end())
        {
            CGameInstance::GetInstance()->Update_Player_Hp(NewValue, it->second.GetBaseValue());
        }
#endif
    }

    else if (AttributeKey == "fAimSpread")
    {
        auto it = m_Attributes.find("fAimSpread");
        if (it != m_Attributes.end())
        {
#ifdef LOAD_UI
            CGameManager::GetInstance()->Update_Shake_Value(NewValue);
#endif
        }
    }
}

CCharacter_AttributeSet* CCharacter_AttributeSet::Create()
{
    CCharacter_AttributeSet* pInstance = new CCharacter_AttributeSet();

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed To Created : CCharacter_AttributeSet");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CCharacter_AttributeSet::Free()
{
    __super::Free();
}
