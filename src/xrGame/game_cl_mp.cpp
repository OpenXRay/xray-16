#include "StdAfx.h"
#include "game_cl_mp.h"
#include "xr_level_controller.h"
#include "xrMessages.h"
#include "Actor.h"
#include "ExplosiveItem.h"
#include "Level.h"
#include "CustomZone.h"
#include "game_base_kill_type.h"
#include "game_base_menu_events.h"
#include "UIGameCustom.h"
#include "ui/UIInventoryUtilities.h"
#include "ui/UIMessagesWindow.h"
#include "ui/UIChatWnd.h"
#include "ui/UIGameLog.h"
#include "ui/UIMainIngameWnd.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/XML/UITextureMaster.h"
#include "ui/UIVotingCategory.h"
#include "ui/UIMPAdminMenu.h"
#include "ui/UIVote.h"
#include "ui/UIMessageBoxEx.h"
#include "ui/KillMessageStruct.h"
#include "ui/UISpeechMenu.h"
#include "UIGameMP.h"

#include "string_table.h"
#include "clsid_game.h"
#include "MainMenu.h"
#include "WeaponKnife.h"
#include "RegistryFuncs.h"
#include "xrGameSpy/xrGameSpy_MainDefs.h"
#include "screenshot_server.h"
#include "xrCore/Compression/ppmd_compressor.h"
#include "xrCore/Compression/rt_compressor.h"
#include "game_cl_mp_snd_messages.h"

#include "reward_event_generator.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "reward_manager.h"
#include "login_manager.h"
#include "stats_submitter.h"

#include "xrServer_info.h" //for enum_server_info_type

#define KILLEVENT_ICONS "ui" DELIMITER "ui_hud_mp_icon_death"
#define RADIATION_ICONS "ui" DELIMITER "ui_mn_radiations_hard"
#define BLOODLOSS_ICONS "ui" DELIMITER "ui_mn_wounds_hard"
#define RANK_ICONS "ui" DELIMITER "ui_mp_icon_rank"

#define KILLEVENT_GRID_WIDTH 64
#define KILLEVENT_GRID_HEIGHT 64

BOOL g_draw_downloads = FALSE;

game_cl_mp::game_cl_mp()
{
    m_bVotingActive = false;
    m_pVoteStartWindow = NULL;
    m_pAdminMenuWindow = NULL;
    m_pVoteRespondWindow = NULL;
    m_pMessageBox = NULL;

    m_pSndMessages.clear();
    LoadSndMessages();
    m_bJustRestarted = true;
    m_pSndMessagesInPlay.clear();
    m_aMessageMenus.clear();

    m_bSpectatorSelected = FALSE;
    //-------------------------------------
    m_u8SpectatorModes = 0xff;
    m_bSpectator_FreeFly = true;
    m_bSpectator_FirstEye = true;
    m_bSpectator_LookAt = true;
    m_bSpectator_FreeLook = true;
    m_bSpectator_TeamCamera = true;
    m_cur_MenuID = u32(-1);
    //-------------------------------------
    LoadBonuses();
    //-------------------------------------
    buffer_for_compress = NULL;
    buffer_for_compress_size = 0;
    //-----------------------------------------------------------
    //-----------------------------------------------------------
    /*	pBuySpawnMsgBox		= new CUIMessageBoxEx();
    //.	pBuySpawnMsgBox->SetWorkPhase(GAME_PHASE_INPROGRESS);
    pBuySpawnMsgBox->Init("message_box_buy_spawn");
    pBuySpawnMsgBox->AddCallback("msg_box", MESSAGE_BOX_YES_CLICKED, CUIWndCallback::void_function(this, &game_cl_mp::OnBuySpawn));
    string1024	BuySpawnText;
    xr_sprintf(BuySpawnText, "You can buy a spawn for %d $. Press Yes to pay.",
        abs(m_iSpawn_Cost));
    pBuySpawnMsgBox->SetText(BuySpawnText);
*/ //-----------------------------------------------------------
    m_reward_generator = NULL;
    m_ready_to_open_buy_menu = true;
    m_reward_manager = NULL;
};

game_cl_mp::~game_cl_mp()
{
    /*	TODO: check if shaders are deleted automatically...
    CL_TEAM_DATA_LIST_it it = TeamList.begin();
    for(;it!=TeamList.end();++it)
    {
        if (it->IndicatorShader)
            it->IndicatorShader.destroy();
        if (it->InvincibleShader)
            it->InvincibleShader.destroy();
    };
    */
    TeamList.clear();
    /*	TODO: check if shaders are deleted automatically...
        if (m_EquipmentIconsShader)
            m_EquipmentIconsShader.destroy();

        if (m_KillEventIconsShader)
            m_KillEventIconsShader.destroy();

        if (m_RadiationIconsShader)
            m_RadiationIconsShader.destroy();

        if (m_BloodLossIconsShader)
            m_BloodLossIconsShader.destroy();
            */

    // delete_data(m_pSndMessagesInPlay);
    delete_data(m_pSndMessages);

    deinit_compress_buffer();

    //	xr_delete(m_pSpeechMenu);
    DestroyMessagesMenus();

    //	xr_delete(pBuySpawnMsgBox);

    m_pBonusList.clear();

    xr_delete(m_pVoteRespondWindow);
    xr_delete(m_pVoteStartWindow);
    xr_delete(m_pAdminMenuWindow);
    xr_delete(m_pMessageBox);

    xr_delete(m_reward_generator);
    xr_delete(m_reward_manager);
    local_player = NULL;
};

bool game_cl_mp::CanBeReady() { return true; }
bool game_cl_mp::NeedToSendReady_Actor(int key, game_PlayerState* ps)
{
    return ((GAME_PHASE_PENDING == Phase()) || true == ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)) &&
        (kWPN_FIRE == key);
}

bool game_cl_mp::NeedToSendReady_Spectator(int key, game_PlayerState* ps)
{
    return (GAME_PHASE_PENDING == Phase() && kWPN_FIRE == key) ||
        (kJUMP == key && GAME_PHASE_INPROGRESS == Phase() && CanBeReady() && ps->DeathTime > 1000);
}

