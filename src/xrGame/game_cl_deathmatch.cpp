#include "stdafx.h"
#include "game_cl_deathmatch.h"
#include "xrMessages.h"
#include "UIGameDM.h"
#include "Spectator.h"
#include "Level.h"
#include "xr_level_controller.h"
#include "actor.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UISkinSelector.h"
#include "ui/UIPdaWnd.h"
#include "ui/UIMapDesc.h"
#include "ui/UIMessageBoxEx.h"
#include "ui/UIVote.h"
#include "gamepersistent.h"
#include "string_table.h"
#include "map_manager.h"
#include "map_location.h"
#include "clsid_game.h"
#include "ui/UIActorMenu.h"
#include "weapon.h"

#include "game_cl_base_weapon_usage_statistic.h"
#include "reward_event_generator.h"

#include "game_cl_deathmatch_snd_messages.h"
#include "game_base_menu_events.h"

#include "ActorCondition.h"

#ifdef _new_buy_wnd
#include "ui\UIMpTradeWnd.h"
#else
#include "ui\UIBuyWnd.h"
#endif

#define TEAM0_MENU "deathmatch_team0"

game_cl_Deathmatch::game_cl_Deathmatch()
{
    pCurBuyMenu = NULL;

    PresetItemsTeam0.clear();
    PlayerDefItems.clear();
    pCurPresetItems = NULL;
    ;

    pCurSkinMenu = NULL;

    m_bBuyEnabled = TRUE;

    m_bSkinSelected = FALSE;

    m_game_ui = NULL;

    m_iCurrentPlayersMoney = 0;
    Actor_Spawn_Effect = "";

    LoadSndMessages();
    m_cl_dwWarmUp_Time = 0;
    m_bMenuCalledFromReady = FALSE;
    m_bFirstRun = TRUE;
}

void game_cl_Deathmatch::Init()
{
    LoadTeamData(TEAM0_MENU);

    if (pSettings->line_exist("deathmatch_gamedata", "actor_spawn_effect"))
        Actor_Spawn_Effect = pSettings->r_string("deathmatch_gamedata", "actor_spawn_effect");
}

game_cl_Deathmatch::~game_cl_Deathmatch()
{
    PresetItemsTeam0.clear();
    PlayerDefItems.clear();

    xr_delete(pCurBuyMenu);
    xr_delete(pCurSkinMenu);
}

void game_cl_Deathmatch::SetGameUI(CUIGameCustom* uigame)
{
    inherited::SetGameUI(uigame);
    m_game_ui = smart_cast<CUIGameDM*>(uigame);
    R_ASSERT(m_game_ui);
};

CUIGameCustom* game_cl_Deathmatch::createGameUI()
{
    if (GEnv.isDedicatedServer)
        return NULL;

    CLASS_ID clsid = CLSID_GAME_UI_DEATHMATCH;
    m_game_ui = smart_cast<CUIGameDM*>(NEW_INSTANCE(clsid));
    R_ASSERT(m_game_ui);
    m_game_ui->Load();
    m_game_ui->SetClGame(this);
    return m_game_ui;
}

void game_cl_Deathmatch::SetCurrentSkinMenu()
{
    if (!pCurSkinMenu)
        pCurSkinMenu = InitSkinMenu(0);
}

void game_cl_Deathmatch::net_import_state(NET_Packet& P)
{
    inherited::net_import_state(P);

    m_s32FragLimit = P.r_s32();
    m_s32TimeLimit = P.r_s32() * 60000;
    m_u32ForceRespawn = P.r_u32() * 1000;
    m_cl_dwWarmUp_Time = P.r_u32();
    m_bDamageBlockIndicators = !!P.r_u8();
    // Teams
    u16 t_count;
    P.r_u16(t_count);
    teams.clear();

    for (u16 t_it = 0; t_it < t_count; ++t_it)
    {
        game_TeamState ts;
        P.r(&ts, sizeof(game_TeamState));
        teams.push_back(ts);
    };

    switch (Phase())
    {
    case GAME_PHASE_PLAYER_SCORES:
    {
        P.r_stringZ(WinnerName);
        bool NeedSndMessage = (xr_strlen(WinnerName) != 0);
        if (NeedSndMessage && local_player && !xr_strcmp(WinnerName, local_player->getName()))
        {
            PlaySndMessage(ID_YOU_WON);
        }
        if (NeedSndMessage && m_reward_generator)
        {
            m_reward_generator->OnRoundEnd();
            m_reward_generator->CommitBestResults();
        }
    }
    break;
    }
}

void game_cl_Deathmatch::net_import_update(NET_Packet& P)
{
    inherited::net_import_update(P);
    //-----------------------------------
    if (pCurBuyMenu && local_player)
    {
        if (local_player->rank != pCurBuyMenu->GetRank() && !pCurBuyMenu->IsIgnoreMoneyAndRank())
        {
            pCurBuyMenu->SetRank(local_player->rank);
            LoadDefItemsForRank(pCurBuyMenu);
            ChangeItemsCosts(pCurBuyMenu);
        }
    }
}

