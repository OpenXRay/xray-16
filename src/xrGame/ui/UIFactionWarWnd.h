////////////////////////////////////////////////////////////////////////////
//	Module 		: UIFactionWarWnd.h
//	Created 	: 26.12.2007
//	Author		: Evgeniy Sokolov
//	Description : UI Faction War window class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Callbacks/UIWndCallback.h"
#include "FactionState.h"
#include "UIWarState.h"

class CUIStatic;
class CGameTask;
class CUIXml;
class CUIProgressBar;
class CUIFrameLineWnd;

class CUIFactionWarWnd : public CUIWindow, public CUIWndCallback
{
    using inherited = CUIWindow;

    bool m_initialized;

	CUIFrameWindow*	m_background;
	CUIFrameWindow*	m_center_background;

	CUITextWnd*			m_target_static;
	CUITextWnd*			m_target_caption;       // our
	Fvector2			m_tc_pos;
	CUITextWnd*			m_target_desc;
	Fvector2			m_td_pos;
	CUIStatic*			m_state_static;

	CUIFrameLineWnd*	m_static_line1;
	CUIFrameLineWnd*	m_static_line2;
	CUIFrameLineWnd*	m_static_line3;
	CUIFrameLineWnd*	m_static_line4;
	CUIFrameLineWnd*	m_static_line_left;
	CUIFrameLineWnd*	m_static_line_right;

	CUIStatic*			m_our_icon;
	CUIStatic*			m_our_icon_over;
	CUITextWnd*			m_our_name;
	CUIStatic*			m_st_our_frac_info;
	CUIStatic*			m_st_our_mem_count;
	CUIStatic*			m_st_our_resource;
	
	CUIProgressBar*		m_pb_our_state;
	CUIProgressBar*		m_pb_our_mem_count;
	CUIProgressBar*		m_pb_our_resource;

	CUIStatic*			m_enemy_icon;
	CUIStatic*			m_enemy_icon_over;
	CUITextWnd*			m_enemy_name;
	CUIStatic*			m_st_enemy_frac_info;
	CUIStatic*			m_st_enemy_mem_count;
	CUIStatic*			m_st_enemy_resource;

	CUIProgressBar*		m_pb_enemy_state;
	CUIProgressBar*		m_pb_enemy_mem_count;
	CUIProgressBar*		m_pb_enemy_resource;

	CUIWindow*			m_war_states_parent;
	float				m_war_states_dx;
	float				m_war_states_xcenter;
	enum				{ max_war_state = FactionState::war_state_count };
	UIWarState*			m_war_state[max_war_state];

	enum				{ max_bonuce = 6 };
	CUIStatic*			m_our_bonuces[max_bonuce];
	CUIStatic*			m_enemy_bonuces[max_bonuce];

	// ----------------------------------------
	//shared_str			m_our_faction_id;
	//shared_str			m_enemy_faction_id;
	u32					m_update_delay;
	u32					m_previous_time;

	FactionState		m_our_faction;
	FactionState		m_enemy_faction;

	int					m_max_member_count;
	float				m_max_resource;
	float				m_max_power;

public:
						CUIFactionWarWnd		();
	virtual				~CUIFactionWarWnd		();

	virtual void		SendMessage				( CUIWindow* pWnd, s16 msg, void* pData );
	virtual void 		Show					( bool status );
	virtual void		Update					();

			void		ShowInfo				( bool status );

			void		Reset					();
			void		Init					();
			
			bool		InitFactions			();
			void		UpdateInfo				();
			void		UpdateWarStates			( FactionState const& faction );
//			void		set_amount_state_vs		( int value );
			void		set_amount_our_bonus	( int value );
			void		set_amount_enemy_bonus	( int value );

			UIHint*		hint_wnd;
protected:
			int			get_max_member_count	();
			float		get_max_resource		();
			float		get_max_power			();

}; // class CUIFactionWarWnd
