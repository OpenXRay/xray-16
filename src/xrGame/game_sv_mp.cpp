#include "stdafx.h"
#include "game_sv_mp.h"
#include "xrServer.h"
#include "xrMessages.h"
#include "xrServer_Object_Base.h"
#include "xrServer_Objects.h"
#include "Level.h"
#include "xrserver_objects_alife_monsters.h"
#include "actor.h"
#include "xrEngine/XR_IOConsole.h"
#include "xrEngine/IGame_Persistent.h"
#include "date_time.h"
#include "game_cl_base.h"
#include "Spectator.h"
#include "Grenade.h"
#include "Inventory.h"
#include "Artefact.h"
#include "MPPlayersBag.h"
#include "WeaponKnife.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "xrGameSpyServer.h"
#include "xrNetServer/NET_Messages.h"
#include "xrCore/xr_token.h"

#include "game_sv_mp_vote_flags.h"
#include "player_name_modifyer.h"

u32 g_dwMaxCorpses = 10;
//-----------------------------------------------------------------
BOOL g_sv_mp_bSpectator_FreeFly = FALSE;
BOOL g_sv_mp_bSpectator_FirstEye = TRUE;
BOOL g_sv_mp_bSpectator_LookAt = TRUE;
BOOL g_sv_mp_bSpectator_FreeLook = TRUE;
BOOL g_sv_mp_bSpectator_TeamCamera = TRUE;
int g_sv_mp_iDumpStatsPeriod = 0;
int g_sv_mp_iDumpStats_last = 0;
BOOL g_sv_mp_bCountParticipants = FALSE;
float g_sv_mp_fVoteQuota = VOTE_QUOTA;
float g_sv_mp_fVoteTime = VOTE_LENGTH_TIME;
BOOL g_sv_mp_save_proxy_screenshots = FALSE;
BOOL g_sv_mp_save_proxy_configs = FALSE;
//-----------------------------------------------------------------
u32 g_sv_adm_menu_ban_time = 1;
int g_sv_adm_menu_ping_limit = 25;
//-----------------------------------------------------------------

extern const xr_token round_end_result_str[];

#include "ui/UIBuyWndShared.h"

game_sv_mp::game_sv_mp()
    : inherited(), m_bRankUp_Allowed(false), m_bVotingReal(false),
      m_uVoteStartTime(0), m_u8SpectatorModes(0)
{
    m_strWeaponsData = new CItemMgr();
    m_bVotingActive = false;
    //------------------------------------------------------
    //	g_pGamePersistent->Environment().SetWeather("mp_weather");
    m_aRanks.clear();
    //------------------------------------------------------
    round_end_reason = eRoundEnd_Force; // unknown
    m_async_stats_request_time = 0;
    round_statistics_dump_fn[0] = 0;
}

game_sv_mp::~game_sv_mp() { xr_delete(m_strWeaponsData); }
void game_sv_mp::Update()
{
    inherited::Update();

    // remove corpses if their number exceed limit
    for (u32 i = 0; i < m_CorpseList.size();)
    {
        if (m_CorpseList.size() <= g_dwMaxCorpses)
            break;

        u16 CorpseID = m_CorpseList[i];

        CSE_Abstract* pCorpseObj = get_entity_from_eid(CorpseID);

        if (!pCorpseObj)
        {
            m_CorpseList.erase(m_CorpseList.begin() + i);
            Msg("corpse [%d] not found [%d]", CorpseID, Device.dwFrame);
            continue;
        }
        if (!pCorpseObj->children.empty())
        {
            Msg("corpse [%d] childern not empty [%d]", CorpseID, Device.dwFrame);
            i++;
            continue;
        }

        //---------------------------------------------
        NET_Packet P;
        u_EventGen(P, GE_DESTROY, CorpseID);
        Level().Send(P, net_flags(TRUE, TRUE));
        m_CorpseList.erase(m_CorpseList.begin() + i);
        Msg("corpse [%d] send destroy [%d]", CorpseID, Device.dwFrame);
    }

    if (IsVotingEnabled() && IsVotingActive())
        UpdateVote();
    //-------------------------------------------------------
    UpdatePlayersMoney();

    if (g_sv_mp_iDumpStatsPeriod)
    {
        int curr_minutes = iFloor(Device.fTimeGlobal / 60.0f);
        if (g_sv_mp_iDumpStats_last + g_sv_mp_iDumpStatsPeriod <= curr_minutes)
        {
            if (Phase() == GAME_PHASE_INPROGRESS)
            {
                DumpOnlineStatistic();
                DumpRoundStatistics();
                g_sv_mp_iDumpStats_last = curr_minutes;
            }
        }
    }
}

void game_sv_mp::OnRoundStart()
{
    inherited::OnRoundStart();

    if (g_pGameLevel && Level().game)
    {
        Game().m_WeaponUsageStatistic->Clear();
        StartToDumpStatistics();
    }

    m_CorpseList.clear();

    switch_Phase(GAME_PHASE_INPROGRESS);
    ++m_round;
    m_round_start_time = Level().timeServer();
    timestamp(m_round_start_time_str);

    // clear "ready" flag
    struct ready_clearer
    {
        void operator()(IClient* client)
        {
            xrClientData* tmp_client = static_cast<xrClientData*>(client);
            game_PlayerState* tmp_ps = tmp_client->ps;
            if (!tmp_ps)
                return;
            tmp_ps->resetFlag(GAME_PLAYER_FLAG_READY + GAME_PLAYER_FLAG_VERY_VERY_DEAD);
            tmp_ps->m_online_time = Level().timeServer();
        }
    };
    ready_clearer tmp_functor;
    m_server->ForEachClientDo(tmp_functor);
    m_async_stats_request_time = 0;
    m_server->ClearDisconnectedPool();

    // 1. We have to destroy all delayed events
    CleanDelayedEvents();

    // 2. We have to destroy all player-entities and entities
    m_server->SLS_Clear();

    // 3. We have to create them at respawn points and/or specified positions
    m_server->SLS_Default();

    // send "RoundStarted" Message To Allclients
    NET_Packet P;
    //	P.w_begin			(M_GAMEMESSAGE);
    GenerateGameMessage(P);
    P.w_u32(GAME_EVENT_ROUND_STARTED);
    u_EventSend(P);
    signal_Syncronize();
}

void game_sv_mp::OnRoundEnd()
{
    inherited::OnRoundEnd();

    string64 res_str;
    xr_strcpy(res_str, get_token_name(round_end_result_str, round_end_reason));

    OnVoteStop();

    switch_Phase(GAME_PHASE_PENDING);
    // send "RoundOver" Message To All clients
    NET_Packet P;
    //	P.w_begin			(M_GAMEMESSAGE);
    GenerateGameMessage(P);
    P.w_u32(GAME_EVENT_ROUND_END);
    P.w_stringZ(res_str);
    u_EventSend(P);
    //-------------------------------------------------------
    CleanDelayedEvents();
    struct event_clearer
    {
        GameEventQueue* event_queue;
        IClient* server_client;
        event_clearer(GameEventQueue* eq, IClient* sc)
        {
            event_queue = eq;
            server_client = sc;
        };
        void operator()(IClient* client)
        {
            if (client != server_client)
            {
                event_queue->SetIgnoreEventsFor(true, client->ID);
            }
        }
    };

    event_clearer tmp_functor(m_event_queue, m_server->GetServerClient());
    m_server->ForEachClientDo(tmp_functor);
}

struct real_sender
{
    xrServer* server_for_send;
    NET_Packet* P;
    u32 flags_to_send;

    real_sender(xrServer* server, NET_Packet* Packet, u32 flags = DPNSEND_GUARANTEED)
    {
        server_for_send = server;
        P = Packet;
        flags_to_send = flags;
    }
    void operator()(IClient* client)
    {
        xrClientData* tmp_client = static_cast<xrClientData*>(client);
        game_PlayerState* ps = tmp_client->ps;
        if (!ps || !tmp_client->net_Ready)
            return;

        server_for_send->SendTo(client->ID, *P, flags_to_send);
    }
};

void game_sv_mp::KillPlayer(ClientID id_who, u16 GameID)
{
    IGameObject* pObject = Level().Objects.net_Find(GameID);
    if (!pObject || !smart_cast<CActor*>(pObject))
        return;
    // Remove everything
    xrClientData* xrCData = m_server->ID_to_client(id_who);
#ifdef DEBUG
    if (xrCData && xrCData->ps && xrCData->ps->getName())
        Msg("--- Killing player [%s]", xrCData->ps->getName());
#endif // #ifdef DEBUG

    if (xrCData && xrCData->ps && xrCData->ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
#ifdef DEBUG
        Msg("--- Killing dead player [%s]", xrCData->ps->getName());
#endif // #ifdef DEBUG
        return;
    }
    if (xrCData)
    {
        //-------------------------------------------------------
        OnPlayerKillPlayer(xrCData->ps, xrCData->ps, KT_HIT, SKT_NONE, NULL);
        if (xrCData->ps)
            xrCData->ps->m_bClearRun = false;
    };
    //-------------------------------------------------------
    CActor* pActor = smart_cast<CActor*>(pObject);
    if (pActor)
    {
        if (!pActor->g_Alive())
        {
            Msg("! WARNING: Actor already died");
            return;
        }
        pActor->set_death_time();
    }
    //-------------------------------------------------------
    u16 PlayerID = (xrCData != 0) ? xrCData->ps->GameID : GameID;
    //-------------------------------------------------------
    SendPlayerKilledMessage(PlayerID, KT_HIT, PlayerID, 0, SKT_NONE);
    //-------------------------------------------------------
    /*AllowDeadBodyRemove			(id_who, GameID);
    m_CorpseList.push_back		(GameID);*/
    // Kill Player on all clients
    NET_Packet P;
    u_EventGen(P, GE_DIE, PlayerID);
    P.w_u16(PlayerID);
    P.w_clientID(id_who);

    u_EventSend(P, net_flags(TRUE, TRUE, FALSE, TRUE));

    if (xrCData)
        SetPlayersDefItems(xrCData->ps);
    signal_Syncronize();
    //-------------------------------------------------------
};

