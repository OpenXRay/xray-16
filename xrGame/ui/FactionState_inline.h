////////////////////////////////////////////////////////////////////////////
//	Module 		: FactionState_inline.h
//	Created 	: 23.01.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Faction War state class
////////////////////////////////////////////////////////////////////////////

#pragma once

int    FactionState::get_actor_goodwill() const
{
	return m_actor_goodwill;
}
void   FactionState::set_actor_goodwill( int gw )
{
	m_actor_goodwill = gw;
}
LPCSTR FactionState::get_faction_id() const
{
	return m_id.c_str();
}
shared_str const& FactionState::get_faction_id2() const
{
	return m_id;
}
void   FactionState::set_faction_id( LPCSTR id )
{
	m_id._set( id );
}
void   FactionState::set_faction_id2( shared_str const& id )
{
	m_id._set( id );
}
LPCSTR FactionState::get_name() const
{
	return m_name.c_str();
}
void   FactionState::set_name( LPCSTR name )
{
	m_name._set( name );
}
LPCSTR FactionState::get_icon() const
{
	return m_icon.c_str();
}
void   FactionState::set_icon( LPCSTR icon )
{
	m_icon._set( icon );
}

LPCSTR FactionState::get_icon_big() const
{
	return m_icon_big.c_str();
}
void   FactionState::set_icon_big( LPCSTR icon_b )
{	m_icon_big._set( icon_b );
}

LPCSTR FactionState::get_target() const
{
	return m_target.c_str();
}
void   FactionState::set_target( LPCSTR target )
{
	m_target._set( target );
}

LPCSTR FactionState::get_target_desc() const
{
	return m_target_desc.c_str();
}
void   FactionState::set_target_desc( LPCSTR target_desc )
{
	m_target_desc._set( target_desc );
}

LPCSTR FactionState::get_location() const
{
	return m_location.c_str();
}
void   FactionState::set_location( LPCSTR location )
{
	m_location._set( location );
}

LPCSTR FactionState::get_war_state( u8 index ) const
{
	VERIFY( 0 <= index && index < war_state_count );
	return	m_war_state_str[index].c_str();	
}

LPCSTR FactionState::get_war_state_hint( u8 index ) const
{
	VERIFY( 0 <= index && index < war_state_count );
	return	m_war_state_hint_str[index].c_str();	
}

LPCSTR FactionState::get_war_state1() const			{	return	m_war_state_str[0].c_str();		}
void   FactionState::set_war_state1( LPCSTR icon )	{			m_war_state_str[0]._set( icon );}
LPCSTR FactionState::get_war_state2() const			{	return	m_war_state_str[1].c_str();		}
void   FactionState::set_war_state2( LPCSTR icon )	{			m_war_state_str[1]._set( icon );}
LPCSTR FactionState::get_war_state3() const			{	return	m_war_state_str[2].c_str();		}
void   FactionState::set_war_state3( LPCSTR icon )	{			m_war_state_str[2]._set( icon );}
LPCSTR FactionState::get_war_state4() const			{	return	m_war_state_str[3].c_str();		}
void   FactionState::set_war_state4( LPCSTR icon )	{			m_war_state_str[3]._set( icon );}
LPCSTR FactionState::get_war_state5() const			{	return	m_war_state_str[4].c_str();		}
void   FactionState::set_war_state5( LPCSTR icon )	{			m_war_state_str[4]._set( icon );}

LPCSTR FactionState::get_war_state_hint1() const		{	return	m_war_state_hint_str[0].c_str();		}
void   FactionState::set_war_state_hint1( LPCSTR text )	{			m_war_state_hint_str[0]._set( text );}
LPCSTR FactionState::get_war_state_hint2() const		{	return	m_war_state_hint_str[1].c_str();		}
void   FactionState::set_war_state_hint2( LPCSTR text )	{			m_war_state_hint_str[1]._set( text );}
LPCSTR FactionState::get_war_state_hint3() const		{	return	m_war_state_hint_str[2].c_str();		}
void   FactionState::set_war_state_hint3( LPCSTR text )	{			m_war_state_hint_str[2]._set( text );}
LPCSTR FactionState::get_war_state_hint4() const		{	return	m_war_state_hint_str[3].c_str();		}
void   FactionState::set_war_state_hint4( LPCSTR text )	{			m_war_state_hint_str[3]._set( text );}
LPCSTR FactionState::get_war_state_hint5() const		{	return	m_war_state_hint_str[4].c_str();		}
void   FactionState::set_war_state_hint5( LPCSTR text )	{			m_war_state_hint_str[4]._set( text );}
