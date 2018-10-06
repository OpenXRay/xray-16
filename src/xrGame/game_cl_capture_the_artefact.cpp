#include "StdAfx.h"
#include "xr_level_controller.h"
#include "map_manager.h"
#include "map_location.h"
#include "Actor.h"
#include "ActorCondition.h"
#include "Artefact.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UISkinSelector.h"
#include "ui/UIPdaWnd.h"
#include "ui/UIMapDesc.h"
#include "ui/UIVote.h"
#include "ui/TeamInfo.h"
#include "game_base_menu_events.h"
#include "string_table.h"
#include "game_cl_capture_the_artefact.h"
#include "clsid_game.h"
#include "Actor.h"
#include "Weapon.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "xrNetServer/NET_Messages.h"

#include "xrEngine/IGame_Persistent.h"
#include "ui/UIActorMenu.h"
#include "ui/UIDemoPlayControl.h"

#include "game_cl_capturetheartefact_snd_msg.h"
#include "game_cl_teamdeathmatch_snd_messages.h"
#include "game_cl_artefacthunt_snd_msg.h"
#include "game_cl_deathmatch_snd_messages.h"

#include "reward_event_generator.h"

//#define TEAM0_MENU		"artefacthunt_team0"
#define GREENTEAM_MENU "capturetheartefact_team1"
#define BLUETEAM_MENU "capturetheartefact_team2"
#define BASECOST_SECTION "capturetheartefact_base_cost"
#define MESSAGE_MENUS "capturetheartefact_messages_menu"

#define ARTEFACT_NEUTRAL "mp_af_neutral_location"
#define FREE_ARTEFACT_FRIEND "mp_free_af_friend_location"
#define FRIEND_LOCATION "mp_friend_location"
#define ARTEFACT_FRIEND "mp_af_friend_location"
#define ARTEFACT_ENEMY "mp_af_enemy_location"

#define CLIENT_CTA_LOG

game_cl_CaptureTheArtefact::game_cl_CaptureTheArtefact()
{
    m_game_ui = NULL;
    spawn_cost = -10000;

    m_bTeamSelected = FALSE;
    m_bSkinSelected = FALSE;
    m_bReadMapDesc = FALSE;
    m_winnerTeamShowed = FALSE;

    m_curReinforcementTime = 0;
    m_maxReinforcementTime = 0;

    m_currentWarmupTime = 0;
    m_inWarmup = false;
    m_s32TimeLimit = 0;

    maxScore = 0;
    greenTeamScore = 0;
    blueTeamScore = 0;

    greenArtefactOwner = 0;
    blueArtefactOwner = 0;

    m_player_on_base = false;
    m_allow_buy = false;
    m_bFriendlyIndicators = false;
    m_bFriendlyNames = false;
    m_bBearerCantSprint = false;
    m_bCanActivateArtefact = false;
    m_bShowPlayersNames = false;
    m_dwVoteEndTime = 0;

    // sendedSpawnMe = false;
    haveGotUpdate = false;

    buy_amount = 0;
    total_money = 0;
    last_money = -1;

    LoadSndMessages();
}
game_cl_CaptureTheArtefact::~game_cl_CaptureTheArtefact() {}
void game_cl_CaptureTheArtefact::SetGameUI(CUIGameCustom* already_created_ui)
{
    VERIFY(already_created_ui);
    // !!! dangerous code...
    m_game_ui = static_cast<CUIGameCTA*>(already_created_ui);
    m_captions_manager.Init(this, m_game_ui);
    m_game_ui->UpdateTeamPanels();
}
void game_cl_CaptureTheArtefact::Init()
{
    inherited::Init();
    spawn_cost = READ_IF_EXISTS(pSettings, r_s32, "capturetheartefact_gamedata", "spawn_cost", -10000);
    LoadTeamData(GREENTEAM_MENU);
    LoadTeamData(BLUETEAM_MENU);
}

void game_cl_CaptureTheArtefact::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);

    if (GEnv.isDedicatedServer)
        return;

    if ((Level().IsDemoPlayStarted() || Level().IsDemoPlayFinished()) && m_game_ui)
    {
        game_PlayerState* lookat_player = Game().lookat_player();
        if (lookat_player)
        {
            m_game_ui->SetRank(static_cast<ETeam>(lookat_player->team), lookat_player->rank);
            UpdateMoneyIndicator();
        }
    }

    switch (Phase())
    {
    case GAME_PHASE_INPROGRESS:
    {
        if (m_game_ui)
        {
            if (local_player && !local_player->IsSkip())
            {
                if (!m_bReadMapDesc && Level().CurrentEntity())
                {
                    m_bReadMapDesc = m_game_ui->ShowServerInfo() ? TRUE : FALSE;
                    GetActiveVoting();
                }

                UpdateMoneyIndicator();

                if ((local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)) &&
                    (static_cast<ETeam>(local_player->team) != etSpectatorsTeam))
                {
                    /*if (!sendedSpawnMe)
                        SpawnMe();*/

                    if ((local_player->money_for_round + spawn_cost + buy_amount) >= 0)
                    {
                        m_captions_manager.CanCallBuySpawn(true);
                    }
                    else
                    {
                        m_captions_manager.CanCallBuySpawn(false);
                    }
                    if (local_player->testFlag(GAME_PLAYER_FLAG_READY))
                    {
                        // m_captions_manager.CanSpawn(true);
                        m_captions_manager.CanCallBuySpawn(false);
                    }
                    else
                    {
                        // m_captions_manager.CanSpawn(false);
                    }
                }
            }
            if (InWarmUp())
            {
                m_game_ui->SetReinforcementTimes(0, 0);
            }
            else
            {
                m_game_ui->SetReinforcementTimes(m_curReinforcementTime, m_maxReinforcementTime);
            }
            u32 current_time = Level().timeServer();
            UpdateVotingTime(current_time);
            UpdateWarmupTime(current_time);
            UpdateTimeLimit(current_time);
        }
        /*if (Level().CurrentControlEntity()){
                CGameObject* GO = smart_cast<CGameObject*>(Level().CurrentControlEntity());
                Msg("---I'm ready (ID = %d) sending player ready packet !!!", GO->ID());
                NET_Packet			P;
                GO->u_EventGen		(P,GE_GAME_EVENT,GO->ID()	);
                P.w_u16(GAME_EVENT_PLAYER_READY);
                GO->u_EventSend			(P);
        }*/
    }
    break;
    case GAME_PHASE_PENDING:
    {
        if ((m_game_ui) && (!m_game_ui->IsTeamPanelsShown()))
        {
            m_game_ui->ShowTeamPanels(true);
        }
        m_winnerTeamShowed = FALSE;
    }
    break;
    case GAME_PHASE_PLAYER_SCORES:
    {
        VERIFY(m_game_ui);
        if (!m_winnerTeamShowed)
        {
            if (greenTeamScore > blueTeamScore)
            {
                PlaySndMessage(ID_TEAM1_WIN);
                m_captions_manager.SetWinnerTeam(etGreenTeam);
            }
            else
            {
                PlaySndMessage(ID_TEAM2_WIN);
                m_captions_manager.SetWinnerTeam(etBlueTeam);
            }
            m_winnerTeamShowed = TRUE;
            if (m_reward_generator)
            {
                m_reward_generator->OnRoundEnd();
                m_reward_generator->CommitBestResults();
            }
        }
    }
    break;
    default: {
    }
    break;
    };
    m_captions_manager.ShowCaptions();
}

