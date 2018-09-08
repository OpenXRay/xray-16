#include "StdAfx.h"
#include "player_state_toughy.h"
#include "game_cl_base.h"
#include "game_state_accumulator.h"
#include "Actor.h"
#include "ActorCondition.h"

namespace award_system
{
player_state_toughy::player_state_toughy(game_state_accumulator* owner) : inherited(owner) { m_kills_count = 0; }
u32 const player_state_toughy::get_u32_param() { return m_kills_count; }
void player_state_toughy::reset_game() { m_kills_count = 0; }
void player_state_toughy::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();

    if (!tmp_local_player)
        return;

    if (killer_id != tmp_local_player->GameID)
        return;

    u16 weapon_gid = m_owner->get_object_id(weapon_id);
    if (!m_owner->is_item_in_group(weapon_gid, ammunition_group::gid_pistols) &&
        !m_owner->is_item_in_group(weapon_gid, ammunition_group::gid_assault) &&
        !m_owner->is_item_in_group(weapon_gid, ammunition_group::gid_shotguns) &&
        !m_owner->is_item_in_group(weapon_gid, ammunition_group::gid_knife))
    {
        return;
    }

    CActor* tmp_actor = m_owner->get_players_actor(tmp_local_player->GameID);
    if (!tmp_actor)
        return;

    if (tmp_actor->conditions().IsLimping())
    {
        ++m_kills_count;
    }
}

void player_state_toughy::OnPlayerSpawned(game_PlayerState const* ps)
{
    if (m_owner->get_local_player() == ps)
        reset_game();
}
}