void game_sv_mp::OnEvent(NET_Packet& P, u16 type, u32 time, ClientID sender)
{
    switch (type)
    {
    case GAME_EVENT_PLAYER_KILLED: // playerKillPlayer
    {
#ifdef DEBUG
        xrClientData* l_pC = m_server->ID_to_client(sender);
        if (l_pC && l_pC->ps)
            Msg("--- GAME_EVENT_PLAYER_KILLED: sender [%d][0x%08x]", l_pC->ps->GameID, sender);
#endif // #ifdef DEBUG
        OnPlayerKilled(P);
    }
    break;
    case GAME_EVENT_PLAYER_HITTED: {
#ifdef DEBUG
        xrClientData* l_pC = m_server->ID_to_client(sender);
        if (l_pC && l_pC->ps)
            Msg("--- GAME_EVENT_PLAYER_HITTED: sender [%d][0x%08x]", l_pC->ps->GameID, sender);
#endif // #ifdef DEBUG
        OnPlayerHitted(P);
    }
    break;
    case GAME_EVENT_PLAYER_READY: // cs & dm
    {
        xrClientData* l_pC = m_server->ID_to_client(sender);
        if (!l_pC)
            break;
        OnPlayerReady(l_pC->ID);
    }
    break;
    case GAME_EVENT_PLAYER_BUY_SPAWN:
    {
        xrClientData* l_pC = m_server->ID_to_client(sender);
        if (!l_pC)
            break;
        OnPlayerBuySpawn(l_pC->ID);
    }
    break;
    case GAME_EVENT_VOTE_START:
    {
        if (!IsVotingEnabled())
            break;
        string1024 VoteCommand;
        if (P.r_elapsed() > (sizeof(VoteCommand) - 1))
            break;
        P.r_stringZ_s(VoteCommand);
        OnVoteStart(VoteCommand, sender);
    }
    break;
    case GAME_EVENT_VOTE_YES:
    {
        if (!IsVotingEnabled())
            break;
        OnVoteYes(sender);
    }
    break;
    case GAME_EVENT_VOTE_NO:
    {
        if (!IsVotingEnabled())
            break;
        OnVoteNo(sender);
    }
    break;
    case GAME_EVENT_GET_ACTIVE_VOTE:
    {
        if (!IsVotingActive())
            break;
        SendActiveVotingTo(sender);
    }
    break;
    case GAME_EVENT_PLAYER_NAME: { OnPlayerChangeName(P, sender);
    }
    break;
    case GAME_EVENT_SPEECH_MESSAGE: { OnPlayerSpeechMessage(P, sender);
    }
    break;
    case GAME_EVENT_PLAYER_GAME_MENU:
    {
        OnPlayerGameMenu(P, sender);
        //			OnPlayerSelectSpectator(P, sender);
    }
    break;
    case GAME_EVENT_PLAYER_STARTED: {
#ifdef DEBUG
        Msg("--- Player 0x%08x started.", sender);
#endif // #ifdef DEBUG
        if (CheckPlayerMapName(sender, P))
        {
            m_event_queue->SetIgnoreEventsFor(false, sender);
        }
    }
    break;
    // THIS EVENTS PLAYER MUST RISE ONLY IN DEAD STATE
    case GAME_EVENT_PLAYER_BUYMENU_OPEN:
    {
        xrClientData const* pClient = (xrClientData const*)m_server->ID_to_client(sender);
        if (pClient)
        {
            OnPlayerOpenBuyMenu(pClient);
        }
        else
        {
            VERIFY2(pClient,
                make_string("unknown client [0x%08x] sended GAME_EVENT_PLAYER_BUYMENU_OPEN message", sender.value())
                    .c_str());
#ifndef MASTER_GOLD
            Msg("! ERROR: unknown client [0x%08x] opens buy menu", sender.value());
#endif // #ifndef MASTER_GOLD
        }
    }
    break;
    case GAME_EVENT_PLAYER_BUYMENU_CLOSE:
    {
        xrClientData const* pClient = (xrClientData const*)m_server->ID_to_client(sender);
        if (pClient)
        {
            OnPlayerCloseBuyMenu(pClient);
        }
        else
        {
            VERIFY2(pClient,
                make_string("unknown client [0x%08x] sended GAME_EVENT_PLAYER_BUYMENU_OPEN message", sender.value())
                    .c_str());
#ifndef MASTER_GOLD
            Msg("! ERROR: unknown client [0x%08x] opens buy menu", sender.value());
#endif // #ifndef MASTER_GOLD
        }
    }
    break;
    default: inherited::OnEvent(P, type, time, sender);
    }; // switch
}

bool game_sv_mp::CheckPlayerMapName(ClientID const& clientID, NET_Packet& P)
{
    string256 temp_map_name;
    P.r_stringZ_s(temp_map_name);
    R_ASSERT(Level().name().c_str());

    if (xr_strcmp(Level().name().c_str(), temp_map_name))
    {
        Msg("! Player 0x%08x has incorrect map name", clientID, temp_map_name);
        // ReconnectPlayer(clientID);
        return false;
    }
    return true;
}

LPCSTR GameTypeToString(EGameIDs gt, bool bShort);

void game_sv_mp::ReconnectPlayer(ClientID const& clientID)
{
#ifdef DEBUG
    Msg("--- Reconnecting player 0x%08x", clientID);
#endif // #ifdef DEBUG
    NET_Packet P;
    P.w_begin(M_CHANGE_LEVEL_GAME);
    P.w_stringZ(Level().name().c_str());
    P.w_stringZ(GameTypeToString(Type(), true));
    m_server->SendTo(clientID, P, net_flags(TRUE, TRUE));
}

bool g_bConsoleCommandsCreated = false;
extern float g_fTimeFactor;
#define SAVE_SCREENSHOTS_KEY "-savescreenshots"
void game_sv_mp::Create(shared_str& options)
{
    SetVotingActive(false);
    inherited::Create(options);
    //-------------------------------------------------------------------
    if (!g_bConsoleCommandsCreated)
    {
        g_bConsoleCommandsCreated = true;
    }

    //------------------------------------------------------------------
    LoadRanks();
    //------------------------------------------------------------------
    Set_RankUp_Allowed(false);
    m_cdkey_ban_list.load();
    if (strstr(Core.Params, SAVE_SCREENSHOTS_KEY))
    {
        g_sv_mp_save_proxy_screenshots = TRUE;
    }
};

u8 game_sv_mp::SpectatorModes_Pack()
{
    u8 res = 0;

    res |= g_sv_mp_bSpectator_FreeFly ? (1 << CSpectator::eacFreeFly) : 0;
    res |= g_sv_mp_bSpectator_FirstEye ? (1 << CSpectator::eacFirstEye) : 0;
    res |= g_sv_mp_bSpectator_LookAt ? (1 << CSpectator::eacLookAt) : 0;
    res |= g_sv_mp_bSpectator_FreeLook ? (1 << CSpectator::eacFreeLook) : 0;
    res |= g_sv_mp_bSpectator_TeamCamera ? (1 << CSpectator::eacMaxCam) : 0;
    return res;
}

void game_sv_mp::SpectatorModes_UnPack(u8 SpectrModesPacked)
{
    g_sv_mp_bSpectator_FreeFly = (SpectrModesPacked & (1 << CSpectator::eacFreeFly)) != 0;
    g_sv_mp_bSpectator_FirstEye = (SpectrModesPacked & (1 << CSpectator::eacFirstEye)) != 0;
    g_sv_mp_bSpectator_LookAt = (SpectrModesPacked & (1 << CSpectator::eacLookAt)) != 0;
    g_sv_mp_bSpectator_FreeLook = (SpectrModesPacked & (1 << CSpectator::eacFreeLook)) != 0;
    g_sv_mp_bSpectator_TeamCamera = (SpectrModesPacked & (1 << CSpectator::eacMaxCam)) != 0;
};

void game_sv_mp::net_Export_State(NET_Packet& P, ClientID id_to)
{
    inherited::net_Export_State(P, id_to);
    //-------------------------------------
    m_u8SpectatorModes = SpectatorModes_Pack();

    P.w_u8(m_u8SpectatorModes);
};

void game_sv_mp::RespawnPlayer(ClientID id_who, bool NoSpectator)
{
    //------------------------------------------------------------

    xrClientData* xrCData = m_server->ID_to_client(id_who);
    if (!xrCData || !xrCData->owner)
        return;
    //	game_PlayerState*	ps	=	&(xrCData->ps);
    CSE_Abstract* pOwner = xrCData->owner;
    CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(pOwner);
    CSE_Spectator* pS = smart_cast<CSE_Spectator*>(pOwner);

    if (pA)
    {
        //------------------------------------------------------------
        AllowDeadBodyRemove(id_who, xrCData->ps->GameID);
        //------------------------------------------------------------
        m_CorpseList.push_back(pOwner->ID);
        //------------------------------------------------------------
    };

    if (pA && !NoSpectator)
    {
        //------------------------------------------------------------
        SpawnPlayer(id_who, "spectator");
        //------------------------------------------------------------
    }
    else
    {
        //------------------------------------------------------------
        if (pOwner->owner != m_server->GetServerClient())
        {
            pOwner->owner = (xrClientData*)m_server->GetServerClient();
        };
        //------------------------------------------------------------
        // remove spectator entity
        if (pS)
        {
            NET_Packet P;
            u_EventGen(P, GE_DESTROY, pS->ID);
            //		pObject->u_EventSend		(P);
            Level().Send(P, net_flags(TRUE, TRUE));
        };
        //------------------------------------------------------------
        SpawnPlayer(id_who, "mp_actor");
        //------------------------------------------------------------
        //		SpawnWeaponsForActor(xrCData->owner, ps);
        //------------------------------------------------------------
    };
};

void game_sv_mp::SpawnPlayer(ClientID id, LPCSTR N)
{
    xrClientData* CL = m_server->ID_to_client(id);
    //-------------------------------------------------
    CL->net_PassUpdates = TRUE;
    //-------------------------------------------------
    game_PlayerState* ps_who = CL->ps;
    ps_who->setFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);

    // Spawn "actor"
    CSE_Abstract* E = spawn_begin(N); // create SE

    E->set_name_replace(get_name_id(id)); // name

    E->s_flags.assign(M_SPAWN_OBJECT_LOCAL | M_SPAWN_OBJECT_ASPLAYER); // flags

    CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(E);
    CSE_Spectator* pS = smart_cast<CSE_Spectator*>(E);

    R_ASSERT2(pA || pS, "Respawned Client is not Actor nor Spectator");

    if (pA)
    {
        pA->s_team = u8(ps_who->team);
        assign_RP(pA, ps_who);
        SetSkin(E, pA->s_team, ps_who->skin);
        ps_who->resetFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);
        if (!ps_who->RespawnTime)
        {
            OnPlayerEnteredGame(id);
        };
        ps_who->RespawnTime = Device.dwTimeGlobal;

        Game().m_WeaponUsageStatistic->OnPlayerSpawned(ps_who);
    }
    else if (pS)
    {
        Fvector Pos, Angle;
        //			ps_who->setFlag(GAME_PLAYER_FLAG_CS_SPECTATOR);
        if (!GetPosAngleFromActor(id, Pos, Angle))
            assign_RP(E, ps_who);
        else
        {
            E->o_Angle.set(Angle);
            E->o_Position.set(Pos);
        }
    };

    Msg("* %s [%d] respawned as %s", get_name_id(id), E->ID, (0 == pA) ? "spectator" : "actor");
    spawn_end(E, id);

    ps_who->SetGameID(CL->owner->ID);

    signal_Syncronize();
}

