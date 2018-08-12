#include "StdAfx.h"
#include "sprinter_stopper.h"
#include "game_state_accumulator.h"
#include "game_cl_base.h"
#include "Level.h"
#include "ammunition_groups.h"
#include "Actor.h"

namespace award_system
{
spritnter_stopper::spritnter_stopper(game_state_accumulator* owner) : inherited(owner)
{
    m_sprinter_victim_velocity = 0.0f;
}

void spritnter_stopper::reset_game() { m_sprinter_victim_velocity = 0.0f; }
float const spritnter_stopper::get_float_param() { return m_sprinter_victim_velocity; }
void spritnter_stopper::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if (tmp_local_player->GameID != killer_id)
        return;

    u16 weapon_gid = m_owner->get_active_weapon_of_player(tmp_local_player);
    if (!m_owner->is_item_in_group(weapon_gid, ammunition_group::gid_sniper_rifels))
        return;

    CActor* victim_actor = m_owner->get_players_actor(target_id);
    if (!victim_actor)
        return;

    CEntity::SEntityState state;
    victim_actor->g_State(state);

    m_sprinter_victim_velocity = state.fVelocity;
}

} // namespace award_system