void game_cl_CaptureTheArtefact::UpdateMoneyIndicator()
{
    string256 MoneyStr;
    game_PlayerState* lookat_player = Game().lookat_player();
    if (!lookat_player)
        return;
    // R_ASSERT(lookat_player);
    if (lookat_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        total_money = (lookat_player == local_player) ? local_player->money_for_round + buy_amount :
                                                        lookat_player->money_for_round;
    }
    else
    {
        total_money = lookat_player->money_for_round;
    }
    if (total_money != last_money)
    {
        xr_itoa(total_money, MoneyStr, 10);
        m_game_ui->ChangeTotalMoneyIndicator(MoneyStr);
        last_money = total_money;
    }
}

void game_cl_CaptureTheArtefact::TranslateGameMessage(u32 msg, NET_Packet& P)
{
    CStringTable& st = StringTable();
    string1024 Text;
    // string512 tmp;
    //	LPSTR	Color_Teams[3]		= {"%c[255,255,255,255]", "%c[255,64,255,64]", "%c[255,64,64,255]"};
    char Color_Main[] = "%c[255,192,192,192]";
    char Color_Artefact[] = "%c[255,255,255,0]";
    //	LPSTR	TeamsNames[3]		= {"Zero Team", "Team Green", "Team Blue"};

    switch (msg)
    {
    //-------------------UI MESSAGES
    case GAME_EVENT_ARTEFACT_TAKEN:
    {
        ClientID clientId; // who took the artefact
        u8 artefactOwnerTeam;
        P.r_u8(artefactOwnerTeam);
        P.r_clientID(clientId);

        PLAYERS_MAP_CIT playerIt = players.find(clientId);
        VERIFY2(playerIt != players.end(),
            make_string("player (ClientID = 0x%08x) that took the artefact not found on client site", clientId.value())
                .c_str());

        game_PlayerState const* ps = playerIt->second;
        VERIFY2(ps, make_string("player state (ClientID = 0x%08x) not initialized", clientId.value()).c_str());

        if (ps->team == artefactOwnerTeam)
        {
            // player has returned team artefact
            xr_sprintf(Text, "%s%s %s%s", CTeamInfo::GetTeam_color_tag(ModifyTeam(artefactOwnerTeam) + 1),
                ps->getName(), Color_Main, st.translate("mp_returned_artefact").c_str());
            PlayReturnedTheArtefact(ps);
        }
        else if (ps != local_player)
        {
            // player has captured the artefact
            xr_sprintf(Text, "%s%s %s%s", CTeamInfo::GetTeam_color_tag(ModifyTeam(ps->team) + 1), ps->getName(),
                Color_Main, st.translate("mp_captured_artefact").c_str());

            PlayCapturedTheArtefact(ps);
            if (m_reward_generator)
                m_reward_generator->OnPlayerTakeArtefact(ps);

            if (artefactOwnerTeam == static_cast<u8>(etGreenTeam))
            {
                greenArtefactOwner = ps->GameID;
            }
            else
            {
                blueArtefactOwner = ps->GameID;
            }
        }
        else
        {
            xr_sprintf(Text, "%s%s", Color_Main, *st.translate("mp_you_captured_artefact"));

            PlayCapturedTheArtefact(ps);
            if (m_reward_generator)
                m_reward_generator->OnPlayerTakeArtefact(ps);

            if (artefactOwnerTeam == static_cast<u8>(etGreenTeam))
            {
                greenArtefactOwner = ps->GameID;
            }
            else
            {
                blueArtefactOwner = ps->GameID;
            }
        }
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);
        // Update UI statistics
    }
    break;
    case GAME_EVENT_ARTEFACT_DROPPED: // ahunt
    {
        ClientID clientId; // who dropped the artefact
        u8 artefactOwnerTeam;
        P.r_u8(artefactOwnerTeam);
        P.r_clientID(clientId);

        PLAYERS_MAP_CIT playerIt = players.find(clientId);
        game_PlayerState const* ps = NULL;

        // if client present (not disconnected)
        if (playerIt != players.end())
        {
            ps = playerIt->second;
        }

        if (artefactOwnerTeam == static_cast<u8>(etGreenTeam))
        {
            if (local_player && !local_player->IsSkip())
                Level().MapManager().RemoveMapLocationByObjectID(greenArtefactOwner);
            greenArtefactOwner = 0;
        }
        else
        {
            if (local_player && !local_player->IsSkip())
                Level().MapManager().RemoveMapLocationByObjectID(blueArtefactOwner);
            blueArtefactOwner = 0;
        }
        if (ps)
        {
            xr_sprintf(Text, "%s%s %s%s", CTeamInfo::GetTeam_color_tag(ModifyTeam(ps->team) + 1), ps->getName(),
                Color_Main,
                st.translate("mp_has_dropped_artefact").c_str()); // need translate

            if (m_reward_generator)
                m_reward_generator->OnPlayerDropArtefact(ps);
        }
        else
        {
            xr_sprintf(Text, "%s%s", Color_Main, st.translate("mp_artefact_dropped").c_str());
        }
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);
        // PlaySndMessage(ID_AF_LOST);
    }
    break;
    case GAME_EVENT_ARTEFACT_ONBASE:
    {
        u8 delivererTeam;
        P.r_u8(delivererTeam);
        u16 deliverer_id; // who deliver the artefact
        P.r_u16(deliverer_id);
        game_PlayerState const* ps = GetPlayerByGameID(deliverer_id);

        if (!local_player) // can be NULL, because not actor or spectator spawned yet...
            return;

        if (m_reward_generator)
            m_reward_generator->OnPlayerBringArtefact(ps);

        if (delivererTeam == local_player->team)
        {
            // artefact on base !
            xr_sprintf(Text, "%s%s", Color_Artefact, st.translate("mp_artefact_on_base").c_str());
        }
        else
        {
            // artefact on enemy base !
            xr_sprintf(Text, "%s%s", Color_Artefact, st.translate("mp_artefact_on_enemy_base").c_str());
        }
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);
        PlayDeliveredTheArtefact(ps);
    }
    break;
    default: inherited::TranslateGameMessage(msg, P);
    }
    UpdateMapLocations();
}