void game_sv_mp::AllowDeadBodyRemove(ClientID id, u16 GameID)
{
    CSE_Abstract* pSObject = get_entity_from_eid(GameID);

    if (pSObject)
        pSObject->owner = (xrClientData*)m_server->GetServerClient();

    IGameObject* pObject = Level().Objects.net_Find(GameID);

    if (pObject && smart_cast<CActor*>(pObject))
    {
        CActor* pActor = smart_cast<CActor*>(pObject);
        if (pActor)
        {
            pActor->set_death_time();
            pActor->m_bAllowDeathRemove = true;
        };
    };
};

void game_sv_mp::OnPlayerConnect(ClientID id_who) { inherited::OnPlayerConnect(id_who); }
void game_sv_mp::OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID)
{
    //---------------------------------------------------
    NET_Packet P;
    GenerateGameMessage(P);
    P.w_u32(GAME_EVENT_PLAYER_DISCONNECTED);
    P.w_stringZ(Name);
    u_EventSend(P);
    //---------------------------------------------------
    KillPlayer(id_who, GameID);

    AllowDeadBodyRemove(id_who, GameID);

    CSE_Abstract* pSObject = get_entity_from_eid(GameID);
    if (pSObject)
        m_CorpseList.push_back(GameID);

    inherited::OnPlayerDisconnect(id_who, Name, GameID);
}

void game_sv_mp::SetSkin(CSE_Abstract* E, u16 Team, u16 ID)
{
    if (!E)
        return;
    //-------------------------------------------
    CSE_Visual* pV = smart_cast<CSE_Visual*>(E);
    if (!pV)
        return;
    //-------------------------------------------
    string256 SkinName;
    xr_strcpy(SkinName, pSettings->r_string("mp_skins_path", "skin_path"));
    //загружены ли скины для этой комманды

    if (!TeamList.empty() && TeamList.size() > Team && !TeamList[Team].aSkins.empty())
    {
        //загружено ли достаточно скинов для этой комманды
        if (TeamList[Team].aSkins.size() > ID)
        {
            xr_strcat(SkinName, TeamList[Team].aSkins[ID].c_str());
        }
        else
            xr_strcat(SkinName, TeamList[Team].aSkins[0].c_str());
    }
    else
    {
        R_ASSERT2(0, "Skin not loaded");
    };
    xr_strcat(SkinName, ".ogf");
    Msg("* Skin - %s", SkinName);
    int len = xr_strlen(SkinName);
    R_ASSERT2(len < 64, "Skin Name is too LONG!!!");
    pV->set_visual(SkinName);
    //-------------------------------------------
};

#include "xrEngine/CameraBase.h"

bool game_sv_mp::GetPosAngleFromActor(ClientID id, Fvector& Pos, Fvector& Angle)
{
    xrClientData* xrCData = m_server->ID_to_client(id);
    if (!xrCData || !xrCData->owner)
        return false;

    IGameObject* pObject = Level().Objects.net_Find(xrCData->owner->ID);
    ///	R_ASSERT2	((pObject && smart_cast<CActor*>(pObject)),"Dead Player is not Actor");

    if (!pObject || !smart_cast<CActor*>(pObject))
        return false;

    CActor* pActor = smart_cast<CActor*>(pObject);
    if (!pActor)
        return false;

    Angle.set(-pActor->cam_Active()->pitch, -pActor->cam_Active()->yaw, -pActor->cam_Active()->roll);
    Pos.set(pActor->cam_Active()->vPosition);
    return true;
};

TeamStruct* game_sv_mp::GetTeamData(u32 Team)
{
    VERIFY(TeamList.size());
    if (TeamList.empty())
        return NULL;

    if (TeamList.size() <= Team)
        return NULL;

    return &(TeamList[Team]);
};

/*void	game_sv_mp::SpawnWeaponForActor		(u16 actorId,  LPCSTR N, bool isScope, bool isGrenadeLauncher, bool
isSilencer)
{
        u8 addon_flags = 0;
        if(isScope)
            addon_flags |= CSE_ALifeItemWeapon::eWeaponAddonScope;

        if(isGrenadeLauncher)
            addon_flags |= CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;

        if(isSilencer)
            addon_flags |= CSE_ALifeItemWeapon::eWeaponAddonSilencer;

        SpawnWeapon4Actor(actorId, N, addon_flags);
}*/
void game_sv_mp::ChargeAmmo(CSE_ALifeItemWeapon* weapon, LPCSTR ammo_string,
    game_PlayerState::PLAYER_ITEMS_LIST& playerItems, ammo_diff_t& ammo_diff)
{
    int ammoc_count = _GetItemCount(ammo_string);
    u16 ammo_magsize = weapon->get_ammo_magsize();
    weapon->a_elapsed = 0;

    string512 temp_ammo_class;
    for (int i = 0; i < ammoc_count; ++i)
    {
        _GetItem(ammo_string, i, temp_ammo_class);
        u32 const ammo_id = static_cast<u16>(m_strWeaponsData->GetItemIdx(shared_str(temp_ammo_class)));
        u16 box_size = 0;
        if (pSettings->line_exist(temp_ammo_class, "box_size"))
            box_size = pSettings->r_u16(temp_ammo_class, "box_size");

        game_PlayerState::PLAYER_ITEMS_LIST::iterator temp_iter =
            std::find(playerItems.begin(), playerItems.end(), ammo_id);

        while (temp_iter != playerItems.end())
        {
            weapon->ammo_type = static_cast<u8>(i);
            playerItems.erase(temp_iter);
            if ((ammo_magsize - weapon->a_elapsed) <= box_size)
            {
                ammo_diff.first = shared_str(temp_ammo_class);
                ammo_diff.second = box_size - (ammo_magsize - weapon->a_elapsed);
                weapon->a_elapsed = ammo_magsize;
                break;
            }
            else
            {
                weapon->a_elapsed = weapon->a_elapsed + box_size;
            }
            temp_iter = std::find(playerItems.begin(), playerItems.end(), ammo_id);
        }
        if (weapon->a_elapsed)
            break;
    }
    if (!weapon->a_elapsed)
    {
        _GetItem(ammo_string, 0, temp_ammo_class);
        weapon->ammo_type = 0;
        if (CanChargeFreeAmmo(temp_ammo_class))
        {
            weapon->a_elapsed = ammo_magsize;
        }
    }
}

void game_sv_mp::ChargeGrenades(
    CSE_ALifeItemWeapon* weapon, LPCSTR grenade_string, game_PlayerState::PLAYER_ITEMS_LIST& playerItems)
{
    int grenades_count = _GetItemCount(grenade_string);
    R_ASSERT2(grenades_count <= 4,
        make_string("weapon [%s] has greater than 4 types of grenade [%s]", weapon->s_name.c_str(), grenade_string)
            .c_str());
    weapon->a_elapsed_grenades.unpack_from_byte(0);
    string512 temp_ammo_class;
    for (int i = 0; i < grenades_count; ++i)
    {
        _GetItem(grenade_string, i, temp_ammo_class);
        u32 const ammo_id = static_cast<u16>(m_strWeaponsData->GetItemIdx(shared_str(temp_ammo_class)));

        game_PlayerState::PLAYER_ITEMS_LIST::iterator temp_iter =
            std::find(playerItems.begin(), playerItems.end(), ammo_id);
        if (temp_iter != playerItems.end())
        {
            playerItems.erase(temp_iter);
            weapon->a_elapsed_grenades.grenades_count = 1;
            weapon->a_elapsed_grenades.grenades_type = i;
            break;
        }
    }
}

void game_sv_mp::SetAmmoForWeapon(
    CSE_ALifeItemWeapon* weapon, u8 Addons, game_PlayerState::PLAYER_ITEMS_LIST& playerItems, ammo_diff_t& ammo_diff)
{
    R_ASSERT(weapon);
    R_ASSERT(weapon->s_name.c_str());
    shared_str ammo_classes = pSettings->r_string(weapon->s_name, "ammo_class");
    R_ASSERT2(ammo_classes.size() < 512,
        make_string("ammo_class parameter of [%s] is too large", weapon->s_name.c_str()).c_str());
    VERIFY2(ammo_classes.size(), make_string("ammo_class parameter of [%s] not found", weapon->s_name.c_str()).c_str());

    if (!ammo_classes.size())
    {
#ifdef DEBUG
        Msg("! WARNING: not found ammo_class for [%s]", weapon->s_name.c_str());
#endif
        weapon->a_elapsed = 0;
    }
    else
    {
        ChargeAmmo(weapon, ammo_classes.c_str(), playerItems, ammo_diff);
    }

    if ((Addons & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) ||
        (weapon->m_grenade_launcher_status == ALife::eAddonPermanent))
    {
        shared_str grenade_classes = pSettings->r_string(weapon->s_name, "grenade_class");
        R_ASSERT2(grenade_classes.size() < 512,
            make_string("grenade_class parameter of [%s] is too large", weapon->s_name.c_str()).c_str());
        if (!grenade_classes.size())
        {
#ifdef DEBUG
            Msg("! WARNING: not found grenade_class for [%s]", weapon->s_name.c_str());
#endif
            weapon->a_elapsed_grenades.unpack_from_byte(0);
            return;
        }
        else
        {
            ChargeGrenades(weapon, grenade_classes.c_str(), playerItems);
        }
    }
}

void game_sv_mp::SpawnAmmoDifference(u16 actorId, ammo_diff_t const& ammo_diff)
{
    if (!ammo_diff.first.size() || !ammo_diff.second)
        return;

    CSE_Abstract* ammo_entity = spawn_begin(ammo_diff.first.c_str());
    ammo_entity->ID_Parent = actorId;
    CSE_ALifeItemAmmo* temp_ammo = smart_cast<CSE_ALifeItemAmmo*>(ammo_entity);
    R_ASSERT2(temp_ammo, "ammo difference tries to spawn not an ammo");
    temp_ammo->a_elapsed = ammo_diff.second;
    spawn_end(ammo_entity, m_server->GetServerClient()->ID);
}