IBuyWnd* game_cl_Deathmatch::InitBuyMenu(const shared_str& BasePriceSection, s16 Team)
{
    if (Team == -1)
    {
        Team = local_player->team;
    };

    cl_TeamStruct* pTeamSect = &TeamList[ModifyTeam(Team)];

    IBuyWnd* pMenu = new BUY_WND_TYPE();
    pMenu->Init(pTeamSect->caSection, BasePriceSection);
    return pMenu;
};

CUISkinSelectorWnd* game_cl_Deathmatch::InitSkinMenu(s16 Team)
{
    if (Team == -1)
    {
        Team = local_player->team;
    };

    cl_TeamStruct* pTeamSect = &TeamList[ModifyTeam(Team)];

    CUISkinSelectorWnd* pMenu = new CUISkinSelectorWnd((char*)pTeamSect->caSection.c_str(), Team);
    return pMenu;
};

void game_cl_Deathmatch::OnMapInfoAccept()
{
    if (CanCallSkinMenu())
        pCurSkinMenu->ShowDialog(true);
};

void game_cl_Deathmatch::OnSkinMenuBack() { m_game_ui->ShowServerInfo(); };
void game_cl_Deathmatch::OnSkinMenu_Ok()
{
    IGameObject* l_pObj = Level().CurrentEntity();

    CGameObject* l_pPlayer = smart_cast<CGameObject*>(l_pObj);
    if (!l_pPlayer)
        return;

    NET_Packet P;
    l_pPlayer->u_EventGen(P, GE_GAME_EVENT, l_pPlayer->ID());
    P.w_u16(GAME_EVENT_PLAYER_GAME_MENU);
    P.w_u8(PLAYER_CHANGE_SKIN);

    P.w_s8((u8)pCurSkinMenu->GetActiveIndex());
    l_pPlayer->u_EventSend(P);
    //-----------------------------------------------------------------
    m_bSkinSelected = TRUE;
};

void game_cl_Deathmatch::OnSkinMenu_Cancel()
{
    if (!m_bSkinSelected && !m_bSpectatorSelected)
    {
        if (CanCallSkinMenu() && !pCurSkinMenu->IsShown())
        {
            pCurSkinMenu->ShowDialog(true);
            return;
        }
    }
    m_bMenuCalledFromReady = FALSE;
};

BOOL game_cl_Deathmatch::CanCallBuyMenu()
{
    if (Phase() != GAME_PHASE_INPROGRESS)
        return false;

    if (!is_buy_menu_ready())
        return FALSE;

    if (Level().CurrentEntity() && !smart_cast<CSpectator*>(Level().CurrentEntity()))
    {
        return FALSE;
    };
    if (!m_bSkinSelected || m_bSpectatorSelected)
        return FALSE;
    if (pCurSkinMenu && pCurSkinMenu->IsShown())
    {
        return FALSE;
    };
    if (m_game_ui && m_game_ui->GetActorMenu().IsShown())
    {
        return FALSE;
    }
    /*if (m_game_ui->m_pInventoryMenu && m_game_ui->m_pInventoryMenu->IsShown())
    {
        return FALSE;
    };*/
    return m_bBuyEnabled;
};

BOOL game_cl_Deathmatch::CanCallSkinMenu()
{
    if (Phase() != GAME_PHASE_INPROGRESS)
        return false;
    if (m_game_ui && m_game_ui->GetActorMenu().IsShown())
    {
        return FALSE;
    }
    /*if (m_game_ui->m_pInventoryMenu && m_game_ui->m_pInventoryMenu->IsShown())
    {
        return FALSE;
    };*/
    if (pCurBuyMenu && pCurBuyMenu->IsShown())
    {
        return FALSE;
    };
    SetCurrentSkinMenu();
    if (!pCurSkinMenu)
        return FALSE;
    if (!pCurSkinMenu->IsShown())
        pCurSkinMenu->SetCurSkin(local_player->skin);
    return TRUE;
};

BOOL game_cl_Deathmatch::CanCallInventoryMenu()
{
    if (Phase() != GAME_PHASE_INPROGRESS)
        return false;
    if (Level().CurrentEntity() && !smart_cast<CActor*>(Level().CurrentEntity()))
    {
        return FALSE;
    }
    if (pCurSkinMenu && pCurSkinMenu->IsShown())
    {
        return FALSE;
    };
    if (local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        return FALSE;
    }
    return TRUE;
};

void game_cl_Deathmatch::SetCurrentBuyMenu()
{
    if (GEnv.isDedicatedServer)
        return;

    if (!pCurBuyMenu)
    {
        pCurBuyMenu = InitBuyMenu(GetBaseCostSect(), 0);
        LoadTeamDefaultPresetItems(GetTeamMenu(0), pCurBuyMenu, &PresetItemsTeam0);
        pCurPresetItems = &PresetItemsTeam0;
        LoadDefItemsForRank(pCurBuyMenu);
    }
    //-----------------------------------
    if (m_cl_dwWarmUp_Time != 0)
        pCurBuyMenu->IgnoreMoneyAndRank(true);
    else
        pCurBuyMenu->IgnoreMoneyAndRank(false);
    //-----------------------------------
    if (!local_player)
        return;
};

void game_cl_Deathmatch::ClearBuyMenu()
{
    if (!pCurBuyMenu)
        return;
};

