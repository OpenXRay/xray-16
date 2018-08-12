#include "StdAfx.h"
#include "player_state_multichampion.h"
#include "player_spot_params.h"
#include "game_cl_base.h"
#include "Level.h"
#include "game_state_accumulator.h"

namespace award_system
{
player_multichampion::player_multichampion(game_state_accumulator* owner) : inherited(owner)
{
    m_can_be_multichampion = false;
}

void player_multichampion::reset_game() { m_can_be_multichampion = false; }
void player_multichampion::OnRoundEnd() { m_can_be_multichampion = true; }
u32 const player_multichampion::get_u32_param()
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return 0;

    if (!m_can_be_multichampion)
        return 0;

    s32 max_score = calculate_spots(tmp_local_player);
    s16 min_death = tmp_local_player->m_iDeaths;
    u8 max_arts = tmp_local_player->af_count;

    game_PlayerState* max_score_player = tmp_local_player;
    game_PlayerState* max_arts_player = tmp_local_player;
    game_PlayerState* min_death_player = tmp_local_player;

    for (game_cl_GameState::PLAYERS_MAP_CIT i = Game().players.begin(), ie = Game().players.end(); i != ie; ++i)
    {
        s32 tmp_score = 0;
        s16 tmp_death = 0;
        u8 tmp_arts = 0;
        game_PlayerState* tmp_player = i->second;
        if (tmp_player)
        {
            tmp_score = calculate_spots(tmp_player);
            tmp_death = tmp_player->m_iDeaths;
            tmp_arts = tmp_player->af_count;
        }
        else
        {
            continue;
        }

        if (max_score < tmp_score)
        {
            max_score = tmp_score;
            max_score_player = tmp_player;
        }
        if (min_death > tmp_death)
        {
            min_death = tmp_death;
            min_death_player = tmp_player;
        }
        if (max_arts < tmp_arts)
        {
            max_arts = tmp_arts;
            max_arts_player = tmp_player;
        }
    }

    if ((min_death_player == tmp_local_player) && (max_score_player == tmp_local_player) &&
        (max_arts_player == tmp_local_player))
    {
        m_can_be_multichampion = false;
        return 1;
    }
    return 0;
}

} // namespace award_system
