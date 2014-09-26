////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_upgrade_base.h
//	Created 	: 19.10.2007
//  Modified 	: 27.11.2007
//	Author		: Dmitriy Iassenev, Evgeniy Sokolov
//	Description : inventory upgrade base class
////////////////////////////////////////////////////////////////////////////

#ifndef INVENTORY_UPGRADE_BASE_H_INCLUDED
#define INVENTORY_UPGRADE_BASE_H_INCLUDED

#include <boost/noncopyable.hpp>
#include "object_broker.h"

class CInventoryItem;

namespace inventory
{
namespace upgrade
{

class Manager;
class Group;
class Root;
class Upgrade;
class Property;

// can_install
enum UpgradeStateResult
{
	result_ok,
	result_e_unknown,
	result_e_installed,
	result_e_parents,
	result_e_group,
	result_e_precondition_money,
	result_e_precondition_quest,

	result_count
};

class UpgradeBase :
	private	boost::noncopyable
{
public:
							UpgradeBase();
	virtual					~UpgradeBase();
				void		construct( const shared_str& upgrade_id, Manager& manager_r );
	
	IC	 const	shared_str& id() const;
	IC			LPCSTR		id_str() const;
	IC			bool		is_known() const;

#ifdef DEBUG
	virtual		void		log_hierarchy( LPCSTR nest );
//	virtual		void		test_all_upgrades( CInventoryItem& item );
#endif // DEBUG

	virtual		bool		is_root();
				bool		make_known();
	virtual		bool		contain_upgrade( const shared_str& upgrade_id );
	virtual		void		fill_root_container( Root* root );

	virtual		UpgradeStateResult	can_install( CInventoryItem& item, bool loading );

	virtual		void		highlight_up() {};
	virtual		void		highlight_down() {};

protected:
	typedef xr_vector<Group*>	Groups_type;

private:
	shared_str			m_id;

protected:
	bool				m_known;
	Groups_type			m_depended_groups;

protected:
	void				add_dependent_groups( LPCSTR groups_str, Manager& manager_r );

}; // class UpgradeBase

} // namespace upgrade
} // namespace inventory

#include "inventory_upgrade_base_inline.h"

#endif // INVENTORY_UPGRADE_BASE_H_INCLUDED