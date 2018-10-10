#include "StdAfx.h"
#include "game_cl_teamdeathmatch.h"
#include "xrMessages.h"
#include "Level.h"
#include "UIGameTDM.h"
#include "xr_level_controller.h"
#include "map_manager.h"
#include "map_location.h"
#include "Actor.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UISkinSelector.h"
#include "ui/UIPdaWnd.h"
#include "ui/UIMapDesc.h"
#include "game_base_menu_events.h"
#include "ui/TeamInfo.h"
#include "string_table.h"
#include "clsid_game.h"
#include "ui/UIActorMenu.h"

#define MESSAGE_MENUS "tdm_messages_menu"

#include "game_cl_teamdeathmatch_snd_messages.h"
#include "reward_event_generator.h"

const shared_str game_cl_TeamDeathmatch::GetTeamMenu(s16 team)
{
    switch (team)
    {
    case 0: return "teamdeathmatch_team0"; break;
    case 1: return "teamdeathmatch_team1"; break;
    case 2: return "teamdeathmatch_team2"; break;
    default: NODEFAULT;
    };
#ifdef DEBUG
    return NULL;
#endif // DEBUG
}

game_cl_TeamDeathmatch::game_cl_TeamDeathmatch()
{
    PresetItemsTeam1.clear();
    PresetItemsTeam2.clear();

    m_bTeamSelected = FALSE;
    m_game_ui = NULL;

    m_bShowPlayersNames = false;
    m_bFriendlyIndicators = false;
    m_bFriendlyNames = false;

    LoadSndMessages();
}
void game_cl_TeamDeathmatch::Init()
{
    //	pInventoryMenu	= new CUIInventoryWnd();
    //	pPdaMenu = new CUIPdaWnd();
    //	pMapDesc = new CUIMapDesc();
    //-----------------------------------------------------------
    LoadTeamData(GetTeamMenu(1));
    LoadTeamData(GetTeamMenu(2));
}

game_cl_TeamDeathmatch::~game_cl_TeamDeathmatch()
{
    PresetItemsTeam1.clear();
    PresetItemsTeam2.clear();

    xr_delete(pCurBuyMenu);
    xr_delete(pCurSkinMenu);

    //	xr_delete(pInventoryMenu);
}

void game_cl_TeamDeathmatch::net_import_state(NET_Packet& P)
{
    bool teamsEqual = (!teams.empty()) ? (teams[0].score == teams[1].score) : false;
    u16 old_phase = Phase();
    inherited::net_import_state(P);
    u16 new_phase = Phase();
    m_bFriendlyIndicators = !!P.r_u8();
    m_bFriendlyNames = !!P.r_u8();
    if (!teams.empty())
    {
        if (teamsEqual)
        {
            if (teams[0].score != teams[1].score)
            {
                if (Level().CurrentViewEntity())
                {
                    if (teams[0].score > teams[1].score)
                        PlaySndMessage(ID_TEAM1_LEAD);
                    else
                        PlaySndMessage(ID_TEAM2_LEAD);
                }
            }
        }
        else
        {
            if (teams[0].score == teams[1].score)
                if (Level().CurrentViewEntity())
                    PlaySndMessage(ID_TEAMS_EQUAL);
        }
    };
    if ((old_phase != new_phase) && ((new_phase == GAME_PHASE_TEAM1_SCORES) || (new_phase == GAME_PHASE_TEAM2_SCORES)))
    {
        if (m_reward_generator)
            m_reward_generator->OnRoundEnd();
    }
}
void game_cl_TeamDeathmatch::TranslateGameMessage(u32 msg, NET_Packet& P)
{
    string512 Text;
    //	LPSTR	Color_Teams[3]	= {"%c[255,255,255,255]", "%c[255,64,255,64]", "%c[255,64,64,255]"};
    char Color_Main[] = "%c[255,192,192,192]";
    //	LPSTR	TeamsNames[3]	= {"Zero Team", "Team Green", "Team Blue"};

    switch (msg)
    {
    case GAME_EVENT_PLAYER_JOIN_TEAM: // tdm
    {
        string64 PlayerName;
        P.r_stringZ(PlayerName);
        u16 Team;
        P.r_u16(Team);

        xr_sprintf(Text, "%s%s %s%s %s%s",
            "", // no color
            PlayerName, Color_Main, *StringTable().translate("mp_joined"), CTeamInfo::GetTeam_color_tag(int(Team)),
            CTeamInfo::GetTeam_name(int(Team)));
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);
        //---------------------------------------
        Msg("%s %s %s", PlayerName, *StringTable().translate("mp_joined"), CTeamInfo::GetTeam_name(int(Team)));
    }
    break;

    case PLAYER_CHANGE_TEAM: // tdm
    {
        u16 PlayerID, OldTeam, NewTeam;
        P.r_u16(PlayerID);
        P.r_u16(OldTeam);
        P.r_u16(NewTeam);

        game_PlayerState* pPlayer = GetPlayerByGameID(PlayerID);
        if (!pPlayer)
            break;

        xr_sprintf(Text, "%s%s %s%s %s%s", CTeamInfo::GetTeam_color_tag(int(OldTeam)), pPlayer->getName(), Color_Main,
            *StringTable().translate("mp_switched_to"), CTeamInfo::GetTeam_color_tag(int(NewTeam)),
            CTeamInfo::GetTeam_name(int(NewTeam)));
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);
        //---------------------------------------
        Msg("%s *s %s", pPlayer->getName(), *StringTable().translate("mp_switched_to"), CTeamInfo::GetTeam_name(int(NewTeam)));
    }
    break;

    default: inherited::TranslateGameMessage(msg, P);
    };
}