void game_cl_CaptureTheArtefact::PlayCapturedTheArtefact(game_PlayerState const* capturer)
{
    if (!local_player || !capturer)
        return;

    ETeam my_team = static_cast<ETeam>(local_player->team);
    if (my_team == etGreenTeam)
    {
        if (capturer == local_player)
        {
            PlaySndMessage(ID_AF_TEAM1_TAKE);
        }
        else if (capturer->team == local_player->team)
        {
            PlaySndMessage(ID_AF_TEAM1_TAKE_R);
        }
        else
        {
            PlaySndMessage(ID_AF_TEAM1_TAKE_ENEMY);
        }
    }
    else if (my_team == etBlueTeam)
    {
        if (capturer == local_player)
        {
            PlaySndMessage(ID_AF_TEAM2_TAKE);
        }
        else if (capturer->team == local_player->team)
        {
            PlaySndMessage(ID_AF_TEAM2_TAKE_R);
        }
        else
        {
            PlaySndMessage(ID_AF_TEAM2_TAKE_ENEMY);
        }
    }
}
void game_cl_CaptureTheArtefact::PlayReturnedTheArtefact(game_PlayerState const* returnerer)
{
    if (!local_player || !returnerer)
        return;

    ETeam my_team = static_cast<ETeam>(local_player->team);
    if (my_team == etGreenTeam)
    {
        if (returnerer == local_player)
        {
            PlaySndMessage(ID_AF_TEAM1_RETURNED);
        }
        else if (returnerer->team == local_player->team)
        {
            PlaySndMessage(ID_AF_TEAM1_RETURNED_R);
        }
        else
        {
            PlaySndMessage(ID_AF_TEAM1_RETURNED_ENEMY);
        }
    }
    else if (my_team == etBlueTeam)
    {
        if (returnerer == local_player)
        {
            PlaySndMessage(ID_AF_TEAM2_RETURNED);
        }
        else if (returnerer->team == local_player->team)
        {
            PlaySndMessage(ID_AF_TEAM2_RETURNED_R);
        }
        else
        {
            PlaySndMessage(ID_AF_TEAM2_RETURNED_ENEMY);
        }
    }
}

void game_cl_CaptureTheArtefact::PlayDeliveredTheArtefact(game_PlayerState const* deliverer)
{
    if (!local_player || !deliverer)
        return;
    ETeam my_team = static_cast<ETeam>(local_player->team);
    if (my_team == etGreenTeam)
    {
        if (deliverer == local_player)
        {
            PlaySndMessage(ID_AF_TEAM1_ONBASE);
        }
        else if (deliverer->team == local_player->team)
        {
            PlaySndMessage(ID_AF_TEAM1_ONBASE_R);
        }
        else
        {
            PlaySndMessage(ID_AF_TEAM1_ONBASE_ENEMY);
        }
    }
    else if (my_team == etBlueTeam)
    {
        if (deliverer == local_player)
        {
            PlaySndMessage(ID_AF_TEAM2_ONBASE);
        }
        else if (deliverer->team == local_player->team)
        {
            PlaySndMessage(ID_AF_TEAM2_ONBASE_R);
        }
        else
        {
            PlaySndMessage(ID_AF_TEAM2_ONBASE_ENEMY);
        }
    }
}

void game_cl_CaptureTheArtefact::LoadSndMessages()
{
    LoadSndMessage("cta_snd_messages", "team1_artefact_on_base", ID_AF_TEAM1_ONBASE);
    LoadSndMessage("cta_snd_messages", "team2_artefact_on_base", ID_AF_TEAM2_ONBASE);
    LoadSndMessage("cta_snd_messages", "team1_artefact_on_base_r", ID_AF_TEAM1_ONBASE_R);
    LoadSndMessage("cta_snd_messages", "team2_artefact_on_base_r", ID_AF_TEAM2_ONBASE_R);
    LoadSndMessage("cta_snd_messages", "team1_artefact_on_base_enemy", ID_AF_TEAM1_ONBASE_ENEMY);
    LoadSndMessage("cta_snd_messages", "team2_artefact_on_base_enemy", ID_AF_TEAM2_ONBASE_ENEMY);

    LoadSndMessage("cta_snd_messages", "team1_artefact_returned", ID_AF_TEAM1_RETURNED);
    LoadSndMessage("cta_snd_messages", "team1_artefact_returned_r", ID_AF_TEAM1_RETURNED_R);
    LoadSndMessage("cta_snd_messages", "team1_artefact_returned_enemy", ID_AF_TEAM1_RETURNED_ENEMY);

    LoadSndMessage("cta_snd_messages", "team2_artefact_returned", ID_AF_TEAM2_RETURNED);
    LoadSndMessage("cta_snd_messages", "team2_artefact_returned_r", ID_AF_TEAM2_RETURNED_R);
    LoadSndMessage("cta_snd_messages", "team2_artefact_returned_enemy", ID_AF_TEAM2_RETURNED_ENEMY);

    LoadSndMessage("cta_snd_messages", "team1_artefact_take", ID_AF_TEAM1_TAKE);
    LoadSndMessage("cta_snd_messages", "team2_artefact_take", ID_AF_TEAM2_TAKE);
    LoadSndMessage("cta_snd_messages", "team1_artefact_take_r", ID_AF_TEAM1_TAKE_R);
    LoadSndMessage("cta_snd_messages", "team2_artefact_take_r", ID_AF_TEAM2_TAKE_R);
    LoadSndMessage("cta_snd_messages", "team1_artefact_take_enemy", ID_AF_TEAM1_TAKE_ENEMY);
    LoadSndMessage("cta_snd_messages", "team2_artefact_take_enemy", ID_AF_TEAM2_TAKE_ENEMY);

    LoadSndMessage("cta_snd_messages", "team1_win", ID_TEAM1_WIN);
    LoadSndMessage("cta_snd_messages", "team2_win", ID_TEAM2_WIN);
    LoadSndMessage("cta_snd_messages", "teams_equal", ID_TEAMS_EQUAL);
    LoadSndMessage("cta_snd_messages", "team1_lead", ID_TEAM1_LEAD);
    LoadSndMessage("cta_snd_messages", "team2_lead", ID_TEAM2_LEAD);

    LoadSndMessage("cta_snd_messages", "team1_rank1", ID_TEAM1_RANK_1);
    LoadSndMessage("cta_snd_messages", "team1_rank2", ID_TEAM1_RANK_2);
    LoadSndMessage("cta_snd_messages", "team1_rank3", ID_TEAM1_RANK_3);
    LoadSndMessage("cta_snd_messages", "team1_rank4", ID_TEAM1_RANK_4);

    LoadSndMessage("cta_snd_messages", "team2_rank1", ID_TEAM2_RANK_1);
    LoadSndMessage("cta_snd_messages", "team2_rank2", ID_TEAM2_RANK_2);
    LoadSndMessage("cta_snd_messages", "team2_rank3", ID_TEAM2_RANK_3);
    LoadSndMessage("cta_snd_messages", "team2_rank4", ID_TEAM2_RANK_4);

    LoadSndMessage("dm_snd_messages", "countdown_5", ID_COUNTDOWN_5);
    LoadSndMessage("dm_snd_messages", "countdown_4", ID_COUNTDOWN_4);
    LoadSndMessage("dm_snd_messages", "countdown_3", ID_COUNTDOWN_3);
    LoadSndMessage("dm_snd_messages", "countdown_2", ID_COUNTDOWN_2);
    LoadSndMessage("dm_snd_messages", "countdown_1", ID_COUNTDOWN_1);
}

