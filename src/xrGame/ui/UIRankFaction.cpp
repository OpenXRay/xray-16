////////////////////////////////////////////////////////////////////////////
//	Module 		: UIRankFaction.cpp
//	Created 	: 22.01.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Rank Faction info window class implementation
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UIRankFaction.h"

#include "UIXmlInit.h"
#include "xrUICore/ProgressBar/UIProgressBar.h"
#include "UIHelper.h"

u32 const color_green = 0xff00FF00;
u32 const color_red = 0xffFF0000;
u32 const color_clear = 0xffEEEEFF;

CUIRankFaction::CUIRankFaction()
    : CUIWindow("CUIRankFaction")
{
    m_sn = nullptr;
    m_name = nullptr;
    m_icon = nullptr;
    m_icon_over = nullptr;
    m_location_static = nullptr;
    m_location_value = nullptr;
    m_power_static = nullptr;
    m_power_value = nullptr;

    m_relation_minus = nullptr;
    m_relation_center_minus = nullptr;
    m_relation_center_plus = nullptr;
    m_relation_plus = nullptr;

    m_origin_static = nullptr;
    m_border_minus = nullptr;
    m_border_plus = nullptr;
    m_enemy_static = nullptr;
    m_frined_static = nullptr;

    m_rating_up = nullptr;
    m_rating_down = nullptr;

    m_prev_sn = 0xff;
}

CUIRankFaction::CUIRankFaction(shared_str const& faction_id)
    : CUIWindow("CUIRankFaction")
{
    m_faction_state = faction_id;

    m_sn = nullptr;
    m_name = nullptr;
    m_icon = nullptr;
    m_icon_over = nullptr;
    m_location_static = nullptr;
    m_location_value = nullptr;
    m_power_static = nullptr;
    m_power_value = nullptr;

    m_relation_minus = nullptr;
    m_relation_center_minus = nullptr;
    m_relation_center_plus = nullptr;
    m_relation_plus = nullptr;

    m_origin_static = nullptr;
    m_border_minus = nullptr;
    m_border_plus = nullptr;
    m_enemy_static = nullptr;
    m_frined_static = nullptr;

    m_rating_up = nullptr;
    m_rating_down = nullptr;

    m_prev_sn = 0xff;
}

float CUIRankFaction::get_faction_power() const
{
    return m_faction_state.power;
}

void CUIRankFaction::init_from_xml(CUIXml& xml)
{
    CUIXmlInit::InitWindow(xml, "fraction_stand_wnd", 0, this);

    m_sn = UIHelper::CreateTextWnd(xml, "serial_number", this);
    m_name = UIHelper::CreateTextWnd(xml, "name", this);
    m_icon = UIHelper::CreateStatic(xml, "icon", this);
    m_icon_over = UIHelper::CreateStatic(xml, "icon_over", this);
    m_location_static = UIHelper::CreateTextWnd(xml, "location_static", this);
    m_location_value = UIHelper::CreateTextWnd(xml, "location_value", this);
    m_power_static = UIHelper::CreateTextWnd(xml, "power_static", this);
    m_power_value = UIHelper::CreateTextWnd(xml, "power_value", this);

    m_relation_minus = UIHelper::CreateProgressBar(xml, "relation_minus", this);
    m_relation_center_minus = UIHelper::CreateProgressBar(xml, "relation_center_minus", this);
    m_relation_center_plus = UIHelper::CreateProgressBar(xml, "relation_center_plus", this);
    m_relation_plus = UIHelper::CreateProgressBar(xml, "relation_plus", this);

    m_origin_static = UIHelper::CreateStatic(xml, "origin_static", this);
    m_border_minus = UIHelper::CreateStatic(xml, "border_minus", this);
    m_border_plus = UIHelper::CreateStatic(xml, "border_plus", this);
    m_enemy_static = UIHelper::CreateTextWnd(xml, "enemy_static", this);
    m_frined_static = UIHelper::CreateTextWnd(xml, "frined_static", this);

    m_rating_up = UIHelper::CreateStatic(xml, "rating_up", this);
    m_rating_down = UIHelper::CreateStatic(xml, "rating_down", this);

    Fvector2 pos;
    m_location_static->AdjustWidthToText();
    pos.x = m_location_static->GetWndPos().x + m_location_static->GetWidth() + 10.0f;
    pos.y = m_location_value->GetWndPos().y;
    m_location_value->SetWndPos(pos);

    m_power_static->AdjustWidthToText();
    pos.x = m_power_static->GetWndPos().x + m_power_static->GetWidth() + 10.0f;
    pos.y = m_power_value->GetWndPos().y;
    m_power_value->SetWndPos(pos);

    rating(0);
}

void CUIRankFaction::rating(u8 new_sn, bool force)
{
    if (force || m_prev_sn == 0xff)
    {
        m_rating_up->SetTextureColor(color_clear);
        m_rating_down->SetTextureColor(color_clear);
    }

    if (m_prev_sn < new_sn)
    {
        m_rating_up->SetTextureColor(color_clear);
        m_rating_down->SetTextureColor(color_red);
        m_prev_sn = new_sn;
    }
    else if (m_prev_sn > new_sn)
    {
        m_rating_up->SetTextureColor(color_green);
        m_rating_down->SetTextureColor(color_clear);
        m_prev_sn = new_sn;
    }
}

void CUIRankFaction::update_info(u8 sn)
{
    m_faction_state.update_info();

    string32 buf;
    xr_sprintf(buf, sizeof(buf), "%d", sn);
    m_sn->SetText(buf);

    m_name->SetText(m_faction_state.get_name());
    m_icon->InitTexture(m_faction_state.get_icon());

    m_location_value->SetText(m_faction_state.get_location());

    xr_sprintf(buf, sizeof(buf), "%.0f", m_faction_state.power);
    m_power_value->SetText(buf);

    const float gw = static_cast<float>(m_faction_state.get_actor_goodwill());
    if (gw > 0.0f)
    {
        m_relation_center_minus->SetProgressPos(0.0f);
        m_relation_minus->SetProgressPos(0.0f);

        m_relation_center_plus->SetProgressPos(gw);
        const float gw_plus = m_relation_center_plus->GetRange_max();
        if (gw > gw_plus)
        {
            m_relation_plus->SetProgressPos(gw - gw_plus);
        }
        else
        {
            m_relation_plus->SetProgressPos(0.0f);
        }
    }
    else if (gw < 0.0f)
    {
        m_relation_center_plus->SetProgressPos(0.0f);
        m_relation_plus->SetProgressPos(0.0f);

        m_relation_center_minus->SetProgressPos(-gw);
        const float gw_minus = -1.0f * m_relation_center_minus->GetRange_max();
        if (gw < gw_minus)
        {
            m_relation_minus->SetProgressPos(gw_minus - gw);
        }
        else
        {
            m_relation_minus->SetProgressPos(0.0f);
        }
    }
    else
    {
        m_relation_minus->SetProgressPos(0.0f);
        m_relation_center_minus->SetProgressPos(0.0f);
        m_relation_center_plus->SetProgressPos(0.0f);
        m_relation_plus->SetProgressPos(0.0f);
    }
}
