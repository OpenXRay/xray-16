////////////////////////////////////////////////////////////////////////////
//	Module 		: XR_IOConsole_control.cpp
//	Created 	: 03.10.2008
//	Author		: Evgeniy Sokolov
//	Description : Console`s control-functions class implementation
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XR_IOConsole.h"


void CConsole::add_cmd_history( shared_str const& str )
{
	if ( str.size() == 0 )
	{
		return;
	}
	m_cmd_history.push_back( str );
	if ( m_cmd_history.size() > m_cmd_history_max )
	{
		m_cmd_history.erase( m_cmd_history.begin() );
	}
}

void CConsole::next_cmd_history_idx()
{
	--m_cmd_history_idx;
	if ( m_cmd_history_idx < 0 )
	{
		m_cmd_history_idx = 0;
	}
}

void CConsole::prev_cmd_history_idx()
{
	++m_cmd_history_idx;
	if ( m_cmd_history_idx >= (int)m_cmd_history.size() )
	{
		m_cmd_history_idx = m_cmd_history.size() - 1;
	}
}

void CConsole::reset_cmd_history_idx()
{
	m_cmd_history_idx = -1;
}

void CConsole::next_selected_tip()
{
	++m_select_tip;
	check_next_selected_tip();
}

void CConsole::check_next_selected_tip()
{
	if ( m_select_tip >= (int)m_tips.size() )
	{
		m_select_tip = m_tips.size() - 1;
	}

	int sel_dif = m_select_tip - VIEW_TIPS_COUNT + 1;
	if ( sel_dif < 0 )
	{
		sel_dif = 0;
	}

	if ( sel_dif > m_start_tip )
	{
		m_start_tip = sel_dif;
	}
}

void CConsole::prev_selected_tip()
{
	--m_select_tip;
	check_prev_selected_tip();
}

void CConsole::check_prev_selected_tip()
{
	if ( m_select_tip < 0 )
	{
		m_select_tip = 0;
	}

	if ( m_start_tip > m_select_tip )
	{
		m_start_tip = m_select_tip;
	}
}

void CConsole::reset_selected_tip()
{
	m_select_tip = -1;
	m_start_tip = 0;
	m_disable_tips = false;
}
