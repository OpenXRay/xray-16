#include "StdAfx.h"
#include "player_state_marksman.h"
#include "kills_store.h"
#include "game_cl_base.h"
#include "Level.h"
#include "game_state_accumulator.h"
#include "Actor.h"

namespace award_system
{
player_state_marksman::player_state_marksman(game_state_accumulator* owner) : inherited(owner) { m_spawn_time = 0; }
u32 const player_state_marksman::get_u32_param() { return m_sniper_victims.size(); }
void player_state_marksman::reset_game()
{
    m_spawn_time = 0;
    m_sniper_victims.clear();
}

void player_state_marksman::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if (!m_spawn_time)
        return;

    if (killer_id != tmp_local_player->GameID)
        return;

    u16 weapon_gid = m_owner->get_active_weapon_of_player(tmp_local_player);
    if (!m_owner->is_item_in_group(weapon_gid, ammunition_group::gid_sniper_rifels))
        return;

    game_PlayerState* victim_player = Game().GetPlayerByGameID(target_id);
    if (!victim_player)
        return;

    u16 victim_weapon_gid = m_owner->get_active_weapon_of_player(victim_player);
    if (!m_owner->is_item_in_group(victim_weapon_gid, ammunition_group::gid_sniper_rifels))
        return;

    CActor* killer_actor = m_owner->get_players_actor(killer_id);
    CActor* victim_actor = m_owner->get_players_actor(target_id);

    if (killer_actor && victim_actor)
    {
        float kill_dist = killer_actor->Position().distance_to(victim_actor->Position());
        if (kill_dist >= max_kill_dist)
        {
            m_sniper_victims.insert(std::make_pair(victim_actor->cName(), kill_dist));
        }
    }
}

void player_state_marksman::OnPlayerSpawned(game_PlayerState const* ps)
{
    if (ps == m_owner->get_local_player())
    {
        m_spawn_time = Device.dwTimeGlobal;
        m_sniper_victims.clear();
    }
}

} // namespace award_system