void game_sv_mp::SpawnWeapon4Actor(u16 actorId, LPCSTR N, u8 Addons, game_PlayerState::PLAYER_ITEMS_LIST& playerItems)
{
    if (!N)
        return;

    CSE_Abstract* E = spawn_begin(N);
    E->ID_Parent = actorId;

    ammo_diff_t ammo_diff;
    E->s_flags.assign(M_SPAWN_OBJECT_LOCAL); // flags
    /////////////////////////////////////////////////////////////////////////////////
    CSE_ALifeItemWeapon* pWeapon = smart_cast<CSE_ALifeItemWeapon*>(E);
    if (pWeapon)
    {
        pWeapon->m_addon_flags.assign(Addons);
        SetAmmoForWeapon(pWeapon, Addons, playerItems, ammo_diff);
    };
    /////////////////////////////////////////////////////////////////////////////////

    spawn_end(E, m_server->GetServerClient()->ID);
    SpawnAmmoDifference(actorId, ammo_diff);
};

void game_sv_mp::OnDestroyObject(u16 eid_who)
{
    auto it = std::find(m_CorpseList.begin(), m_CorpseList.end(), eid_who);
    if (it != m_CorpseList.end())
    {
        m_CorpseList.erase(it);
    };
    /*
    for (u32 i=0; i<m_CorpseList.size();)
    {
        if (m_CorpseList[i] == eid_who)
        {
            m_CorpseList.erase(m_CorpseList.begin()+i);
        }
        else i++;
    };
    */
};

bool game_sv_mp::OnNextMap()
{
    if (!m_bMapRotation)
        return false;
    Msg("m_bMapSwitched - %s", m_bMapSwitched ? "true" : "false");
    if (m_bMapSwitched)
        return false;
    if (!m_pMapRotation_List.size())
        return false;

    SMapRot R = m_pMapRotation_List.front();
    m_pMapRotation_List.pop_front();
    m_pMapRotation_List.push_back(R);

    R = m_pMapRotation_List.front();

    Msg("Going to level %s", R.map_name.c_str());
    m_bMapSwitched = true;

    string1024 Command;
    xr_sprintf(Command, "sv_changelevel %s %s", R.map_name.c_str(), R.map_ver.c_str());
    Console->Execute(Command);
    return true;
};

void game_sv_mp::OnPrevMap()
{
    if (!m_bMapRotation)
        return;
    Msg("m_bMapSwitched - %s", m_bMapSwitched ? "true" : "false");
    if (m_bMapSwitched)
        return;
    if (!m_pMapRotation_List.size())
        return;

    SMapRot R = m_pMapRotation_List.back();
    m_pMapRotation_List.pop_back();
    m_pMapRotation_List.push_front(R);

    Msg("Goint to level %s", R.map_name.c_str());
    m_bMapSwitched = true;

    string1024 Command;
    xr_sprintf(Command, "sv_changelevel %s %s", R.map_name.c_str(), R.map_ver.c_str());
    Console->Execute(Command);
};

struct _votecommands
{
    pcstr name;
    pcstr command;
    u16 flag;
};

_votecommands votecommands[] = {{"restart", "g_restart", flVoteRestart},
    {"restart_fast", "g_restart_fast", flVoteRestartFast}, {"kick", "sv_kick", flVoteKick},
    {"ban", "sv_banplayer", flVoteBan}, {"changemap", "sv_changelevel", flVoteMap},
    {"changeweather", "sv_setenvtime", flVoteWeather}, {"changegametype", "sv_changegametype", flVoteGameType},
    {NULL, NULL}};

s32 game_sv_mp::ExcludeBanTimeFromVoteStr(char const* vote_string, char* new_vote_str, u32 new_vote_str_size)
{
    if (!vote_string || !xr_strlen(vote_string))
        return 0;

    s32 ret_time = 0;
    strncpy_s(new_vote_str, new_vote_str_size, vote_string, new_vote_str_size - 1);
    new_vote_str[xr_strlen(vote_string)] = 0;
    char* start_time_str = strrchr(new_vote_str, ' ');
    if (!start_time_str || !xr_strlen(++start_time_str))
        return 0;
    ret_time = atoi(start_time_str);
    *(start_time_str - 1) = 0;
    return ret_time;
}

struct SearcherClientByName
{
    string128 player_name;
    SearcherClientByName(LPCSTR name)
    {
        strncpy_s(player_name, name, sizeof(player_name) - 1);
        xr_strlwr(player_name);
        player_name[xr_strlen(name)] = 0;
    }
    bool operator()(IClient* client)
    {
        xrClientData* temp_client = smart_cast<xrClientData*>(client);

        if (!xr_strcmp(player_name, temp_client->ps->getName()))
        {
            return true;
        }
        return false;
    }
};

void game_sv_mp::OnVoteStart(LPCSTR VoteCommand, ClientID sender)
{
    if (!IsVotingEnabled())
        return;
    char CommandName[256];
    CommandName[0] = 0;
    char CommandParams[256];
    CommandParams[0] = 0;
    string1024 resVoteCommand = "";

    sscanf(VoteCommand, "%255s ", CommandName);
    u32 tmp_command_len = xr_strlen(CommandName) + 1; // + ' '
    if ((tmp_command_len < 256) && (tmp_command_len < xr_strlen(VoteCommand)))
    {
        strncpy_s(CommandParams, VoteCommand + xr_strlen(CommandName) + 1, 255);
        CommandParams[255] = 0;
    }

    int i = 0;
    m_bVotingReal = false;
    while (votecommands[i].command)
    {
        if (!xr_stricmp(votecommands[i].name, CommandName))
        {
            m_bVotingReal = true;
            if (!IsVotingEnabled(votecommands[i].flag))
                return;
            break;
        };
        i++;
    };
    if (!m_bVotingReal && CommandName[0] != '$')
    {
        Msg("Unknown Vote Command - %s", CommandName);
        return;
    };

    //-----------------------------------------------------------------------------
    SetVotingActive(true);
    u32 CurTime = Level().timeServer();
    m_uVoteStartTime = CurTime;
    if (m_bVotingReal)
    {
        if (!xr_stricmp(votecommands[i].name, "changeweather"))
        {
            string256 WeatherTime = "", WeatherName = "";
            sscanf(CommandParams, "%255s %255s", WeatherName, WeatherTime);

            m_pVoteCommand.printf("%s %s", votecommands[i].command, WeatherTime);
            xr_sprintf(resVoteCommand, "%s %s", votecommands[i].name, WeatherName);
        }
        else if (!xr_stricmp(votecommands[i].name, "changemap"))
        {
            string256 LevelName;
            string256 LevelVersion;
            sscanf_s(CommandParams, "%255s %255s", LevelName, sizeof(LevelName), LevelVersion, sizeof(LevelVersion));
#ifdef DEBUG
            Msg("--- Starting vote for changing level to: %s[%s]", LevelName, LevelVersion);
#endif // #ifdef DEBUG
            LevelName[255] = 0;
            LevelVersion[255] = 0;

            LPCSTR sv_vote_command = NULL;
            STRCONCAT(sv_vote_command, votecommands[i].command, " ", LevelName, " ", LevelVersion);
            m_pVoteCommand = sv_vote_command;
            xr_sprintf(resVoteCommand, "%s %s [%s]", votecommands[i].name, LevelName, LevelVersion);
        }
        else if (!xr_stricmp(votecommands[i].name, "kick"))
        {
            SearcherClientByName tmp_predicate(CommandParams);
            IClient* tmp_client = m_server->FindClient(tmp_predicate);
            if (tmp_client)
            {
                m_pVoteCommand.printf("sv_kick_id %u", tmp_client->ID.value());
            }
            else
            {
                m_pVoteCommand.printf("%s %s", votecommands[i].command, CommandParams); // backward compatibility
            }
            xr_strcpy(resVoteCommand, VoteCommand);
        }
        else if (!xr_stricmp(votecommands[i].name, "ban"))
        {
            string256 tmp_victim_name;
            s32 ban_time = ExcludeBanTimeFromVoteStr(CommandParams, tmp_victim_name, sizeof(tmp_victim_name));
            // if (ban_time)
            //{
            SearcherClientByName tmp_predicate(tmp_victim_name);
            IClient* tmp_client = m_server->FindClient(tmp_predicate);
            if (tmp_client)
            {
                m_pVoteCommand.printf("sv_banplayer %u %d", tmp_client->ID.value(), ban_time);
            }
            else
            {
                Msg("! ERROR: can't find player with name %s", tmp_victim_name);
            }
            //} else
            //{
            //	Msg("! ERROR: failed to extract ban time from vote string.");
            //}
            xr_strcpy(resVoteCommand, VoteCommand);
        }
        else
        {
            m_pVoteCommand.printf("%s %s", votecommands[i].command, CommandParams);
            xr_strcpy(resVoteCommand, VoteCommand);
        }
    }
    else
    {
        m_pVoteCommand.printf("%s", VoteCommand + 1);
    };

    struct vote_status_setter
    {
        ClientID senderID;
        xrClientData* pStartedPlayer;
        void operator()(IClient* client)
        {
            xrClientData* tmp_client = static_cast<xrClientData*>(client);
            if (!tmp_client->ps)
                return;
            if (tmp_client->ID == senderID)
            {
                tmp_client->ps->m_bCurrentVoteAgreed = 1;
                pStartedPlayer = tmp_client;
            }
            else
            {
                tmp_client->ps->m_bCurrentVoteAgreed = 2;
            }
        }
    };
    vote_status_setter tmp_functor;
    tmp_functor.senderID = sender;
    tmp_functor.pStartedPlayer = NULL;
    m_server->ForEachClientDo(tmp_functor);

    signal_Syncronize();
    //-----------------------------------------------------------------------------
    NET_Packet P;
    GenerateGameMessage(P);
    P.w_u32(GAME_EVENT_VOTE_START);
    if (m_bVotingReal)
    {
        m_voting_string = resVoteCommand;
    }
    else
    {
        m_voting_string = VoteCommand + 1;
    }
    P.w_stringZ(m_voting_string);
    m_started_player = tmp_functor.pStartedPlayer ? tmp_functor.pStartedPlayer->ps->getName() : "";
    P.w_stringZ(m_started_player);
    P.w_u32(u32(g_sv_mp_fVoteTime * 60000));
    u_EventSend(P);
    //-----------------------------------------------------------------------------
};

