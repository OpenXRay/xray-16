#include "StdAfx.h"
#include "string_table.h"
#include "Level.h"
#include "ui/TeamInfo.h"
#include "game_cl_capture_the_artefact.h"
#include "game_cl_capture_the_artefact_captions_manager.h"
#include "actor.h"

CTAGameClCaptionsManager::CTAGameClCaptionsManager()
    : m_can_spawn(false), dwLastTimeRemains(0)
{
    parent_game_ui = nullptr;
    parent_game_object = nullptr;
    m_press_fire2spect_showed = false;
    m_press_jump2payspaw_showed = false;
    m_can_show_payspawn = false;
    m_can_show_buy = false;
    m_winner_team = etSpectatorsTeam;
    xr_strcpy(timelimit_message, "00:00:00");
}

CTAGameClCaptionsManager::~CTAGameClCaptionsManager() {}
void CTAGameClCaptionsManager::Init(game_cl_CaptureTheArtefact* parent, CUIGameCTA* game_ui)
{
    parent_game_object = parent;
    VERIFY(parent_game_object);
    parent_game_ui = game_ui;
    VERIFY(parent_game_ui);
    dwLastTimeRemains = 10;
}

void CTAGameClCaptionsManager::ResetCaptions() { parent_game_ui->ResetCaptions(); }
void CTAGameClCaptionsManager::ShowInProgressCaptions()
{
    game_PlayerState* ps = parent_game_object->local_player;
    if (!ps)
        return;
    if (ps->IsSkip())
        return;
    if (parent_game_object->InWarmUp())
    {
        parent_game_ui->SetWarmUpCaption(warmup_message);
    }

    if (parent_game_object->HasTimeLimit())
        parent_game_ui->SetTimeMsgCaption(timelimit_message);

    IGameObject* control_entity = Level().CurrentControlEntity();
    if (!control_entity)
        return;

    CStringTable st;
    if (ps->team == static_cast<u8>(etSpectatorsTeam))
    {
        VERIFY(smart_cast<CSpectator*>(control_entity));
        parent_game_ui->SetPressJumpMsgCaption("mp_press_jump2select_team");
        CSpectator* pSpectator = smart_cast<CSpectator*>(control_entity);
        if (pSpectator)
        {
            string1024 SpectatorStr;
            pSpectator->GetSpectatorString(SpectatorStr);
            parent_game_ui->SetSpectatorMsgCaption(SpectatorStr);
        }
        return;
    }
    if (Level().IsDemoPlayStarted())
    {
        CSpectator* pSpectator = smart_cast<CSpectator*>(control_entity);
        if (pSpectator)
        {
            string1024 SpectatorStr;
            pSpectator->GetSpectatorString(SpectatorStr);
            parent_game_ui->SetSpectatorMsgCaption(SpectatorStr);
        }
        return;
    }

    if (m_can_show_buy)
    {
        parent_game_ui->SetPressBuyMsgCaption("mp_press_to_buy");
    }
    if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        if (smart_cast<CActor*>(Level().CurrentControlEntity()))
        {
            parent_game_ui->SetPressJumpMsgCaption("mp_press_fire2spectator");
        }
        else
        {
            if (m_can_show_payspawn)
                parent_game_ui->SetPressJumpMsgCaption("mp_press_jump2pay_spaw");
        }
        return;
    }
}

void CTAGameClCaptionsManager::ShowPendingCaptions() {}
void CTAGameClCaptionsManager::ShowScoreCaptions()
{
    VERIFY(m_winner_team != etSpectatorsTeam);
    CStringTable st;
    LPCSTR team_name = CTeamInfo::GetTeam_name(m_winner_team + 1);
    u32 win_str_size = st.translate("mp_team_wins").size() + xr_strlen(team_name) + 1;
    char* win_str = static_cast<char*>(_alloca(win_str_size));
    xr_sprintf(win_str, win_str_size, st.translate("mp_team_wins").c_str(), team_name);
    parent_game_ui->SetRoundResultCaption(win_str);
}

void CTAGameClCaptionsManager::ShowCaptions()
{
    if (!parent_game_object || !parent_game_ui)
        return;

    ResetCaptions();

    switch (parent_game_object->Phase())
    {
    case GAME_PHASE_PENDING:
    {
        ShowPendingCaptions();
        break;
    };
    case GAME_PHASE_INPROGRESS:
    {
        ShowInProgressCaptions();
        break;
    };
    case GAME_PHASE_PLAYER_SCORES:
    {
        ShowScoreCaptions();
        break;
    };
    };
}

void CTAGameClCaptionsManager::CanCallBuySpawn(bool can_call)
{
    VERIFY(parent_game_ui);
    m_can_show_payspawn = can_call;
}

void CTAGameClCaptionsManager::CanCallBuy(bool can_call)
{
    if (!parent_game_ui)
    {
        m_can_show_buy = false;
        return;
    }
    m_can_show_buy = can_call;
}

void CTAGameClCaptionsManager::CanSpawn(bool can_spawn)
{
    VERIFY(parent_game_ui);
    m_can_spawn = can_spawn;
}

void CTAGameClCaptionsManager::SetWinnerTeam(ETeam wteam) { m_winner_team = wteam; }
void CTAGameClCaptionsManager::ConvertTime2String(string64& str, u32 time)
{
    u32 RHour = time / 3600000;
    time %= 3600000;
    u32 RMinutes = time / 60000;
    time %= 60000;
    u32 RSecs = time / 1000;
    xr_sprintf(str, sizeof(str), "%02d:%02d:%02d", RHour, RMinutes, RSecs);
};

u32 CTAGameClCaptionsManager::SetWarmupTime(u32 current_warmup_time, u32 current_time)
{
    u32 ret_value = 0;
    if (current_time >= current_warmup_time)
    {
        current_warmup_time = current_time;
    }
    u32 time_remains = current_warmup_time - current_time;

    string64 time_str;
    CStringTable st;
    ConvertTime2String(time_str, time_remains);

    warmup_message[0] = 0; // bad style
    if (time_remains > 10000)
    {
        strconcat(sizeof(warmup_message), warmup_message, *st.translate("mp_time2start"), " ", time_str);
    }
    else
    {
        if (time_remains < 1000)
            strconcat(sizeof(warmup_message), warmup_message, *st.translate("mp_go"), "");
        else
        {
            u32 dwCurTimeRemains = time_remains / 1000;
            if (dwLastTimeRemains != dwCurTimeRemains)
            {
                if (dwCurTimeRemains > 0 && dwCurTimeRemains <= 5)
                {
                    ret_value = dwCurTimeRemains;
                }
            }
            dwLastTimeRemains = dwCurTimeRemains;
            xr_itoa(dwCurTimeRemains, time_str, 10);
            strconcat(sizeof(warmup_message), warmup_message, *st.translate("mp_ready"), "...", time_str);
        }
    };
    return ret_value;
}

void CTAGameClCaptionsManager::SetTimeLimit(u32 time_limit, u32 current_time)
{
    if (current_time < time_limit)
    {
        u32 Rest = time_limit - current_time;
        ConvertTime2String(timelimit_message, Rest);
    }
    else
    {
        if (parent_game_ui)
            parent_game_ui->SetTimeMsgCaption("00:00:00");
    }
}
