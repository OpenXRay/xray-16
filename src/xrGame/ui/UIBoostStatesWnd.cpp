#include "StdAfx.h"
#include "UIBoostStatesWnd.h"
#include "UIHelper.h"

CUIBoostStatesWnd::CUIBoostStatesWnd() 
    : CUIWindow(CUIBoostStatesWnd::GetDebugType()) {}

void CUIBoostStatesWnd::InitFromXml(CUIXml& xml, LPCSTR path) 
{
    ZoneScoped;

    CUIXmlInit::InitWindow(xml, path, 0, this);
    XML_NODE stored_root = xml.GetLocalRoot();
    
    XML_NODE new_root = xml.NavigateToNode(path, 0);
    xml.SetLocalRoot(new_root);
    dx = CUIBoostStatesWnd::GetWidth();
    dy = CUIBoostStatesWnd::GetHeight();
    constexpr std::tuple<EBoostParams, cpcstr> booster_list[] = {
        {eBoostHpRestore, "indicator_booster_health"}, 
        {eBoostPowerRestore, "indicator_booster_power"},
        {eBoostRadiationRestore, "indicator_booster_rad"}, 
        {eBoostBleedingRestore, "indicator_booster_wound"},
        {eBoostMaxWeight, "indicator_booster_weight"}, 
        {eBoostRadiationProtection, "indicator_booster_radia"},
        {eBoostTelepaticProtection, "indicator_booster_psy"}, 
        {eBoostChemicalBurnProtection, "indicator_booster_chem"}
    };
    for (auto [type, tpath] : booster_list)
    {
        CUIStatic* booster;
        booster = UIHelper::CreateStatic(xml, tpath, this, false);
        m_ind_boost_state.emplace(type, booster);
    }
    xml.SetLocalRoot(stored_root);
}

void CUIBoostStatesWnd::DrawBoosterIndicators() 
{
    for (const auto& Iter : m_ind_boost_state)
    {
        if (Iter.second && Iter.second->IsShown())
        {
            Iter.second->Update();
            Iter.second->Draw();
        }
    }
}

void CUIBoostStatesWnd::UpdateBoosterIndicators(const CEntityCondition::BOOSTER_MAP& influences) 
{
    LPCSTR str_flag = "ui_slow_blinking_alpha";
    u8 flags = 0;
    flags |= LA_CYCLIC;
    flags |= LA_ONLYALPHA;
    flags |= LA_TEXTURECOLOR;

    for (const auto& [type, item] : m_ind_boost_state)
    {
        if (influences.count(type) && !influences.empty())
        {
            if (!m_ind_boost_state[type]->IsShown())
            {
                m_ind_boost_pos.push_back(type);
                m_ind_boost_state[type]->Show(true);
            }
            if (influences.find(type)->second.fBoostTime <= 3.0f)
            {
                m_ind_boost_state[type]->SetColorAnimation(str_flag, flags);
            }
            else
            {
                m_ind_boost_state[type]->ResetColorAnimation();
            }
        }
        else
        {
            m_ind_boost_state[type]->Show(false);
        }
    }
    if (!m_ind_boost_pos.empty())
    {
        bool rev_add = false;
        int i = 0;
        Fvector2 p = pos;
        for (auto It = m_ind_boost_pos.begin(); It != m_ind_boost_pos.end(); It++)
        {
            if (m_ind_boost_state[*It]->IsShown())
            {
                m_ind_boost_state[*It]->SetWndPos({p.x + dx * i, p.y});
                i++;
                m_ind_boost_state[*It]->Update();
                m_ind_boost_state[*It]->Draw();
            }
            else
            {
                m_ind_boost_pos.erase(It);
                It--;
            }
        }
    }
}