bool game_cl_mp::OnKeyboardPress(int key)
{
    if (inherited::OnKeyboardPress(key))
        return true;

    if (kJUMP == key || kWPN_FIRE == key)
    {
        bool b_need_to_send_ready = false;

        IGameObject* curr = Level().CurrentControlEntity();
        if (!curr)
            return (false);

        bool is_actor = !!smart_cast<CActor*>(curr);
        bool is_spectator = !!smart_cast<CSpectator*>(curr);

        game_PlayerState* ps = local_player;

        if (is_actor)
        {
            b_need_to_send_ready = NeedToSendReady_Actor(key, ps);
        };
        if (is_spectator)
        {
            b_need_to_send_ready = NeedToSendReady_Spectator(key, ps);
        };
        if (b_need_to_send_ready)
        {
            CGameObject* GO = smart_cast<CGameObject*>(curr);
#ifdef DEBUG
            Msg("---I'm ready (ID = %d) sending player ready packet !!!", GO->ID());
#endif // #ifdef DEBUG
            NET_Packet P;
            GO->u_EventGen(P, GE_GAME_EVENT, GO->ID());
            P.w_u16(GAME_EVENT_PLAYER_READY);
            GO->u_EventSend(P);
            return true;
        }
        else
        {
#ifdef DEBUG
            Msg("---I'm not ready, is_actor = %d, is_spectator = %d", is_actor, is_spectator);
#endif // #ifdef DEBUG
            return false;
        }
    };

    u16 game_phase = Phase();
    if ((game_phase != GAME_PHASE_INPROGRESS) && (kQUIT != key) && (kCONSOLE != key) && (kCHAT != key) &&
        (kSHOW_ADMIN_MENU != key) && (kVOTE_BEGIN != key) && (kVOTE != key) && (kVOTEYES != key) && (kVOTENO != key))
    {
        return true;
    }

    if ((game_phase == GAME_PHASE_INPROGRESS) || (game_phase == GAME_PHASE_PENDING))
    {
        switch (key)
        {
        case kCHAT:
        case kCHAT_TEAM:
        {
            CUIChatWnd* pChatWnd = m_game_ui_custom->m_pMessagesWnd->GetChatWnd();
            R_ASSERT(!pChatWnd->IsShown());
            string512 prefix;
            xr_sprintf(
                prefix, "%s> ", StringTable().translate((kCHAT_TEAM == key) ? "st_mp_say_to_team" : "st_mp_say_to_all").c_str());
            pChatWnd->ChatToAll(kCHAT == key);
            pChatWnd->SetEditBoxPrefix(prefix);
            pChatWnd->ShowDialog(false);
            return false;
        }
        break;
        case kSHOW_ADMIN_MENU:
        {
            if (!m_pAdminMenuWindow)
                m_pAdminMenuWindow = new CUIMpAdminMenu();

            if (local_player && local_player->testFlag(GAME_PLAYER_HAS_ADMIN_RIGHTS))
                m_pAdminMenuWindow->ShowDialog(true);
            else
                m_pAdminMenuWindow->ShowMessageBox(CUIMessageBox::MESSAGEBOX_RA_LOGIN);
        }
        break;
        case kVOTE_BEGIN:
        {
            if (IsVotingEnabled() && !IsVotingActive())
                VotingBegin();
            else
                OnCantVoteMsg(
                    StringTable().translate((IsVotingEnabled()) ? "st_mp_only_one_voting" : "st_mp_disabled_voting").c_str());
        }
        break;
        case kVOTE:
        {
            if (IsVotingEnabled() && IsVotingActive())
                Vote();
            else
            {
                if (!IsVotingEnabled())
                    OnCantVoteMsg(StringTable().translate("st_mp_disabled_voting").c_str());
                else
                    OnCantVoteMsg(StringTable().translate("st_mp_no_current_voting").c_str());
            }
        }
        break;
        case kVOTEYES:
        {
            if (IsVotingEnabled() && IsVotingActive())
                SendVoteYesMessage();
        }
        break;
        case kVOTENO:
        {
            if (IsVotingEnabled() && IsVotingActive())
                SendVoteNoMessage();
        }
        break;
        case kSPEECH_MENU_0:
        case kSPEECH_MENU_1:
        {
            if (!local_player || local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
                break;

            u32 MenuID = key - kSPEECH_MENU_0;
            if (MenuID >= m_aMessageMenus.size())
                break;
            cl_MessageMenu* pCurMenu = &(m_aMessageMenus[MenuID]);
            HideMessageMenus();
            if (m_cur_MenuID != MenuID)
            {
                pCurMenu->m_pSpeechMenu->ShowDialog(false);
                m_cur_MenuID = MenuID;
            }
            else
            {
                m_cur_MenuID = u32(-1);
            }
            return true;
        }
        break;
        }
    }

    m_cur_MenuID = u32(-1);
    return false;
}

void game_cl_mp::VotingBegin()
{
    if (!m_pVoteStartWindow)
        m_pVoteStartWindow = new CUIVotingCategory();

    m_pVoteStartWindow->ShowDialog(true);
}

void game_cl_mp::Vote()
{
    if (!m_pVoteRespondWindow)
        m_pVoteRespondWindow = new CUIVote();

    m_pVoteRespondWindow->ShowDialog(true);
}

void game_cl_mp::OnCantVoteMsg(LPCSTR Text)
{
    if (CurrentGameUI())
        CurrentGameUI()->CommonMessageOut(Text);
}

void game_cl_mp::GetActiveVoting()
{
    NET_Packet P;
    u_EventGen(P, GE_GAME_EVENT, 0);
    P.w_u16(GAME_EVENT_GET_ACTIVE_VOTE);
    u_EventSend(P);
}

u32 Color_Teams_u32[3] = { color_rgba(255, 240, 190, 255), color_rgba(64, 255, 64, 255), color_rgba(64, 64, 255, 255) };
constexpr pcstr Color_Teams[3] = {"%c[255,255,240,190]", "%c[255,64,255,64]", "%c[255,64,64,255]"};
constexpr char Color_Main[] = "%c[255,192,192,192]";
u32 Color_Neutral_u32 = color_rgba(255, 0, 255, 255);
constexpr char Color_Red[] = "%c[255,255,1,1]";
constexpr char Color_Green[] = "%c[255,1,255,1]";

void game_cl_mp::TranslateGameMessage(u32 msg, NET_Packet& P)
{
    string4096 Text;

    switch (msg)
    {
    case GAME_EVENT_PLAYER_KILLED: // dm
    {
        OnPlayerKilled(P);
    }
    break;
    case GAME_EVENT_VOTE_START:
    {
        xr_sprintf(Text, "%s%s", Color_Main, *StringTable().translate("mp_voting_started_msg"));
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);
        OnVoteStart(P);
    }
    break;
    case GAME_EVENT_VOTE_STOP:
    {
        xr_sprintf(Text, "%s%s", Color_Main, *StringTable().translate("mp_voting_broken"));
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);

        OnVoteStop(P);
    }
    break;
    case GAME_EVENT_VOTE_END:
    {
        string4096 Reason;
        P.r_stringZ(Reason);
        xr_sprintf(Text, "%s%s", Color_Main, *StringTable().translate(Reason));
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);
        OnVoteEnd(P);
    }
    break;
    case GAME_EVENT_PLAYER_NAME: { OnPlayerChangeName(P);
    }
    break;
    case GAME_EVENT_SPEECH_MESSAGE: { OnSpeechMessage(P);
    }
    break;
    case GAME_EVENT_PLAYERS_MONEY_CHANGED: { OnEventMoneyChanged(P);
    }
    break;
    case GAME_EVENT_PLAYER_GAME_MENU_RESPOND: { OnGameMenuRespond(P);
    }
    break;
    case GAME_EVENT_ROUND_STARTED: { OnGameRoundStarted();
#ifdef DEBUG
        Msg("--- On round started !!!");
#endif // #ifdef DEBUG
    }
    break;
    case GAME_EVENT_ROUND_END:
    {
        string64 reason;
        P.r_stringZ(reason);
#ifdef DEBUG
        Msg("--- On round end !!!");
#endif // #ifdef DEBUG
    }
    break;
    case GAME_EVENT_SERVER_STRING_MESSAGE:
    {
        string1024 mess;
        P.r_stringZ(mess);
        xr_sprintf(Text, "%s%s", Color_Red, *StringTable().translate(mess));
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);
    }
    break;
    case GAME_EVENT_SERVER_DIALOG_MESSAGE:
    {
        string1024 mess;
        P.r_stringZ(mess);
        Msg(mess);
        if (MainMenu() && !GEnv.isDedicatedServer)
        {
            MainMenu()->OnSessionTerminate(mess);
        }
    }
    break;
    case GAME_EVENT_MAKE_DATA:
    {
        clientdata_event_t etype = static_cast<clientdata_event_t>(P.r_u8());
        if (etype == e_screenshot_request)
        {
            screenshot_manager::complete_callback_t compl_cb =
                fastdelegate::MakeDelegate(this, &game_cl_mp::SendCollectedData);
            ss_manager.make_screenshot(compl_cb);
        }
        else if (etype == e_configs_request)
        {
            mp_anticheat::configs_dumper::complete_callback_t compl_cb =
                fastdelegate::MakeDelegate(this, &game_cl_mp::SendCollectedData);
            cd_manager.dump_config(compl_cb);
        }
        else if (etype == e_screenshot_response)
        {
            ClientID tmp_client(P.r_u32());
            shared_str client_name;
            P.r_stringZ(client_name);
            PrepareToReceiveFile(tmp_client, client_name, e_screenshot_response);
        }
        else if (etype == e_configs_response)
        {
            ClientID tmp_client(P.r_u32());
            shared_str client_name;
            P.r_stringZ(client_name);
            PrepareToReceiveFile(tmp_client, client_name, e_configs_response);
        }
        else
        {
            ClientID tmp_client(P.r_u32());
            shared_str error_msg;
            P.r_stringZ(error_msg);
            Msg("! File transfer error: from client [%u]: %s", tmp_client.value(), error_msg.c_str());
        }
    }
    break;
    case GAME_EVENT_RECEIVE_SERVER_LOGO:
    {
        ClientID tmp_client(P.r_u32());
        start_receive_server_info(tmp_client);
    }
    break;
    case GAME_EVENT_PLAYER_BUYMENU_CLOSE: { m_ready_to_open_buy_menu = true;
    }
    break;
    case GAME_EVENT_PLAYERS_INFO_REPLY: { ProcessPlayersInfoReply(P);
    }
    break;
    default: inherited::TranslateGameMessage(msg, P);
    }
}

//////////////////////////////////////////////////////////////////////////

void game_cl_mp::ChatSay(LPCSTR phrase, bool bAll)
{
    s16 team = ModifyTeam(local_player->team) + 1;

    NET_Packet P;
    P.w_begin(M_CHAT_MESSAGE);
    P.w_s16((bAll) ? -1 : local_player->team); // -1 = all, 0 = green, 1 = blue
    P.w_stringZ(local_player->getName());
    P.w_stringZ(phrase);
    P.w_s16(team);
    u_EventSend(P);
}

void game_cl_mp::OnWarnMessage(NET_Packet* P)
{
    u8 msg_type = P->r_u8();
    if (msg_type == 1)
    {
        u16 _ping = P->r_u16();
        u8 _cnt = P->r_u8();
        u8 _total = P->r_u8();

        if (CurrentGameUI())
        {
            string512 _buff;
            xr_sprintf(_buff, "max_ping_warn_%d", _cnt);
            StaticDrawableWrapper* ss = CurrentGameUI()->AddCustomStatic(_buff, true);

            xr_sprintf(_buff, "%d ms.", _ping);
            ss->m_static->TextItemControl()->SetText(_buff);
            CUIWindow* w = ss->m_static->FindChild("auto_static_0");
            if (w)
            {
                xr_sprintf(_buff, "%d/%d", _cnt, _total);
                CUIStatic* s = smart_cast<CUIStatic*>(w);
                s->TextItemControl()->SetText(_buff);
            }
        }
    }
}