BOOL game_cl_CaptureTheArtefact::CanCallBuyMenu()
{
    if (!is_buy_menu_ready())
        return FALSE;

    if (!local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        return m_allow_buy;
    }
    if (local_player->team == etSpectatorsTeam)
    {
        return FALSE;
    }
    return TRUE;
}

BOOL game_cl_CaptureTheArtefact::CanCallInventoryMenu()
{
    if (local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        return FALSE;
    }
    return TRUE;
}

void game_cl_CaptureTheArtefact::OnPlayerEnterBase()
{
    m_captions_manager.CanCallBuy(true);
    m_allow_buy = true;
}

void game_cl_CaptureTheArtefact::OnPlayerLeaveBase()
{
    m_captions_manager.CanCallBuy(false);
    m_allow_buy = false;
}

void game_cl_CaptureTheArtefact::net_import_state(NET_Packet& P)
{
    inherited::net_import_state(P);

    P.r_u16(greenArtefact);
    P.r_u16(blueArtefact);

    P.r_vec3(greenTeamRPos);
    P.r_vec3(blueTeamRPos);

    s32 oldScores = greenTeamScore + blueTeamScore;

    P.r_s32(maxScore);
    P.r_s32(greenTeamScore);
    P.r_s32(blueTeamScore);

    s32 newScores = greenTeamScore + blueTeamScore;

    if ((oldScores != newScores) && (newScores != 0))
    {
        OnTeamScoresChanged();
    }

    // m_game_ui may be zero
    if (m_game_ui)
    {
        m_game_ui->SetScore(maxScore, greenTeamScore, blueTeamScore);
    }

    m_bFriendlyIndicators = !!P.r_u8();
    m_bFriendlyNames = !!P.r_u8();
    m_bBearerCantSprint = !!P.r_u8();
    m_bCanActivateArtefact = !!P.r_u8();
    m_baseRadius = P.r_float();

    m_inWarmup = !!P.r_u8();
    // warning: r_s16 !
    m_s32TimeLimit = static_cast<s32>(P.r_s16() * 60000);

    haveGotUpdate = true;

    UpdateMapLocations();
}

void game_cl_CaptureTheArtefact::net_import_update(NET_Packet& P)
{
    inherited::net_import_update(P);
    P.r_u32(m_curReinforcementTime);
    P.r_u32(m_maxReinforcementTime);
    P.r_u32(m_currentWarmupTime);
}

bool game_cl_CaptureTheArtefact::InWarmUp() const { return m_inWarmup; }
CUIGameCustom* game_cl_CaptureTheArtefact::createGameUI()
{
    if (GEnv.isDedicatedServer)
        return NULL;

    m_game_ui = smart_cast<CUIGameCTA*>(NEW_INSTANCE(CLSID_GAME_UI_CAPTURETHEARTEFACT));
    VERIFY2(m_game_ui, "failed to create Capture The Artefact game UI");
    m_game_ui->Load();
    // m_game_ui->Init		(0);
    // m_game_ui->Init		(1);
    // m_game_ui->Init		(2);
    LoadMessagesMenu(MESSAGE_MENUS);
    return m_game_ui;
}

const shared_str& game_cl_CaptureTheArtefact::GetLocalPlayerTeamSection() const
{
    VERIFY2(TeamList.size() > local_player->team,
        make_string("local_player has not valid team number: %d", local_player->team).c_str());
    cl_TeamStruct const* pTeamSect = &(TeamList[local_player->team]);
    return pTeamSect->caSection;
}
ETeam game_cl_CaptureTheArtefact::GetLocalPlayerTeam() const
{
    VERIFY(local_player);
    // their numbers are equal...
    return static_cast<ETeam>(local_player->team);
}

// receive change scin <----
void game_cl_CaptureTheArtefact::OnGameMenuRespond_ChangeSkin(NET_Packet& P)
{
    s8 NewSkin = P.r_s8();
    local_player->skin = NewSkin;
    m_bSkinSelected = TRUE;
    m_bSpectatorSelected = FALSE;
    Msg("* player [%s][%d] changed skin to %d", local_player->getName(), local_player->GameID, local_player->skin);
    ReInitRewardGenerator(local_player);
    // SpawnMe();
}

void game_cl_CaptureTheArtefact::SpawnMe()
{
    CActor* currActor = smart_cast<CActor*>(Level().CurrentControlEntity());
    if (!currActor)
        return;

    /*CGameObject*	go = smart_cast<CGameObject*>(curr);
    VERIFY			(go);*/

    NET_Packet packet;
    currActor->u_EventGen(packet, GE_GAME_EVENT, currActor->ID());
    packet.w_u16(GAME_EVENT_PLAYER_READY);
    currActor->u_EventSend(packet, net_flags(TRUE, TRUE));
    // sendedSpawnMe = true;
}

// receive change team <----
void game_cl_CaptureTheArtefact::OnGameMenuRespond_ChangeTeam(NET_Packet& P)
{
    u8 newTeam = P.r_u8();
    local_player->team = newTeam;
    m_bTeamSelected = TRUE;
    VERIFY(local_player);
    Msg("* player [%s][%d] changed team to %d", local_player->getName(), local_player->GameID, local_player->team);
    /*shared_str const & teamSection = GetLocalPlayerTeamSection();
    m_game_ui->UpdateBuyMenu(teamSection, BASECOST_SECTION);
    m_game_ui->UpdateSkinMenu(teamSection);*/
    OnTeamChanged();
    if (m_reward_generator)
        m_reward_generator->OnPlayerChangeTeam(local_player->team);
    if (CanCallSkinMenu())
    {
        m_game_ui->ShowSkinMenu(local_player->skin);
    }
}

