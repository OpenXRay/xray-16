#include "StdAfx.h"
#include "player_state_achilles_heel.h"
#include "game_state_accumulator.h"
#include "game_cl_base.h"
#include "Level.h"
#include "ammunition_groups.h"

namespace award_system
{
achilles_heel_kill::achilles_heel_kill(game_state_accumulator* owner) : inherited(owner)
{
    m_achilles_kill_was = false;
}

u32 const achilles_heel_kill::get_u32_param() { return m_achilles_kill_was ? 1 : 0; }
void achilles_heel_kill::reset_game() { m_achilles_kill_was = false; }
void achilles_heel_kill::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if (tmp_local_player->GameID != killer_id)
        return;

    if (kill_type.second != SKT_EYESHOT)
        return;

    u16 killer_armor = m_owner->get_armor_of_player(tmp_local_player);
    if (killer_armor)
        return;

    u16 weapon_gid = m_owner->get_active_weapon_of_player(tmp_local_player);
    if (!m_owner->is_item_in_group(weapon_gid, ammunition_group::gid_pistols))
        return;

    game_PlayerState* victim_player = Game().GetPlayerByGameID(target_id);
    if (!victim_player)
        return;

    u16 victim_armor = m_owner->get_armor_of_player(victim_player);
    if (!victim_armor)
        return;

    u16 victim_weapon = m_owner->get_active_weapon_of_player(victim_player);
    if (m_owner->is_item_in_group(victim_armor, ammunition_group::gid_exo_outfit) &&
        m_owner->is_item_in_group(victim_weapon, ammunition_group::gid_cool_weapons))
    {
        m_achilles_kill_was = true;
    }
}

} // namespace award_system