void game_sv_mp::SendActiveVotingTo(ClientID const& receiver)
{
    NET_Packet P;
    GenerateGameMessage(P);
    P.w_u32(GAME_EVENT_VOTE_START);
    P.w_stringZ(m_voting_string);
    P.w_stringZ(m_started_player);
    u32 CurTime = Level().timeServer();
    u32 EndVoteTime = m_uVoteStartTime + u32(g_sv_mp_fVoteTime * 60000);
    if (EndVoteTime <= CurTime)
        return;
    P.w_u32(EndVoteTime - CurTime);
    m_server->SendTo(receiver, P, net_flags(TRUE, TRUE));
}

void game_sv_mp::UpdateVote()
{
    if (!IsVotingEnabled() || !IsVotingActive())
        return;

    struct vote_updator
    {
        u32 NumAgreed;
        u32 NumParticipated;
        u32 NumToCount;
        void operator()(IClient* client)
        {
            xrClientData* tmp_client = static_cast<xrClientData*>(client);
            game_PlayerState* ps = tmp_client->ps;
            if (!ps || !tmp_client->net_Ready || ps->IsSkip())
                return;
            if ((ps->m_bCurrentVoteAgreed != 2) && (ps->m_bCurrentVoteAgreed != 1))
                ++NumParticipated;
            if (ps->m_bCurrentVoteAgreed == 1)
                ++NumAgreed;
            ++NumToCount;
        }
    };
    vote_updator vote_stats;
    vote_stats.NumAgreed = 0;
    vote_stats.NumParticipated = 0;
    vote_stats.NumToCount = 0;
    m_server->ForEachClientDo(vote_stats);

    u32 NumAgainst = (vote_stats.NumToCount - vote_stats.NumAgreed);

    bool VoteSucceed = false;
    u32 CurTime = Level().timeServer();

    if (m_uVoteStartTime + u32(g_sv_mp_fVoteTime * 60000) > CurTime)
    {
        if (vote_stats.NumAgreed > (NumAgainst + vote_stats.NumParticipated))
        {
            VoteSucceed = true;
        }
        else
        {
            return;
        }
    }
    else
    {
        if (g_sv_mp_bCountParticipants)
            VoteSucceed =
                (float(vote_stats.NumAgreed) / float(vote_stats.NumParticipated + NumAgainst)) >= g_sv_mp_fVoteQuota;
        else
            VoteSucceed = (float(vote_stats.NumAgreed) / float(vote_stats.NumToCount)) >= g_sv_mp_fVoteQuota;
    };

    SetVotingActive(false);

    if (!VoteSucceed)
    {
        NET_Packet P;
        GenerateGameMessage(P);
        P.w_u32(GAME_EVENT_VOTE_END);
        P.w_stringZ("st_mp_voting_failed");
        u_EventSend(P);
        return;
    };

    NET_Packet P;
    GenerateGameMessage(P);
    P.w_u32(GAME_EVENT_VOTE_END);
    P.w_stringZ("st_mp_voting_succeed");
    u_EventSend(P);

    if (m_bVotingReal && m_pVoteCommand.size())
    {
        Console->Execute(m_pVoteCommand.c_str());
    }
};

void game_sv_mp::OnVoteYes(ClientID sender)
{
    game_PlayerState* ps = get_id(sender);
    if (!ps)
        return;
    ps->m_bCurrentVoteAgreed = 1;
    signal_Syncronize();
};

void game_sv_mp::OnVoteNo(ClientID sender)
{
    game_PlayerState* ps = get_id(sender);
    if (!ps)
        return;
    ps->m_bCurrentVoteAgreed = 0;
    signal_Syncronize();
};

void game_sv_mp::OnVoteStop()
{
    if (!IsVotingActive())
        return;
    SetVotingActive(false);
    //-----------------------------------------------------------------
    NET_Packet P;
    GenerateGameMessage(P);
    P.w_u32(GAME_EVENT_VOTE_STOP);
    u_EventSend(P);
    //-----------------------------------------------------------------
    signal_Syncronize();
};

void game_sv_mp::OnPlayerEnteredGame(ClientID id_who)
{
    xrClientData* xrCData = m_server->ID_to_client(id_who);
    if (!xrCData)
        return;

    NET_Packet P;
    GenerateGameMessage(P);
    P.w_u32(GAME_EVENT_PLAYER_ENTERED_GAME);
    VERIFY(xrCData->ps);
    P.w_stringZ(xrCData->ps->getName());
    u_EventSend(P);
};

void game_sv_mp::ClearPlayerItems(game_PlayerState* ps)
{
    ps->pItemList.clear();
    ps->LastBuyAcount = 0;
    //	ps->m_bClearRun = false;
};

void game_sv_mp::SetPlayersDefItems(game_PlayerState* ps)
{
    ClearPlayerItems(ps);
    if (ps->team < 0)
        return;
    //-------------------------------------------
    // fill player with default items
    if (ps->team < s16(TeamList.size()))
    {
        DEF_ITEMS_LIST aDefItems = TeamList[ps->team].aDefaultItems;

        for (u16 i = 0; i < aDefItems.size(); i++)
        {
            ps->pItemList.push_back(aDefItems[i]);
        }
    };
    //---------------------------------------------------
    string16 RankStr;
    string256 ItemStr;
    string256 NewItemStr;
    char tmp[5];
    for (int i = 1; i <= ps->rank; i++)
    {
        strconcat(sizeof(RankStr), RankStr, "rank_", xr_itoa(i, tmp, 10));
        if (!pSettings->section_exist(RankStr))
            continue;
        for (u32 it = 0; it < ps->pItemList.size(); it++)
        {
            u16* pItemID = &(ps->pItemList[it]);
            //			WeaponDataStruct* pWpnS = NULL;
            //			if (!GetTeamItem_ByID(&pWpnS, &(TeamList[ps->team].aWeapons), *pItemID)) continue;
            if (m_strWeaponsData->GetItemsCount() <= *pItemID)
                continue;
            shared_str WeaponName = m_strWeaponsData->GetItemName((*pItemID) & 0x00FF);
            //			strconcat(ItemStr, "def_item_repl_", pWpnS->WeaponName.c_str());
            strconcat(sizeof(ItemStr), ItemStr, "def_item_repl_", *WeaponName);
            if (!pSettings->line_exist(RankStr, ItemStr))
                continue;

            xr_strcpy(NewItemStr, sizeof(NewItemStr), pSettings->r_string(RankStr, ItemStr));
            //			if (!GetTeamItem_ByName(&pWpnS, &(TeamList[ps->team].aWeapons), NewItemStr)) continue;
            if (m_strWeaponsData->GetItemIdx(NewItemStr) == u32(-1))
                continue;

            //			*pItemID = pWpnS->SlotItem_ID;
            *pItemID = u16(m_strWeaponsData->GetItemIdx(NewItemStr) & 0xffff);
        }
    }
    //---------------------------------------------------
    for (u32 it = 0; it < ps->pItemList.size(); it++)
    {
        u16* pItemID = &(ps->pItemList[it]);
        //		WeaponDataStruct* pWpnS = NULL;
        //		if (!GetTeamItem_ByID(&pWpnS, &(TeamList[ps->team].aWeapons), *pItemID)) continue;
        if (m_strWeaponsData->GetItemsCount() <= *pItemID)
            continue;

        shared_str WeaponName = m_strWeaponsData->GetItemName((*pItemID) & 0x00FF);
        if (!xr_strcmp(*WeaponName, "mp_wpn_knife"))
            continue;
        u16 AmmoID = u16(-1);
        if (pSettings->line_exist(WeaponName, "ammo_class"))
        {
            string1024 wpnAmmos, BaseAmmoName;
            xr_strcpy(wpnAmmos, pSettings->r_string(WeaponName, "ammo_class"));
            _GetItem(wpnAmmos, 0, BaseAmmoName);
            AmmoID = u16(m_strWeaponsData->GetItemIdx(BaseAmmoName) & 0xffff);
        };
        //		if (!pWpnS->WeaponBaseAmmo.size()) continue;
        //		WeaponDataStruct* pWpnAmmo = NULL;
        //		if (!GetTeamItem_ByName(&pWpnAmmo, &(TeamList[ps->team].aWeapons), *(pWpnS->WeaponBaseAmmo))) continue;
        if (AmmoID == u16(-1))
            continue;

        //		ps->pItemList.push_back(pWpnAmmo->SlotItem_ID);
        //		ps->pItemList.push_back(pWpnAmmo->SlotItem_ID);
        // if (Type() == eGameIDArtefactHunt)
        //{
        ps->pItemList.push_back(AmmoID);
        ps->pItemList.push_back(AmmoID);
        //}
    };
};

void game_sv_mp::ClearPlayerState(game_PlayerState* ps)
{
    if (!ps)
        return;

    ps->m_iRivalKills = 0;
    ps->m_iSelfKills = 0;
    ps->m_iTeamKills = 0;
    ps->m_iDeaths = 0;

    ps->m_iKillsInRowCurr = 0;
    ps->m_iKillsInRowMax = 0;

    ps->lasthitter = 0;
    ps->lasthitweapon = 0;

    ClearPlayerItems(ps);
};

void game_sv_mp::OnPlayerKilled(NET_Packet P)
{
    u16 KilledID = P.r_u16();
    KILL_TYPE KillType = KILL_TYPE(P.r_u8());
    u16 KillerID = P.r_u16();
    u16 WeaponID = P.r_u16();
    SPECIAL_KILL_TYPE SpecialKill = SPECIAL_KILL_TYPE(P.r_u8());

    game_PlayerState* ps_killer = get_eid(KillerID);
    game_PlayerState* ps_killed = get_eid(KilledID);

    if (!ps_killed)
    {
        CEntity* entity = smart_cast<CEntity*>(Level().Objects.net_Find(KilledID));

#ifndef MASTER_GOLD
        Msg("! ERROR:  killed entity is null ! (entitty [%d][%s]), killer id [%d][%s], Frame [%d]", KilledID,
            entity ? entity->cName().c_str() : "unknown", KillerID, ps_killer ? ps_killer->getName() : "unknown",
            Device.dwFrame);
#endif // #ifndef MASTER_GOLD
        return;
    }
#ifdef MP_LOGGING
    Msg("--- Player [%d] killed player [%d], Frame [%d]", KillerID, KilledID, Device.dwFrame);
#endif

    CSE_Abstract* pWeaponA = get_entity_from_eid(WeaponID);

    OnPlayerKillPlayer(ps_killer, ps_killed, KillType, SpecialKill, pWeaponA);
    if (KillType == KT_BLEEDING)
    {
        Game().m_WeaponUsageStatistic->OnBleedKill(ps_killer, ps_killed, WeaponID);
    }
    //---------------------------------------------------
    SendPlayerKilledMessage((ps_killed) ? ps_killed->GameID : KilledID, KillType,
        (ps_killer) ? ps_killer->GameID : KillerID, WeaponID, SpecialKill);
};