void game_cl_CaptureTheArtefact::UpdateMapLocations()
{
    if (GEnv.isDedicatedServer)
        return;
    // updating firends indicator
    if (!local_player)
        return;
    if (local_player->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
        return;

    PLAYERS_MAP_IT ie = players.end();
    for (PLAYERS_MAP_IT tempPlayerIt = players.begin(); tempPlayerIt != ie; ++tempPlayerIt)
    {
        game_PlayerState* ps = tempPlayerIt->second;
        if ((ps->team == local_player->team) && (ps != local_player) &&
            (!ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)))
        {
            Level().MapManager().AddMapLocation(FRIEND_LOCATION, ps->GameID);
        }
        else
        {
            Level().MapManager().RemoveMapLocationByObjectID(ps->GameID);
        }
    };
    // updating the artefacts
    CMapLocation* tempLocation;
    if (greenArtefact && blueArtefact)
    {
        Level().MapManager().RemoveMapLocationByObjectID(greenArtefact);
        Level().MapManager().RemoveMapLocationByObjectID(blueArtefact);
        if (local_player->team == etGreenTeam)
        {
            tempLocation = Level().MapManager().AddMapLocation(FREE_ARTEFACT_FRIEND, greenArtefact);
            VERIFY(tempLocation);
            tempLocation->EnablePointer();
            tempLocation = Level().MapManager().AddMapLocation(ARTEFACT_NEUTRAL, blueArtefact);
            VERIFY(tempLocation);
            tempLocation->EnablePointer();
        }
        else
        {
            tempLocation = Level().MapManager().AddMapLocation(ARTEFACT_NEUTRAL, greenArtefact);
            VERIFY(tempLocation);
            tempLocation->EnablePointer();
            tempLocation = Level().MapManager().AddMapLocation(FREE_ARTEFACT_FRIEND, blueArtefact);
            VERIFY(tempLocation);
            tempLocation->EnablePointer();
        }
    }

    if (local_player->team == static_cast<u8>(etGreenTeam))
    {
        if (greenArtefactOwner)
        {
            Level().MapManager().RemoveMapLocationByObjectID(greenArtefact);
            tempLocation = Level().MapManager().AddMapLocation(ARTEFACT_ENEMY, greenArtefactOwner);
            VERIFY(tempLocation);
            tempLocation->EnablePointer();
        }
        if (blueArtefactOwner)
        {
            Level().MapManager().RemoveMapLocationByObjectID(blueArtefact);
            tempLocation = Level().MapManager().AddMapLocation(ARTEFACT_FRIEND, blueArtefactOwner);
            VERIFY(tempLocation);
            tempLocation->EnablePointer();
        }
    }
    else if (local_player->team == static_cast<u8>(etBlueTeam))
    {
        if (blueArtefactOwner)
        {
            Level().MapManager().RemoveMapLocationByObjectID(blueArtefact);
            tempLocation = Level().MapManager().AddMapLocation(ARTEFACT_ENEMY, blueArtefactOwner);
            VERIFY(tempLocation);
            tempLocation->EnablePointer();
        }
        if (greenArtefactOwner)
        {
            Level().MapManager().RemoveMapLocationByObjectID(greenArtefact);
            tempLocation = Level().MapManager().AddMapLocation(ARTEFACT_FRIEND, greenArtefactOwner);
            VERIFY(tempLocation);
            tempLocation->EnablePointer();
        }
    }
}