bool game_cl_Deathmatch::CanBeReady()
{
    if (!local_player)
        return false;

    m_bMenuCalledFromReady = TRUE;

    SetCurrentSkinMenu();

    SetCurrentBuyMenu();

    if (pCurBuyMenu && !pCurBuyMenu->IsShown())
    {
        pCurBuyMenu->ResetItems();
        SetBuyMenuItems(&PlayerDefItems);
    }

    if (!m_bSkinSelected)
    {
        m_bMenuCalledFromReady = FALSE;
        if (CanCallSkinMenu())
            pCurSkinMenu->ShowDialog(true);

        return false;
    };

    if (pCurBuyMenu)
    {
        const preset_items& _p = pCurBuyMenu->GetPreset(_preset_idx_last);
        bool Passed = false;
        Passed =
            (_p.size() == 0) ? 1 : (s32(pCurBuyMenu->GetPresetCost(_preset_idx_last)) <= local_player->money_for_round);
        Passed |= pCurBuyMenu->IsIgnoreMoneyAndRank();
        if (!Passed)
        {
            if (CanCallBuyMenu())
            {
                ShowBuyMenu();
            }
            return false;
        }
        m_bMenuCalledFromReady = FALSE;
        OnBuyMenu_Ok();
        return true;
    };
    // m_bMenuCalledFromReady = FALSE;
    return true;
};

void game_cl_Deathmatch::OnSpectatorSelect()
{
    m_bMenuCalledFromReady = FALSE;
    m_bSkinSelected = FALSE;
    inherited::OnSpectatorSelect();
};

pcstr game_cl_Deathmatch::getTeamSection(int Team) { return "deathmatch_team0"; };
void game_cl_Deathmatch::Check_Invincible_Players(){};

void game_cl_Deathmatch::ConvertTime2String(string64* str, u32 Time)
{
    if (!str)
        return;

    u32 RHour = Time / 3600000;
    Time %= 3600000;
    u32 RMinutes = Time / 60000;
    Time %= 60000;
    u32 RSecs = Time / 1000;

    xr_sprintf(*str, "%02d:%02d:%02d", RHour, RMinutes, RSecs);
};

int game_cl_Deathmatch::GetPlayersPlace(game_PlayerState* ps)
{
    if (!ps)
        return -1;
    game_cl_GameState::PLAYERS_MAP_IT I = Game().players.begin();
    game_cl_GameState::PLAYERS_MAP_IT E = Game().players.end();

    // create temporary map (sort by kills)
    xr_vector<game_PlayerState*> Players;
    for (; I != E; ++I)
        Players.push_back(I->second);
    std::sort(Players.begin(), Players.end(), DM_Compare_Players);

    int Place = 1;
    for (u32 i = 0; i < Players.size(); i++)
    {
        if (Players[i] == ps)
            return Place;
        Place++;
    };
    return -1;
}

string16 places[] = {"1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th", "9th", "10th", "11th", "12th", "13th",
    "15th", "15th", "16th", "17th", "18th", "19th", "20th", "21th", "22th", "23th", "24th", "25th", "26th", "27th",
    "28th", "29th", "30th", "31th", "32th"};

void game_cl_Deathmatch::OnConnected()
{
    inherited::OnConnected();
    if (m_game_ui)
    {
        VERIFY(!GEnv.isDedicatedServer);
        m_game_ui = smart_cast<CUIGameDM*>(CurrentGameUI());
        m_game_ui->SetClGame(this);
    }
}

