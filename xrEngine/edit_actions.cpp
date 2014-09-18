////////////////////////////////////////////////////////////////////////////
//	Module 		: edit_actions.cpp
//	Created 	: 04.03.2008
//	Author		: Evgeniy Sokolov
//	Description : edit actions chars class implementation
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "edit_actions.h"
#include "line_edit_control.h"
#include "xr_input.h"
#include <locale.h>

namespace text_editor
{

base::base():	m_previous_action( NULL )
{
}

base::~base()
{
	xr_delete( m_previous_action );
}

void base::on_assign( base* const prev_action )
{
	m_previous_action = prev_action;
}

void base::on_key_press( line_edit_control* const control )
{
	if ( m_previous_action )
	{
		m_previous_action->on_key_press( control );
	}
}

// -------------------------------------------------------------------------------------------------

callback_base::callback_base( Callback const& callback, key_state state )
{
	m_callback  = callback;
	m_run_state = state;
}

callback_base::~callback_base()
{
}

void callback_base::on_key_press( line_edit_control* const control )
{
	if ( control->get_key_state( m_run_state ) )
	{
		m_callback();
		return;
	}
	base::on_key_press( control );
}

// -------------------------------------------------------------------------------------------------

type_pair::type_pair( u32 dik, char c, char c_shift, bool b_translate )
{
	init( dik, c, c_shift, b_translate );
}

type_pair::~type_pair()
{
}

void type_pair::init( u32 dik, char c, char c_shift, bool b_translate )
{
	m_translate	= b_translate;
	m_dik = dik;
	m_char = c;
	m_char_shift = c_shift;
}


void type_pair::on_key_press( line_edit_control* const control )
{
	char c = 0;
	if( m_translate )
	{
		c				= m_char;
		char c_shift	= m_char_shift;
		string128		buff;
		buff[0]			= 0;
		
		/*
		//setlocale( LC_ALL, "" ); // User-default

		// The following 3 lines looks useless

		LPSTR			loc;
		STRCONCAT		( loc, ".", itoa( GetACP(), code_page, 10 ) );
		setlocale		( LC_ALL, loc );*/

		static _locale_t current_locale = _create_locale(LC_ALL, "");
		
		if ( pInput->get_dik_name( m_dik, buff, sizeof(buff) ) )
		{
			if ( _isalpha_l(buff[0], current_locale) || buff[0] == char(-1) ) // "ÿ" = -1
			{
				_strlwr_l	(buff, current_locale);
				c			= buff[0];
				_strupr_l	(buff, current_locale);
				c_shift	= buff[0];
			}
		}

		//setlocale( LC_ALL, "C" );	// restore to ANSI

		if ( control->get_key_state( ks_Shift ) != control->get_key_state( ks_CapsLock ) )
		{
			c = c_shift;
		}
	}
	else
	{
		c = m_char;
		if ( control->get_key_state( ks_Shift ) != control->get_key_state( ks_CapsLock ) )
		{
			c = m_char_shift;
		}
	}
	control->insert_character( c );
}

// -------------------------------------------------------------------------------------------------

key_state_base::key_state_base( key_state state, base* type_pair )
:m_type_pair(type_pair),m_state(state)
{
}

key_state_base::~key_state_base()
{
	xr_delete(m_type_pair);
}

void key_state_base::on_key_press( line_edit_control* const control )
{
	control->set_key_state( m_state, true );
	if(m_type_pair)
		m_type_pair->on_key_press(control);
}

} // namespace text_editor
