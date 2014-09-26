////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_upgrade_group.h
//	Created 	: 22.10.2007
//  Modified 	: 27.11.2007
//	Author		: Evgeniy Sokolov 
//	Description : inventory upgrade group class
////////////////////////////////////////////////////////////////////////////

#ifndef INVENTORY_UPGRADE_GROUP_H_INCLUDED
#define INVENTORY_UPGRADE_GROUP_H_INCLUDED

#include "inventory_upgrade_manager.h"

namespace inventory
{
namespace upgrade
{

class Group : private boost::noncopyable
{
public:
							Group();
	virtual					~Group();
				void		construct( const shared_str& group_id, UpgradeBase& parent_upgrade, Manager& manager_r );
				void		add_parent_upgrade( UpgradeBase& parent_upgrade );

	IC	 const	shared_str& id() const;
	IC			LPCSTR		id_str() const;

#ifdef DEBUG
				void		log_hierarchy( LPCSTR nesting );
#endif // DEBUG

				void		fill_root( Root* root );

				UpgradeStateResult	can_install( CInventoryItem& item, UpgradeBase& test_upgrade, bool loading );
				
				void		highlight_up();
				void		highlight_down();

private:
	typedef xr_vector<UpgradeBase*>		Upgrades_type;

private:
	shared_str				m_id;

	Upgrades_type			m_parent_upgrades;
	Upgrades_type			m_included_upgrades;

}; // class group

} // namespace upgrade
} // namespace inventory

#include "inventory_upgrade_group_inline.h"

#endif // INVENTORY_UPGRADE_GROUP_H_INCLUDED
