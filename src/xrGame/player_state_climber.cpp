#include "StdAfx.h"
#include "player_state_climber.h"
#include "game_cl_base.h"
#include "Level.h"
#include "game_state_accumulator.h"

namespace award_system
{
player_state_climber::player_state_climber(game_state_accumulator* owner) : inherited(owner)
{
    m_player_is_climber = false;
}

u32 const player_state_climber::get_u32_param()
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return 0;

    return m_player_is_climber ? tmp_local_player->rank : 0;
}

void player_state_climber::OnPlayerRankChanged()
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    u8 max_rank = 0;
    u8 max_count = 0;
    game_PlayerState* max_player_rank = NULL;

    for (game_cl_GameState::PLAYERS_MAP_CIT i = Game().players.begin(), ie = Game().players.end(); i != ie; ++i)
    {
        u8 player_rank = i->second->rank;
        if (player_rank > max_rank)
        {
            max_player_rank = i->second;
            max_count = 1;
            max_rank = max_player_rank->rank;
        }
        else if (player_rank == max_rank)
        {
            ++max_count;
        }
    }
    if (max_count && (max_player_rank == tmp_local_player))
        m_player_is_climber = true;
}

} // namespace award_system