void game_cl_TeamDeathmatch::SetGameUI(CUIGameCustom* uigame)
{
    inherited::SetGameUI(uigame);
    m_game_ui = smart_cast<CUIGameTDM*>(uigame);
    R_ASSERT(m_game_ui);
};

CUIGameCustom* game_cl_TeamDeathmatch::createGameUI()
{
    if (GEnv.isDedicatedServer)
        return NULL;

    CLASS_ID clsid = CLSID_GAME_UI_TEAMDEATHMATCH;
    m_game_ui = smart_cast<CUIGameTDM*>(NEW_INSTANCE(clsid));
    R_ASSERT(m_game_ui);
    m_game_ui->Load();
    m_game_ui->SetClGame(this);
    LoadMessagesMenu(MESSAGE_MENUS);
    return m_game_ui;
}

void game_cl_TeamDeathmatch::GetMapEntities(xr_vector<SZoneMapEntityData>& dst)
{
    SZoneMapEntityData D;
    u32 color_self_team = 0xff00ff00;
    D.color = color_self_team;

    s16 local_team = local_player->team;

    PLAYERS_MAP_IT it = players.begin();
    for (; it != players.end(); ++it)
    {
        if (local_team == it->second->team)
        {
            u16 id = it->second->GameID;
            IGameObject* pObject = Level().Objects.net_Find(id);
            if (!pObject)
                continue;
            if (!pObject || !smart_cast<CActor*>(pObject))
                continue;

            VERIFY(pObject);
            D.pos = pObject->Position();
            dst.push_back(D);
        }
    }
}

void game_cl_TeamDeathmatch::OnMapInfoAccept()
{
    if (CanCallTeamSelectMenu())
        m_game_ui->m_pUITeamSelectWnd->ShowDialog(true);
    //.		m_game_ui->StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);
};

