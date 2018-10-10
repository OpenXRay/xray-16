#include "StdAfx.h"
#include "player_state_params.h"
#include "game_cl_base.h"
#include "game_state_accumulator.h"
#include "Actor.h"

namespace award_system
{
player_death_counter::player_death_counter(game_state_accumulator* owner) : inherited(owner) {}
u32 const player_death_counter::get_u32_param()
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    u32 death_count = tmp_local_player ? static_cast<u32>(tmp_local_player->m_iDeaths) : u32(-1);
    return death_count;
}

// player_artdeliver_counter

player_artdeliver_counter::player_artdeliver_counter(game_state_accumulator* owner) : inherited(owner) {}
u32 const player_artdeliver_counter::get_u32_param()
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    u32 arts_count = tmp_local_player ? static_cast<u32>(tmp_local_player->af_count) : 0;
    return arts_count;
}

// player_rawkill_counter

player_rawkill_counter::player_rawkill_counter(game_state_accumulator* owner) : inherited(owner) { m_raw_kills = 0; }
u32 const player_rawkill_counter::get_u32_param()
{
    // u32 rawkill_count = m_local_player ? m_local_player->m_iKillsInRowCurr : 0;
    return m_raw_kills;
}

void player_rawkill_counter::reset_game() { m_raw_kills = 0; };
void player_rawkill_counter::OnPlayerSpawned(game_PlayerState const* ps)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if (ps == tmp_local_player)
    {
        m_raw_kills = 0;
    }
}

void player_rawkill_counter::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if (killer_id == tmp_local_player->GameID)
    {
        ++m_raw_kills;
    }
}

player_state_move::player_state_move(game_state_accumulator* owner) : inherited(owner) {}
u32 const player_state_move::get_u32_param()
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return 0;

    CActor* tmp_actor = m_owner->get_players_actor(tmp_local_player->GameID);
    if (!tmp_actor)
        return 0;

    CEntity::SEntityState state;
    tmp_actor->g_State(state);

    return state.bCrouch | (state.bSprint << 1) | (state.bJump << 2) | (state.bFall << 3);
}

player_state_velocity::player_state_velocity(game_state_accumulator* owner) : inherited(owner) {}
float const player_state_velocity::get_float_param()
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return 0.0f;

    CActor* tmp_actor = m_owner->get_players_actor(tmp_local_player->GameID);
    if (!tmp_actor)
        return 0.0f;

    CEntity::SEntityState state;
    tmp_actor->g_State(state);

    return state.fVelocity;
}

player_state_ang_velocity::player_state_ang_velocity(game_state_accumulator* owner) : inherited(owner) {}
float const player_state_ang_velocity::get_float_param()
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return 0.0f;

    CActor* tmp_actor = m_owner->get_players_actor(tmp_local_player->GameID);
    if (!tmp_actor)
        return 0.0f;

    CEntity::SEntityState state;
    tmp_actor->g_State(state);

    return state.fAVelocity;
}

} // namespace award_system
