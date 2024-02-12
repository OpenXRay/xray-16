////////////////////////////////////////////////////////////////////////////
//	Module 		: UIRankFaction.h
//	Created 	: 22.01.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Rank Faction info window class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "FactionState.h"

class CUIXml;
class CUIStatic;
class CUIProgressBar;
class CActor;

class CUIRankFaction final : public CUIWindow
{
    FactionState m_faction_state;

    CUIStatic* m_sn;
    CUIStatic* m_name;
    CUIStatic* m_icon;
    CUIStatic* m_icon_over;
    CUIStatic* m_location_static;
    CUIStatic* m_location_value;
    CUIStatic* m_power_static;
    CUIStatic* m_power_value;

    CUIProgressBar* m_relation_minus;
    CUIProgressBar* m_relation_center_minus;
    CUIProgressBar* m_relation_center_plus;
    CUIProgressBar* m_relation_plus;

    CUIStatic* m_origin_static;
    CUIStatic* m_border_minus;
    CUIStatic* m_border_plus;
    CUIStatic* m_enemy_static;
    CUIStatic* m_frined_static;

    CUIStatic* m_rating_up;
    CUIStatic* m_rating_down;

    u8 m_prev_sn;

public:
    CUIRankFaction();
    CUIRankFaction(shared_str const& faction_id);

    void init_from_xml(CUIXml& xml);
    void rating(u8 new_sn, bool force = false);
    void update_info(u8 sn);
    float get_faction_power() const;
    IC u8 get_cur_sn() const { return m_prev_sn; }

    pcstr GetDebugType() override { return "CUIRankFaction"; }
}; // class CUIRankFaction