void game_cl_Deathmatch::shedule_Update(u32 dt)
{
    CStringTable st;

    inherited::shedule_Update(dt);

    if (GEnv.isDedicatedServer)
        return;

    // fake
    if (m_game_ui)
    {
        m_game_ui->SetTimeMsgCaption(NULL);
        m_game_ui->SetRoundResultCaption(NULL);
        m_game_ui->SetSpectatorMsgCaption(NULL);
        m_game_ui->SetPressJumpMsgCaption(NULL);
        m_game_ui->SetPressBuyMsgCaption(NULL);
        m_game_ui->SetForceRespawnTimeCaption(NULL);
        m_game_ui->SetWarmUpCaption(NULL);
    };
    //	if (CurrentGameUI() && CurrentGameUI()->UIMainIngameWnd)
    //		CurrentGameUI()->UIMainIngameWnd->ZoneCounter().SetText("");

    switch (Phase())
    {
    case GAME_PHASE_INPROGRESS:
    {
        // m_game_ui->ShowPlayersList(false);

        Check_Invincible_Players();

        if (!m_game_ui)
            break;

        if (m_s32TimeLimit && m_cl_dwWarmUp_Time == 0)
        {
            if (Level().timeServer() < (m_start_time + m_s32TimeLimit))
            {
                u32 lts = Level().timeServer();
                u32 Rest = (m_start_time + m_s32TimeLimit) - lts;
                string64 S;
                ConvertTime2String(&S, Rest);
                m_game_ui->SetTimeMsgCaption(S);
            }
            else
            {
                m_game_ui->SetTimeMsgCaption("00:00:00");
            }
        };
        game_PlayerState* lookat_player = Game().lookat_player();
        if (local_player && !local_player->IsSkip())
        {
            if (m_bFirstRun)
            {
                m_bFirstRun = FALSE;
                if (!Level().IsDemoPlayStarted() && Level().CurrentEntity())
                {
                    VERIFY(m_game_ui);
                    m_bFirstRun = m_game_ui->ShowServerInfo() ? FALSE : TRUE;
                }

                GetActiveVoting();
            };

            if (lookat_player)
            {
                string256 MoneyStr;
                xr_sprintf(MoneyStr, "%d", lookat_player->money_for_round);
                m_game_ui->ChangeTotalMoneyIndicator(MoneyStr);
            }

            m_game_ui->SetPressJumpMsgCaption(NULL);
            m_game_ui->SetPressBuyMsgCaption(NULL);

            if (m_cl_dwWarmUp_Time > Level().timeServer())
            {
                u32 TimeRemains = m_cl_dwWarmUp_Time - Level().timeServer();
                string64 S;
                ConvertTime2String(&S, TimeRemains);
                string1024 tmpStr = "";
                if (TimeRemains > 10000)
                    strconcat(sizeof(tmpStr), tmpStr, *st.translate("mp_time2start"), " ", S);
                else
                {
                    if (TimeRemains < 1000)
                        strconcat(sizeof(tmpStr), tmpStr, *st.translate("mp_go"), "");
                    else
                    {
                        static u32 dwLastTimeRemains = 10;
                        u32 dwCurTimeRemains = TimeRemains / 1000;
                        if (dwLastTimeRemains != dwCurTimeRemains)
                        {
                            if (dwCurTimeRemains > 0 && dwCurTimeRemains <= 5)
                                PlaySndMessage(ID_COUNTDOWN_1 + dwCurTimeRemains - 1);
                        }
                        dwLastTimeRemains = dwCurTimeRemains;
                        xr_itoa(dwCurTimeRemains, S, 10);
                        strconcat(sizeof(tmpStr), tmpStr, *st.translate("mp_ready"), "...", S);
                    }
                };

                m_game_ui->SetWarmUpCaption(tmpStr);
            }

            if (Level().CurrentEntity() && smart_cast<CSpectator*>(Level().CurrentEntity()))
            {
                if (!(pCurBuyMenu && pCurBuyMenu->IsShown()) && !(pCurSkinMenu && pCurSkinMenu->IsShown()) &&
                    !m_game_ui->IsServerInfoShown() && (CurrentGameUI() && CurrentGameUI()->GameIndicatorsShown()))
                {
                    if (!m_bSkinSelected)
                        m_game_ui->SetPressJumpMsgCaption("mp_press_jump2select_skin");
                    else
                        m_game_ui->SetPressJumpMsgCaption("mp_press_jump2start");

                    if (CanCallBuyMenu())
                        m_game_ui->SetPressBuyMsgCaption("mp_press_to_buy");
                };
            };

            if (Level().CurrentControlEntity() && smart_cast<CSpectator*>(Level().CurrentControlEntity()) &&
                (CurrentGameUI()->GameIndicatorsShown()))
            {
                CSpectator* pSpectator = smart_cast<CSpectator*>(Level().CurrentControlEntity());
                if (pSpectator)
                {
                    string1024 SpectatorStr = "";
                    pSpectator->GetSpectatorString(SpectatorStr);
                    m_game_ui->SetSpectatorMsgCaption(SpectatorStr);
                }
            }

            u32 CurTime = Level().timeServer();
            if (IsVotingEnabled() && IsVotingActive() && m_dwVoteEndTime >= CurTime)
            {
                u32 TimeLeft = m_dwVoteEndTime - Level().timeServer();
                string1024 VoteTimeResStr;
                u32 SecsLeft = (TimeLeft % 60000) / 1000;
                u32 MinitsLeft = (TimeLeft - SecsLeft) / 60000;

                u32 NumAgreed = 0;
                PLAYERS_MAP_IT I;
                I = players.begin();
                for (; I != players.end(); ++I)
                {
                    game_PlayerState* ps = I->second;
                    if (ps->m_bCurrentVoteAgreed == 1)
                        NumAgreed++;
                }

                xr_sprintf(VoteTimeResStr, st.translate("mp_timeleft").c_str(), MinitsLeft, SecsLeft,
                    float(NumAgreed) / players.size());
                m_game_ui->SetVoteTimeResultMsg(VoteTimeResStr);
            };

            if (local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD) && m_u32ForceRespawn &&
                !local_player->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
            {
                u32 Rest = m_u32ForceRespawn - local_player->DeathTime;
                string64 S;
                ConvertTime2String(&S, Rest);
                string128 FullS;
                xr_sprintf(FullS, "%s : %s", *st.translate("mp_time2respawn"), S);

                m_game_ui->SetForceRespawnTimeCaption(FullS);
            };

            if (Level().CurrentViewEntity())
            {
                game_PlayerState* ps = GetPlayerByGameID(Level().CurrentViewEntity()->ID());

                if (ps && m_game_ui)
                    m_game_ui->SetRank(ps->team, ps->rank);

                if (ps && m_game_ui)
                    m_game_ui->SetFraglimit(ps->frags(), m_s32FragLimit);
            }
        };
    }
    break;
    case GAME_PHASE_PENDING:
    {
        if (!m_game_ui)
            break;

        m_game_ui->UpdateTeamPanels();
        m_game_ui->ShowPlayersList(true);
    }
    break;
    case GAME_PHASE_PLAYER_SCORES:
    {
        if (!m_game_ui)
            break;

        string128 resstring;
        xr_sprintf(resstring, st.translate("mp_player_wins").c_str(), WinnerName);
        m_game_ui->SetRoundResultCaption(resstring);

        SetScore();
        m_game_ui->UpdateTeamPanels();
        m_game_ui->ShowPlayersList(true);
    }
    break;
    };

    //-----------------------------------------
    if (!CanCallBuyMenu())
        HideBuyMenu();

    if (pCurSkinMenu && pCurSkinMenu->IsShown() && !CanCallSkinMenu())
        pCurSkinMenu->HideDialog();
    //-----------------------------------------------

    //-----------------------------------------------
    // if (m_game_ui->m_pInventoryMenu && m_game_ui->m_pInventoryMenu->IsShown() && !CanCallInventoryMenu())
    //	StartStopMenu(m_game_ui->m_pInventoryMenu,true);
    if (m_game_ui && m_game_ui->GetActorMenu().IsShown() && !CanCallInventoryMenu())
    {
        m_game_ui->HideActorMenu();
    }

    //-----------------------------------------

    u32 cur_game_state = Phase();
    // if(m_game_ui->m_pMapDesc && m_game_ui->m_pMapDesc->IsShown() && cur_game_state!=GAME_PHASE_INPROGRESS)
    //{
    //	m_game_ui->m_pMapDesc->HideDialog();
    //}

    if (pCurSkinMenu && pCurSkinMenu->IsShown() && cur_game_state != GAME_PHASE_INPROGRESS)
    {
        pCurSkinMenu->HideDialog();
    }
}