void game_sv_mp::OnPlayerHitted(NET_Packet P)
{
    u16 id_hitted = P.r_u16();
    u16 id_hitter = P.r_u16();
    float dHealth = P.r_float() * 100;
    game_PlayerState* PSHitter = get_eid(id_hitter);
    if (!PSHitter)
        return;
    game_PlayerState* PSHitted = get_eid(id_hitted);
    if (!PSHitted)
        return;
    if (PSHitted == PSHitter)
        return;
    if (!CheckTeams() || (PSHitted->team != PSHitter->team))
    {
        Rank_Struct* pCurRank = &(m_aRanks[PSHitter->rank]);
        Player_AddExperience(PSHitter, dHealth * pCurRank->m_aRankDiff_ExpBonus[PSHitted->rank]);
    };
};

void game_sv_mp::SendPlayerKilledMessage(
    u16 KilledID, KILL_TYPE KillType, u16 KillerID, u16 WeaponID, SPECIAL_KILL_TYPE SpecialKill)
{
#ifndef MASTER_GOLD
    Msg("---Server: sending player [%d] killed message...", KillerID);
#endif // #ifndef MASTER_GOLD
    NET_Packet P;
    GenerateGameMessage(P);
    P.w_u32(GAME_EVENT_PLAYER_KILLED);

    P.w_u8(u8(KillType));
    P.w_u16(KilledID);
    P.w_u16(KillerID);
    P.w_u16(WeaponID);
    P.w_u8(u8(SpecialKill));

    struct player_killed_sender
    {
        NET_Packet* P;
        xrServer* server_for_send;
        void operator()(IClient* client)
        {
            xrClientData* tmp_client = static_cast<xrClientData*>(client);
            game_PlayerState* ps = tmp_client->ps;
            if (!ps || !tmp_client->net_Ready)
                return;
            server_for_send->SecureSendTo(tmp_client, *P);
        }
    };

    player_killed_sender sender;
    sender.P = &P;
    sender.server_for_send = m_server;
    m_server->ForEachClientDoSender(sender);
};

void game_sv_mp::OnPlayerSpeechMessage(NET_Packet& P, ClientID sender)
{
    xrClientData* pClient = (xrClientData*)m_server->ID_to_client(sender);

    if (!pClient || !pClient->net_Ready)
        return;
    game_PlayerState* ps = pClient->ps;
    if (!ps)
        return;

    if (pClient->owner)
    {
        NET_Packet NP;
        GenerateGameMessage(NP);
        NP.w_u32(GAME_EVENT_SPEECH_MESSAGE);
        NP.w_u16(ps->GameID);
        NP.w_u8(P.r_u8());
        NP.w_u8(P.r_u8());
        NP.w_u8(P.r_u8());
        real_sender tmp_sender(m_server, &NP, net_flags(TRUE, TRUE, TRUE));
        m_server->ForEachClientDoSender(tmp_sender);
    };
};

void game_sv_mp::OnPlayerGameMenu(NET_Packet& P, ClientID sender)
{
    u8 SubEvent = P.r_u8();
    switch (SubEvent)
    {
    case PLAYER_SELECT_SPECTATOR: { OnPlayerSelectSpectator(P, sender);
    }
    break;
    case PLAYER_CHANGE_TEAM: { OnPlayerSelectTeam(P, sender);
    }
    break;
    case PLAYER_CHANGE_SKIN: { OnPlayerSelectSkin(P, sender);
    }
    break;
    }
}
void game_sv_mp::OnPlayerSelectSpectator(NET_Packet& P, ClientID sender)
{
    xrClientData* pClient = (xrClientData*)m_server->ID_to_client(sender);

    if (!pClient || !pClient->net_Ready)
        return;
    game_PlayerState* ps = pClient->ps;
    if (!ps)
        return;

    KillPlayer(sender, ps->GameID);
    ps->setFlag(GAME_PLAYER_FLAG_SPECTATOR);
    //-------------------------------------------
    if (pClient->owner)
    {
        CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(pClient->owner);
        if (pA)
        {
            AllowDeadBodyRemove(sender, ps->GameID);
            m_CorpseList.push_back(ps->GameID);

            SpawnPlayer(sender, "spectator");
        };
    }
}

void game_sv_mp::LoadRanks()
{
    m_aRanks.clear();
    int NumRanks = 0;
    while (1)
    {
        string256 RankSect;
        xr_sprintf(RankSect, "rank_%d", NumRanks);
        if (!pSettings->section_exist(RankSect))
            break;
        NumRanks++;
    };

    for (int i = 0;; i++)
    {
        string256 RankSect;
        xr_sprintf(RankSect, "rank_%d", i);
        if (!pSettings->section_exist(RankSect))
            break;
        Rank_Struct NewRank;

        NewRank.m_sTitle = pSettings->r_string(RankSect, "rank_name");
        NewRank.m_iBonusMoney = READ_IF_EXISTS(pSettings, r_s32, RankSect, "rank_aquire_money", 0);
        shared_str RDEB_str = pSettings->r_string(RankSect, "rank_diff_exp_bonus");
        int RDEB_Count = _GetItemCount(RDEB_str.c_str());
        for (int r = 0; r < RDEB_Count; r++)
        {
            string16 temp;
            float f = 1.0f;
            if (r <= NumRanks)
                f = float(atof(_GetItem(RDEB_str.c_str(), r, temp)));
            NewRank.m_aRankDiff_ExpBonus.push_back(f);
        };

        shared_str sTerms = pSettings->r_string(RankSect, "rank_exp");
        int TermsCount = _GetItemCount(sTerms.c_str());
        R_ASSERT2((TermsCount != 0 && TermsCount <= MAX_TERMS), "Error Number of Terms for Rank");

        for (int t = 0; t < TermsCount; t++)
        {
            string16 temp;
            NewRank.m_iTerms[t] = atoi(_GetItem(sTerms.c_str(), t, temp));
        }
        m_aRanks.push_back(NewRank);
    };
};

void game_sv_mp::Player_AddExperience(game_PlayerState* ps, float Exp)
{
    if (!ps)
        return;

    ps->experience_New += Exp;

    if (Player_Check_Rank(ps) && Player_RankUp_Allowed())
    {
        Player_Rank_Up(ps);
    }
};

bool game_sv_mp::Player_Check_Rank(game_PlayerState* ps)
{
    if (!ps)
        return false;
    if (ps->rank == m_aRanks.size() - 1)
        return false;
    int NextExp = m_aRanks[ps->rank + 1].m_iTerms[0];
    if ((ps->experience_Real + ps->experience_New) < NextExp)
        return false;
    return true;
}

void game_sv_mp::Player_Rank_Up(game_PlayerState* ps)
{
    if (!ps)
        return;

    if (ps->rank == m_aRanks.size() - 1)
        return;

    ps->rank++;
    Player_AddBonusMoney(ps, m_aRanks[ps->rank].m_iBonusMoney, SKT_NEWRANK);
    Player_ExperienceFin(ps);
};

void game_sv_mp::Player_ExperienceFin(game_PlayerState* ps)
{
    if (!ps)
        return;
    ps->experience_Real += ps->experience_New;
    ps->experience_New = 0;
}

void game_sv_mp::UpdatePlayersMoney()
{
    struct player_money_updator
    {
        xrServer* m_server;
        game_sv_mp* m_owner;

        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            game_PlayerState* ps = l_pC->ps;
            if (!l_pC || !l_pC->net_Ready || !ps)
                return;
            if (!ps->money_added && ps->m_aBonusMoney.empty())
                return;
            //-----------------------------------------------------------
            NET_Packet P;

            m_owner->GenerateGameMessage(P);
            P.w_u32(GAME_EVENT_PLAYERS_MONEY_CHANGED);

            P.w_s32(ps->money_for_round);
            P.w_s32(ps->money_added);
            ps->money_added = 0;
            P.w_u8(u8(ps->m_aBonusMoney.size() & 0xff));
            if (!ps->m_aBonusMoney.empty())
            {
                for (u32 i = 0; i < ps->m_aBonusMoney.size(); i++)
                {
                    Bonus_Money_Struct* pBMS = &(ps->m_aBonusMoney[i]);
                    P.w_s32(pBMS->Money);
                    P.w_u8(u8(pBMS->Reason & 0xff));
                    if (pBMS->Reason == SKT_KIR)
                        P.w_u8(pBMS->Kills);
                };
                ps->m_aBonusMoney.clear();
            };

            m_server->SendTo(l_pC->ID, P);
        }
    };

    player_money_updator tmp_functor;
    tmp_functor.m_server = m_server;
    tmp_functor.m_owner = this;
    m_server->ForEachClientDoSender(tmp_functor);
};
/*
bool	game_sv_mp::GetTeamItem_ByID		(WeaponDataStruct** pRes, TEAM_WPN_LIST* pWpnList, u16 ItemID)
{
    if (!pWpnList) return false;
    TEAM_WPN_LIST_it pWpnI	= std::find(pWpnList->begin(), pWpnList->end(), (ItemID));
    if (pWpnI == pWpnList->end() || !((*pWpnI) == (ItemID))) return false;
    *pRes = &(*pWpnI);
    return true;
};

bool	game_sv_mp::GetTeamItem_ByName		(WeaponDataStruct** pRes,TEAM_WPN_LIST* pWpnList, LPCSTR ItemName)
{
    if (!pWpnList) return false;
    TEAM_WPN_LIST_it pWpnI	= std::find(pWpnList->begin(), pWpnList->end(), ItemName);
    if (pWpnI == pWpnList->end() || !((*pWpnI) == ItemName)) return false;
    *pRes = &(*pWpnI);
    return true;
};
*/
void game_sv_mp::Player_AddBonusMoney(game_PlayerState* ps, s32 MoneyAmount, SPECIAL_KILL_TYPE Reason, u8 Kill)
{
    if (!ps)
        return;
    //-----------------------------
    if (MoneyAmount)
        ps->m_aBonusMoney.push_back(Bonus_Money_Struct(MoneyAmount, u8(Reason & 0xff), Kill));
    //-----------------------------
    Player_AddMoney(ps, MoneyAmount);
    //-----------------------------
    ps->money_added -= MoneyAmount;
}
void game_sv_mp::Player_AddMoney(game_PlayerState* ps, s32 MoneyAmount)
{
    if (!ps)
        return;
    TeamStruct* pTeam = GetTeamData(u8(ps->team));

    s64 TotalMoney = ps->money_for_round;

    TotalMoney += MoneyAmount;
    ps->money_added += MoneyAmount;

    if (TotalMoney < pTeam->m_iM_Min)
        TotalMoney = pTeam->m_iM_Min;
    if (TotalMoney > 1000000)
        TotalMoney = 1000000;

    ps->money_for_round = s32(TotalMoney);
    //---------------------------------------
    Game().m_WeaponUsageStatistic->OnPlayerAddMoney(ps, MoneyAmount);
    //---------------------------------------
};
//---------------------------------------------------------------------
extern u32 g_sv_dwMaxClientPing;
void game_sv_mp::ReadOptions(shared_str& options)
{
    inherited::ReadOptions(options);

    u8 SpectatorModes = SpectatorModes_Pack();
    SpectatorModes = u8(get_option_i(*options, "spectrmds", s32(SpectatorModes)) & 0x00ff);
    SpectatorModes_UnPack(SpectatorModes);

    g_sv_dwMaxClientPing = get_option_i(*options, "maxping", g_sv_dwMaxClientPing);

    string64 StartTime, TimeFactor;
    xr_strcpy(StartTime, get_option_s(*options, "estime", "9:00"));
    xr_strcpy(TimeFactor, get_option_s(*options, "etimef", "1"));

    u32 hours = 0, mins = 0;
    sscanf(StartTime, "%d:%d", &hours, &mins);
    u64 StartEnvGameTime = generate_time(1, 1, 1, hours, mins, 0, 0);
    float EnvTimeFactor = float(atof(TimeFactor)) * GetEnvironmentGameTimeFactor();

    SetEnvironmentGameTimeFactor(StartEnvGameTime, EnvTimeFactor);
    SetGameTimeFactor(StartEnvGameTime, g_fTimeFactor);
};

