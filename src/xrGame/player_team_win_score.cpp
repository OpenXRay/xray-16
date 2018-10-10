#include "StdAfx.h"
#include "player_team_win_score.h"
#include "game_cl_capture_the_artefact.h"
#include "game_cl_artefacthunt.h"
#include "game_state_accumulator.h"

namespace award_system
{
player_team_win_score::player_team_win_score(game_state_accumulator* owner) : inherited(owner) { m_win_score = 0; };
void player_team_win_score::reset_game() { m_win_score = 0; }
void player_team_win_score::OnRoundStart() { reset_game(); }
void player_team_win_score::OnRoundEnd() { save_round_scores(); }
void player_team_win_score::save_round_scores()
{
    m_green_team_score = 0;
    m_blue_team_score = 0;
    m_player_team = etSpectatorsTeam;

    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    switch (Game().Type())
    {
    case eGameIDCaptureTheArtefact:
    {
        game_cl_CaptureTheArtefact* tmp_game = smart_cast<game_cl_CaptureTheArtefact*>(Level().game);
        m_green_team_score = tmp_game->GetGreenTeamScore();
        m_blue_team_score = tmp_game->GetBlueTeamScore();
        m_player_team = tmp_local_player->team;
    }
    break;
    case eGameIDArtefactHunt:
    case eGameIDTeamDeathmatch:
    {
        game_cl_TeamDeathmatch* tmp_game = smart_cast<game_cl_TeamDeathmatch*>(Level().game);
        m_green_team_score = tmp_game->GetGreenTeamScore();
        m_blue_team_score = tmp_game->GetBlueTeamScore();
        if (tmp_local_player->team > 0)
        {
            m_player_team = static_cast<u8>(tmp_game->ModifyTeam(tmp_local_player->team));
        }
    }
    break;
    case eGameIDDeathmatch:
    {
        game_cl_Deathmatch* tmp_game = smart_cast<game_cl_Deathmatch*>(Level().game);
        if (!xr_strcmp(tmp_local_player->getName(), tmp_game->WinnerName))
        {
            m_win_score = tmp_local_player->m_iRivalKills;
            return;
        }
    }
    break;
    }; // switch (Game().Type())
    if (static_cast<ETeam>(m_player_team) == etGreenTeam)
    {
        m_win_score = (m_green_team_score > m_blue_team_score) ? m_green_team_score : 0;
    }
    else if (static_cast<ETeam>(m_player_team) == etBlueTeam)
    {
        m_win_score = (m_blue_team_score > m_green_team_score) ? m_blue_team_score : 0;
    }
}

player_enemy_team_score::player_enemy_team_score(game_state_accumulator* owner) : inherited(owner)
{
    m_enemy_team_score = 0;
}

void player_enemy_team_score::reset_game()
{
    inherited::reset_game();
    m_enemy_team_score = 0;
}

void player_enemy_team_score::OnRoundEnd() { save_round_scores(); }
void player_enemy_team_score::save_round_scores()
{
    inherited::save_round_scores();
    if (static_cast<ETeam>(m_player_team) == etGreenTeam)
    {
        m_enemy_team_score = m_blue_team_score;
    }
    else if (static_cast<ETeam>(m_player_team) == etBlueTeam)
    {
        m_enemy_team_score = m_green_team_score;
    }
}

player_runtime_win_score::player_runtime_win_score(game_state_accumulator* owner) : inherited(owner) {}
u32 const player_runtime_win_score::get_u32_param()
{
    u32 ret_score = 0;
    if (static_cast<ETeam>(m_player_team) == etGreenTeam)
    {
        ret_score = m_green_team_score;
    }
    else if (static_cast<ETeam>(m_player_team) == etBlueTeam)
    {
        ret_score = m_blue_team_score;
    }
    return ret_score;
}

void player_runtime_win_score::OnPlayerBringArtefact(game_PlayerState const* ps) { save_round_scores(); }
player_runtime_enemy_team_score::player_runtime_enemy_team_score(game_state_accumulator* owner) : inherited(owner) {}
void player_runtime_enemy_team_score::OnPlayerBringArtefact(game_PlayerState const* ps) { save_round_scores(); }
} // namespace award_system
