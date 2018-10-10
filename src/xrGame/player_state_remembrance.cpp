#include "StdAfx.h"
#include "player_state_remembrance.h"
#include "kills_store.h"
#include "game_cl_base.h"
#include "Level.h"
#include "game_state_accumulator.h"
#include "Actor.h"

namespace award_system
{
player_state_remembrance::player_state_remembrance(game_state_accumulator* owner) : inherited(owner)
{
    m_is_remembrance = false;
}

u32 const player_state_remembrance::get_u32_param() { return m_is_remembrance ? 1 : 0; }
void player_state_remembrance::reset_game() { m_is_remembrance = false; }
void player_state_remembrance::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if (killer_id != tmp_local_player->GameID)
        return;

    u16 kill_weapon_id = m_owner->get_object_id(weapon_id);

    if (!m_owner->is_item_in_group(kill_weapon_id, ammunition_group::gid_hand_grenades))
        return;

    if (!tmp_local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
        return;

    game_PlayerState* victim_player = Game().GetPlayerByGameID(target_id);
    if (!victim_player)
        return;

    CActor* killer_actor = m_owner->get_players_actor(killer_id);
    CActor* victim_actor = m_owner->get_players_actor(target_id);

    if (!killer_actor || !victim_actor)
        return;

    float kill_dist = killer_actor->Position().distance_to(victim_actor->Position());

    if (kill_dist <= max_kill_dist)
    {
        m_is_remembrance = true;
    }
}

} // namespace award_system
