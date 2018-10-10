#include "StdAfx.h"
#include "ammunition_groups.h"
#include "ui/UIBuyWndShared.h"
#include "xrCore/xr_ini.h"

namespace award_system
{
ammunition_group::ammunition_group() {}
ammunition_group::~ammunition_group() {}
void ammunition_group::init_ammunition_group(
    CItemMgr const* item_manager, enum_group_id gid, shared_str const& weapons_string)
{
    string256 dststr;
    u32 count = _GetItemCount(weapons_string.c_str());
    for (u32 i = 0; i < count; ++i)
    {
        _GetItem(weapons_string.c_str(), i, dststr);
        u32 itm_index = item_manager->GetItemIdx(dststr);
        if (itm_index != u32(-1))
        {
            VERIFY((itm_index & 0xffff0000) == 0);
            m_wpn_groups.push_back(std::make_pair(static_cast<u16>(itm_index), gid));
        }
    };
}

static char const* dm_team_data = "deathmatch_team0";

void ammunition_group::init(CItemMgr const* item_manager)
{
    m_wpn_groups.clear();

    shared_str tmp_string;

    init_ammunition_group(item_manager, gid_knife, "mp_wpn_knife");

    tmp_string = pSettings->r_string(dm_team_data, "pistols");
    init_ammunition_group(item_manager, gid_pistols, tmp_string.c_str());

    tmp_string = pSettings->r_string(dm_team_data, "shotgun");
    init_ammunition_group(item_manager, gid_shotguns, tmp_string.c_str());

    tmp_string = pSettings->r_string(dm_team_data, "assault");
    init_ammunition_group(item_manager, gid_assault, tmp_string.c_str());

    tmp_string = pSettings->r_string(dm_team_data, "sniper_rifles");
    init_ammunition_group(item_manager, gid_sniper_rifels, tmp_string.c_str());

    tmp_string = pSettings->r_string(dm_team_data, "heavy_weapons");
    init_ammunition_group(item_manager, gid_heavy_weapons, tmp_string.c_str());

    tmp_string = pSettings->r_string(dm_team_data, "gid_exo_outfit");
    init_ammunition_group(item_manager, gid_exo_outfit, tmp_string.c_str());

    tmp_string = pSettings->r_string(dm_team_data, "gid_gauss_rifle");
    init_ammunition_group(item_manager, gid_gauss_rifle, tmp_string.c_str());

    tmp_string = pSettings->r_string(dm_team_data, "gid_double_barred");
    init_ammunition_group(item_manager, gid_double_barred, tmp_string.c_str());

    tmp_string = pSettings->r_string(dm_team_data, "gid_hand_grenades");
    init_ammunition_group(item_manager, gid_hand_grenades, tmp_string.c_str());

    tmp_string = pSettings->r_string(dm_team_data, "gid_cool_weapons");
    init_ammunition_group(item_manager, gid_cool_weapons, tmp_string.c_str());
}

bool ammunition_group::is_item_in_group(u16 item_id, enum_group_id gid) const
{
    if (gid == gid_any)
        return true;

    ammun_groups_map_t::const_iterator tmp_iter =
        std::find(m_wpn_groups.begin(), m_wpn_groups.end(), ammun_groups_map_t::value_type(item_id & 0x00ff, gid));

    if (tmp_iter == m_wpn_groups.end())
        return false;

    return true;
}

} // namespace award_system
