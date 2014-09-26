////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_upgrade_root.h
//	Created 	: 19.10.2007
//  Modified 	: 27.11.2007
//	Author		: Evgeniy Sokolov
//	Description : inventory upgrade root class
////////////////////////////////////////////////////////////////////////////

#ifndef INVENTORY_UPGRADE_ROOT_H_INCLUDED
#define INVENTORY_UPGRADE_ROOT_H_INCLUDED

#include "inventory_upgrade_manager.h"

namespace inventory
{
namespace upgrade
{
	
class Root : public UpgradeBase
{
private:
	typedef		UpgradeBase				inherited;
	typedef		xr_vector<Upgrade*>		Upgrades_vec;

public:
							Root();
	virtual					~Root();
				void		construct( const shared_str& root_id, Manager& manager_r );
	IC			LPCSTR		scheme() const;

				void		add_upgrade( Upgrade* upgr );
	virtual		bool		is_root();

#ifdef DEBUG
	virtual		void		log_hierarchy( LPCSTR nest );
				void		test_all_upgrades( CInventoryItem& item );
#endif // DEBUG

	virtual		bool		contain_upgrade( const shared_str& upgrade_id );
				bool		verify_scheme_index( const Ivector2& scheme_index );
				Upgrade*	get_upgrade_by_index( Ivector2 const& index );

				void		highlight_hierarchy( shared_str const& upgrade_id );
				void		reset_highlight();

protected:
	shared_str				m_upgrade_scheme;
	Upgrades_vec			m_contained_upgrades;

}; // class Root

} // namespace upgrade
} // namespace inventory

#include "inventory_upgrade_root_inline.h"

#endif // INVENTORY_UPGRADE_ROOT_H_INCLUDED