void game_cl_Deathmatch::SetScore()
{
    if (Level().CurrentViewEntity() && m_game_ui)
    {
        game_PlayerState* ps = GetPlayerByGameID(Level().CurrentViewEntity()->ID());
        if (ps && m_game_ui)
            m_game_ui->SetRank(ps->team, ps->rank);

        if (ps && m_game_ui)
            m_game_ui->SetFraglimit(ps->frags(), m_s32FragLimit);
    }
};

bool game_cl_Deathmatch::OnKeyboardPress(int key)
{
    if (inherited::OnKeyboardPress(key))
        return true;

    if (Level().IsDemoPlay() && (key != kSCORES) && (key != kCROUCH))
        return false;

    if (kSCORES == key && Phase() == GAME_PHASE_INPROGRESS)
    {
        if (m_game_ui)
            m_game_ui->ShowFragList(true);
        return true;
    };

    if (kINVENTORY == key)
    {
        if (Level().CurrentControlEntity() && smart_cast<CActor*>(Level().CurrentControlEntity()))
        {
            if (m_game_ui)
            {
                if (m_game_ui->GetActorMenu().IsShown())
                {
                    m_game_ui->HideActorMenu();
                }
                else
                {
                    if (CanCallInventoryMenu())
                    {
                        m_game_ui->ShowActorMenu();
                    }
                }
                return true;
            }
        }
    }

    if (kBUY == key)
    {
        if (pCurBuyMenu && pCurBuyMenu->IsShown())
            HideBuyMenu();
        else
        {
            if (CanCallBuyMenu())
            {
                SetCurrentBuyMenu();

                if (!pCurBuyMenu)
                    return true;

                pCurBuyMenu->ResetItems();

                if (!pCurBuyMenu->IsShown())
                    SetBuyMenuItems(&PlayerDefItems);

                //				LoadDefItemsForRank(pCurBuyMenu);
                ShowBuyMenu();
            }
        };

        return true;
    };

    if (kSKIN == key)
    {
        if (pCurSkinMenu && pCurSkinMenu->IsShown())
            pCurSkinMenu->HideDialog();
        else
        {
            if (CanCallSkinMenu())
            {
                SetCurrentSkinMenu();
                pCurSkinMenu->ShowDialog(true);
            }
        }
        return true;
    };

    /*
        if( kMAP == key)
        {
            if (m_game_ui)
            {
                if (m_game_ui->m_pPdaMenu && m_game_ui->m_pPdaMenu->IsShown())
                    StartStopMenu(m_game_ui->m_pPdaMenu,true);
                else
                {
                    m_game_ui->m_pPdaMenu->SetActiveSubdialog(eptMap);
                    StartStopMenu(m_game_ui->m_pPdaMenu,true);
                };
                return true;
            }
        };
    */
    return false;
}

bool game_cl_Deathmatch::OnKeyboardRelease(int key)
{
    if (inherited::OnKeyboardRelease(key))
        return true;
    if (kSCORES == key)
    {
        if (m_game_ui)
        {
            m_game_ui->ShowFragList(false);
        };
        return true;
    };
    return false;
}

