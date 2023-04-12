#pragma once
#include "game_cl_base.h"

class game_cl_Single : public game_cl_GameState
{
    typedef game_cl_GameState inherited;

public:
    game_cl_Single();
    virtual CUIGameCustom* createGameUI();
    virtual pcstr getTeamSection(int Team);
    virtual bool IsServerControlHits() { return true; };
    virtual ALife::_TIME_ID GetStartGameTime();
    virtual ALife::_TIME_ID GetGameTime();
    virtual float GetGameTimeFactor();
    virtual void SetGameTimeFactor(const float fTimeFactor);

    virtual ALife::_TIME_ID GetEnvironmentGameTime();
    virtual float GetEnvironmentGameTimeFactor();
    virtual void SetEnvironmentGameTimeFactor(const float fTimeFactor);
    virtual void SetEnvironmentGameTimeFactor(ALife::_TIME_ID GameTime, const float fTimeFactor) override;

    void OnDifficultyChanged();
};

// game difficulty
enum ESingleGameDifficulty : u32
{
    egdNovice = 0,
    egdStalker = 1,
    egdVeteran = 2,
    egdMaster = 3,
    egdCount
};

extern ESingleGameDifficulty g_SingleGameDifficulty;
extern const xr_token difficulty_type_token[];
