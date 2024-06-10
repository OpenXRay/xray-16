#include "StdAfx.h"
#include "UIBoostStatesWnd.h"
#include "UIHelper.h"

CUIBoostStatesWnd::CUIBoostStatesWnd() 
    : CUIWindow(CUIBoostStatesWnd::GetDebugType()) 
{
    bHorizontal = true;
    bInverse = false;
    dx = 0.f;
    dy = 0.f;
    max_item = 8;
}

void CUIBoostStatesWnd::InitFromXml(CUIXml& xml, LPCSTR path) 
{
    ZoneScoped;

    CUIXmlInit::InitWindow(xml, path, 0, this);
    XML_NODE stored_root = xml.GetLocalRoot();
    
    XML_NODE new_root = xml.NavigateToNode(path, 0);
    xml.SetLocalRoot(new_root);

    dx = xml.ReadAttribFlt("settings", 0, "dx", GetWidth());
    dy = xml.ReadAttribFlt("settings", 0, "dy", GetHeight());
    bHorizontal = (xml.ReadAttribInt("settings", 0, "horz_align", 1) == 1);
    bInverse = (xml.ReadAttribInt("settings", 0, "inverse", 0) == 1);
    max_item = xml.ReadAttribInt("settings", 0, "max_item", 8);
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
        if (influences.empty())
        {
            item->Show(false);
            continue;
        }
        CEntityCondition::BOOSTER_MAP::const_iterator It = influences.find(type);
        if (It != influences.end())
        {
            if (!item->IsShown())
            {
                m_ind_boost_pos.push_back(type);
                item->Show(true);
            }
            if (It->second.fBoostTime <= 3.0f)
            {
                item->SetColorAnimation(str_flag, flags);
            }
            else
            {
                item->ResetColorAnimation();
            }
        }
        else
        {
            item->Show(false);
        }
    }

    if (!m_ind_boost_pos.empty())
    {
        u8 i = 0,j = 0,max = max_item - 1;
        if (bInverse)
        {
            for (auto It = m_ind_boost_pos.end() - 1; It >= m_ind_boost_pos.begin(); It--)
            {
                if (m_ind_boost_state[*It]->IsShown())
                {
                    (bHorizontal ? m_ind_boost_state[*It]->SetWndPos({dx * i, dy * j}) :
                                   m_ind_boost_state[*It]->SetWndPos({dx * j, dy * i}));
                    if (i >= max_item)
                    {
                        i = 0;
                        j++;
                    }
                    else
                    {
                        i++;
                    }
                    m_ind_boost_state[*It]->Update();
                    m_ind_boost_state[*It]->Draw();
                }
                else
                {
                    m_ind_boost_pos.erase(It);
                }
            }
        }
        else
        {
            for (auto It = m_ind_boost_pos.begin(); It != m_ind_boost_pos.end(); It++)
            {
                if (m_ind_boost_state[*It]->IsShown())
                {
                    (bHorizontal ? m_ind_boost_state[*It]->SetWndPos({dx * i, dy * j}) :
                                   m_ind_boost_state[*It]->SetWndPos({dx * j, dy * i}));
                    if (i >= max)
                    {
                        i = 0;
                        j++;
                    }
                    else
                    {
                        i++;
                    }
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
}
