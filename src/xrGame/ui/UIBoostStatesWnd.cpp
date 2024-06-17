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
    maxItem = 8;
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
    maxItem = xml.ReadAttribInt("settings", 0, "max_item", 8);
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
        indBoostState.emplace(type, booster);
    }
    xml.SetLocalRoot(stored_root);
}

void CUIBoostStatesWnd::DrawBoosterIndicators()
{
    if (indBoostPos.empty())
        return;
    for (const auto& [type, item] : indBoostState)
    {
        if (item && item->IsShown())
        {
            item->Update();
            item->Draw();
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

    for (const auto& [type, item] : indBoostState)
    {
        if (influences.empty())
        {
            item->Show(false);
            continue;
        }
        CEntityCondition::BOOSTER_MAP::const_iterator it = influences.find(type);
        if (it != influences.end())
        {
            if (!item->IsShown())
            {
                indBoostPos.push_back(type);
                item->Show(true);
            }
            if (it->second.fBoostTime <= 3.0f)
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
    if (!influences.empty() || !indBoostPos.empty())
    {
        UpdateBoosterPosition(influences);
    }
}

void CUIBoostStatesWnd::UpdateBoosterPosition(const CEntityCondition::BOOSTER_MAP& influences)
{
    if (indBoostPos.empty() && !influences.empty())
    {
        for (const auto& [type, item] : indBoostState)
        {
            if (item && item->IsShown())
            {
                item->Show(false);
            }
        }
    }
    if (indBoostPos.empty())
        return;
    u8 i = 0, j = 0, max = maxItem - 1;
    if (bInverse)
    {
        for (auto it = indBoostPos.end() - 1; it >= indBoostPos.begin(); it--)
        {
            xr_map<EBoostParams, CUIStatic*>::const_iterator item = indBoostState.find(*it);
            if (item->second->IsShown())
            {
                (bHorizontal ? item->second->SetWndPos({dx * i, dy * j}) :
                	item->second->SetWndPos({dx * j, dy * i}));
                if (i >= maxItem)
                {
                    i = 0;
                    j++;
                }
                else
                {
                    i++;
                }
                item->second->Update();
                item->second->Draw();
            }
            else
            {
                indBoostPos.erase(it);
            }
        }
    }
    else
    {
        for (auto it = indBoostPos.begin(); it != indBoostPos.end(); it++)
        {
            xr_map<EBoostParams, CUIStatic*>::const_iterator item = indBoostState.find(*it);
            if (item->second->IsShown())
            {
                (bHorizontal ? item->second->SetWndPos({dx * i, dy * j}) :
                	item->second->SetWndPos({dx * j, dy * i}));
                if (i >= max)
                {
                    i = 0;
                    j++;
                }
                else
                {
                    i++;
                }
                item->second->Update();
                item->second->Draw();
            }
            else
            {
                indBoostPos.erase(it);
                it--;
            }
        }
    }
}