#define MAX_VOTE_PARAMS 5
void game_cl_Deathmatch::OnVoteStart(NET_Packet& P)
{
    CStringTable st;
    inherited::OnVoteStart(P);

    string1024 Command = "";
    string64 Player = "";
    P.r_stringZ(Command);
    P.r_stringZ(Player);
    m_dwVoteEndTime = Level().timeServer() + P.r_u32();

    if (m_game_ui)
    {
        string4096 CmdName = "";
        string1024 NewCmd;
        xr_strcpy(NewCmd, Command);
        string1024 CmdParams[MAX_VOTE_PARAMS] = {"", "", "", "", ""};
        sscanf(Command, "%s %s %s %s %s %s", CmdName, CmdParams[0], CmdParams[1], CmdParams[2], CmdParams[3],
            CmdParams[4]);

        if (!xr_strcmp(CmdName, "restart"))
        {
            xr_sprintf(NewCmd, "%s", *st.translate("mp_restart"));
        }
        else if (!xr_strcmp(CmdName, "restart_fast"))
        {
            xr_sprintf(NewCmd, "%s", *st.translate("mp_restart_fast"));
        }
        else if (!xr_strcmp(CmdName, "kick"))
        {
            xr_sprintf(NewCmd, "%s %s", *st.translate("mp_kick"), CmdParams[0]);
            for (int i = 1; i < MAX_VOTE_PARAMS; i++)
            {
                if (xr_strlen(CmdParams[i]))
                {
                    xr_strcat(NewCmd, " ");
                    xr_strcat(NewCmd, CmdParams[i]);
                }
            }
        }
        else if (!xr_strcmp(CmdName, "ban"))
        {
            xr_sprintf(NewCmd, "%s %s", *st.translate("mp_ban"), CmdParams[0]);
            for (int i = 1; i < MAX_VOTE_PARAMS; i++)
            {
                if (xr_strlen(CmdParams[i]))
                {
                    xr_strcat(NewCmd, " ");
                    xr_strcat(NewCmd, CmdParams[i]);
                }
            }
        }
        else if (!xr_strcmp(CmdName, "changemap"))
        {
            xr_sprintf(NewCmd, "%s %s", *st.translate("mp_change_map"), *st.translate(CmdParams[0]));
        }
        else if (!xr_strcmp(CmdName, "changeweather"))
        {
            xr_sprintf(NewCmd, "%s %s", *st.translate("mp_change_weather"), *st.translate(CmdParams[0]));
        }

        string1024 VoteStr;
        xr_sprintf(VoteStr, *st.translate("mp_voting_started"), NewCmd, Player);

        m_game_ui->SetVoteMessage(VoteStr);
        m_game_ui->SetVoteTimeResultMsg("");
        if (!m_pVoteRespondWindow)
            m_pVoteRespondWindow = new CUIVote();
        m_pVoteRespondWindow->SetVoting(VoteStr);
    };
};

void game_cl_Deathmatch::OnVoteStop(NET_Packet& P)
{
    inherited::OnVoteStop(P);
    if (m_game_ui)
    {
        m_game_ui->SetVoteMessage(NULL);
        m_game_ui->SetVoteTimeResultMsg(NULL);
    }
};

void game_cl_Deathmatch::OnVoteEnd(NET_Packet& P)
{
    inherited::OnVoteEnd(P);
    if (m_game_ui)
    {
        m_game_ui->SetVoteMessage(NULL);
        m_game_ui->SetVoteTimeResultMsg(NULL);
    }
};

void game_cl_Deathmatch::GetMapEntities(xr_vector<SZoneMapEntityData>& dst)
{
    /*
    SZoneMapEntityData D;
    u32 color_self_team		=		0xff00ff00;
    D.color					=		color_self_team;

    PLAYERS_MAP_IT it = players.begin();
    for(;it!=players.end();++it)
    {
        game_PlayerState* ps = it->second;
        u16 id = ps->GameID;
        if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)) continue;
        IGameObject* pObject = Level().Objects.net_Find(id);
        if (!pObject) continue;
        if (!pObject || !smart_cast<CActor*>(pObject)) continue;

        VERIFY(pObject);
        D.pos = pObject->Position();
        dst.push_back(D);
    }
    */
}

bool game_cl_Deathmatch::IsEnemy(game_PlayerState* ps) { return true; }
bool game_cl_Deathmatch::IsEnemy(CEntityAlive* ea1, CEntityAlive* ea2) { return true; };
void game_cl_Deathmatch::OnRender()
{
    game_PlayerState* lookat_player = Game().lookat_player();
    if (m_bDamageBlockIndicators && local_player && (local_player == lookat_player))
    {
        PLAYERS_MAP_IT it = players.begin();
        for (; it != players.end(); ++it)
        {
            game_PlayerState* ps = it->second;
            u16 id = ps->GameID;
            if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
                continue;
            if (!ps->testFlag(GAME_PLAYER_FLAG_INVINCIBLE))
                continue;
            IGameObject* pObject = Level().Objects.net_Find(id);
            if (!pObject)
                continue;
            if (!pObject || !smart_cast<CActor*>(pObject))
                continue;
            if (ps == local_player)
                continue;
            if (!IsEnemy(ps))
                continue;
            cl_TeamStruct* pTS = &TeamList[ModifyTeam(ps->team)];

            VERIFY(pObject);
            CActor* pActor = smart_cast<CActor*>(pObject);
            VERIFY(pActor);
            pActor->RenderIndicator(pTS->IndicatorPos, pTS->Indicator_r1, pTS->Indicator_r2, pTS->InvincibleShader);
        }
    };
}