void game_cl_CaptureTheArtefact::OnSpawn(IGameObject* pObj)
{
    inherited::OnSpawn(pObj);

    if (GEnv.isDedicatedServer)
        return;

    CArtefact* pArtefact = smart_cast<CArtefact*>(pObj);
    if (pArtefact)
    {
        Level().MapManager().AddMapLocation(ARTEFACT_NEUTRAL, pObj->ID())->EnablePointer();
        /*if (OnServer()) // huck :( - server logic must be ONLY ON SERVER !!!
        {
            if (GetGreenArtefactID() == pArtefact->ID())
            {
                pArtefact->MoveTo(GetGreenArtefactRPoint());
            } else if (GetBlueArtefactID() == pArtefact->ID())
            {
                pArtefact->MoveTo(GetBlueArtefactRPoint());
            } else
            {
                VERIFY2(false, "unknown artefact in game");
            }
        }*/
        return;
    }
    CActor* pActor = smart_cast<CActor*>(pObj);
    if (pActor && local_player)
    {
        game_PlayerState* ps = GetPlayerByGameID(pActor->ID());
        if (!ps)
            return;

        if (m_reward_generator)
        {
            m_reward_generator->init_bone_groups(pActor);
            m_reward_generator->OnPlayerSpawned(ps);
        }

        // VERIFY(ps);
        if ((ps->team == local_player->team) && (ps != local_player))
        {
            Level().MapManager().AddMapLocation(FRIEND_LOCATION, pObj->ID());
        }
        if (ps == local_player)
        {
            buy_amount = 0;
            if (m_game_ui)
            {
                m_game_ui->HideBuyMenu();
            }
        }
    }
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

void game_cl_CaptureTheArtefact::SetInvinciblePlayer(u16 const gameId, bool const invincible)
{
    IGameObject* pObject = Level().Objects.net_Find(gameId);

    if (!pObject)
        return;

    if (!smart_cast<CActor*>(pObject))
        return;

    CActor* pActor = static_cast<CActor*>(pObject);
    VERIFY(pActor);
    /*if (invincible)
        Msg("---Player %d is invincible now...", gameId);
    else
        Msg("---Player %d is not invincible now...", gameId);*/

    pActor->conditions().SetCanBeHarmedState(!invincible);
}

void game_cl_CaptureTheArtefact::OnPlayerFlagsChanged(game_PlayerState* ps)
{
    inherited::OnPlayerFlagsChanged(ps);
    if (!ps)
        return;

    if (local_player == ps)
    {
        if (!m_player_on_base && ps->testFlag(GAME_PLAYER_FLAG_ONBASE))
        {
            OnPlayerEnterBase();
            m_player_on_base = true;
        }
        if (m_player_on_base && !ps->testFlag(GAME_PLAYER_FLAG_ONBASE))
        {
            OnPlayerLeaveBase();
            m_player_on_base = false;
        }
        if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
        {
            m_captions_manager.CanCallBuy(true);
            if (m_game_ui)
            {
                m_game_ui->HideActorMenu();
                if (m_game_ui->GetBuyWnd())
                    m_game_ui->HideBuyMenu();
            }
        }
        else
        {
            if (m_game_ui && m_game_ui->IsBuySpawnShown())
            {
                m_game_ui->HideBuySpawn();
            }
        }
    }
    SetInvinciblePlayer(ps->GameID, ps->testFlag(GAME_PLAYER_FLAG_INVINCIBLE));
}

void game_cl_CaptureTheArtefact::OnNewPlayerConnected(ClientID const& newClient)
{
    if (m_game_ui)
    {
        m_game_ui->UpdatePlayer(newClient);
    }
}

bool game_cl_CaptureTheArtefact::OnKeyboardPress(int key)
{
    if (inherited::OnKeyboardPress(key))
        return true;

    if (Level().IsDemoPlay() && (key != kSCORES) && (key != kCROUCH))
        return false;

    if ((Phase() == GAME_PHASE_INPROGRESS) && (m_game_ui) && (local_player && !local_player->IsSkip()))
    {
        switch (key)
        {
        case kSCORES:
        {
            m_game_ui->ShowTeamPanels(true);
            return true;
        }
        break;
        case kSKIN:
        {
            if (CanCallSkinMenu())
            {
                VERIFY(local_player);
                m_game_ui->ShowSkinMenu(local_player->skin);
            }
        }
        break;
        case kTEAM:
        {
            if (CanCallTeamSelectMenu())
            {
                m_game_ui->ShowTeamSelectMenu();
            }
        }
        break;
        case kINVENTORY:
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
        break;
        case kBUY:
        {
            if (CanCallBuyMenu())
            {
                m_game_ui->ShowBuyMenu();
            }
        }
        break;
        };
    }
    return false;
}

bool game_cl_CaptureTheArtefact::OnKeyboardRelease(int key)
{
    if (inherited::OnKeyboardRelease(key))
        return true;

    if (kSCORES == key)
    {
        if (m_game_ui && (Phase() == GAME_PHASE_INPROGRESS))
        {
            m_game_ui->ShowTeamPanels(false);
        };
        return true;
    };
    return false;
}

BOOL game_cl_CaptureTheArtefact::CanCallTeamSelectMenu()
{
    VERIFY2(local_player, "local player not initialized");
    if (Phase() != GAME_PHASE_INPROGRESS)
    {
        return FALSE;
    }
    if (m_game_ui->IsTeamSelectShown())
    {
        return FALSE;
    };

    /*if (pCurBuyMenu && pCurBuyMenu->IsShown())
    {
        return FALSE;
    };
    if (pCurSkinMenu && pCurSkinMenu->IsShown())
    {
        return FALSE;
    };

    m_game_ui->m_pUITeamSelectWnd->SetCurTeam(ModifyTeam(local_player->team));*/
    return TRUE;
};

bool game_cl_CaptureTheArtefact::CanBeReady()
{
    VERIFY2(local_player, "local player not initialized");
    if (!m_bTeamSelected)
    {
        if (CanCallTeamSelectMenu())
        {
            m_game_ui->ShowTeamSelectMenu();
        }
        return false;
    }
    if (!m_bSkinSelected)
    {
        if (CanCallSkinMenu())
        {
            m_game_ui->ShowSkinMenu(local_player->skin);
        }
        return false;
    }
#ifndef MASTER_GOLD
    Msg("---CanBeReady = true: [%s][%d]", local_player->getName(), local_player->GameID);
#endif // #ifndef MASTER_GOLD
    return true;
}

bool game_cl_CaptureTheArtefact::NeedToSendReady_Actor(int key, game_PlayerState* ps)
{
    return inherited::NeedToSendReady_Actor(key, ps);
}

bool game_cl_CaptureTheArtefact::NeedToSendReady_Spectator(int key, game_PlayerState* ps)
{
    bool res = inherited::NeedToSendReady_Spectator(key, ps);
    u32 gphase = Phase();
    if ((gphase == GAME_PHASE_INPROGRESS) && (key == kJUMP) && (!m_game_ui->IsBuySpawnShown()))
    {
        if (local_player->testFlag(GAME_PLAYER_FLAG_READY))
        {
            return res;
        }
        if (InWarmUp())
        {
            return res;
        }
        if (((local_player->money_for_round + spawn_cost + buy_amount) >= 0) &&
            (static_cast<ETeam>(local_player->team) != etSpectatorsTeam))
        {
            m_game_ui->ShowBuySpawn(spawn_cost);
        }
        return false;
    }
    return res;
}

void game_cl_CaptureTheArtefact::OnBuySpawnMenu_Ok()
{
    IGameObject* curr = Level().CurrentEntity();
    if (!curr)
        return;

    CGameObject* go = smart_cast<CGameObject*>(curr);
    NET_Packet packet;
    go->u_EventGen(packet, GE_GAME_EVENT, go->ID());
    packet.w_u16(GAME_EVENT_PLAYER_BUY_SPAWN);
    go->u_EventSend(packet);
};

void game_cl_CaptureTheArtefact::OnSpectatorSelect()
{
    m_bTeamSelected = FALSE;
    m_bSkinSelected = FALSE;
    inherited::OnSpectatorSelect();
}

void game_cl_CaptureTheArtefact::OnMapInfoAccept()
{
    if (CanCallTeamSelectMenu())
    {
        m_game_ui->ShowTeamSelectMenu();
    }
}

void game_cl_CaptureTheArtefact::OnTeamChanged()
{
    inherited::OnTeamChanged();
    if (!m_game_ui)
        return;

    m_game_ui->UpdatePlayer(local_svdpnid);

    if (local_player->team == etSpectatorsTeam)
        return;

    shared_str const& temp_section = GetLocalPlayerTeamSection();

    m_game_ui->UpdateBuyMenu(temp_section, BASECOST_SECTION);
    m_game_ui->UpdateSkinMenu(temp_section);
    m_game_ui->SetRank(static_cast<ETeam>(local_player->team), local_player->rank);
    m_game_ui->ReInitPlayerDefItems();
    ReInitRewardGenerator(local_player);
    UpdateMapLocations();
}

void game_cl_CaptureTheArtefact::OnGameRoundStarted()
{
    inherited::OnGameRoundStarted();
    if (local_player && IsLocalPlayerInitialized())
    {
        OnTeamChanged(); // updates buy menu...
#ifdef DEBUG
        Msg("--- CTA: Round started !!!");
#endif // #ifdef DEBUG
    }
    if (m_reward_generator)
        m_reward_generator->OnRoundStart();
}

void game_cl_CaptureTheArtefact::OnTeamScoresChanged()
{
    if (greenTeamScore > blueTeamScore)
    {
        PlaySndMessage(ID_TEAM1_LEAD);
        return;
    }
    else if (blueTeamScore > greenTeamScore)
    {
        PlaySndMessage(ID_TEAM2_LEAD);
        return;
    }
    PlaySndMessage(ID_TEAMS_EQUAL);
}

void game_cl_CaptureTheArtefact::OnRankChanged(u8 OldRank)
{
    inherited::OnRankChanged(OldRank);
    if (m_game_ui)
    {
        VERIFY(local_player);
        ETeam player_team = static_cast<ETeam>(local_player->team);
        if (player_team == etSpectatorsTeam)
            return;

        m_game_ui->SetRank(player_team, local_player->rank);
        m_game_ui->ReInitPlayerDefItems();
    }
    PlayRankChangedSnd();
}

void game_cl_CaptureTheArtefact::PlayRankChangedSnd()
{
    if (!local_player)
        return;

    if (local_player->rank >= 4) // need to add new sound
        return;

    ETeam my_team = static_cast<ETeam>(local_player->team);

    int rank_index = local_player->rank - 1;
    if (rank_index < 0)
        return;

    if (my_team == etGreenTeam)
    {
        PlaySndMessage(ID_TEAM1_RANK_1 + rank_index);
    }
    else if (my_team == etBlueTeam)
    {
        PlaySndMessage(ID_TEAM2_RANK_1 + rank_index);
    }
}

#define MAX_VOTE_PARAMS 5
void game_cl_CaptureTheArtefact::OnVoteStart(NET_Packet& P)
{
    inherited::OnVoteStart(P);
    static char const* ttable[6][2] = {{"restart", "mp_restart"}, {"restart_fast", "mp_restart_fast"},
        {"kick", "mp_kick"}, {"ban", "mp_ban"}, {"changemap", "mp_change_map"}, {"changeweather", "mp_change_weather"}};

    if (!m_game_ui)
        return;

    u32 psize = P.B.count + 1;
    char* command = static_cast<char*>(_alloca(psize));
    char* player = static_cast<char*>(_alloca(psize));
    char* cmd_name = static_cast<char*>(_alloca(psize));
    char* tcmd_name = cmd_name;
    static constexpr pcstr scans_format = "%s %s %s %s %s";

    char* args[MAX_VOTE_PARAMS];
    for (u32 i = 0; i < MAX_VOTE_PARAMS; ++i)
    {
        args[i] = static_cast<char*>(_alloca(psize + 1));
    }

    P.r_stringZ(command);
    P.r_stringZ(player);

    m_dwVoteEndTime = Level().timeServer() + P.r_u32();

    command[psize - 1] = 0;
    player[psize - 1] = 0;

    sscanf(command, "%s", cmd_name);
    u32 cmd_len = xr_strlen(cmd_name);
    u32 tcmd_len = cmd_len;

#ifdef CLIENT_CTA_LOG
    Msg("---Received vote begin message: (command: %s), (player: %s)", command, player);
#endif

    if (!cmd_len)
        return;

#ifdef CLIENT_CTA_LOG
    Msg("---Vote command: %s", cmd_name);
#endif

    int args_count = sscanf(command + cmd_len, scans_format, args[0], args[1], args[2], args[3], args[4]);
    if (args_count < 0)
        args_count = 0;

#ifdef CLIENT_CTA_LOG
    Msg("---Args count: %d", args_count);
#endif

    for (u32 i = 0; i < 6; ++i)
    {
        if (!xr_strcmp(cmd_name, ttable[i][0]))
        {
            pcstr ted_str = StringTable().translate(ttable[i][1]).c_str();
            VERIFY(ted_str);
            tcmd_len = xr_strlen(ted_str) + 1;
            tcmd_name = static_cast<char*>(_alloca(tcmd_len));
            xr_strcpy(tcmd_name, tcmd_len, ted_str);
#ifdef CLIENT_CTA_LOG
            Msg("---Translated command to: %s", tcmd_name);
#endif
            break;
        }
    }

    u32 vstr_size = (args_count * (psize + 1)) + tcmd_len + 1;
    char* vstr = static_cast<char*>(_alloca(vstr_size));
    xr_strcpy(vstr, vstr_size, tcmd_name);
    for (int i = 0; i < args_count; ++i)
    {
#ifdef CLIENT_CTA_LOG
        Msg("---Next cat iteration state: %s", vstr);
#endif
        xr_strcat(vstr, vstr_size, " ");
        xr_strcat(vstr, vstr_size, StringTable().translate(args[i]).c_str());
    }
    pcstr t_vote_str = StringTable().translate("mp_voting_started").c_str();
    VERIFY(t_vote_str);
    u32 fin_str_size = xr_strlen(t_vote_str) + vstr_size + xr_strlen(player) + 1;
    char* fin_str = static_cast<char*>(_alloca(fin_str_size));

#ifdef CLIENT_CTA_LOG
    Msg("---Making finally string: (t_vote_str: %s), (vstr: %s), (player: %s)", t_vote_str, vstr, player);
#endif

    xr_sprintf(fin_str, fin_str_size, t_vote_str, vstr, player);

#ifdef CLIENT_CTA_LOG
    Msg("---Starting vote: %s", fin_str);
#endif

    m_game_ui->SetVoteMessage(fin_str);
    m_game_ui->SetVoteTimeResultMsg("");

    if (!m_pVoteRespondWindow)
        m_pVoteRespondWindow = new CUIVote();
    m_pVoteRespondWindow->SetVoting(fin_str);
}

void game_cl_CaptureTheArtefact::UpdateWarmupTime(u32 current_time)
{
    if (!m_inWarmup)
        return;
    u32 remain_seconds = m_captions_manager.SetWarmupTime(m_currentWarmupTime, current_time);
    if (remain_seconds != 0)
    {
        PlaySndMessage(ID_COUNTDOWN_1 + remain_seconds - 1);
    }
}

void game_cl_CaptureTheArtefact::UpdateTimeLimit(u32 current_time)
{
    m_captions_manager.SetTimeLimit(m_start_time + m_s32TimeLimit, current_time);
}

bool game_cl_CaptureTheArtefact::HasTimeLimit() const { return (m_s32TimeLimit && !InWarmUp()); }
void game_cl_CaptureTheArtefact::UpdateVotingTime(u32 current_time)
{
    if (IsVotingEnabled() && IsVotingActive() && (m_dwVoteEndTime >= current_time))
    {
        u32 TimeLeft = m_dwVoteEndTime - current_time;
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

        xr_sprintf(VoteTimeResStr, StringTable().translate("mp_timeleft").c_str(), MinitsLeft, SecsLeft,
            float(NumAgreed) / players.size());
        if (m_game_ui)
            m_game_ui->SetVoteTimeResultMsg(VoteTimeResStr);
    };
}

void game_cl_CaptureTheArtefact::OnVoteStop(NET_Packet& P)
{
    inherited::OnVoteStop(P);
#ifdef CLIENT_CTA_LOG
    Msg("---Voting stoped...");
#endif
    if (m_game_ui)
    {
        m_game_ui->SetVoteMessage(NULL);
        m_game_ui->SetVoteTimeResultMsg(NULL);
    }
}
void game_cl_CaptureTheArtefact::OnVoteEnd(NET_Packet& P)
{
    inherited::OnVoteEnd(P);
    if (m_game_ui)
    {
        m_game_ui->SetVoteMessage(NULL);
        m_game_ui->SetVoteTimeResultMsg(NULL);
    }
}

void game_cl_CaptureTheArtefact::OnTeamMenuBack()
{
    if (local_player->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
    {
        m_game_ui->ShowServerInfo();
    }
}
void game_cl_CaptureTheArtefact::OnTeamMenu_Cancel()
{
    if (m_bTeamSelected)
    {
        m_game_ui->ShowTeamSelectMenu();
    }
}

void game_cl_CaptureTheArtefact::OnTeamSelect(int Team)
{
    bool NeedToSendTeamSelect = true;
    if (Team != -1)
    {
        if (Team == local_player->team && m_bSkinSelected)
        {
            NeedToSendTeamSelect = false;
        }
        else
        {
            NeedToSendTeamSelect = true;
        }
    }

    if (NeedToSendTeamSelect)
    {
        NET_Packet P;
        u_EventGen(P, GE_GAME_EVENT, local_player->GameID);
        P.w_u16(GAME_EVENT_PLAYER_GAME_MENU);
        P.w_u8(PLAYER_CHANGE_TEAM);
        P.w_s16(static_cast<s16>(Team));
        // P.w_u32			(0);
        u_EventSend(P);
        //-----------------------------------------------------------------
        m_bSkinSelected = FALSE;
    };
    //-----------------------------------------------------------------
    m_bTeamSelected = TRUE;
}

void game_cl_CaptureTheArtefact::OnSkinMenuBack()
{
    VERIFY(m_game_ui);
    if (CanCallTeamSelectMenu())
    {
        m_game_ui->ShowTeamSelectMenu();
    }
}

void game_cl_CaptureTheArtefact::OnSkinMenu_Ok()
{
    VERIFY(m_game_ui);
    NET_Packet P;

    // sending request for selecting actor scin
    u_EventGen(P, GE_GAME_EVENT, local_player->GameID);
    P.w_u16(GAME_EVENT_PLAYER_GAME_MENU);
    P.w_u8(PLAYER_CHANGE_SKIN);
    P.w_s8(m_game_ui->GetSelectedSkinIndex());
    u_EventSend(P);
}

void game_cl_CaptureTheArtefact::OnSkinMenu_Cancel()
{
    VERIFY(m_game_ui);
    if ((!m_bSkinSelected) && CanCallTeamSelectMenu())
    {
        m_game_ui->ShowTeamSelectMenu();
    }
}

void game_cl_CaptureTheArtefact::OnSwitchPhase(u32 old_phase, u32 new_phase)
{
    if (!m_game_ui)
        return;
    m_game_ui->UpdateTeamPanels();
    if (new_phase == GAME_PHASE_INPROGRESS)
    {
        if (m_game_ui->IsTeamPanelsShown())
        {
            m_game_ui->ShowTeamPanels(false);
        }
    }
    else if (new_phase == GAME_PHASE_PLAYER_SCORES)
    {
        if (!m_game_ui->IsTeamPanelsShown())
        {
            m_game_ui->ShowTeamPanels(true);
        }
    }
}

u16 game_cl_CaptureTheArtefact::GetGreenArtefactOwnerID() const
{
    R_ASSERT2(haveGotUpdate, "synchronization problem: not received client update, while try to get values");
    return greenArtefactOwner;
}

u16 game_cl_CaptureTheArtefact::GetBlueArtefactOwnerID() const
{
    R_ASSERT2(haveGotUpdate, "synchronization problem: not received client update, while try to get values");
    return blueArtefactOwner;
}

Fvector const& game_cl_CaptureTheArtefact::GetGreenArtefactRPoint() const
{
    R_ASSERT2(haveGotUpdate, "synchronization problem: not received client update, while try to get values");
    return greenTeamRPos;
}

Fvector const& game_cl_CaptureTheArtefact::GetBlueArtefactRPoint() const
{
    R_ASSERT2(haveGotUpdate, "synchronization problem: not received client update, while try to get values");
    return blueTeamRPos;
}

float game_cl_CaptureTheArtefact::GetBaseRadius() const
{
    R_ASSERT2(haveGotUpdate, "synchronization problem: not received client update, while try to get values");
    return m_baseRadius;
}
u16 game_cl_CaptureTheArtefact::GetGreenArtefactID() const
{
    R_ASSERT2(haveGotUpdate, "synchronization problem: not received client update, while try to get values");
    return greenArtefact;
}

u16 game_cl_CaptureTheArtefact::GetBlueArtefactID() const
{
    R_ASSERT2(haveGotUpdate, "synchronization problem: not received client update, while try to get values");
    return blueArtefact;
}

bool game_cl_CaptureTheArtefact::IsEnemy(game_PlayerState* ps)
{
    VERIFY(ps && local_player);
    return (ps->team != local_player->team);
}

#define PLAYER_NAME_COLOR 0xff40ff40
void game_cl_CaptureTheArtefact::OnRender()
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

            VERIFY(pObject);
            CActor* pActor = smart_cast<CActor*>(pObject);
            VERIFY(pActor);
            Fvector IPos = pTS->IndicatorPos;

            if (ps->testFlag(GAME_PLAYER_FLAG_INVINCIBLE) && !ps->IsSkip() && (ps->team != etSpectatorsTeam) &&
                (ps != local_player))
            {
                // pActor->RenderIndicator(IPos, pTS->Indicator_r1, pTS->Indicator_r2, pTS->InvincibleShader);
            }

            if (IsEnemy(ps))
                continue;

            if (ps == local_player)
                continue;

            float dup = 0.0f;
            if (m_bShowPlayersNames)
            {
                IPos.y -= pTS->Indicator_r2;
                VERIFY(ps->getName());
                string64 upper_name;
                xr_strcpy(upper_name, ps->getName());
                _strupr(upper_name);
                pActor->RenderText(upper_name, IPos, &dup, PLAYER_NAME_COLOR);
            }
            if (m_bFriendlyIndicators)
            {
                IPos.y += dup;
                pActor->RenderIndicator(IPos, pTS->Indicator_r1, pTS->Indicator_r2, pTS->IndicatorShader);
            };
        }
    };
    inherited::OnRender();
}

