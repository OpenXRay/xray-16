////////////////////////////////////////////////////////////////////////////
//	Module 		: UIRankFaction.h
//	Created 	: 22.01.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Rank Faction info window class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "UIWindow.h"
#include "FactionState.h"

class CUIXml;
class CUIStatic;
class CUITextWnd;
class CUIProgressBar;
class CActor;

class CUIRankFaction : public CUIWindow
{
private:
	FactionState	m_faction_state;

	CUITextWnd*		m_sn;
	CUITextWnd*		m_name;
	CUIStatic*		m_icon;
	CUIStatic*		m_icon_over;
	CUITextWnd*		m_location_static;
	CUITextWnd*		m_location_value;
	CUITextWnd*		m_power_static;
	CUITextWnd*		m_power_value;
	
	CUIProgressBar*		m_relation_minus;
	CUIProgressBar*		m_relation_center_minus;
	CUIProgressBar*		m_relation_center_plus;
	CUIProgressBar*		m_relation_plus;
	
	CUIStatic*		m_origin_static;
	CUIStatic*		m_border_minus;
	CUIStatic*		m_border_plus;
	CUITextWnd*		m_enemy_static;
	CUITextWnd*		m_frined_static;

	CUIStatic*		m_rating_up;
	CUIStatic*		m_rating_down;

	u8				m_prev_sn;

public:
			CUIRankFaction		();
			CUIRankFaction		( shared_str const& faction_id );
	virtual	~CUIRankFaction	();

	void	init_from_xml		( CUIXml& xml );
	void	rating				( u8 new_sn, bool force = false );
	void	update_info			( u8 sn );
	float	get_faction_power	();
	IC	u8	get_cur_sn			()	{	return m_prev_sn; }

}; // class CUIRankFaction
