#ifndef AMMUNITION_GROUPS_INCLUDED
#define AMMUNITION_GROUPS_INCLUDED

#include "../xrServerEntities/associative_vector.h"

class CItemMgr;

namespace award_system
{

class ammunition_group
{
public:
	enum enum_group_id
	{
		gid_knife				= 0x00,
		gid_pistols,
		gid_assault,
		gid_shotguns,
		gid_sniper_rifels,
		gid_gauss_rifle,
		gid_heavy_weapons,
		gid_exo_outfit,
		gid_double_barred,
		gid_hand_grenades,
		gid_cool_weapons,
	};//enum_group_id
	static u16 const gid_any	= u16(-1);
			
			ammunition_group		();
			~ammunition_group		();

	void	init					(CItemMgr const * item_manager);
	bool	is_item_in_group		(u16 item_id, enum_group_id gid) const;
private:
	void	init_ammunition_group	(CItemMgr const * item_manager,
									 enum_group_id gid,
									 shared_str const & weapons_string);

	typedef xr_vector<std::pair<u16, enum_group_id> >	ammun_groups_map_t;
	ammun_groups_map_t				m_wpn_groups;
};

} //namespace award_system

#endif //#ifndef AMMUNITION_GROUPS_INCLUDED