bool game_cl_CaptureTheArtefact::PlayerCanSprint(CActor* pActor)
{
    VERIFY(pActor);
    if (m_bBearerCantSprint)
        return true;

    u16 greenArtefactOwner = GetGreenArtefactOwnerID();
    u16 blueArtefactOwner = GetBlueArtefactOwnerID();
    u16 myID = pActor->ID();
    if ((myID == greenArtefactOwner) || (myID == blueArtefactOwner))
    {
        return false;
    }
    return true;
}

bool game_cl_CaptureTheArtefact::CanActivateArtefact() const { return m_bCanActivateArtefact; }

pcstr game_cl_CaptureTheArtefact::getTeamSection(int Team)
{
    switch (Team)
    {
    case 0: { return "capturetheartefact_team1";
    }
    break;
    case 1: { return "capturetheartefact_team2";
    }
    break;
    default: NODEFAULT;
    };
#ifdef DEBUG
    return NULL;
#endif
}

LPCSTR game_cl_CaptureTheArtefact::GetGameScore(string32& score_dest)
{
    xr_sprintf(score_dest, "[%d:%d]", greenTeamScore, blueTeamScore);
    return score_dest;
}

void game_cl_CaptureTheArtefact::OnConnected()
{
    inherited::OnConnected();
    if (m_game_ui)
    {
        VERIFY(!GEnv.isDedicatedServer);
        m_game_ui = smart_cast<CUIGameCTA*>(CurrentGameUI());
        m_game_ui->SetClGame(this);
    }
}