IC bool DM_Compare_Players(game_PlayerState* p1, game_PlayerState* p2)
{
    if (p1->testFlag(GAME_PLAYER_FLAG_SPECTATOR) && !p2->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
        return false;
    if (!p1->testFlag(GAME_PLAYER_FLAG_SPECTATOR) && p2->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
        return true;
    if (p1->frags() == p2->frags())
    {
        return p1->m_iDeaths < p2->m_iDeaths;
    }
    return p1->frags() > p2->frags();
};

void game_cl_Deathmatch::PlayParticleEffect(LPCSTR EffName, Fvector& pos)
{
    if (!EffName)
        return;
    // вычислить позицию и направленность партикла
    Fmatrix M;
    M.translate(pos);

    //	CParticlesPlayer::MakeXFORM(pObj,0,Fvector().set(0.f,1.f,0.f),Fvector().set(0.f,0.f,0.f),pos);

    // установить particles
    CParticlesObject* ps = NULL;

    ps = CParticlesObject::Create(EffName, TRUE);

    ps->UpdateParent(M, Fvector().set(0.f, 0.f, 0.f));
    GamePersistent().ps_needtoplay.push_back(ps);
}

void game_cl_Deathmatch::OnSpawn(IGameObject* pObj)
{
    inherited::OnSpawn(pObj);
    if (!pObj)
        return;
    CActor* pActor = smart_cast<CActor*>(pObj);
    if (pActor)
    {
        if (xr_strlen(Actor_Spawn_Effect))
            PlayParticleEffect(Actor_Spawn_Effect.c_str(), pObj->Position());
        game_PlayerState* ps = GetPlayerByGameID(pActor->ID());

        if (ps && m_reward_generator)
        {
            m_reward_generator->OnPlayerSpawned(ps);
            m_reward_generator->init_bone_groups(pActor);
        }
    };
    if (smart_cast<CWeapon*>(pObj))
    {
        if (pObj->H_Parent())
        {
            game_PlayerState* ps = GetPlayerByGameID(pObj->H_Parent()->ID());
            if (ps)
            {
                m_WeaponUsageStatistic->OnWeaponBought(ps, pObj->cNameSect().c_str());
            }
        }
    }
}

void game_cl_Deathmatch::LoadSndMessages()
{
    LoadSndMessage("dm_snd_messages", "you_won", ID_YOU_WON);

    LoadSndMessage("dm_snd_messages", "dm_rank1", ID_RANK_1);
    LoadSndMessage("dm_snd_messages", "dm_rank2", ID_RANK_2);
    LoadSndMessage("dm_snd_messages", "dm_rank3", ID_RANK_3);
    LoadSndMessage("dm_snd_messages", "dm_rank4", ID_RANK_4);

    LoadSndMessage("dm_snd_messages", "countdown_5", ID_COUNTDOWN_5);
    LoadSndMessage("dm_snd_messages", "countdown_4", ID_COUNTDOWN_4);
    LoadSndMessage("dm_snd_messages", "countdown_3", ID_COUNTDOWN_3);
    LoadSndMessage("dm_snd_messages", "countdown_2", ID_COUNTDOWN_2);
    LoadSndMessage("dm_snd_messages", "countdown_1", ID_COUNTDOWN_1);
};

void game_cl_Deathmatch::OnSwitchPhase_InProgress()
{
    HideBuyMenu();
    LoadTeamDefaultPresetItems(GetTeamMenu(0), pCurBuyMenu, &PresetItemsTeam0);
};

void game_cl_Deathmatch::OnSwitchPhase(u32 old_phase, u32 new_phase)
{
    inherited::OnSwitchPhase(old_phase, new_phase);

    if (!m_game_ui)
        return;

    switch (new_phase)
    {
    case GAME_PHASE_INPROGRESS:
    {
        WinnerName[0] = 0;
        m_game_ui->ShowPlayersList(false);
        m_game_ui->UpdateTeamPanels();
    }
    break;
    case GAME_PHASE_PLAYER_SCORES:
    {
        m_game_ui->UpdateTeamPanels();
        if (local_player)
        {
            if (!xr_strcmp(WinnerName, local_player->getName()))
            {
                PlaySndMessage(ID_YOU_WON);
            }
        }
    }
    break;
    default: {
    }
    break;
    };
}

void game_cl_Deathmatch::OnGameRoundStarted()
{
    inherited::OnGameRoundStarted();
    if (pCurBuyMenu && pCurBuyMenu->IsShown())
        pCurBuyMenu->HideDialog();

    if (local_player)
    {
        if (pCurBuyMenu)
        {
            pCurBuyMenu->IgnoreMoneyAndRank(false);
            pCurBuyMenu->SetRank(local_player->rank);
        }
        ClearBuyMenu();
        LoadDefItemsForRank(pCurBuyMenu);
        ChangeItemsCosts(pCurBuyMenu);
        if (pCurBuyMenu && pCurPresetItems)
        {
            LoadTeamDefaultPresetItems(GetTeamMenu(local_player->team), pCurBuyMenu, pCurPresetItems);
            ReInitRewardGenerator(local_player);
        }
    }
    if (pCurBuyMenu)
        pCurBuyMenu->ClearPreset(_preset_idx_last);
    //-----------------------------------------------------------------
    if (m_game_ui && m_game_ui->GetActorMenu().IsShown())
    {
        m_game_ui->HideActorMenu();
    }
}

void game_cl_Deathmatch::OnRankChanged(u8 OldRank)
{
    inherited::OnRankChanged(OldRank);
    if (pCurBuyMenu)
        pCurBuyMenu->SetRank(local_player->rank);
    LoadDefItemsForRank(pCurBuyMenu);
    ChangeItemsCosts(pCurBuyMenu);
    //---------------------------------------------
    PlayRankChangesSndMessage();
};

void game_cl_Deathmatch::PlayRankChangesSndMessage()
{
    if (local_player)
    {
        switch (local_player->rank)
        {
        case 0: break;
        default: PlaySndMessage(ID_RANK_0 + local_player->rank); break;
        }
    }
}

void game_cl_Deathmatch::OnTeamChanged()
{
    if (!pCurBuyMenu)
        return;
    if (pCurBuyMenu)
        pCurBuyMenu->SetRank(local_player->rank);
    LoadDefItemsForRank(pCurBuyMenu);
    ChangeItemsCosts(pCurBuyMenu);
};

void game_cl_Deathmatch::LoadPlayerDefItems(pcstr TeamName, IBuyWnd* pBuyMenu)
{
    if (!local_player)
        return;
    LoadTeamDefaultPresetItems(TeamName, pBuyMenu, &PlayerDefItems);
};

void game_cl_Deathmatch::OnGameMenuRespond_ChangeSkin(NET_Packet& P)
{
    s8 NewSkin = P.r_s8();
    local_player->skin = NewSkin;

    if (pCurSkinMenu && pCurSkinMenu->IsShown())
        pCurSkinMenu->HideDialog();

    // if (m_game_ui->m_pMapDesc && m_game_ui->m_pMapDesc->IsShown())
    //	m_game_ui->m_pMapDesc->HideDialog();

    SetCurrentSkinMenu();
    if (pCurSkinMenu)
        pCurSkinMenu->SetCurSkin(local_player->skin);
    SetCurrentBuyMenu();
    ReInitRewardGenerator(local_player);
    m_bSpectatorSelected = FALSE;

    if (m_bMenuCalledFromReady)
    {
        OnKeyboardPress(kJUMP);
    }
};

void game_cl_Deathmatch::OnPlayerFlagsChanged(game_PlayerState* ps)
{
    inherited::OnPlayerFlagsChanged(ps);
    if (!ps)
        return;
    if (m_game_ui)
        m_game_ui->UpdateTeamPanels();

    if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        if (ps == local_player && m_game_ui && m_game_ui->GetActorMenu().IsShown())
        {
            m_game_ui->HideActorMenu();
        }
        return;
    }

    IGameObject* pObject = Level().Objects.net_Find(ps->GameID);
    if (!pObject)
        return;

    if (!smart_cast<CActor*>(pObject))
        return;

    CActor* pActor = smart_cast<CActor*>(pObject);
    if (!pActor)
        return;

    pActor->conditions().SetCanBeHarmedState(!ps->testFlag(GAME_PLAYER_FLAG_INVINCIBLE));
};

void game_cl_Deathmatch::SendPickUpEvent(u16 ID_who, u16 ID_what)
{
    NET_Packet P;
    u_EventGen(P, GE_OWNERSHIP_TAKE_MP_FORCED, ID_who);
    P.w_u16(ID_what);
    u_EventSend(P);
};

const shared_str game_cl_Deathmatch::GetTeamMenu(s16 team) { return TEAM0_MENU; }
#define SELF_LOCATION "mp_self_location"
void game_cl_Deathmatch::UpdateMapLocations()
{
    inherited::UpdateMapLocations();
    if (local_player)
    {
        if (!Level().MapManager().HasMapLocation(SELF_LOCATION, local_player->GameID))
        {
            (Level().MapManager().AddMapLocation(SELF_LOCATION, local_player->GameID))->EnablePointer();
        }
    }
}

void game_cl_Deathmatch::ShowBuyMenu()
{
    if (!local_player)
        return;
    if (!pCurBuyMenu || pCurBuyMenu->IsShown())
        return;

    pCurBuyMenu->ShowDialog(true);

    if (local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        const preset_items& _p = pCurBuyMenu->GetPreset(_preset_idx_last);
        if (_p.size() != 0)
            pCurBuyMenu->TryUsePreset(_preset_idx_last);
    }
};

void game_cl_Deathmatch::HideBuyMenu()
{
    if (!pCurBuyMenu || !pCurBuyMenu->IsShown())
        return;
    pCurBuyMenu->HideDialog();
}

s16 game_cl_Deathmatch::ModifyTeam(s16 Team) { return Team; }
bool game_cl_Deathmatch::IsPlayerInTeam(game_PlayerState* ps, ETeam team)
{
    if (ps->testFlag(GAME_PLAYER_FLAG_SPECTATOR) && (team == etSpectatorsTeam))
    {
        return true;
    }
    else if (!ps->testFlag(GAME_PLAYER_FLAG_SPECTATOR) && (team == etGreenTeam))
    {
        return true;
    }
    return false;
}

LPCSTR game_cl_Deathmatch::GetGameScore(string32& score_dest)
{
    s32 frags = local_player ? local_player->frags() : 0;
    xr_sprintf(score_dest, "[%d/%d]", frags, m_s32FragLimit);
    return score_dest;
}