static bool g_bConsoleCommandsCreated_MP = false;
void game_sv_mp::ConsoleCommands_Create(){};

void game_sv_mp::ConsoleCommands_Clear(){};

void game_sv_mp::RenewAllActorsHealth()
{
    struct actor_health_renewer
    {
        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            VERIFY2(l_pC->ps, make_string("player state of client, ClientID = 0x%08x", l_pC->ID.value()).c_str());
            if (!l_pC || !l_pC->ps)
            {
                return;
            }
            if (l_pC->ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
            {
#ifdef DEBUG
                Msg("--- Actor has dead flag in player state");
#endif // #ifdef DEBUG
                return;
            }
            // this hack (client objects on server) must be deleted !!!
            CActor* pActor = smart_cast<CActor*>(Level().Objects.net_Find(l_pC->ps->GameID));
            if (!pActor) // if player is spectator
                return;

            if (!pActor->g_Alive())
            {
#ifndef MASTER_GOLD
                Msg("! ERROR: dead actor !!!");
#endif // #ifndef MASTER_GOLD
                return;
            }

            VERIFY2(pActor,
                make_string("client object on server of actor GameID = 0x%08x, not found", l_pC->ps->GameID).c_str());
            if (pActor)
            {
                pActor->SetfHealth(pActor->GetMaxHealth());
            }
        }
    };
    actor_health_renewer tmp_functor;
    m_server->ForEachClientDo(tmp_functor);
    signal_Syncronize();
}

void game_sv_mp::DestroyGameItem(CSE_Abstract* entity)
{
    //	R_ASSERT2( entity, "entity not found for destroying" );
    VERIFY2(entity, "entity not found for destroying");
    if (!entity)
    {
        Msg("! ERROR: entity not found for destroying");
        return;
    }

    NET_Packet P;
    P.w_begin(M_EVENT);
    P.w_u32(Device.dwTimeGlobal - 2 * NET_Latency);
    P.w_u16(GE_DESTROY);
    P.w_u16(entity->ID);
    Level().Send(P, net_flags(TRUE, TRUE));
    // m_server->Perform_destroy(entity, net_flags(TRUE,TRUE));
}

void game_sv_mp::RejectGameItem(CSE_Abstract* entity)
{
    //	R_ASSERT2( entity, "entity not found for rejecting" );
    VERIFY2(entity, "entity not found for rejecting");
    if (!entity)
    {
        Msg("! ERROR: entity not found for rejecting");
        return;
    }

    if (smart_cast<CSE_ALifeItemGrenade*>(entity))
    {
        CGrenade* grenade = smart_cast<CGrenade*>(Level().Objects.net_Find(entity->ID));
        if (grenade && grenade->DropGrenade())
            return;
    }

    CSE_Abstract* e_parent = get_entity_from_eid(entity->ID_Parent);

    //	R_ASSERT2( e_parent, make_string( "RejectGameItem: parent not found. entity_id = [%d], parent_id = [%d]",
    // entity->ID, entity->ID_Parent ).c_str() );
    VERIFY2(e_parent, make_string("RejectGameItem: parent not found. entity_id = [%d], parent_id = [%d]", entity->ID,
                          entity->ID_Parent)
                          .c_str());
    if (!e_parent)
    {
        Msg("! ERROR (RejectGameItem): parent not found. entity_id = [%d], parent_id = [%d]", entity->ID,
            entity->ID_Parent);
        return;
    }

    NET_Packet P;
    u_EventGen(P, GE_OWNERSHIP_REJECT, e_parent->ID);
    P.w_u16(entity->ID);
    Level().Send(P, net_flags(TRUE, TRUE));
}

#include "string_table.h"
void game_sv_mp::DumpOnlineStatistic()
{
    xrGameSpyServer* srv = smart_cast<xrGameSpyServer*>(m_server);

    string_path fn;
    FS.update_path(fn, "$logs$", "mp_stats\\");
    xr_strcat(fn, srv->HostName.c_str());
    xr_strcat(fn, "\\online_dump.ltx");

    string64 t_stamp;
    timestamp(t_stamp);

    CInifile ini(fn, FALSE, FALSE, TRUE);
    shared_str current_section = "global";
    string256 str_buff;

    ini.w_string(current_section.c_str(), "dump_time", t_stamp);

    ini.w_u32(current_section.c_str(), "players_total_cnt", m_server->GetClientsCount());

    xr_sprintf(str_buff, "\"%s\"", CStringTable().translate(Level().name().c_str()).c_str());
    ini.w_string(current_section.c_str(), "current_map_name", str_buff);

    xr_sprintf(str_buff, "%s", CStringTable().translate(type_name()).c_str());
    ini.w_string(current_section.c_str(), "game_mode", str_buff);

    auto it = m_pMapRotation_List.begin();
    auto it_e = m_pMapRotation_List.end();
    for (u32 idx = 0; it != it_e; ++it, ++idx)
    {
        string16 num_buf;
        xr_sprintf(num_buf, "%d", idx);
        xr_sprintf(str_buff, "\"%s\"", CStringTable().translate((*it).map_name.c_str()).c_str());
        ini.w_string("map_rotation", num_buf, str_buff);
    }

    struct player_stats_writer
    {
        game_sv_mp* m_owner;
        xrServer* m_server;
        u32 player_index;
        CInifile* ini;

        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);

            if (!l_pC->ps)
                return;

            if (m_server->GetServerClient() == l_pC && GEnv.isDedicatedServer)
                return;

            if (!l_pC->net_Ready)
                return;

            string16 num_buf;
            xr_sprintf(num_buf, "player_%d", player_index);
            ++player_index;

            m_owner->WritePlayerStats(*ini, num_buf, l_pC);
        }
    };
    player_stats_writer tmp_functor;
    tmp_functor.m_owner = this;
    tmp_functor.m_server = m_server;
    tmp_functor.ini = &ini;
    tmp_functor.player_index = 0;
    m_server->ForEachClientDo(tmp_functor);
    WriteGameState(ini, current_section.c_str(), false);
}

void game_sv_mp::WritePlayerStats(CInifile& ini, LPCSTR sect, xrClientData* pCl)
{
    ini.w_string(sect, "player_name", pCl->ps->getName());
    if (pCl->ps->m_account.is_online())
    {
        ini.w_u32(sect, "player_profile_id", pCl->ps->m_account.profile_id());
    }
    ini.w_u32(sect, "player_team", pCl->ps->team);
    ini.w_u32(sect, "kills_rival", pCl->ps->m_iRivalKills);
    ini.w_u32(sect, "kills_self", pCl->ps->m_iSelfKills);
    ini.w_u32(sect, "team_kills", pCl->ps->m_iTeamKills);
    ini.w_u32(sect, "deaths", pCl->ps->m_iDeaths);

    ini.w_string(sect, "player_ip", pCl->m_cAddress.to_string().c_str());
    ini.w_string(sect, "player_unique_digest", pCl->m_cdkey_digest.c_str());
    ini.w_u32(sect, "kills_in_row", pCl->ps->m_iKillsInRowMax);
    ini.w_u32(sect, "rank", pCl->ps->rank);
    ini.w_u32(sect, "artefacts", pCl->ps->af_count);
    ini.w_u32(sect, "ping", pCl->ps->ping);
    ini.w_u32(sect, "money", pCl->ps->money_for_round);
    ini.w_u32(sect, "online_time_sec", (Level().timeServer() - pCl->ps->m_online_time) / 1000);

    if (Game().m_WeaponUsageStatistic->CollectData())
    {
        Player_Statistic& plstats = *(Game().m_WeaponUsageStatistic->FindPlayer(pCl->ps->getName()));
        u32 hs = plstats.m_dwSpecialKills[0];
        u32 bks = plstats.m_dwSpecialKills[1];
        u32 knf = plstats.m_dwSpecialKills[2];
        u32 es = plstats.m_dwSpecialKills[3];

        ini.w_u32(sect, "headshots_kills", hs);
        ini.w_u32(sect, "backstab_kills", bks);
        ini.w_u32(sect, "knife_kills", knf);
        ini.w_u32(sect, "eye_kills", es);
    }
}

void game_sv_mp::WriteGameState(CInifile& ini, LPCSTR sect, bool bRoundResult)
{
    if (!bRoundResult)
        ini.w_u32(sect, "online_time_sec", Device.dwTimeGlobal / 1000);
}

void game_sv_mp::async_statistics_collector::operator()(IClient* client)
{
    async_responses.insert(std::make_pair(client->ID, false));
}

bool game_sv_mp::async_statistics_collector::all_ready() const
{
    for (responses_t::const_iterator i = async_responses.begin(), ie = async_responses.end(); i != ie; ++i)
    {
        if (!i->second)
            return false;
    }
    return true;
}

