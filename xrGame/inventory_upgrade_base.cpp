////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_upgrade_base.cpp
//	Created 	: 19.10.2007
//  Modified 	: 27.11.2007
//	Author		: Evgeniy Sokolov
//	Description : inventory upgrade base class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"

#include "inventory_upgrade_base.h"
#include "inventory_upgrade_manager.h"
#include "inventory_upgrade_group.h"
#include "inventory_upgrade.h"

extern int g_upgrades_log;

namespace inventory
{
namespace upgrade
{

UpgradeBase::UpgradeBase()
{
}

UpgradeBase::~UpgradeBase()
{
}

void UpgradeBase::construct( const shared_str& upgrade_id, Manager& manager_r )
{
	m_id._set( upgrade_id );
	m_known = false;
	
	VERIFY2( pSettings->section_exist( m_id ), make_string( "Section of upgrade [%s] not exist!", m_id.c_str() ) );
}

void UpgradeBase::add_dependent_groups( LPCSTR groups_str, Manager& manager_r )
{
	u32 const buffer_size	= (xr_strlen(groups_str) + 1) * sizeof(char);
	PSTR	temp = (PSTR)_alloca( buffer_size );

	for ( int n = _GetItemCount( groups_str ), i = 0; i < n; ++i )
	{
		Group* group_p = manager_r.add_group( _GetItem( groups_str, i, temp, buffer_size ), *this );

		if ( std::find( m_depended_groups.begin(), m_depended_groups.end(), group_p ) == m_depended_groups.end() )
		{
			m_depended_groups.push_back( group_p );
		}
	}
}

#ifdef DEBUG

void UpgradeBase::log_hierarchy( LPCSTR nest )
{
	Groups_type::iterator ib = m_depended_groups.begin();
	Groups_type::iterator ie = m_depended_groups.end();
	for ( ; ib != ie ; ++ib )
	{
		(*ib)->log_hierarchy( nest );
	}
}
/*
void UpgradeBase::test_all_upgrades( CInventoryItem& item )
{
	Groups_type::iterator ib = m_depended_groups.begin();
	Groups_type::iterator ie = m_depended_groups.end();
	for ( ; ib != ie ; ++ib )
	{
		(*ib)->test_all_upgrades( item );
	}
}
*/
#endif // DEBUG

bool UpgradeBase::is_root()
{
	return false;
}

bool UpgradeBase::make_known()
{
	m_known = true;
	return true;
}

bool UpgradeBase::contain_upgrade( const shared_str& upgrade_id )
{
	return ( m_id._get() == upgrade_id._get() );
}

void UpgradeBase::fill_root_container( Root* root )
{
//!=R_ASSERT2( 0, make_string( "! Can`t fill <%s> in <UpgradeBase::fill_root_container> for root = %s", id_str(), root->id_str() ) );
	Groups_type::iterator ib = m_depended_groups.begin();
	Groups_type::iterator ie = m_depended_groups.end();
	for ( ; ib != ie ; ++ib )
	{
		(*ib)->fill_root( root );
	}
}

UpgradeStateResult UpgradeBase::can_install( CInventoryItem& item, bool loading )
{
	if ( !m_known && !loading )
	{
		if ( g_upgrades_log == 1 )
		{
			Msg( "- Upgrade <%s> (id = %d) is in mode <unknown>.", id_str(), item.object_id() );
		}
		return result_e_unknown;
	}

	if ( item.has_upgrade( m_id ) )
	{
		if ( g_upgrades_log == 1 )
		{
			Msg( "- Upgrade <%s> (id = %d) is installed already.", id_str(), item.object_id() );
		}
		/*if ( loading )
		{
			FATAL( make_string( "Loading item: Upgrade <%s> (id = %d) is installed already.", id_str(), item.object_id() ).c_str() );
		}*/
		return result_e_installed; // true
	}
	return result_ok;
}

} // namespace upgrade
} // namespace inventory
