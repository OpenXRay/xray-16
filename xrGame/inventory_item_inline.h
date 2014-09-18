////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item_inline.h
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Victor Reutsky, Yuri Dobronravin, Sokolov Evgeniy
//	Description : Inventory item inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	bool CInventoryItem::useful_for_NPC					() const
{
	return				(Useful() && m_flags.test(Fuseful_for_NPC));
}

IC CInventoryItem::Upgrades_type const& CInventoryItem::upgardes() const
{
	return m_upgrades;
}

template <typename T>
IC bool CInventoryItem::process_if_exists( LPCSTR section, LPCSTR name, T (CInifile::*method)(LPCSTR, LPCSTR) const, T& value, bool test )
{
	if ( !pSettings->line_exist( section, name ) )
	{
		return false;
	}
	LPCSTR str = pSettings->r_string( section, name );
	if ( !str || !xr_strlen(str) )
	{
		return false;
	}

	if ( !test )
	{
		value = value + (pSettings->*method)( section, name ); // add
	}
	return true;
}

template <typename T>
IC bool CInventoryItem::process_if_exists_set( LPCSTR section, LPCSTR name, T (CInifile::*method)(LPCSTR, LPCSTR) const, T& value, bool test )
{
	if ( !pSettings->line_exist( section, name ) )
	{
		return false;
	}
	LPCSTR str = pSettings->r_string( section, name );
	if ( !str || !xr_strlen(str) )
	{
		return false;
	}

	if ( !test )
	{
		value = (pSettings->*method)( section, name );    // set
	}
	return true;
}
