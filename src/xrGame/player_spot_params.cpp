#include "StdAfx.h"
#include "player_spot_params.h"
#include "game_base.h"
#include "game_cl_capture_the_artefact.h"
#include "game_cl_artefacthunt.h"
#include "game_state_accumulator.h"

namespace award_system
{
u32 const calculate_spots(game_PlayerState* ps)
{
    return ps->m_iRivalKills - (ps->m_iTeamKills * 2) - ps->m_iSelfKills + (ps->af_count * 3);
};

player_spots_counter::player_spots_counter(game_state_accumulator* owner) : inherited(owner) {}
u32 const player_spots_counter::get_u32_param()
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (tmp_local_player)
        return calculate_spots(tmp_local_player);

    return 0;
}

u32 const player_spots_with_top_enemy_divider::get_top_enemy_player_score()
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return 0;

    s8 enemy_team = -1;
    switch (Game().Type())
    {
    case eGameIDCaptureTheArtefact:
    {
        if (tmp_local_player->team == etGreenTeam)
        {
            enemy_team = etBlueTeam;
        }
        else if (tmp_local_player->team == etBlueTeam)
        {
            enemy_team = etGreenTeam;
        }
        else
        {
            return 0;
        }
    }
    break;
    case eGameIDArtefactHunt:
    case eGameIDTeamDeathmatch:
    {
        game_cl_TeamDeathmatch* tmp_game = smart_cast<game_cl_TeamDeathmatch*>(Level().game);
        s16 player_team = tmp_game->ModifyTeam(tmp_local_player->team);
        if (player_team == etGreenTeam)
        {
            enemy_team = etBlueTeam + 1;
        }
        else if (player_team == etBlueTeam)
        {
            enemy_team = etGreenTeam + 1;
        }
        else
        {
            return 0;
        }
    }
    break;
    default: return 0;
    }; // switch (Game().Type())

    s32 max_score = 0;

    for (game_cl_GameState::PLAYERS_MAP_CIT i = Game().players.begin(), ie = Game().players.end(); i != ie; ++i)
    {
        s32 tmp_score = 0;
        game_PlayerState* tmp_player = i->second;
        if (tmp_player && (tmp_player->team == enemy_team))
        {
            tmp_score = calculate_spots(tmp_player);
        }
        if (max_score < tmp_score)
            max_score = tmp_score;
    }
    if (max_score < 0)
        max_score = 0;

    return max_score;
}

float const player_spots_with_top_enemy_divider::get_float_param()
{
    float my_spots_count = static_cast<float>(player_spots_counter::get_u32_param());
    u32 top_enemy = get_top_enemy_player_score();
    if (top_enemy == 0)
    {
        return my_spots_count;
    }
    return my_spots_count / top_enemy;
}

} // namespace award_system
