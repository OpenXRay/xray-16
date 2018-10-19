#ifndef PLAYER_TEAM_WIN_SCORE_INCLUDED
#define PLAYER_TEAM_WIN_SCORE_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class player_team_win_score : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_team_win_score(game_state_accumulator* owner);
    virtual ~player_team_win_score(){};

    virtual void update(){};
    virtual u32 const get_u32_param() { return m_win_score; };
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game();

    virtual void OnRoundEnd();
    virtual void OnRoundStart();

protected:
    void save_round_scores();
    u32 m_win_score;

    s32 m_green_team_score;
    s32 m_blue_team_score;
    u8 m_player_team;
}; // class player_team_win_score

class player_enemy_team_score : public player_team_win_score
{
    typedef player_team_win_score inherited;

public:
    player_enemy_team_score(game_state_accumulator* owner);
    virtual ~player_enemy_team_score(){};

    virtual u32 const get_u32_param() { return m_enemy_team_score; };
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game();

    virtual void OnRoundEnd();

protected:
    void save_round_scores();
    u32 m_enemy_team_score;

}; // class player_enemy_team_score

class player_runtime_win_score : public player_team_win_score
{
    typedef player_team_win_score inherited;

public:
    player_runtime_win_score(game_state_accumulator* owner);
    virtual ~player_runtime_win_score(){};
    virtual u32 const get_u32_param();

    virtual void OnPlayerBringArtefact(game_PlayerState const* ps);
}; // class player_runtime_win_score

class player_runtime_enemy_team_score : public player_enemy_team_score
{
    typedef player_enemy_team_score inherited;

public:
    player_runtime_enemy_team_score(game_state_accumulator* owner);
    virtual ~player_runtime_enemy_team_score(){};

    virtual void OnPlayerBringArtefact(game_PlayerState const* ps);
}; // class player_runtime_enemy_team_score

} // namespace award_system

#endif //#ifndef PLAYER_TEAM_WIN_SCORE_INCLUDED
