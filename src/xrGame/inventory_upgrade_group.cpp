////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_upgrade_group.cpp
//	Created 	: 22.10.2007
//  Modified 	: 27.11.2007
//	Author		: Evgeniy Sokolov
//	Description : inventory upgrade group class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "inventory_upgrade_group.h"
#include "inventory_upgrade.h"

namespace inventory
{
namespace upgrade
{

Group::Group()
{
}

Group::~Group()
{
}

void Group::construct( const shared_str& group_id, UpgradeBase& parent_upgrade, Manager& manager_r )
{
	m_id._set( group_id );
	add_parent_upgrade( parent_upgrade );

	VERIFY2( pSettings->section_exist( m_id ),
		make_string( "Upgrade <%s> : group section [%s] does not exist!" , parent_upgrade.id_str(), m_id.c_str() ) );
	
	LPCSTR	upgrades_str = pSettings->r_string(m_id, "elements");
	VERIFY2( upgrades_str, make_string( "in upgrade group <%s> elements are empty!", m_id.c_str() ) );

	u32 const buffer_size	= (xr_strlen(upgrades_str) + 1) * sizeof(char);
	PSTR	temp  = (PSTR)_alloca( buffer_size );
	for ( int n = _GetItemCount(upgrades_str), i = 0; i < n; ++i )
	{
		UpgradeBase* upgrade_p = (UpgradeBase*)manager_r.add_upgrade( _GetItem( upgrades_str, i, temp, buffer_size ), *this );
		m_included_upgrades.push_back( upgrade_p );
	}
}

void Group::add_parent_upgrade( UpgradeBase& parent_upgrade )
{
	if ( std::find( m_parent_upgrades.begin(), m_parent_upgrades.end(), &parent_upgrade ) == m_parent_upgrades.end() )
	{
		m_parent_upgrades.push_back( &parent_upgrade );
	}
}

#ifdef DEBUG

void Group::log_hierarchy( LPCSTR nest )
{
	u32 sz = (xr_strlen(nest) + 4) * sizeof(char);
	PSTR	nest2 = (PSTR)_alloca( sz );
	xr_strcpy( nest2, sz, nest );
	xr_strcat( nest2, sz, "   " );
	Msg( "%s(g) %s", nest2, m_id.c_str() );

	Upgrades_type::iterator ib = m_included_upgrades.begin();
	Upgrades_type::iterator ie = m_included_upgrades.end();
	for ( ; ib != ie ; ++ib )
	{
		(*ib)->log_hierarchy( nest2 );
	}
}

#endif // DEBUG

void Group::fill_root( Root* root )
{
	Upgrades_type::iterator ib = m_included_upgrades.begin();
	Upgrades_type::iterator ie = m_included_upgrades.end();
	for ( ; ib != ie ; ++ib )
	{
		(*ib)->fill_root_container( root );
	}
}

UpgradeStateResult Group::can_install( CInventoryItem& item, UpgradeBase& test_upgrade, bool loading )
{
	Upgrades_type::iterator ib = m_parent_upgrades.begin();
	Upgrades_type::iterator ie = m_parent_upgrades.end();
	for ( ; ib != ie ; ++ib )
	{
		if ( (*ib)->is_root() )
		{
			continue;
		}
//		if ( !item.has_upgrade( (*ib)->id() ) )
		Upgrade* U = smart_cast<Upgrade*>(*ib);
		if ( !item.has_upgrade_group( U->parent_group_id() ) )
		{
			if ( loading )
			{
				FATAL( make_string( "Loading item: Upgrade <%s> of inventory item [%s] (id = %d) can`t be installed! Error = result_e_parents",
					test_upgrade.id_str(), item.m_section_id.c_str(), item.object_id() ).c_str() );
			}
			return result_e_parents;
		}
	}
	
	ib = m_included_upgrades.begin();
	ie = m_included_upgrades.end();
	for ( ; ib != ie ; ++ib )
	{
		if ( (*ib) == &test_upgrade )
		{
			continue;
		}
		if ( item.has_upgrade( (*ib)->id() ) )
		{
			if ( loading )
			{
				FATAL( make_string( "Loading item: Upgrade <%s> of inventory item [%s] (id = %d) can`t be installed! Error = result_e_group",
					test_upgrade.id_str(), item.m_section_id.c_str(), item.object_id() ).c_str() );
			}
			return result_e_group;
		}
	}

	return result_ok;
}

void Group::highlight_up()
{
	Upgrades_type::iterator ib = m_included_upgrades.begin();
	Upgrades_type::iterator ie = m_included_upgrades.end();
	for ( ; ib != ie ; ++ib )
	{
		(*ib)->highlight_up();
	}
}

void Group::highlight_down()
{
	Upgrades_type::iterator ib = m_parent_upgrades.begin();
	Upgrades_type::iterator ie = m_parent_upgrades.end();
	for ( ; ib != ie ; ++ib )
	{
		(*ib)->highlight_down();
	}
}

} // namespace upgrade
} // namespace inventory