void game_cl_TeamDeathmatch::OnTeamMenuBack()
{
    if (local_player->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
    {
        m_game_ui->ShowServerInfo();
        //.		m_game_ui->StartStopMenu(m_game_ui->m_pMapDesc, true);
    }
};

void game_cl_TeamDeathmatch::OnTeamMenu_Cancel()
{
    m_game_ui->m_pUITeamSelectWnd->HideDialog();
    //.	m_game_ui->StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);

    if (!m_bTeamSelected && !m_bSpectatorSelected)
    {
        if (CanCallTeamSelectMenu() && !m_game_ui->m_pUITeamSelectWnd->IsShown())
        {
            m_game_ui->m_pUITeamSelectWnd->ShowDialog(true);
            //.			m_game_ui->StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);
            return;
        }
    }
    m_bMenuCalledFromReady = FALSE;
};

void game_cl_TeamDeathmatch::OnSkinMenuBack()
{
    if (CanCallTeamSelectMenu())
        m_game_ui->m_pUITeamSelectWnd->ShowDialog(true);
    //.		m_game_ui->StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);
};

void game_cl_TeamDeathmatch::OnSpectatorSelect()
{
    m_bTeamSelected = FALSE;
    inherited::OnSpectatorSelect();
}

void game_cl_TeamDeathmatch::OnTeamSelect(int Team)
{
    bool NeedToSendTeamSelect = true;
    if (Team != -1)
    {
        if (Team + 1 == local_player->team && m_bSkinSelected)
            NeedToSendTeamSelect = false;
        else
        {
            NeedToSendTeamSelect = true;
        }
    }

    if (NeedToSendTeamSelect)
    {
        IGameObject* l_pObj = Level().CurrentEntity();

        CGameObject* l_pPlayer = smart_cast<CGameObject*>(l_pObj);
        if (!l_pPlayer)
            return;

        NET_Packet P;
        l_pPlayer->u_EventGen(P, GE_GAME_EVENT, l_pPlayer->ID());
        P.w_u16(GAME_EVENT_PLAYER_GAME_MENU);
        P.w_u8(PLAYER_CHANGE_TEAM);

        P.w_s16(s16(Team + 1));
        // P.w_u32			(0);
        l_pPlayer->u_EventSend(P);
        //-----------------------------------------------------------------
        m_bSkinSelected = FALSE;
    };
    //-----------------------------------------------------------------
    m_bTeamSelected = TRUE;
    //---------------------------
    //	if (m_bMenuCalledFromReady)
    //	{
    //		OnKeyboardPress(kJUMP);
    //	}
};
//-----------------------------------------------------------------
void game_cl_TeamDeathmatch::SetCurrentBuyMenu()
{
    if (!local_player)
        return;
    if (!local_player->team || local_player->skin == -1)
        return;
    if (GEnv.isDedicatedServer)
        return;

    if (!pCurBuyMenu)
    {
        s16 team_index = local_player->team == 1 ? 1 : 2;
        pCurBuyMenu = InitBuyMenu(GetBaseCostSect(), team_index);
        if (team_index == 1)
        {
            LoadTeamDefaultPresetItems(GetTeamMenu(team_index), pCurBuyMenu, &PresetItemsTeam1);
            pCurPresetItems = &PresetItemsTeam1;
        }
        else
        {
            LoadTeamDefaultPresetItems(GetTeamMenu(team_index), pCurBuyMenu, &PresetItemsTeam2);
            pCurPresetItems = &PresetItemsTeam2;
        };
        LoadDefItemsForRank(pCurBuyMenu);
    };

    if (!pCurBuyMenu)
        return;

    //-----------------------------------
    if (m_cl_dwWarmUp_Time != 0)
        pCurBuyMenu->IgnoreMoneyAndRank(true);
    else
        pCurBuyMenu->IgnoreMoneyAndRank(false);
    //-----------------------------------
};

void game_cl_TeamDeathmatch::SetCurrentSkinMenu()
{
    s16 new_team;
    if (!local_player)
        return;
    if (local_player->team == 1)
    {
        new_team = 1;
    }
    else
    {
        new_team = 2;
    }
    if (pCurSkinMenu && pCurSkinMenu->GetTeam() == new_team)
        return;

    if (pCurSkinMenu && new_team != pCurSkinMenu->GetTeam())
        if (pCurSkinMenu->IsShown())
            pCurSkinMenu->HideDialog();
    //.			m_game_ui->StartStopMenu(pCurSkinMenu,true);

    xr_delete(pCurSkinMenu);
    pCurSkinMenu = InitSkinMenu(new_team);
};

bool game_cl_TeamDeathmatch::CanBeReady()
{
    if (!local_player)
        return false;

    m_bMenuCalledFromReady = TRUE;

    if (!m_bTeamSelected)
    {
        m_bMenuCalledFromReady = FALSE;
        if (CanCallTeamSelectMenu())
            m_game_ui->m_pUITeamSelectWnd->ShowDialog(true);
        //.			m_game_ui->StartStopMenu(m_game_ui->m_pUITeamSelectWnd,true);

        return false;
    }

    return inherited::CanBeReady();
};

pcstr game_cl_TeamDeathmatch::getTeamSection(int Team)
{
    switch (Team)
    {
    case 1: return "teamdeathmatch_team1";
    case 2: return "teamdeathmatch_team2";
    default: return nullptr;
    };
};

#include "string_table.h"
#include "ui/TeamInfo.h"

void game_cl_TeamDeathmatch::shedule_Update(u32 dt)
{
    string512 msg;

    inherited::shedule_Update(dt);

    if (!m_game_ui)
        return;
    //---------------------------------------------------------
    if (m_game_ui->m_pUITeamSelectWnd && m_game_ui->m_pUITeamSelectWnd->IsShown() && !CanCallTeamSelectMenu())
        m_game_ui->m_pUITeamSelectWnd->HideDialog();
    //.		m_game_ui->StartStopMenu(m_game_ui->m_pUITeamSelectWnd,true);
    //---------------------------------------------------------

    if (m_game_ui)
        m_game_ui->SetBuyMsgCaption(NULL);

    switch (m_phase)
    {
    case GAME_PHASE_TEAM1_SCORES:
    {
        xr_sprintf(msg, StringTable().translate("mp_team_wins").c_str(), CTeamInfo::GetTeam_name(1));
        m_game_ui->SetRoundResultCaption(msg);

        m_game_ui->UpdateTeamPanels();
        m_game_ui->ShowPlayersList(true);

        SetScore();
    }
    break;
    case GAME_PHASE_TEAM2_SCORES:
    {
        xr_sprintf(msg, StringTable().translate("mp_team_wins").c_str(), CTeamInfo::GetTeam_name(2));
        m_game_ui->SetRoundResultCaption(msg);

        m_game_ui->UpdateTeamPanels();
        m_game_ui->ShowPlayersList(true);

        SetScore();
    }
    break;
    case GAME_PHASE_INPROGRESS:
    {
        if (local_player && !local_player->IsSkip())
        {
            if (Level().CurrentEntity() && smart_cast<CSpectator*>(Level().CurrentEntity()))
            {
                if (!(pCurBuyMenu && pCurBuyMenu->IsShown()) && !(pCurSkinMenu && pCurSkinMenu->IsShown()) &&
                    !m_game_ui->IsServerInfoShown() && (CurrentGameUI() && CurrentGameUI()->GameIndicatorsShown()))
                {
                    if (!m_bTeamSelected)
                        m_game_ui->SetPressJumpMsgCaption("mp_press_jump2select_team");
                };
            };
            SetScore();

            if (local_player->testFlag(GAME_PLAYER_FLAG_ONBASE) &&
                !local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
            {
                string1024 msg;
                xr_sprintf(msg, *StringTable().translate("mp_press_to_buy"), "B");
                if (m_game_ui)
                    m_game_ui->SetBuyMsgCaption(msg);
                m_bBuyEnabled = true;
            }
            else if (!local_player->testFlag(GAME_PLAYER_FLAG_ONBASE) &&
                !local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
            {
                m_bBuyEnabled = false;
            }
            else if (local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
            {
                m_bBuyEnabled = true;
            }
        };
    }
    break;
    default: {
    }
    break;
    };
}

void game_cl_TeamDeathmatch::SetScore()
{
    if (local_player)
    {
        s16 lt = local_player->team;
        if (lt >= 0)
        {
            if (m_game_ui)
                m_game_ui->SetScoreCaption(teams[0].score, teams[1].score);
        }
    }
};

bool game_cl_TeamDeathmatch::OnKeyboardPress(int key)
{
    if (inherited::OnKeyboardPress(key))
        return true;
    if (kTEAM == key)
    {
        if (m_game_ui && CanCallTeamSelectMenu())
            m_game_ui->m_pUITeamSelectWnd->ShowDialog(true);

        return true;
    };

    return false;
}

bool game_cl_TeamDeathmatch::IsEnemy(game_PlayerState* ps)
{
    if (!local_player)
        return false;
    return local_player->team != ps->team;
};

bool game_cl_TeamDeathmatch::IsEnemy(CEntityAlive* ea1, CEntityAlive* ea2) { return (ea1->g_Team() != ea2->g_Team()); };
#define PLAYER_NAME_COLOR 0xff40ff40

void game_cl_TeamDeathmatch::OnRender()
{
    game_PlayerState* lookat_player = Game().lookat_player();
    if (local_player && (local_player == lookat_player) && (m_bShowPlayersNames || m_bFriendlyIndicators))
    {
        cl_TeamStruct* pTS = &TeamList[ModifyTeam(local_player->team)];
        PLAYERS_MAP_IT it = players.begin();
        for (; it != players.end(); ++it)
        {
            game_PlayerState* ps = it->second;
            u16 id = ps->GameID;
            if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
                continue;
            IGameObject* pObject = Level().Objects.net_Find(id);
            if (!pObject)
                continue;
            if (!pObject || !smart_cast<CActor*>(pObject))
                continue;
            if (IsEnemy(ps))
                continue;

            if (ps == local_player)
                continue;

            float dup = 0.0f;
            if (/*m_bFriendlyNames &&*/ m_bShowPlayersNames)
            {
                VERIFY(pObject);
                CActor* pActor = smart_cast<CActor*>(pObject);
                VERIFY(pActor);
                Fvector IPos = pTS->IndicatorPos;
                IPos.y -= pTS->Indicator_r2;
                pActor->RenderText(ps->getName(), IPos, &dup, PLAYER_NAME_COLOR);
            }
            if (m_bFriendlyIndicators)
            {
                VERIFY(pObject);
                CActor* pActor = smart_cast<CActor*>(pObject);
                VERIFY(pActor);
                Fvector IPos = pTS->IndicatorPos;
                IPos.y += dup;
                pActor->RenderIndicator(IPos, pTS->Indicator_r1, pTS->Indicator_r2, pTS->IndicatorShader);
            };
        }
    };
    inherited::OnRender();
}

BOOL game_cl_TeamDeathmatch::CanCallBuyMenu()
{
    if (Phase() != GAME_PHASE_INPROGRESS)
        return FALSE;

    if (!m_game_ui)
        return FALSE;

    if (m_game_ui->m_pUITeamSelectWnd && m_game_ui->m_pUITeamSelectWnd->IsShown())
        return FALSE;

    if (!m_bTeamSelected)
        return FALSE;

    if (!m_bSkinSelected)
        return FALSE;

    if (!is_buy_menu_ready())
        return FALSE;

    if (!m_bSkinSelected || m_bSpectatorSelected)
        return FALSE;

    if (pCurSkinMenu && pCurSkinMenu->IsShown())
        return FALSE;

    if (m_game_ui && m_game_ui->GetActorMenu().IsShown())
        return FALSE;

    return m_bBuyEnabled;
};

BOOL game_cl_TeamDeathmatch::CanCallSkinMenu()
{
    if (!m_game_ui)
        return FALSE;
    if (m_game_ui->m_pUITeamSelectWnd && m_game_ui->m_pUITeamSelectWnd->IsShown())
        return FALSE;
    if (!m_bTeamSelected)
        return FALSE;

    return inherited::CanCallSkinMenu();
};

BOOL game_cl_TeamDeathmatch::CanCallInventoryMenu()
{
    if (!m_game_ui)
        return FALSE;
    if (m_game_ui->m_pUITeamSelectWnd && m_game_ui->m_pUITeamSelectWnd->IsShown())
        return FALSE;

    return inherited::CanCallInventoryMenu();
};

BOOL game_cl_TeamDeathmatch::CanCallTeamSelectMenu()
{
    if (Phase() != GAME_PHASE_INPROGRESS)
        return false;
    if (!local_player)
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
    if (pCurSkinMenu && pCurSkinMenu->IsShown())
    {
        return FALSE;
    };

    m_game_ui->m_pUITeamSelectWnd->SetCurTeam(ModifyTeam(local_player->team));
    return TRUE;
};

#define FRIEND_LOCATION "mp_friend_location"

void game_cl_TeamDeathmatch::UpdateMapLocations()
{
    inherited::UpdateMapLocations();
    if (local_player)
    {
        PLAYERS_MAP_IT it = players.begin();
        for (; it != players.end(); ++it)
        {
            game_PlayerState* ps = it->second;
            u16 id = ps->GameID;
            if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
            {
                Level().MapManager().RemoveMapLocation(FRIEND_LOCATION, id);
                continue;
            };

            IGameObject* pObject = Level().Objects.net_Find(id);
            if (!pObject || !smart_cast<CActor*>(pObject))
                continue;
            if (IsEnemy(ps))
            {
                if (Level().MapManager().HasMapLocation(FRIEND_LOCATION, id))
                {
                    Level().MapManager().RemoveMapLocation(FRIEND_LOCATION, id);
                };
                continue;
            };
            if (!Level().MapManager().HasMapLocation(FRIEND_LOCATION, id))
            {
                (Level().MapManager().AddMapLocation(FRIEND_LOCATION, id))->EnablePointer();
            }
        }
    };
};

void game_cl_TeamDeathmatch::LoadSndMessages()
{
    //	LoadSndMessage("dm_snd_messages", "you_won", ID_YOU_WON);
    LoadSndMessage("tdm_snd_messages", "team1_win", ID_TEAM1_WIN);
    LoadSndMessage("tdm_snd_messages", "team2_win", ID_TEAM2_WIN);
    LoadSndMessage("tdm_snd_messages", "teams_equal", ID_TEAMS_EQUAL);
    LoadSndMessage("tdm_snd_messages", "team1_lead", ID_TEAM1_LEAD);
    LoadSndMessage("tdm_snd_messages", "team2_lead", ID_TEAM2_LEAD);

    LoadSndMessage("tdm_snd_messages", "team1_rank1", ID_TEAM1_RANK_1);
    LoadSndMessage("tdm_snd_messages", "team1_rank2", ID_TEAM1_RANK_2);
    LoadSndMessage("tdm_snd_messages", "team1_rank3", ID_TEAM1_RANK_3);
    LoadSndMessage("tdm_snd_messages", "team1_rank4", ID_TEAM1_RANK_4);

    LoadSndMessage("tdm_snd_messages", "team2_rank1", ID_TEAM2_RANK_1);
    LoadSndMessage("tdm_snd_messages", "team2_rank2", ID_TEAM2_RANK_2);
    LoadSndMessage("tdm_snd_messages", "team2_rank3", ID_TEAM2_RANK_3);
    LoadSndMessage("tdm_snd_messages", "team2_rank4", ID_TEAM2_RANK_4);
};

void game_cl_TeamDeathmatch::OnSwitchPhase_InProgress()
{
    HideBuyMenu();
    if (!m_bSkinSelected)
        m_bTeamSelected = FALSE;
};

void game_cl_TeamDeathmatch::OnSwitchPhase(u32 old_phase, u32 new_phase)
{
    inherited::OnSwitchPhase(old_phase, new_phase);
    switch (new_phase)
    {
    case GAME_PHASE_TEAM1_SCORES:
    {
        if (Level().CurrentViewEntity())
            PlaySndMessage(ID_TEAM1_WIN);
    }
    break;
    case GAME_PHASE_TEAM2_SCORES:
    {
        if (Level().CurrentViewEntity())
            PlaySndMessage(ID_TEAM2_WIN);
    }
    break;
    default: {
    }
    break;
    };
}

void game_cl_TeamDeathmatch::OnTeamChanged()
{
    xr_delete(pCurBuyMenu);
    SetCurrentBuyMenu();
    if (pCurBuyMenu)
    {
        ReInitRewardGenerator(local_player);
    }
    inherited::OnTeamChanged();
};

void game_cl_TeamDeathmatch::PlayRankChangesSndMessage()
{
    if (local_player)
    {
        switch (local_player->rank)
        {
        case 0: break;
        default:
            if (local_player->team == 1)
                PlaySndMessage(ID_TEAM1_RANK_0 + local_player->rank);
            if (local_player->team == 2)
                PlaySndMessage(ID_TEAM2_RANK_0 + local_player->rank);
            break;
        }
    }
};

void game_cl_TeamDeathmatch::OnGameMenuRespond_ChangeTeam(NET_Packet& P)
{
    s16 OldTeam = local_player->team;
    local_player->team = u8(P.r_s16() & 0x00ff);
    if (OldTeam != local_player->team)
    {
        OnTeamChanged();
        if (m_reward_generator)
            m_reward_generator->OnPlayerChangeTeam(local_player->team);
    }

    SetCurrentSkinMenu();
    if (pCurSkinMenu)
    {
        pCurSkinMenu->SetCurSkin(local_player->skin);
        if (CanCallSkinMenu())
            pCurSkinMenu->ShowDialog(true);
        //.			m_game_ui->StartStopMenu(pCurSkinMenu, true);
    }
};

bool game_cl_TeamDeathmatch::IsPlayerInTeam(game_PlayerState* ps, ETeam team)
{
    if (ps->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
    {
        if (team == etSpectatorsTeam)
        {
            return true;
        }
        return false;
    }
    return (ModifyTeam(s16(ps->team)) == s16(team));
}

LPCSTR game_cl_TeamDeathmatch::GetGameScore(string32& score_dest)
{
    xr_sprintf(score_dest, "[%d:%d]", teams[0].score, teams[1].score);
    return score_dest;
}

void game_cl_TeamDeathmatch::OnConnected()
{
    inherited::OnConnected();
    m_game_ui = smart_cast<CUIGameTDM*>(CurrentGameUI());
}