void game_cl_mp::OnChatMessage(NET_Packet* P)
{
    shared_str PlayerName;
    shared_str ChatMsg;
    s16 team;

    P->r_s16();
    P->r_stringZ(PlayerName);
    P->r_stringZ(ChatMsg);
    P->r_s16(team);

    ///#ifdef DEBUG
    switch (team)
    {
    case 0: Msg("%s: %s : %s", *StringTable().translate("mp_chat"), PlayerName.c_str(), ChatMsg.c_str()); break;
    case 1: Msg("- %s: %s : %s", *StringTable().translate("mp_chat"), PlayerName.c_str(), ChatMsg.c_str()); break;
    case 2: Msg("@ %s: %s : %s", *StringTable().translate("mp_chat"), PlayerName.c_str(), ChatMsg.c_str()); break;
    }

    //#endif
    if (GEnv.isDedicatedServer)
        return;

    if (team < 0 || 2 < team)
    {
        team = 0;
    }

    LPSTR colPlayerName;
    STRCONCAT(colPlayerName, Color_Teams[team], PlayerName, ":%c[default]");
    if (Level().CurrentViewEntity() && CurrentGameUI())
        CurrentGameUI()->m_pMessagesWnd->AddChatMessage(ChatMsg, colPlayerName);
};

void game_cl_mp::shedule_Update(u32 dt)
{
    UpdateSndMessages();

    inherited::shedule_Update(dt);
    //-----------------------------------------

    if (GEnv.isDedicatedServer)
        return;

    if (m_reward_generator)
        m_reward_generator->update();
    if (m_reward_manager)
        m_reward_manager->update_tasks();

    switch (Phase())
    {
    case GAME_PHASE_PENDING:
    {
        // CUIChatWnd* pChatWnd = CurrentGameUI()->m_pMessagesWnd->GetChatWnd();
        // if (pChatWnd && pChatWnd->IsShown())
        //	StartStopMenu(pChatWnd, false);

        if (m_bJustRestarted)
        {
            if (Level().CurrentViewEntity())
            {
                PlaySndMessage(ID_READY);
                m_bJustRestarted = false;
            };
        }
    }
    break;
    case GAME_PHASE_INPROGRESS:
    {
        if (!local_player || local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
        {
            HideMessageMenus();
        };
    }
    break;
    default:
    {
        CUIChatWnd* pChatWnd = CurrentGameUI()->m_pMessagesWnd->GetChatWnd();
        if (pChatWnd && pChatWnd->IsShown())
            pChatWnd->HideDialog();
    }
    break;
    }
    UpdateMapLocations();

    u32 cur_game_state = Phase();

    if ((cur_game_state != GAME_PHASE_INPROGRESS) && (cur_game_state != GAME_PHASE_PENDING))
    {
        if (m_pVoteStartWindow && m_pVoteStartWindow->IsShown())
        {
            m_pVoteStartWindow->HideDialog();
        }
        if (m_pMessageBox && m_pMessageBox->IsShown())
        {
            m_pMessageBox->HideDialog();
        }
        if (m_pVoteRespondWindow && m_pVoteRespondWindow->IsShown()) // && IsVotingActive())
        {
            m_pVoteRespondWindow->HideDialog();
        }
    }
}

void game_cl_mp::SendStartVoteMessage(LPCSTR args)
{
    if (!args)
        return;
    if (!IsVotingEnabled())
        return;
    NET_Packet P;
    Game().u_EventGen(P, GE_GAME_EVENT, Game().local_player->GameID);
    P.w_u16(GAME_EVENT_VOTE_START);
    P.w_stringZ(args);
    Game().u_EventSend(P);
};

void game_cl_mp::SendVoteYesMessage()
{
    if (!IsVotingEnabled() || !IsVotingActive())
        return;
    NET_Packet P;
    Game().u_EventGen(P, GE_GAME_EVENT, Game().local_player->GameID);
    P.w_u16(GAME_EVENT_VOTE_YES);
    Game().u_EventSend(P);
};
void game_cl_mp::SendVoteNoMessage()
{
    if (!IsVotingEnabled() || !IsVotingActive())
        return;
    NET_Packet P;
    Game().u_EventGen(P, GE_GAME_EVENT, Game().local_player->GameID);
    P.w_u16(GAME_EVENT_VOTE_NO);
    Game().u_EventSend(P);
};

void game_cl_mp::OnVoteStart(NET_Packet& P) { SetVotingActive(true); };
void game_cl_mp::OnVoteStop(NET_Packet& P)
{
    SetVotingActive(false);
    if (m_pVoteRespondWindow && m_pVoteRespondWindow->IsShown())
    {
        m_pVoteRespondWindow->HideDialog();
    }
};

void game_cl_mp::OnVoteEnd(NET_Packet& P) { SetVotingActive(false); };
void game_cl_mp::OnPlayerVoted(game_PlayerState* ps)
{
    if (!IsVotingActive())
        return;
    if (ps->m_bCurrentVoteAgreed == 2)
        return;

    string1024 resStr;
    xr_sprintf(resStr, "%s\"%s\" %s%s %s\"%s\"", Color_Teams[ps->team], ps->getName(), Color_Main,
        *StringTable().translate("mp_voted"), ps->m_bCurrentVoteAgreed ? Color_Green : Color_Red,
        *StringTable().translate(ps->m_bCurrentVoteAgreed ? "mp_voted_yes" : "mp_voted_no"));
    if (CurrentGameUI())
        CurrentGameUI()->CommonMessageOut(resStr);
}
void game_cl_mp::LoadTeamData(const shared_str& TeamName)
{
    cl_TeamStruct Team;
    Team.IndicatorPos.set(0.f, 0.f, 0.f);
    Team.Indicator_r1 = 0.f;
    Team.Indicator_r2 = 0.f;

    Team.caSection = TeamName;
    if (pSettings->section_exist(TeamName))
    {
        Team.Indicator_r1 = pSettings->r_float(TeamName, "indicator_r1");
        Team.Indicator_r2 = pSettings->r_float(TeamName, "indicator_r2");

        Team.IndicatorPos.x = pSettings->r_float(TeamName, "indicator_x");
        Team.IndicatorPos.y = pSettings->r_float(TeamName, "indicator_y");
        Team.IndicatorPos.z = pSettings->r_float(TeamName, "indicator_z");

        LPCSTR ShaderType = pSettings->r_string(TeamName, "indicator_shader");
        LPCSTR ShaderTexture = pSettings->r_string(TeamName, "indicator_texture");
        Team.IndicatorShader->create(ShaderType, ShaderTexture);

        ShaderType = pSettings->r_string(TeamName, "invincible_shader");
        ShaderTexture = pSettings->r_string(TeamName, "invincible_texture");
        Team.InvincibleShader->create(ShaderType, ShaderTexture);
    };
    TeamList.push_back(Team);
}

void game_cl_mp::OnSwitchPhase_InProgress(){};

void game_cl_mp::OnSwitchPhase(u32 old_phase, u32 new_phase)
{
    inherited::OnSwitchPhase(old_phase, new_phase);
    switch (new_phase)
    {
    case GAME_PHASE_INPROGRESS:
    {
        if (m_reward_generator)
            m_reward_generator->OnRoundStart();

        m_bSpectatorSelected = FALSE;

        if (CurrentGameUI())
        {
            CurrentGameUI()->ShowGameIndicators(true);
            CurrentGameUI()->m_pMessagesWnd->PendingMode(false);
        }
    }
    break;
    case GAME_PHASE_PENDING:
    {
        m_bJustRestarted = true;
        HideMessageMenus();
        if (old_phase == GAME_PHASE_INPROGRESS)
        {
            if (CurrentGameUI())
            {
                CurrentGameUI()->ShowGameIndicators(true);
                CurrentGameUI()->m_pMessagesWnd->PendingMode(true);
            }
        }
    };

    case GAME_PHASE_TEAM1_SCORES:
    case GAME_PHASE_TEAM2_SCORES:
    case GAME_PHASE_TEAM1_ELIMINATED:
    case GAME_PHASE_TEAM2_ELIMINATED:
    case GAME_PHASE_TEAMS_IN_A_DRAW:
    case GAME_PHASE_PLAYER_SCORES: { HideMessageMenus();
    }
    break;
    default:
    {
        if (g_hud && CurrentGameUI())
            CurrentGameUI()->ShowGameIndicators(false);
        HideMessageMenus();
    }
    break;
    }
}

const ui_shader& game_cl_mp::GetEquipmentIconsShader()
{
    if (m_EquipmentIconsShader->inited())
        return m_EquipmentIconsShader;

    m_EquipmentIconsShader->create("hud" DELIMITER "default", "ui" DELIMITER "ui_mp_icon_kill");
    return m_EquipmentIconsShader;
}

const ui_shader& game_cl_mp::GetKillEventIconsShader()
{
    return GetEquipmentIconsShader();
    /*
    if (m_KillEventIconsShader) return m_KillEventIconsShader;

    m_KillEventIconsShader.create("hud" DELIMITER "default", KILLEVENT_ICONS);
    return m_KillEventIconsShader;
    */
}

const ui_shader& game_cl_mp::GetRadiationIconsShader()
{
    return GetEquipmentIconsShader();
    /*
    if (m_RadiationIconsShader) return m_RadiationIconsShader;

    m_RadiationIconsShader.create("hud" DELIMITER "default", RADIATION_ICONS);
    return m_RadiationIconsShader;
    */
}

const ui_shader& game_cl_mp::GetBloodLossIconsShader()
{
    return GetEquipmentIconsShader();
    /*
    if (m_BloodLossIconsShader) return m_BloodLossIconsShader;

    m_BloodLossIconsShader.create("hud" DELIMITER "default", BLOODLOSS_ICONS);
    return m_BloodLossIconsShader;
    */
}
const ui_shader& game_cl_mp::GetRankIconsShader()
{
    if (m_RankIconsShader->inited())
        return m_RankIconsShader;

    m_RankIconsShader->create("hud" DELIMITER "default", RANK_ICONS);
    return m_RankIconsShader;
}

void game_cl_mp::OnPlayerKilled(NET_Packet& P)
{
    //-----------------------------------------------------------
    KILL_TYPE KillType = KILL_TYPE(P.r_u8());
    u16 KilledID = P.r_u16();
    u16 KillerID = P.r_u16();
    u16 WeaponID = P.r_u16();
    SPECIAL_KILL_TYPE SpecialKill = SPECIAL_KILL_TYPE(P.r_u8());
    if (m_reward_generator)
        m_reward_generator->OnPlayerKilled(KillerID, KilledID, WeaponID, std::make_pair(KillType, SpecialKill));
    //-----------------------------------------------------------
    IGameObject* pOKiller = Level().Objects.net_Find(KillerID);
    IGameObject* pWeapon = Level().Objects.net_Find(WeaponID);

    game_PlayerState* pPlayer = GetPlayerByGameID(KilledID);
    if (!pPlayer)
    {
#ifndef MASTER_GOLD
        Msg("! Non existant player[%d] killed by [%d] with [%d]", KilledID, KillerID, WeaponID);
#endif // #ifndef MASTER_GOLD
        return;
    }
    R_ASSERT(pPlayer);
    game_PlayerState* pKiller = GetPlayerByGameID(KillerID);
    //	R_ASSERT(pKiller);
    //-----------------------------------------------------------
    KillMessageStruct KMS;
    KMS.m_victim.m_name = pPlayer->getName();
    KMS.m_victim.m_color = Color_Teams_u32[ModifyTeam(pPlayer->team) + 1];

    KMS.m_killer.m_name = NULL;
    KMS.m_killer.m_color = color_rgba(255, 255, 255, 255);

    switch (KillType)
    {
    //-----------------------------------------------------------
    case KT_HIT: // from hit
    {
        string1024 sWeapon = "", sSpecial = "";
        if (pWeapon)
        {
            CInventoryItem* pIItem = smart_cast<CInventoryItem*>(pWeapon);
            if (pIItem)
            {
                KMS.m_initiator.m_shader = GetEquipmentIconsShader();
                if (smart_cast<CExplosiveItem*>(pIItem))
                {
                    KMS.m_initiator.m_shader = GetKillEventIconsShader();
                    KMS.m_initiator.m_rect.x1 = 1;
                    KMS.m_initiator.m_rect.y1 = 202;
                    KMS.m_initiator.m_rect.x2 = KMS.m_initiator.m_rect.x1 + 31;
                    KMS.m_initiator.m_rect.y2 = KMS.m_initiator.m_rect.y1 + 30;
                    xr_sprintf(sWeapon, *StringTable().translate("mp_by_explosion"));
                }
                else
                {
                    KMS.m_initiator.m_rect = pIItem->GetKillMsgRect();
                    KMS.m_initiator.m_rect.rb.add(KMS.m_initiator.m_rect.lt);
                    xr_sprintf(sWeapon, "%s %s", StringTable().translate("mp_from").c_str(), pIItem->NameShort());
                }
            }
            else
            {
                CCustomZone* pAnomaly = smart_cast<CCustomZone*>(pWeapon);
                if (pAnomaly)
                {
                    KMS.m_initiator.m_shader = GetKillEventIconsShader();
                    KMS.m_initiator.m_rect.x1 = 1;
                    KMS.m_initiator.m_rect.y1 = 202;
                    KMS.m_initiator.m_rect.x2 = KMS.m_initiator.m_rect.x1 + 31;
                    KMS.m_initiator.m_rect.y2 = KMS.m_initiator.m_rect.y1 + 30;
                    xr_sprintf(sWeapon, *StringTable().translate("mp_by_anomaly"));
                }
            }
        }

        if (pKiller || pOKiller)
        {
            if (!pKiller)
            {
                CCustomZone* pAnomaly = smart_cast<CCustomZone*>(pOKiller);
                if (pAnomaly)
                {
                    KMS.m_initiator.m_shader = GetKillEventIconsShader();
                    KMS.m_initiator.m_rect.x1 = 1;
                    KMS.m_initiator.m_rect.y1 = 202;
                    KMS.m_initiator.m_rect.x2 = KMS.m_initiator.m_rect.x1 + 31;
                    KMS.m_initiator.m_rect.y2 = KMS.m_initiator.m_rect.y1 + 30;
                    Msg("%s killed by anomaly", *KMS.m_victim.m_name);
                    break;
                }
            };

            if (pKiller)
            {
                KMS.m_killer.m_name = pKiller ? pKiller->getName() : *(pOKiller->cNameSect());
                KMS.m_killer.m_color = pKiller ? Color_Teams_u32[ModifyTeam(pKiller->team) + 1] : Color_Neutral_u32;
            };
        };
        //-------------------------------------------
        switch (SpecialKill)
        {
        case SKT_NONE: // not special
        {
            if (pOKiller && pOKiller == Level().CurrentViewEntity())
            {
                // if (pWeapon && pWeapon->CLS_ID == CLSID_OBJECT_W_KNIFE)
                if (smart_cast<CWeaponKnife*>(pWeapon))
                {
                    PlaySndMessage(ID_BUTCHER);
                }
            };
        }
        break;
        case SKT_HEADSHOT: // Head Shot
        {
            auto it = std::find(m_pBonusList.begin(), m_pBonusList.end(), "headshot");
            if (it != m_pBonusList.end() && (*it == "headshot"))
            {
                Bonus_Struct* pBS = &(*it);
                KMS.m_ext_info.m_shader = pBS->IconShader;
                KMS.m_ext_info.m_rect.x1 = pBS->IconRects[0].x1;
                KMS.m_ext_info.m_rect.y1 = pBS->IconRects[0].y1;
                KMS.m_ext_info.m_rect.x2 = pBS->IconRects[0].x1 + pBS->IconRects[0].x2;
                KMS.m_ext_info.m_rect.y2 = pBS->IconRects[0].y1 + pBS->IconRects[0].y2;
            };

            xr_sprintf(sSpecial, *StringTable().translate("mp_with_headshot"));

            if (pOKiller && pOKiller == Level().CurrentViewEntity())
                PlaySndMessage(ID_HEADSHOT);
        }
        break;
        case SKT_EYESHOT:
        {
            auto it = std::find(m_pBonusList.begin(), m_pBonusList.end(), "eyeshot");
            if (it != m_pBonusList.end() && (*it == "eyeshot"))
            {
                Bonus_Struct* pBS = &(*it);
                KMS.m_ext_info.m_shader = pBS->IconShader;
                KMS.m_ext_info.m_rect.x1 = pBS->IconRects[0].x1;
                KMS.m_ext_info.m_rect.y1 = pBS->IconRects[0].y1;
                KMS.m_ext_info.m_rect.x2 = pBS->IconRects[0].x1 + pBS->IconRects[0].x2;
                KMS.m_ext_info.m_rect.y2 = pBS->IconRects[0].y1 + pBS->IconRects[0].y2;
            };

            xr_sprintf(sSpecial, *StringTable().translate("mp_with_eyeshot"));

            if (pOKiller && pOKiller == Level().CurrentViewEntity())
                PlaySndMessage(ID_ASSASSIN);
        }
        break;
        case SKT_BACKSTAB: // BackStab
        {
            auto it = std::find(m_pBonusList.begin(), m_pBonusList.end(), "backstab");
            if (it != m_pBonusList.end() && (*it == "backstab"))
            {
                Bonus_Struct* pBS = &(*it);
                KMS.m_ext_info.m_shader = pBS->IconShader;
                KMS.m_ext_info.m_rect.x1 = pBS->IconRects[0].x1;
                KMS.m_ext_info.m_rect.y1 = pBS->IconRects[0].y1;
                KMS.m_ext_info.m_rect.x2 = pBS->IconRects[0].x1 + pBS->IconRects[0].x2;
                KMS.m_ext_info.m_rect.y2 = pBS->IconRects[0].y1 + pBS->IconRects[0].y2;
            };

            xr_sprintf(sSpecial, *StringTable().translate("mp_with_backstab"));
            if (pOKiller && pOKiller == Level().CurrentViewEntity())
                PlaySndMessage(ID_ASSASSIN);
        }
        break;
        }
        // suicide
        if (KilledID == KillerID)
        {
            KMS.m_victim.m_name = NULL;

            KMS.m_ext_info.m_shader = GetKillEventIconsShader();
            KMS.m_ext_info.m_rect.x1 = 32;
            KMS.m_ext_info.m_rect.y1 = 202;
            KMS.m_ext_info.m_rect.x2 = KMS.m_ext_info.m_rect.x1 + 30;
            KMS.m_ext_info.m_rect.y2 = KMS.m_ext_info.m_rect.y1 + 30;
            //-------------------------------------
            Msg(sWeapon[0] ? "%s killed himself by %s" : "%s killed himself", *KMS.m_killer.m_name,
                sWeapon[0] ? sWeapon + 5 : "");
        }
        else
        {
            //-------------------------------------
            Msg("%s killed %s %s%s", *KMS.m_killer.m_name, *KMS.m_victim.m_name, sWeapon, sSpecial[0] ? sSpecial : "");
        }
    }
    break;
    //-----------------------------------------------------------
    case KT_BLEEDING: // from bleeding
    {
        KMS.m_initiator.m_shader = GetBloodLossIconsShader();
        KMS.m_initiator.m_rect.x1 = 238;
        KMS.m_initiator.m_rect.y1 = 31;
        KMS.m_initiator.m_rect.x2 = KMS.m_initiator.m_rect.x1 + 17;
        KMS.m_initiator.m_rect.y2 = KMS.m_initiator.m_rect.y1 + 26;

        if (!pKiller)
        {
            CCustomZone* pAnomaly = smart_cast<CCustomZone*>(pOKiller);
            if (pAnomaly)
            {
                KMS.m_ext_info.m_shader = GetKillEventIconsShader();
                KMS.m_ext_info.m_rect.x1 = 1;
                KMS.m_ext_info.m_rect.y1 = 202;
                KMS.m_ext_info.m_rect.x2 = KMS.m_ext_info.m_rect.x1 + 31;
                KMS.m_ext_info.m_rect.y2 = KMS.m_ext_info.m_rect.y1 + 30;

                Msg("%s died from bleeding, thanks to anomaly", *KMS.m_victim.m_name);
                break;
            }
        };

        if (pKiller)
        {
            KMS.m_killer.m_name = pKiller ? pKiller->getName() : *(pOKiller->cNameSect());
            KMS.m_killer.m_color = pKiller ? Color_Teams_u32[ModifyTeam(pKiller->team) + 1] : Color_Neutral_u32;
            //-----------------------------------------------------------------------
            Msg("%s died from bleeding, thanks to %s ", *KMS.m_victim.m_name, *KMS.m_killer.m_name);
        }
        else
        {
            //-----------------------------------------------------------------
            Msg("%s died from bleeding", *KMS.m_victim.m_name);
        };
    }
    break;
    //-----------------------------------------------------------
    case KT_RADIATION: // from radiation
    {
        KMS.m_initiator.m_shader = GetRadiationIconsShader();
        KMS.m_initiator.m_rect.x1 = 215;
        KMS.m_initiator.m_rect.y1 = 195;
        KMS.m_initiator.m_rect.x2 = KMS.m_initiator.m_rect.x1 + 24;
        KMS.m_initiator.m_rect.y2 = KMS.m_initiator.m_rect.y1 + 24;
        //---------------------------------------------------------
        Msg("%s killed by radiation", *KMS.m_victim.m_name);
    }
    break;
    default: break;
    }
    if (CurrentGameUI() && CurrentGameUI()->m_pMessagesWnd)
        CurrentGameUI()->m_pMessagesWnd->AddLogMessage(KMS);
};

extern void WritePlayerName_ToRegistry(LPSTR name);

void game_cl_mp::OnPlayerChangeName(NET_Packet& P)
{
    u16 ObjID = P.r_u16();
    s16 Team = P.r_s16();
    string1024 OldName, NewName;
    P.r_stringZ(OldName);
    P.r_stringZ(NewName);

    string1024 resStr;
    xr_sprintf(resStr, "%s\"%s\" %s%s %s\"%s\"", Color_Teams[Team], OldName, Color_Main, *StringTable().translate("mp_is_now"),
        Color_Teams[Team], NewName);
    if (CurrentGameUI())
        CurrentGameUI()->CommonMessageOut(resStr);
    Msg(NewName);
    //-------------------------------------------
    IGameObject* pObj = Level().Objects.net_Find(ObjID);
    if (pObj)
    {
        pObj->cName_set(NewName);
    }

    if (Game().local_player && Game().local_player->GameID == ObjID)
    {
        WritePlayerName_ToRegistry(NewName);
    }
}

void game_cl_mp::LoadSndMessages()
{
    LoadSndMessage("mp_snd_messages", "headshot", ID_HEADSHOT);
    LoadSndMessage("mp_snd_messages", "butcher", ID_BUTCHER);
    LoadSndMessage("mp_snd_messages", "assassin", ID_ASSASSIN);
    LoadSndMessage("mp_snd_messages", "ready", ID_READY);
    LoadSndMessage("mp_snd_messages", "match_started", ID_MATCH_STARTED);
};

void game_cl_mp::OnRankChanged(u8 OldRank)
{
    string256 tmp;
    string1024 RankStr;
    xr_sprintf(tmp, "rank_%d", local_player->rank);
    xr_sprintf(RankStr, "%s : %s", *StringTable().translate("mp_your_rank"),
        *StringTable().translate(READ_IF_EXISTS(pSettings, r_string, tmp, "rank_name", "")));
    if (CurrentGameUI())
        CurrentGameUI()->CommonMessageOut(RankStr);
#ifdef DEBUG
    Msg("- %s", RankStr);
#endif
    if (m_reward_generator)
        m_reward_generator->OnPlayerRankdChanged();
};

void game_cl_mp::net_import_update(NET_Packet& P)
{
    u8 OldRank = u8(-1);
    s16 OldTeam = -1;
    if (local_player)
    {
        OldRank = local_player->rank;
        OldTeam = local_player->team;
    };
    //---------------------------------------------
    inherited::net_import_update(P);
    //---------------------------------------------
    if (local_player)
    {
        if (OldTeam != local_player->team)
            OnTeamChanged();
        if (OldRank != local_player->rank)
            OnRankChanged(OldRank);
    };
}

void game_cl_mp::net_import_state(NET_Packet& P)
{
    u8 OldRank = u8(-1);
    s16 OldTeam = -1;
    if (local_player)
    {
        OldRank = local_player->rank;
        OldTeam = local_player->team;
    };

    inherited::net_import_state(P);

    if (local_player)
    {
        if (OldTeam != local_player->team)
            OnTeamChanged();
        if (OldRank != local_player->rank)
            OnRankChanged(OldRank);
    };
    //-------------------------------------------------------------
    m_u8SpectatorModes = P.r_u8();

    m_bSpectator_FreeFly = (m_u8SpectatorModes & (1 << CSpectator::eacFreeFly)) != 0;
    m_bSpectator_FirstEye = (m_u8SpectatorModes & (1 << CSpectator::eacFirstEye)) != 0;
    m_bSpectator_LookAt = (m_u8SpectatorModes & (1 << CSpectator::eacLookAt)) != 0;
    m_bSpectator_FreeLook = (m_u8SpectatorModes & (1 << CSpectator::eacFreeLook)) != 0;
    m_bSpectator_TeamCamera = (m_u8SpectatorModes & (1 << CSpectator::eacMaxCam)) != 0;
}

bool game_cl_mp::Is_Spectator_Camera_Allowed(CSpectator::EActorCameras Camera)
{
    if (Level().IsDemoPlay()) // all cameras allowed in demo play mode
        return true;
    /*
    switch (Camera)
    {
    case CSpectator::eacFreeFly		 : return m_bSpectator_FreeFly	;
    case CSpectator::eacFirstEye	 : return m_bSpectator_FirstEye	;
    case CSpectator::eacLookAt		 : return m_bSpectator_LookAt	;
    case CSpectator::eacFreeLook	 : return m_bSpectator_FreeLook	;
    }
    return false;
    */
    return (!!(m_u8SpectatorModes & (1 << Camera)));
};

void game_cl_mp::OnEventMoneyChanged(NET_Packet& P)
{
    if (!local_player)
        return;

    // CUIGameDM* pUIDM = smart_cast<CUIGameDM*>(m_game_ui_custom);
    VERIFY2(m_game_ui_custom, "game ui not initialized");
    local_player->money_for_round = P.r_s32();
    OnMoneyChanged();
    {
        string256 MoneyStr;
        xr_itoa(local_player->money_for_round, MoneyStr, 10);
        m_game_ui_custom->ChangeTotalMoneyIndicator(MoneyStr);
    }
    s32 Money_Added = P.r_s32();
    if (Money_Added != 0)
    {
        string256 MoneyStr;
        xr_sprintf(MoneyStr, (Money_Added > 0) ? "+%d" : "%d", Money_Added);
        m_game_ui_custom->DisplayMoneyChange(MoneyStr);
    };
    u8 NumBonuses = P.r_u8();
    s32 TotalBonusMoney = 0;
    shared_str BonusStr = (NumBonuses > 1) ? "Your bonuses : " : ((NumBonuses == 1) ? "Your bonus : " : "");
    for (u8 i = 0; i < NumBonuses; i++)
    {
        s32 BonusMoney = P.r_s32();
        SPECIAL_KILL_TYPE BonusReason = SPECIAL_KILL_TYPE(P.r_u8());
        u8 BonusKills = (BonusReason == SKT_KIR) ? P.r_u8() : 0;
        TotalBonusMoney += BonusMoney;
        //---------------------------------------------------------
        KillMessageStruct BMS;
        string256 MoneyStr;
        if (BonusMoney >= 0)
            xr_sprintf(MoneyStr, "+%d", BonusMoney);
        else
            xr_sprintf(MoneyStr, "-%d", BonusMoney);
        BMS.m_victim.m_name = MoneyStr;
        BMS.m_victim.m_color = 0xff00ff00;
        u32 RectID = 0;
        //---------------------------------------------------------
        shared_str BName = "";
        switch (BonusReason)
        {
        case SKT_HEADSHOT: { BName = "headshot";
        }
        break;
        case SKT_BACKSTAB: { BName = "backstab";
        }
        break;
        case SKT_KNIFEKILL: { BName = "knife_kill";
        }
        break;
        case SKT_EYESHOT: { BName = "eyeshot";
        }
        break;
        case SKT_PDA: { BName = "pda_taken";
        }
        break;
        case SKT_KIR:
        {
            BName.printf("%d_kill_in_row", BonusKills);

            xr_sprintf(MoneyStr, sizeof(MoneyStr), "%d", BonusKills);
            BMS.m_killer.m_name = MoneyStr;
            BMS.m_killer.m_color = 0xffff0000;
        }
        break;
        case SKT_NEWRANK:
        {
            BName = "new_rank";
            s16 player_team = ModifyTeam(local_player->team);
            R_ASSERT((player_team == 0) || (player_team == 1));
            RectID = ((local_player->rank) * 2) + player_team;
        }
        break;
        };
        auto it = std::find(m_pBonusList.begin(), m_pBonusList.end(), BName.c_str());
        if (it != m_pBonusList.end() && (*it == BName.c_str()))
        {
            Bonus_Struct* pBS = &(*it);

            BMS.m_initiator.m_shader = pBS->IconShader;
            BMS.m_initiator.m_rect.x1 = pBS->IconRects[RectID].x1;
            BMS.m_initiator.m_rect.y1 = pBS->IconRects[RectID].y1;
            BMS.m_initiator.m_rect.x2 = pBS->IconRects[RectID].x1 + pBS->IconRects[RectID].x2;
            BMS.m_initiator.m_rect.y2 = pBS->IconRects[RectID].y1 + pBS->IconRects[RectID].y2;
        };

        m_game_ui_custom->DisplayMoneyBonus(&BMS);
    };
};

void game_cl_mp::OnSpectatorSelect()
{
    IGameObject* l_pObj = Level().CurrentEntity();

    CGameObject* l_pPlayer = smart_cast<CGameObject*>(l_pObj);
    if (!l_pPlayer)
        return;

    NET_Packet P;
    l_pPlayer->u_EventGen(P, GE_GAME_EVENT, l_pPlayer->ID());
    //	P.w_u16(GAME_EVENT_PLAYER_SELECT_SPECTATOR);
    P.w_u16(GAME_EVENT_PLAYER_GAME_MENU);
    P.w_u8(PLAYER_SELECT_SPECTATOR);
    l_pPlayer->u_EventSend(P);

    m_bSpectatorSelected = TRUE;
};

void game_cl_mp::OnGameMenuRespond(NET_Packet& P)
{
    u8 Respond = P.r_u8();
    switch (Respond)
    {
    case PLAYER_SELECT_SPECTATOR: { OnGameMenuRespond_Spectator(P);
    }
    break;
    case PLAYER_CHANGE_TEAM: { OnGameMenuRespond_ChangeTeam(P);
    }
    break;
    case PLAYER_CHANGE_SKIN: { OnGameMenuRespond_ChangeSkin(P);
    }
    break;
    }
};

void game_cl_mp::OnGameRoundStarted()
{
    //			xr_sprintf(Text, "%sRound started !!!",Color_Main);
    string512 Text;
    xr_sprintf(Text, "%s%s", Color_Main, *StringTable().translate("mp_match_started"));
    if (CurrentGameUI())
        CurrentGameUI()->CommonMessageOut(Text);
    OnSwitchPhase_InProgress();
    //-------------------------------
    PlaySndMessage(ID_MATCH_STARTED);

    if (local_player && IsLocalPlayerInitialized())
    {
        OnTeamChanged();
        OnMoneyChanged();
    }
    SendPlayerStarted();
    m_ready_to_open_buy_menu = true;
}

void game_cl_mp::SendPlayerStarted()
{
    LPCSTR map_name = Level().name().c_str();
    R_ASSERT2(map_name && (xr_strlen(map_name) > 0), "map name not present");

    NET_Packet P;
    u_EventGen(P, GE_GAME_EVENT, 0);
    P.w_u16(GAME_EVENT_PLAYER_STARTED);
    P.w_stringZ(map_name);
    u_EventSend(P);
}

void game_cl_mp::OnBuySpawn(CUIWindow* pWnd, void* p) { OnBuySpawnMenu_Ok(); };
void game_cl_mp::LoadBonuses()
{
    if (!pSettings->section_exist("mp_bonus_money"))
        return;
    m_pBonusList.clear();
    u32 BonusCount = pSettings->line_count("mp_bonus_money");
    for (u32 i = 0; i < BonusCount; i++)
    {
        LPCSTR line, name;
        pSettings->r_line("mp_bonus_money", i, &name, &line);
        //-------------------------------------
        string1024 tmp0, tmp1, IconStr;
        _GetItem(line, 0, tmp0);
        _GetItem(line, 1, tmp1);
        if (strstr(name, "kill_in_row"))
        {
            xr_sprintf(tmp1, "%s Kill", tmp1);
            xr_sprintf(IconStr, "kill_in_row");
        }
        else
            xr_sprintf(IconStr, "%s", name);

        //-------------------------------------
        Bonus_Struct NewBonus;
        NewBonus.BonusTypeName = name;
        NewBonus.BonusName = tmp1;
        NewBonus.MoneyStr = tmp0;
        NewBonus.Money = atol(tmp0);
        //-------------------------------------
        if (!strstr(name, "new_rank"))
        {
            string1024 IconShader, IconX, IconY, IconW, IconH;
            xr_sprintf(IconShader, "%s_shader", IconStr);
            xr_sprintf(IconX, "%s_x", IconStr);
            xr_sprintf(IconY, "%s_y", IconStr);
            xr_sprintf(IconW, "%s_w", IconStr);
            xr_sprintf(IconH, "%s_h", IconStr);
            if (pSettings->line_exist("mp_bonus_icons", IconShader))
            {
                NewBonus.IconShader->create("hud" DELIMITER "default", pSettings->r_string("mp_bonus_icons", IconShader));
            }
            Frect IconRect;
            IconRect.x1 = READ_IF_EXISTS(pSettings, r_float, "mp_bonus_icons", IconX, 0);
            IconRect.y1 = READ_IF_EXISTS(pSettings, r_float, "mp_bonus_icons", IconY, 0);
            IconRect.x2 = READ_IF_EXISTS(pSettings, r_float, "mp_bonus_icons", IconW, 0);
            IconRect.y2 = READ_IF_EXISTS(pSettings, r_float, "mp_bonus_icons", IconH, 0);
            NewBonus.IconRects.push_back(IconRect);
        }
        else
        {
            CUITextureMaster::GetTextureShader("ui_hud_status_blue_01", NewBonus.IconShader);

            Frect IconRect;
            for (u32 r = 1; r <= 5; r++)
            {
                string256 rankstr;

                xr_sprintf(rankstr, "ui_hud_status_green_0%d", r);
                IconRect = CUITextureMaster::GetTextureRect(rankstr);
                IconRect.x2 -= IconRect.x1;
                IconRect.y2 -= IconRect.y1;
                NewBonus.IconRects.push_back(IconRect);

                xr_sprintf(rankstr, "ui_hud_status_blue_0%d", r);
                IconRect = CUITextureMaster::GetTextureRect(rankstr);
                IconRect.x2 -= IconRect.x1;
                IconRect.y2 -= IconRect.y1;
                NewBonus.IconRects.push_back(IconRect);
            }
        };
        //--------------------------------------
        m_pBonusList.push_back(NewBonus);
    };
};

void game_cl_mp::OnRadminMessage(u16 type, NET_Packet* P)
{
    switch (type)
    {
    case M_REMOTE_CONTROL_AUTH:
    {
        string4096 buff;
        P->r_stringZ(buff);
        if (!GEnv.isDedicatedServer)
        {
            if (!m_pAdminMenuWindow)
                m_pAdminMenuWindow = new CUIMpAdminMenu();

            if (0 == xr_stricmp(buff, "Access permitted."))
                m_pAdminMenuWindow->ShowDialog(true);
            else
                m_pAdminMenuWindow->ShowMessageBox(CUIMessageBox::MESSAGEBOX_OK, buff);
        }

        Msg("# srv: %s", buff);
    }
    break;
    case M_REMOTE_CONTROL_CMD:
    {
        string4096 buff;
        P->r_stringZ(buff);
        Msg("# srv: %s", buff);
    }
    break;
    }
}

void game_cl_mp::OnConnected()
{
    SendPlayerStarted();
    inherited::OnConnected();
};

void __stdcall game_cl_mp::sending_screenshot_callback(
    file_transfer::sending_status_t status, u32 bytes_sent, u32 data_size)
{
    switch (status)
    {
    case file_transfer::sending_data: {
#ifdef DEBUG
        Msg("* screenshot: %d of %d bytes sent ...", bytes_sent, data_size);
#endif
    }
    break;
    case file_transfer::sending_aborted_by_user: { Msg("* screenshot: sending aborted by user...");
    }
    break;
    case file_transfer::sending_rejected_by_peer: { Msg("* screenshot: sending rejected by peer ...");
    }
    break;
    case file_transfer::sending_complete: {
#ifdef DEBUG
        Msg("* screenshot: sending complete successfully !");
#endif
    }
    break;
    };
}

void game_cl_mp::reinit_compress_buffer(u32 need_size)
{
    if (buffer_for_compress && (need_size <= buffer_for_compress_size))
        return;

    Msg("* reiniting compression buffer.");
    buffer_for_compress_size = need_size * 2;
    void* new_buffer = xr_realloc(buffer_for_compress, buffer_for_compress_size);
    buffer_for_compress = static_cast<u8*>(new_buffer);
}

void game_cl_mp::deinit_compress_buffer() { xr_free(buffer_for_compress); }
void game_cl_mp::SendCollectedData(u8 const* buffer, u32 buffer_size, u32 uncompressed_size)
{
    if (!buffer_size)
    {
        Msg("! ERROR: CL: no data to send...");
        return;
    }
    file_transfer::sending_state_callback_t sending_cb =
        fastdelegate::MakeDelegate(this, &game_cl_mp::sending_screenshot_callback);

    // screenshot is compressing in screenshot manager ...
    /*reinit_compress_buffer(buffer_size);

    u32 compressed_image_size = ppmd_compress(
        buffer_for_compress,
        buffer_for_compress_size,
        buffer,
        buffer_size
    );*/

    upload_memory_writer.clear();
    upload_memory_writer.w(buffer, buffer_size);

    Level().m_file_transfer->start_transfer_file(
        upload_memory_writer.pointer(), upload_memory_writer.size(), sending_cb, uncompressed_size);
};

xr_string game_cl_mp::generate_file_name(const xr_string& base_name, const time_t* date_time)
{
    xr_string res = sanitize_filename(base_name);

    time_t file_time = (date_time != nullptr) ? *date_time : time(nullptr);

    tm time_splitted;
    if (localtime_safe(&file_time, &time_splitted) != nullptr)
    {
        string16 date_str = {};
        xr_sprintf(date_str, "%02d%02d%02d-%02d%02d%02d_", time_splitted.tm_year % 100, time_splitted.tm_mon,
            time_splitted.tm_mday, time_splitted.tm_hour, time_splitted.tm_min, time_splitted.tm_sec);
        res = xr_string(date_str) + res;
    }
    return res;
}

xr_string game_cl_mp::sanitize_filename(const xr_string& base_name)
{
    xr_string res = base_name;

    for (size_t i = 0; i < res.length(); ++i)
    {
        static const char* DENIED_SYMBOLS = "/\\" DELIMITER "?%%*:|\"<>.";
        if (strchr(DENIED_SYMBOLS, res[i]) != nullptr)
        {
            res[i] = '_';
        }
    }
    return res;
}

void game_cl_mp::start_receive_server_info(ClientID const& svclient_id)
{
    fr_callback_binder* tmp_binder = get_receiver_cb_binder();
    R_ASSERT2(tmp_binder, "not found free file receiver");
    tmp_binder->m_file_name = "";
    tmp_binder->m_owner = this;
    tmp_binder->m_active = true;
    tmp_binder->m_downloaded_size = 0; // initial value for rendering
    tmp_binder->m_max_size = 1; // avoiding division by zero

    file_transfer::receiving_state_callback_t receiving_cb_info =
        fastdelegate::MakeDelegate(tmp_binder, &game_cl_mp::fr_callback_binder::receiving_serverinfo_callback);

    tmp_binder->m_frnode =
        Level().m_file_transfer->start_receive_file(tmp_binder->m_writer, svclient_id, receiving_cb_info);

    R_ASSERT2(tmp_binder->m_frnode, "failed to initialise server logo receiving");
}

void game_cl_mp::PrepareToReceiveFile(
    ClientID const& from_client, shared_str const& client_session_id, clientdata_event_t response_event)
{
    fr_callback_binder* tmp_binder = get_receiver_cb_binder();
    if (!tmp_binder)
    {
        Msg("! ERROR: CL: not enough receive channels (max is 32)");
        return;
    }

    if (g_draw_downloads)
    {
        draw_downloads(true);
    }
    else
    {
        draw_downloads(false);
    }

    xr_string base_name = client_session_id.c_str();
    tmp_binder->m_file_name = generate_file_name(base_name).c_str();
    tmp_binder->m_owner = this;
    tmp_binder->m_active = true;
    tmp_binder->m_downloaded_size = 0; // initial value for rendering
    tmp_binder->m_max_size = 1; // avoiding division by zero
    tmp_binder->m_response_type = response_event;

    file_transfer::receiving_state_callback_t receiving_cb =
        fastdelegate::MakeDelegate(tmp_binder, &game_cl_mp::fr_callback_binder::receiving_file_callback);

    tmp_binder->m_frnode = Level().m_file_transfer->start_receive_file(tmp_binder->m_writer, from_client, receiving_cb);
    if (!tmp_binder->m_frnode)
    {
        Msg("* screenshot: receiving failed ...");
        tmp_binder->m_active = false;
    }
}

void __stdcall game_cl_mp::fr_callback_binder::receiving_file_callback(
    file_transfer::receiving_status_t status, u32 bytes_received, u32 data_size)
{
    if (g_draw_downloads)
    {
        m_owner->draw_downloads(true);
    }
    else
    {
        m_owner->draw_downloads(false);
    }
    switch (status)
    {
    case file_transfer::receiving_data:
    {
        Msg("* file: %d of %d bytes received ...", bytes_received, data_size);
        m_downloaded_size = bytes_received;
        m_max_size = data_size;
    }
    break;
    case file_transfer::receiving_aborted_by_peer:
    {
        Msg("* file: receiving aborted by peer...");
        m_active = false;
    }
    break;
    case file_transfer::receiving_aborted_by_user:
    {
        Msg("* file: receiving aborted by user...");
        m_active = false;
    }
    break;
    case file_transfer::receiving_timeout:
    {
        Msg("* file: receiving timeout...");
        m_active = false;
    }
    break;
    case file_transfer::receiving_complete:
    {
        Msg("* file: download complete successfully !");
        switch (m_response_type)
        {
        case e_screenshot_response:
        {
            m_owner->decompress_and_save_screenshot(
                m_file_name.c_str(), m_writer.pointer(), m_writer.size(), m_frnode->get_user_param());
        }
        break;
        case e_configs_response:
        {
            m_owner->decompress_and_process_config(
                m_file_name.c_str(), m_writer.pointer(), m_writer.size(), m_frnode->get_user_param());
        }
        break;
        default: NODEFAULT;
        }; // switch (m_response_type)

        m_active = false;
    }
    break;
    };
}

void __stdcall game_cl_mp::fr_callback_binder::receiving_serverinfo_callback(
    file_transfer::receiving_status_t status, u32 bytes_received, u32 data_size)
{
    switch (status)
    {
    case file_transfer::receiving_data: {
#ifdef DEBUG
        Msg("* serverinfo: %d of %d bytes received ...", bytes_received, data_size);
#endif
        m_downloaded_size = bytes_received;
        m_max_size = data_size;
    }
    break;
    case file_transfer::receiving_aborted_by_peer:
    {
        m_owner->extract_server_info(NULL, 0);
        Msg("* serverinfo: server logo transfer aborted ...");
        m_active = false;
    }
    break;
    case file_transfer::receiving_aborted_by_user:
    {
        m_owner->extract_server_info(NULL, 0);
        Msg("* serverinfo: receiving aborted by user...");
        m_active = false;
    }
    break;
    case file_transfer::receiving_timeout:
    {
        m_owner->extract_server_info(NULL, 0);
        Msg("* serverinfo: receiving timeout...");
        m_active = false;
    }
    break;
    case file_transfer::receiving_complete:
    {
        Msg("* serverinfo: download complete successfully !");
        R_ASSERT2(m_owner->m_game_ui_custom || GEnv.isDedicatedServer, "game ui not initialized");
        if (m_owner->m_game_ui_custom)
            m_owner->extract_server_info(m_writer.pointer(), m_writer.size());
        m_active = false;
    }
    break;
    };
};

void game_cl_mp::decompress_and_save_screenshot(LPCSTR file_name, u8* data, u32 data_size, u32 file_size)
{
    if (!file_size)
    {
        Msg("! ERROR: file size to save is 0...");
        return;
    }

    reinit_compress_buffer(file_size);

    u32 original_size = ppmd_decompress(buffer_for_compress, buffer_for_compress_size, data, data_size);

    if (original_size != file_size)
    {
        Msg("! WARNING: original and downloaded file size are different !");
    }
    string_path screen_shot_path;
    FS.update_path(screen_shot_path, "$screenshots$", file_name);
    xr_strcat(screen_shot_path, ".jpg");

    IWriter* ftosave = FS.w_open(screen_shot_path);
    if (!ftosave)
    {
        Msg("! ERROR: failed to create file [%s]", file_name);
        return;
    }
    ftosave->w(buffer_for_compress, file_size);
    FS.w_close(ftosave);
}

void game_cl_mp::decompress_and_process_config(LPCSTR file_name, u8* data, u32 data_size, u32 file_size)
{
    if (!file_size)
    {
        Msg("! ERROR: file size to save is 0...");
        return;
    }

    reinit_compress_buffer(file_size);
    ppmd_yield_callback_t tmp_cb;

    u32 original_size = ppmd_decompress_mt(buffer_for_compress, buffer_for_compress_size, data, data_size, tmp_cb);

    if (original_size != file_size)
    {
        Msg("! WARNING: original and downloaded file size are different !");
    }
    string_path screen_shot_path;
    FS.update_path(screen_shot_path, "$screenshots$", file_name);
    xr_strcat(screen_shot_path, ".ltx");

    IWriter* ftosave = FS.w_open(screen_shot_path);
    if (!ftosave)
    {
        Msg("! ERROR: failed to create file [%s]", file_name);
        return;
    }
    ftosave->w(buffer_for_compress, file_size);
    FS.w_close(ftosave);
    string256 tmp_diff;
    if (!cd_verifyer.verify(buffer_for_compress, file_size, tmp_diff))
    {
        add_detected_cheater(file_name, tmp_diff);
        Msg("! CHEATER detected: %s, %s", file_name, tmp_diff);
    }
}

game_cl_mp::fr_callback_binder* game_cl_mp::get_receiver_cb_binder()
{
    for (u32 i = 0; i < MAX_PLAYERS_COUNT; ++i)
    {
        if (!m_client_receiver_cbs[i].m_active)
        {
            return &m_client_receiver_cbs[i];
        }
    }
    return NULL;
}

void game_cl_mp::add_detected_cheater(shared_str const& file_name, string256 diff)
{
    detected_cheater_t tmp_cheater;
    tmp_cheater.m_file_name = file_name;
    xr_strcpy(tmp_cheater.m_diff, diff);
    tmp_cheater.m_detect_time = Device.dwTimeGlobal;
    m_detected_cheaters.push_back(tmp_cheater);
}

struct old_detected_cheater
{
    bool operator()(game_cl_mp::detected_cheater_t const& cheater)
    {
        if (cheater.m_detect_time + game_cl_mp::detected_cheater_t::max_showing_time <= Device.dwTimeGlobal)
            return true;
        return false;
    }
};

void game_cl_mp::draw_all_active_binder_states()
{
    // drawing download states ..
    CGameFont* F = UI().Font().pFontDI;
    F->SetHeightI(0.015f);
    F->OutSetI(0.1f, 0.2f);
    F->SetColor(color_xrgb(0, 255, 0));

    for (u32 i = 0; i < MAX_PLAYERS_COUNT; ++i)
    {
        if (m_client_receiver_cbs[i].m_active)
        {
            fr_callback_binder& tmp_br = m_client_receiver_cbs[i];
            F->OutNext("%s : %02u %% ", tmp_br.m_file_name.c_str(),
                int((float(tmp_br.m_downloaded_size) / tmp_br.m_max_size) * 100));
        }
    }
    F->SetColor(color_xrgb(255, 0, 0));
    for (cheaters_collection_t::iterator i = m_detected_cheaters.begin(), ie = m_detected_cheaters.end(); i != ie; ++i)
    {
        F->OutNext("%s : cheater suspect ...", i->m_file_name.c_str());
    }

    m_detected_cheaters.erase(
        std::remove_if(m_detected_cheaters.begin(), m_detected_cheaters.end(), old_detected_cheater()),
        m_detected_cheaters.end());
}

void game_cl_mp::draw_downloads(bool draw) { ss_manager.set_draw_downloads(draw); }
void game_cl_mp::extract_server_info(u8* data_ptr, u32 data_size)
{
    UIGameMP* tmp_ui_mp_game = smart_cast<UIGameMP*>(m_game_ui_custom);
    if (!data_ptr)
    {
        tmp_ui_mp_game->SetServerLogo(NULL, 0);
        return;
    }
    using namespace file_transfer;
    buffer_vector<const_buffer_t> tmp_vector(_alloca(sizeof(const_buffer_t) * 2), 2);
    split_received_to_buffers(data_ptr, data_size, tmp_vector);
    if (tmp_vector.empty())
    {
        Msg("! ERROR: received corrupted server info");
        return;
    }
    tmp_ui_mp_game->SetServerLogo(tmp_vector[0].first, tmp_vector[0].second);
    if (tmp_vector.size() > 1)
    {
        tmp_ui_mp_game->SetServerRules(tmp_vector[1].first, tmp_vector[1].second);
    }
}

void game_cl_mp::AddRewardTask(u32 const award_id)
{
    IGameObject* tmp_view_entity = Level().CurrentViewEntity();
    if ((tmp_view_entity && local_player) && (tmp_view_entity->ID() == local_player->GameID))
    {
        m_reward_manager->add_task(award_id);
    }
}

void game_cl_mp::ReInitRewardGenerator(game_PlayerState* local_ps)
{
    if (!m_reward_generator)
    {
        m_reward_generator = new award_system::reward_event_generator(u32(-1));
        m_reward_manager = new award_system::reward_manager(this);
    }
    m_reward_generator->init_player(local_ps);
}

bool game_cl_mp::IsLocalPlayerInitialized() const
{
    game_cl_GameState::PLAYERS_MAP const& playersMap = Game().players;
    game_cl_GameState::PLAYERS_MAP::const_iterator pi = playersMap.find(local_svdpnid);
    return pi != playersMap.end();
}

bool game_cl_mp::RequestPlayersInfo(player_info_reply_cb_t const pinfo_repl_cb)
{
    if (m_players_info_reply)
        return false;

    m_players_info_reply = pinfo_repl_cb;
    NET_Packet tmp_packet;
    u_EventGen(tmp_packet, GE_REQUEST_PLAYERS_INFO, 0);
    Level().Send(tmp_packet);
    return true;
}

void game_cl_mp::ProcessPlayersInfoReply(NET_Packet& P)
{
    shared_str tmp_fake_str;
    u32 info_count = 0;
    while (!P.r_eof())
    {
        ClientID tmp_client;
        P.r_clientID(tmp_client);
        ++info_count;
        PLAYERS_MAP_IT tmp_iter = players.find(tmp_client);
        if (tmp_iter == players.end())
        {
            VERIFY("client not found");
            P.r_stringZ(tmp_fake_str);
            P.r_stringZ(tmp_fake_str);
            continue;
        }
        VERIFY(tmp_iter->second);
        P.r_stringZ(tmp_iter->second->m_player_ip);
        P.r_stringZ(tmp_iter->second->m_player_digest);
    }
    VERIFY2(m_players_info_reply, "info reply callback not binded");
    if (m_players_info_reply)
    {
        player_info_reply_cb_t tmp_cb = m_players_info_reply;
        m_players_info_reply.clear();
        tmp_cb(info_count);
    }
}
