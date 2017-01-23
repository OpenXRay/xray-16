////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_upgrade_root.cpp
//	Created 	: 19.10.2007
//  Modified 	: 27.11.2007
//	Author		: Evgeniy Sokolov
//	Description : inventory upgrade root class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "inventory_upgrade.h"
#include "inventory_upgrade_root.h"
#include "inventory_upgrade_group.h"

extern int g_upgrades_log;

namespace inventory
{
namespace upgrade
{

Root::Root()
{
}

Root::~Root()
{
}

void Root::construct( const shared_str& root_id, Manager& manager_r )
{
	inherited::construct( root_id, manager_r );
	m_known = true;

	if( !pSettings->line_exist( root_id, "upgrades" ) )
	{
		return;
	}
	LPCSTR upgrade_groups_str = pSettings->r_string( root_id, "upgrades" );
	if ( !upgrade_groups_str || !xr_strlen( upgrade_groups_str ) )
	{
		return;
	}	
	add_dependent_groups( upgrade_groups_str, manager_r );

	LPCSTR	upgrade_scheme_str = pSettings->r_string( root_id, "upgrade_scheme" );
	VERIFY2( upgrade_scheme_str, make_string( "In inventory item <%s> `upgrade_scheme` is empty!", root_id.c_str() ) );
	m_upgrade_scheme._set( upgrade_scheme_str );

	inherited::fill_root_container( this );
}

void Root::add_upgrade( Upgrade* upgr )
{
	Upgrades_vec::iterator ib = m_contained_upgrades.begin();
	Upgrades_vec::iterator ie = m_contained_upgrades.end();
	for ( ; ib != ie ; ++ib )
	{
		if ( upgr == (*ib) )
		{
			return;
		}
	}

	Ivector2  scheme_index = upgr->get_scheme_index();
	VERIFY2( verify_scheme_index( scheme_index ),
		make_string( "in upgrade <%s> for item <%s> scheme index [%d, %d] is duplicated !",
		upgr->id_str(), id_str(), scheme_index.x, scheme_index.y )
		);
	m_contained_upgrades.push_back( upgr );
}

bool Root::is_root()
{
	return true;
}

#ifdef DEBUG

void Root::log_hierarchy( LPCSTR nest )
{
	u32 sz =  (xr_strlen(nest) + 4) * sizeof(char);
	PSTR	nest2 = (PSTR)_alloca( sz );
	xr_strcpy( nest2, sz, nest );
	Msg( "%s[r] %s", nest2, id_str() );

	inherited::log_hierarchy( nest2 );
}

void Root::test_all_upgrades( CInventoryItem& item )
{
	Upgrades_vec::iterator ib = m_contained_upgrades.begin();
	Upgrades_vec::iterator ie = m_contained_upgrades.end();
	for ( ; ib != ie ; ++ib )
	{
		bool res = item.verify_upgrade( (*ib)->section() );

		if ( g_upgrades_log == 1 )
		{
			Msg( "# Checking upgrade <%s> (id = %d) is successful: %s ", (*ib)->section(), item.object_id(), res ? "OK" : "FAILED" );
		}
	}
}

#endif // DEBUG

bool Root::contain_upgrade( const shared_str& upgrade_id )
{
	if ( inherited::contain_upgrade( upgrade_id ) )
	{
		return true;
	}

	Upgrades_vec::iterator ib = m_contained_upgrades.begin();
	Upgrades_vec::iterator ie = m_contained_upgrades.end();
	for ( ; ib != ie ; ++ib )
	{
		if ( (*ib)->contain_upgrade( upgrade_id ) )
		{
			return true;
		}
	}
	return false;
}

bool Root::verify_scheme_index( const Ivector2& scheme_index )
{
	Upgrades_vec::iterator ib = m_contained_upgrades.begin();
	Upgrades_vec::iterator ie = m_contained_upgrades.end();
	for ( ; ib != ie ; ++ib )
	{
		if ( (*ib)->check_scheme_index( scheme_index ) )
		{
			return false;
		}
	}
	return true;
}

Upgrade* Root::get_upgrade_by_index( Ivector2 const& index )
{
	Upgrades_vec::iterator ib = m_contained_upgrades.begin();
	Upgrades_vec::iterator ie = m_contained_upgrades.end();
	for ( ; ib != ie ; ++ib )
	{
		if ( (*ib)->check_scheme_index( index ) )
		{
			return (*ib);
		}
	}
	return NULL;
}

void Root::highlight_hierarchy( shared_str const& upgrade_id )
{
	Upgrades_vec::iterator ib = m_contained_upgrades.begin();
	Upgrades_vec::iterator ie = m_contained_upgrades.end();
	for ( ; ib != ie ; ++ib )
	{
		if ( (*ib)->id()._get() == upgrade_id._get() )
		{
//			(*ib)->highlight_up();
			(*ib)->highlight_down();
			return;
		}
	}
}

void Root::reset_highlight()
{
	Upgrades_vec::iterator ib = m_contained_upgrades.begin();
	Upgrades_vec::iterator ie = m_contained_upgrades.end();
	for ( ; ib != ie ; ++ib )
	{
		(*ib)->set_highlight( false );
	}
}

} // namespace upgrade
} // namespace inventory