void game_sv_mp::async_statistics_collector::set_responded(ClientID clientID)
{
    responses_t::iterator tmp_iter = async_responses.find(clientID);
    if (tmp_iter != async_responses.end())
    {
        tmp_iter->second = true;
    }
}
void game_sv_mp::AskAllToUpdateStatistics()
{
    NET_Packet P;
    P.w_begin(M_STATISTIC_UPDATE);
    P.w_u32(m_async_stats_request_time);
    m_server->SendBroadcast(BroadcastCID, P, net_flags(TRUE, TRUE));
}

void game_sv_mp::DumpRoundStatisticsAsync()
{
    if (!g_sv_mp_iDumpStatsPeriod)
        return;

    m_async_stats.async_responses.clear();
    m_server->ForEachClientDo(m_async_stats);
    m_async_stats_request_time = Device.dwTimeGlobal;
    AskAllToUpdateStatistics();
}

bool game_sv_mp::CheckStatisticsReady()
{
    if (!g_sv_mp_iDumpStatsPeriod)
        return true;

    if (!m_async_stats_request_time)
        return true;

    if (m_async_stats.all_ready() || (m_async_stats_request_time + g_sv_dwMaxClientPing) < Device.dwTimeGlobal)
    {
        DumpRoundStatistics();
        FinishToDumpStatistics();
        m_async_stats_request_time = 0;
        return true;
    }
    return false;
}

void game_sv_mp::StartToDumpStatistics()
{
    if (!g_sv_mp_iDumpStatsPeriod)
        return;

    if (xr_strlen(round_statistics_dump_fn))
    {
        StopToDumpStatistics();
    }

    xrGameSpyServer* srv = smart_cast<xrGameSpyServer*>(m_server);
    FS.update_path(round_statistics_dump_fn, "$logs$", "mp_stats\\");
    string64 t_stamp;
    timestamp(t_stamp);
    xr_strcat(round_statistics_dump_fn, srv->HostName.c_str());
    xr_strcat(round_statistics_dump_fn, "\\games\\dmp");
    xr_strcat(round_statistics_dump_fn, t_stamp);
    xr_strcat(round_statistics_dump_fn, ".ltx");
}

void game_sv_mp::StopToDumpStatistics()
{
    if (xr_strlen(round_statistics_dump_fn))
    {
        remove(round_statistics_dump_fn);
    }
    FinishToDumpStatistics();
}

void game_sv_mp::FinishToDumpStatistics() { round_statistics_dump_fn[0] = 0; }
void game_sv_mp::DumpRoundStatistics()
{
    if (!g_sv_mp_iDumpStatsPeriod)
        return;
    if (!xr_strlen(round_statistics_dump_fn))
        return;

    CInifile ini(round_statistics_dump_fn, FALSE, FALSE, TRUE);
    shared_str current_section = "global";
    string256 str_buff;

    ini.w_string(current_section.c_str(), "start_time", m_round_start_time_str);

    string64 str_current_time;
    timestamp(str_current_time);
    ini.w_string(current_section.c_str(), "end_time", str_current_time);

    xr_sprintf(str_buff, "%s", CStringTable().translate(type_name()).c_str());
    ini.w_string(current_section.c_str(), "game_mode", str_buff);

    xr_sprintf(str_buff, "\"%s\"", CStringTable().translate(Level().name().c_str()).c_str());
    ini.w_string(current_section.c_str(), "current_map_name", str_buff);

    xr_sprintf(str_buff, "\"%s\"", Level().name().c_str());
    ini.w_string(current_section.c_str(), "current_map_name_internal", str_buff);

    struct player_stats_writer
    {
        game_sv_mp* m_owner;
        xrServer* m_server;
        u32 player_index;
        CInifile* ini;

        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);

            if (m_server->GetServerClient() == l_pC && GEnv.isDedicatedServer)
                return;
            if (!l_pC->m_cdkey_digest.size())
                return;
            if (!l_pC->ps)
                return;

            string16 num_buf;
            xr_sprintf(num_buf, "player_%d", player_index);
            ++player_index;

            m_owner->WritePlayerStats(*ini, num_buf, l_pC);
        }
    };
    player_stats_writer tmp_functor;
    tmp_functor.m_owner = this;
    tmp_functor.m_server = m_server;
    tmp_functor.ini = &ini;
    tmp_functor.player_index = 0;
    m_server->ForEachClientDo(tmp_functor);

    WriteGameState(ini, current_section.c_str(), true);

    Game().m_WeaponUsageStatistic->SaveDataLtx(ini);
    // Game().m_WeaponUsageStatistic->Clear();
}

void game_sv_mp::DestroyAllPlayerItems(ClientID id_who) // except rukzak
{
    xrClientData* xrCData = m_server->ID_to_client(id_who);

    VERIFY2(xrCData, make_string("client (ClientID = 0x%08x) not found", id_who.value()).c_str());
    VERIFY(xrCData->ps);
    game_PlayerState* ps = xrCData->ps;
#ifndef MASTER_GOLD
    Msg("---Destroying player [%s] items before spawning new bought items.", ps->getName());
#endif // #ifndef MASTER_GOLD

    CActor* pActor = smart_cast<CActor*>(Level().Objects.net_Find(ps->GameID));
    if (!pActor)
        return;

    TIItemContainer::const_iterator iie = pActor->inventory().m_all.end();
    for (TIItemContainer::const_iterator ii = pActor->inventory().m_all.begin(); ii != iie; ++ii)
    {
        //		VERIFY(*ii);
        R_ASSERT2(*ii,
            make_string("PIItem in player`s inventory not found. Destroy all items of actor[%d]", ps->GameID).c_str());

        u16 object_id = (*ii)->object().ID();
        CSE_Abstract* tempEntity = m_server->ID_to_entity(object_id);

        //		R_ASSERT2( tempEntity, make_string("entity not found [%d]. Destroy all items of actor[%d]", object_id,
        // ps->GameID).c_str() );
        VERIFY2(tempEntity,
            make_string("entity not found [%d]. Destroy all items of actor[%d]", object_id, ps->GameID).c_str());
        if (!tempEntity)
        {
            Msg("! ERROR: entity not found [%d]. Destroy all items of actor[%d]", object_id, ps->GameID);
            continue;
        }

        if (smart_cast<CMPPlayersBag*>(*ii))
            continue;

        if (smart_cast<CWeaponKnife*>(*ii))
            continue;

        if (smart_cast<CArtefact*>(*ii))
            continue;

        DestroyGameItem(tempEntity);
    }
}

void game_sv_mp::SvSendChatMessage(LPCSTR str)
{
    NET_Packet P;
    P.w_begin(M_CHAT_MESSAGE);
    P.w_s16(-1);
    P.w_stringZ("ServerAdmin");
    P.w_stringZ(str);
    P.w_s16(0);
    u_EventSend(P);
}

bool game_sv_mp::IsPlayerBanned(char const* hexstr_digest, shared_str& by_who)
{
    if (!hexstr_digest || !xr_strlen(hexstr_digest))
        return false;
    return m_cdkey_ban_list.is_player_banned(hexstr_digest, by_who);
}

IClient* game_sv_mp::BanPlayer(ClientID const& client_id, s32 ban_time_sec, xrClientData* initiator)
{
    if (client_id == m_server->GetServerClient()->ID)
    {
        Msg("! ERROR: can't ban server client.");
        return NULL;
    }
    xrClientData* client_to_ban = static_cast<xrClientData*>(m_server->ID_to_client(client_id));

    if (!client_to_ban)
        return NULL;

    if (client_to_ban->m_admin_rights.m_has_admin_rights)
    {
        Msg("! ERROR: Can't ban player with admin rights");
        return NULL;
    }
    m_cdkey_ban_list.ban_player(client_to_ban, ban_time_sec, initiator);
    return client_to_ban;
}

void game_sv_mp::BanPlayerDirectly(char const* client_hex_digest, s32 ban_time_sec, xrClientData* initiator)
{
    m_cdkey_ban_list.ban_player_ll(client_hex_digest, ban_time_sec, initiator);
}

void game_sv_mp::UnBanPlayer(size_t banned_player_index)
{
    m_cdkey_ban_list.unban_player_by_index(banned_player_index);
}

void game_sv_mp::PrintBanList(char const* filter = NULL) { m_cdkey_ban_list.print_ban_list(filter); }
void game_sv_mp::SetCanOpenBuyMenu(ClientID id)
{
    NET_Packet bm_ready;
    bm_ready.w_begin(M_GAMEMESSAGE);
    bm_ready.w_u32(GAME_EVENT_PLAYER_BUYMENU_CLOSE);
    m_server->SendTo(id, bm_ready, net_flags(TRUE, TRUE));
}

void game_sv_mp::OnPlayerChangeName(NET_Packet& P, ClientID sender)
{
    string1024 received_name = "";
    P.r_stringZ_s(received_name);
    string256 NewName;
    modify_player_name(received_name, NewName);

    xrClientData* pClient = (xrClientData*)m_server->ID_to_client(sender);

    if (!pClient || !pClient->net_Ready)
        return;
    game_PlayerState* ps = pClient->ps;
    if (!ps)
        return;

    xrGameSpyServer* sv = smart_cast<xrGameSpyServer*>(m_server);
    if (sv && sv->IsPublicServer())
    {
        Msg("Player \"%s\" try to change name on \"%s\" at public server.", ps->getName(), NewName);

        NET_Packet P;
        GenerateGameMessage(P);
        P.w_u32(GAME_EVENT_SERVER_STRING_MESSAGE);
        P.w_stringZ("Server is public. Can\'t change player name!");
        m_server->SendTo(sender, P);
        return;
    }

    shared_str old_name = ps->getName();
    pClient->name = NewName;
    ps->m_account.set_player_name(NewName);
    CheckPlayerName(pClient);

    if (pClient->owner)
    {
        NET_Packet P;
        GenerateGameMessage(P);
        P.w_u32(GAME_EVENT_PLAYER_NAME);
        P.w_u16(pClient->owner->ID);
        P.w_s16(ps->team);
        P.w_stringZ(old_name.c_str());
        P.w_stringZ(ps->getName());
        //---------------------------------------------------
        real_sender tmp_functor(m_server, &P);
        m_server->ForEachClientDoSender(tmp_functor);
        //---------------------------------------------------
        pClient->owner->set_name_replace(ps->getName());
    };

    Game().m_WeaponUsageStatistic->ChangePlayerName(old_name.c_str(), ps->getName());

    signal_Syncronize();
};
