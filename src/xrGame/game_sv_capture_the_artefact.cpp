#include "stdafx.h"
#include "game_sv_capture_the_artefact.h"
#include "xrserver_objects_alife_monsters.h"
#include "Level.h"
#include "xrserver.h"
#include "Inventory.h"
#include "CustomZone.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/CameraManager.h"
#include "Actor.h"
#include "Artefact.h"
#include "game_cl_base.h"
#include "xr_level_controller.h"
#include "hudItem.h"
#include "weapon.h"
#include "eatable_item_object.h"
#include "Missile.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "Common/LevelGameDef.h"
#include "clsid_game.h"
#include "ui\UIBuyWndShared.h"
#include "UIGameCTA.h"
#include "string_table.h"
#include "xrEngine/xr_ioconsole.h"
#include "xrNetServer/NET_Messages.h"

//-------------------------------------------------------------
u32 g_sv_cta_dwInvincibleTime = 5; // 5 seconds
// u32			g_sv_cta_dwAnomalySetLengthTime	=		3;	//3 seconds
u32 g_sv_cta_artefactReturningTime = 45; // 45 seconds
u32 g_sv_cta_activatedArtefactRet = 0;
// s32			g_sv_cta_ScoreLimit				=		3;
u32 g_sv_cta_PlayerScoresDelayTime = 3; // 3 seconds
float g_sv_cta_artefactsBaseRadius = 1.0f;
u32 g_sv_cta_rankUpToArtsCountDiv = 1;
//-------------------------------------------------------------
extern BOOL g_sv_dm_bAnomaliesEnabled;
extern u32 g_sv_dm_dwAnomalySetLengthTime;
extern BOOL g_sv_dm_bPDAHunt;
extern BOOL g_sv_dm_bDamageBlockIndicators;
extern u32 g_sv_dm_dwWarmUp_MaxTime;
extern s32 g_sv_dm_dwTimeLimit;
//-------------------------------------------------------------
extern BOOL g_sv_tdm_bAutoTeamBalance;
extern BOOL g_sv_tdm_bAutoTeamSwap;
extern BOOL g_sv_tdm_bFriendlyIndicators;
extern BOOL g_sv_tdm_bFriendlyNames;
extern float g_sv_tdm_fFriendlyFireModifier;
//-------------------------------------------------------------
extern int g_sv_tdm_iTeamKillLimit;
extern int g_sv_tdm_bTeamKillPunishment;
//-------------------------------------------------------
extern int g_sv_ah_iReinforcementTime;
extern BOOL g_sv_ah_bBearerCantSprint;
extern int g_sv_ah_dwArtefactsNum;
extern BOOL g_sv_ah_bBearerCantSprint;
//-------------------------------------------------------
BOOL game_sv_CaptureTheArtefact::isAnomaliesEnabled() { return g_sv_dm_bAnomaliesEnabled; };
BOOL game_sv_CaptureTheArtefact::isPDAHuntEnabled() { return g_sv_dm_bPDAHunt; };
u32 game_sv_CaptureTheArtefact::Get_InvincibilityTime_msec() { return g_sv_cta_dwInvincibleTime * 1000; };
u32 game_sv_CaptureTheArtefact::Get_AnomalySetLengthTime_msec() { return g_sv_dm_dwAnomalySetLengthTime * 60 * 1000; };
u32 game_sv_CaptureTheArtefact::Get_ArtefactReturningTime_msec() { return g_sv_cta_artefactReturningTime * 1000; };
u32 game_sv_CaptureTheArtefact::Get_ActivatedArtefactRet() { return g_sv_cta_activatedArtefactRet; };
u32 game_sv_CaptureTheArtefact::Get_PlayerScoresDelayTime_msec() { return g_sv_cta_PlayerScoresDelayTime * 1000; };
BOOL game_sv_CaptureTheArtefact::isFriendlyFireEnabled() { return (int(g_sv_tdm_fFriendlyFireModifier * 100.0f) > 0); };
float game_sv_CaptureTheArtefact::GetFriendlyFire()
{
    return (int(g_sv_tdm_fFriendlyFireModifier * 100.0f) > 0) ? g_sv_tdm_fFriendlyFireModifier : 0.0f;
};
int game_sv_CaptureTheArtefact::Get_TeamKillLimit() { return g_sv_tdm_iTeamKillLimit; };
BOOL game_sv_CaptureTheArtefact::Get_TeamKillPunishment() { return g_sv_tdm_bTeamKillPunishment; };
BOOL game_sv_CaptureTheArtefact::Get_FriendlyIndicators() { return g_sv_tdm_bFriendlyIndicators; };
BOOL game_sv_CaptureTheArtefact::Get_FriendlyNames() { return g_sv_tdm_bFriendlyNames; };
int game_sv_CaptureTheArtefact::Get_ReinforcementTime_msec()
{
    return g_sv_ah_iReinforcementTime ? g_sv_ah_iReinforcementTime * 1000 : 1000;
};
s32 game_sv_CaptureTheArtefact::Get_ScoreLimit() { return g_sv_ah_dwArtefactsNum; };
BOOL game_sv_CaptureTheArtefact::Get_BearerCanSprint() { return !g_sv_ah_bBearerCantSprint; };
u32 game_sv_CaptureTheArtefact::GetWarmUpTime() { return g_sv_dm_dwWarmUp_MaxTime; };
s32 game_sv_CaptureTheArtefact::GetTimeLimit() { return g_sv_dm_dwTimeLimit; };

game_sv_CaptureTheArtefact::game_sv_CaptureTheArtefact()
    : m_dwLastAnomalyStartTime(0), m_iMoney_for_BuySpawn(0),
      nextReinforcementTime(0), currentTime(0)
{
    m_type = eGameIDCaptureTheArtefact;
    roundStarted = FALSE;
    teams_swaped = false;
    m_bSpectatorMode = false;

    m_dwWarmUp_CurTime = 0;
    m_bInWarmUp = false;

    m_dwSM_SwitchDelta = 0;
    m_dwSM_LastSwitchTime = 0;
    m_dwSM_CurViewEntity = 0;
    m_pSM_CurViewEntity = nullptr;
}

game_sv_CaptureTheArtefact::~game_sv_CaptureTheArtefact() {}
LPCSTR game_sv_CaptureTheArtefact::type_name() const { return "capturetheartefact"; }
void game_sv_CaptureTheArtefact::Update()
{
    inherited::Update();

    switch (Phase())
    {
    case GAME_PHASE_INPROGRESS:
        CheckForArtefactDelivering();
        currentTime = Level().timeServer();

        CheckForWarmap(currentTime);
        ResetTimeoutInvincibility(currentTime);
        CheckAnomalyUpdate(currentTime);
        CheckForArtefactReturning(currentTime);

        if (m_bSpectatorMode)
        {
            SM_CheckViewSwitching();
        }

        if (Get_ReinforcementTime_msec())
        {
            if (nextReinforcementTime <= currentTime)
            {
                RespawnDeadPlayers();
                nextReinforcementTime = currentTime + Get_ReinforcementTime_msec();
            }
        }
        if (CheckForRoundEnd())
        {
            DumpRoundStatisticsAsync();
            switch_Phase(GAME_PHASE_PLAYER_SCORES);
            // nextReinforcementTime will be used for delaying scores phase
            nextReinforcementTime = currentTime + Get_PlayerScoresDelayTime_msec();
            signal_Syncronize();
        }
        break;
    case GAME_PHASE_PENDING:
        CheckStatisticsReady();
        if (!roundStarted &&
            Level().m_bGameConfigStarted) ////in case of starting server stage (net_start 1..6) we can't do restart ....
        {
            if (CheckForAllPlayersReady())
            {
                if (HasMapRotation() && SwitchToNextMap())
                {
                    if (g_sv_tdm_bAutoTeamSwap && teams_swaped)
                    {
                        OnNextMap();
                        break;
                    }
                    else if (!g_sv_tdm_bAutoTeamSwap)
                    {
                        OnNextMap();
                        break;
                    }
                }
                OnRoundStart();
            }
            else if (CheckForRoundStart())
            {
                OnRoundStart();
            }
        }
        break;
    case GAME_PHASE_PLAYER_SCORES:
    {
        currentTime = Level().timeServer();
        if (nextReinforcementTime <= currentTime)
        {
            OnRoundEnd(); // OnRoundEnd("Finish");
        }
    }
    break;
    }
}

void game_sv_CaptureTheArtefact::SM_CheckViewSwitching()
{
    if (!m_pSM_CurViewEntity || !smart_cast<CActor*>(m_pSM_CurViewEntity) ||
        m_dwSM_LastSwitchTime < Level().timeServer())
        SM_SwitchOnNextActivePlayer();

    CUIGameCTA* gameCTA = smart_cast<CUIGameCTA*>(CurrentGameUI());
    if (gameCTA)
    {
        IGameObject* pObject = Level().CurrentViewEntity();
        if (pObject && smart_cast<CActor*>(pObject))
        {
            string1024 Text;
            xr_sprintf(Text, "Following %s", pObject->cName().c_str());

            gameCTA->SetSpectrModeMsgCaption(Text);
        }
        else
            gameCTA->SetSpectrModeMsgCaption("Server works in spectator mode");
    }
}

void game_sv_CaptureTheArtefact::SM_SwitchOnPlayer(IGameObject* pNewObject)
{
    if (!pNewObject)
        return;

    Level().SetEntity(pNewObject);

    if (pNewObject != m_pSM_CurViewEntity)
    {
        CActor* pActor = smart_cast<CActor*>(m_pSM_CurViewEntity);

        if (pActor)
            pActor->inventory().Items_SetCurrentEntityHud(false);
    }

    CActor* pActor = smart_cast<CActor*>(pNewObject);
    if (pActor)
    {
        pActor->inventory().Items_SetCurrentEntityHud(true);
    }
    m_pSM_CurViewEntity = pNewObject;
    m_dwSM_CurViewEntity = pNewObject->ID();
    m_dwSM_LastSwitchTime = Level().timeServer() + m_dwSM_SwitchDelta;
}

void game_sv_CaptureTheArtefact::SM_SwitchOnNextActivePlayer()
{
    struct next_active_player_switcher
    {
        xrClientData* PossiblePlayers[MAX_PLAYERS_COUNT];
        u32 PPlayersCount;
        next_active_player_switcher() { PPlayersCount = 0; }
        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            game_PlayerState* ps = l_pC->ps;
            if (!ps)
                return;
            if (!l_pC->net_Ready)
                return;
            if (ps->IsSkip())
                return;
            if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
                return;
            PossiblePlayers[PPlayersCount++] = l_pC;
        }
    };

    next_active_player_switcher tmp_functor;
    m_server->ForEachClientDo(tmp_functor);

    IGameObject* pNewObject = NULL;
    if (!tmp_functor.PPlayersCount)
    {
        xrClientData* C = (xrClientData*)m_server->GetServerClient();
        pNewObject = Level().Objects.net_Find(C->ps->GameID);
    }
    else
    {
        xrClientData* C = tmp_functor.PossiblePlayers[::Random.randI((int)tmp_functor.PPlayersCount)];

        pNewObject = Level().Objects.net_Find(C->ps->GameID);
        CActor* pActor = smart_cast<CActor*>(pNewObject);

        if (!pActor || !pActor->g_Alive() || !pActor->inventory().ActiveItem())
            return;
    };
    SM_SwitchOnPlayer(pNewObject);
#ifndef MASTER_GOLD
    Msg("---SM Switched on player %s", pNewObject->cName().c_str());
#endif // #ifndef MASTER_GOLD
};

bool game_sv_CaptureTheArtefact::CheckForRoundStart()
{
    if (m_bFastRestart)
    {
        return true;
    };
    return false;
}

void game_sv_CaptureTheArtefact::CheckForWarmap(u32 currentTime)
{
    if (m_dwWarmUp_CurTime == 0 && !m_bInWarmUp)
        return;
    if (m_dwWarmUp_CurTime < currentTime)
    {
        m_dwWarmUp_CurTime = 0;
        m_bInWarmUp = false;
        Console->Execute("g_restart_fast");
    };
}

void game_sv_CaptureTheArtefact::net_Export_State(NET_Packet& P, ClientID id_to)
{
    inherited::net_Export_State(P, id_to);
    VERIFY(TeamList.size() >= 2);
    // this is a bad style, because each update state time there is a 4 unnecessary bytes transmitting
    // but client needs to know Game identifiers of artefact objects
    VERIFY(teams.size() >= 2);

    MyTeam& greenTeam = teams[etGreenTeam];
    MyTeam& blueTeam = teams[etBlueTeam];

    if (greenTeam.artefact && blueTeam.artefact)
    {
        P.w_u16(greenTeam.artefact->ID);
        P.w_u16(blueTeam.artefact->ID);
    }
    else
    {
        P.w_u16(0);
        P.w_u16(0);
    }

    P.w_vec3(greenTeam.artefactRPoint.P);
    P.w_vec3(blueTeam.artefactRPoint.P);

    P.w_s32(Get_ScoreLimit());
    P.w_s32(greenTeam.score);
    P.w_s32(blueTeam.score);

    P.w_u8(u8(Get_FriendlyIndicators()));
    P.w_u8(u8(Get_FriendlyNames()));
    P.w_u8(u8(Get_BearerCanSprint()));
    P.w_u8(u8(Get_ActivatedArtefactRet() != 0));
    P.w_float(g_sv_cta_artefactsBaseRadius);

    P.w_u8(u8(m_bInWarmUp));
    P.w_s16(static_cast<s16>(GetTimeLimit()));
}
void game_sv_CaptureTheArtefact::net_Export_Update(NET_Packet& P, ClientID id_to, ClientID id)
{
    inherited::net_Export_Update(P, id_to, id);
    if (nextReinforcementTime <= currentTime)
    {
        P.w_u32(0);
    }
    else
    {
        P.w_u32(nextReinforcementTime - currentTime);
    }
    P.w_u32(Get_ReinforcementTime_msec());
    P.w_u32(m_dwWarmUp_CurTime);
    // P.w_u8(u8(m_bInWarmUp)); in net_Export_State
}

BOOL game_sv_CaptureTheArtefact::CheckForAllPlayersReady()
{
    if (!m_server->GetServerClient())
        return FALSE;
    // Check if all players ready
    struct ready_checker
    {
        ClientID serverID;
        u32 ready;
        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            game_PlayerState* ps = l_pC->ps;
            if (!ps)
                return;

            if (!l_pC->net_Ready)
            {
                if (l_pC->ID == serverID)
                {
                    return;
                }
                //++ready;	in this case we shoud kick player with voting...
            };
            if (ps->team == etSpectatorsTeam)
            {
                ++ready;
                return;
            }
            if (ps->testFlag(GAME_PLAYER_FLAG_READY) || ps->IsSkip())
                ++ready;
        }
    };
    ready_checker tmp_functor;
    tmp_functor.serverID = m_server->GetServerClient()->ID;
    tmp_functor.ready = 0;
    m_server->ForEachClientDo(tmp_functor);
    u32 cnt = get_players_count();
    if (tmp_functor.ready == cnt && tmp_functor.ready != 0)
        return TRUE;
    return FALSE;
}

void game_sv_CaptureTheArtefact::OnPlayerConnect(ClientID id_who)
{
    inherited::OnPlayerConnect(id_who);

    xrClientData* xrCData = m_server->ID_to_client(id_who);
    game_PlayerState* ps_who = get_id(id_who);

    if (!xrCData->flags.bReconnect)
    {
        ps_who->clear();
        ps_who->team = etSpectatorsTeam;
        ps_who->skin = -1;
    };
    ps_who->setFlag(GAME_PLAYER_FLAG_SPECTATOR);

    ps_who->resetFlag(GAME_PLAYER_FLAG_SKIP);

    if ((GEnv.isDedicatedServer || m_bSpectatorMode) && (xrCData == m_server->GetServerClient()))
    {
        ps_who->setFlag(GAME_PLAYER_FLAG_SKIP);
        return;
    }

    /*if (!xrCData->flags.bReconnect)
        Money_SetStart				(id_who);

    SetPlayersDefItems				(ps_who);*/
}
void game_sv_CaptureTheArtefact::OnPlayerConnectFinished(ClientID id_who)
{
    inherited::OnPlayerConnectFinished(id_who);

    xrClientData* xrCData = m_server->ID_to_client(id_who);
    VERIFY(xrCData && xrCData->ps);

    NET_Packet P;
    GenerateGameMessage(P);
    P.w_u32(GAME_EVENT_PLAYER_CONNECTED);
    P.w_clientID(xrCData->ID);
    xrCData->ps->team = etSpectatorsTeam;
    xrCData->ps->setFlag(GAME_PLAYER_FLAG_SPECTATOR);
    xrCData->ps->m_iTeamKills = 0;
    xrCData->ps->net_Export(P, TRUE);
    u_EventSend(P);

    SetPlayersDefItems(xrCData->ps);
    if (!xrCData->flags.bReconnect)
    {
        Money_SetStart(xrCData->ps);
    }
    SpawnPlayer(id_who, "spectator");

    xrCData->net_Ready = TRUE;
}

void game_sv_CaptureTheArtefact::OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID)
{
    CSE_Abstract* actor = m_server->ID_to_entity(GameID);
    if (!actor)
    {
        Msg("! WARNING: actor [%d] not found, on player disconnect", GameID);
        inherited::OnPlayerDisconnect(id_who, Name, GameID);
        return;
    }

    VERIFY2(actor, make_string("actor not found (GameID = 0x%08x)", GameID).c_str());

    TeamsMap::iterator te = teams.end();
    TeamsMap::iterator artefactOwnerTeam =
        std::find_if(teams.begin(), te, [&](const TeamPair& tp) { return SearchOwnerIdFunctor()(tp, GameID); });
    if (artefactOwnerTeam != te)
    {
        DropArtefact(artefactOwnerTeam->second.artefactOwner, artefactOwnerTeam->second.artefact);
    }

    InvincibilityTimeouts::iterator ti = m_invTimeouts.find(id_who);
    if (ti != m_invTimeouts.end())
        m_invTimeouts.erase(ti);

    inherited::OnPlayerDisconnect(id_who, Name, GameID);
}

void game_sv_CaptureTheArtefact::OnPlayerReady(ClientID id_who)
{
    if (m_phase == GAME_PHASE_PENDING)
    {
        game_PlayerState* ps = get_id(id_who);
        if (ps)
        {
            if (ps->testFlag(GAME_PLAYER_FLAG_READY))
            {
                ps->resetFlag(GAME_PLAYER_FLAG_READY);
            }
            else
            {
                ps->setFlag(GAME_PLAYER_FLAG_READY);
            }
            signal_Syncronize();
        }
    }
    else if (m_phase == GAME_PHASE_INPROGRESS)
    {
        xrClientData* xrCData = (xrClientData*)m_server->ID_to_client(id_who);
        game_PlayerState* ps = get_id(id_who);
        CSE_Abstract* pOwner = xrCData->owner;

        xrClientData* xrSCData = (xrClientData*)m_server->GetServerClient();
        if (xrSCData && xrSCData->ID == id_who && m_bSpectatorMode)
        {
            SM_SwitchOnNextActivePlayer();
            return;
        }

        // it's a fake :(
        if (!ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD) || (Get_ReinforcementTime_msec() == 0) || ps->IsSkip())
        {
            return;
        }
//------------------------------------------------------------

#ifndef MASTER_GOLD
        VERIFY(xrCData->ps);
        Msg("---Respawning player %s - he's ready", xrCData->ps->getName());
#endif // #ifndef MASTER_GOLD
        RespawnPlayer(id_who, false);
        pOwner = xrCData->owner;
        CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(pOwner);
        if (pA)
        {
            TGameIDToBoughtFlag::const_iterator buyer_iter = m_dead_buyers.find(id_who);
            int buy_value = 0;
            if (buyer_iter != m_dead_buyers.end())
            {
                buy_value = buyer_iter->second;
            }
            if (buy_value == 0)
            {
                ClearPlayerItems(ps);
                SetPlayersDefItems(ps);
            }
            SpawnWeaponsForActor(pOwner, ps);
            TeamStruct* pTeam = GetTeamData(u8(ps->team));
            VERIFY(pTeam);
            Player_AddMoney(ps, pTeam->m_iM_ClearRunBonus);
        }
        //-------------------------------
    }
}

void game_sv_CaptureTheArtefact::Create(shared_str& options)
{
    inherited::Create(options);

    R_ASSERT2(rpoints[0].size(), "rpoints for green team players not found");
    R_ASSERT2(rpoints[1].size(), "rpoints for blue team players not found");

    shared_str m_sBaseWeaponCostSection;
    m_sBaseWeaponCostSection._set("capturetheartefact_base_cost");
    // todo: get this value from config
    if (!pSettings->section_exist(m_sBaseWeaponCostSection))
    {
        VERIFY2(0, "No section for base weapon cost for this type of the Game!");
        return;
    };
    m_strWeaponsData->Load(m_sBaseWeaponCostSection);

    LoadTeamData(etGreenTeam, "capturetheartefact_team1");
    LoadTeamData(etBlueTeam, "capturetheartefact_team2");

    LoadArtefactRPoints();
    switch_Phase(GAME_PHASE_PENDING);
    VERIFY(teams.size() >= 2);
    teams[etGreenTeam].score = 0;
    teams[etBlueTeam].score = 0;
#ifndef MASTER_GOLD
    Msg("---Starting new round, scores: [ %d : %d ]", teams[etGreenTeam].score, teams[etBlueTeam].score);
#endif // #ifndef MASTER_GOLD
    m_iMoney_for_BuySpawn = READ_IF_EXISTS(pSettings, r_s32, "capturetheartefact_gamedata", "spawn_cost", -10000);
    m_not_free_ammo_str = READ_IF_EXISTS(pSettings, r_string, "capturetheartefact_gamedata", "not_free_ammo", "");
}

void game_sv_CaptureTheArtefact::OnRoundStart()
{
    m_AnomalyIds.clear(); // important !!!

    m_dwSM_LastSwitchTime = 0;
    m_dwSM_CurViewEntity = 0;
    m_pSM_CurViewEntity = NULL;

    // warmap times
    m_dwWarmUp_CurTime = 0;
    m_bInWarmUp = false;
    if (!m_bFastRestart)
    {
        if (GetWarmUpTime() != 0)
        {
            m_dwWarmUp_CurTime = Level().timeServer() + GetWarmUpTime() * 1000;
            m_bInWarmUp = true;
        }
    }

    bool m_bFastRestartBefore = m_bFastRestart; // fake, because next inherited::OnRoundStart() sets it to false :(
    inherited::OnRoundStart();
    roundStarted = TRUE;
    // Respawn all players and some info

    if ((round_end_reason != eRoundEnd_Force) && (round_end_reason != eRoundEnd_GameRestartedFast))
    {
        if (g_sv_tdm_bAutoTeamSwap)
            SwapTeams();

        if (g_sv_tdm_bAutoTeamBalance)
            BalanceTeams();
    }
    if (round_end_reason == eRoundEnd_GameRestarted)
    {
        teams_swaped = false;
    }

    struct spectator_spawner
    {
        game_sv_CaptureTheArtefact* m_owner;
        void operator()(IClient* client)
        {
            // init
            xrClientData* l_pC = static_cast<xrClientData*>(client);

            if (!l_pC || !l_pC->net_Ready || !l_pC->ps)
                return;
            game_PlayerState* ps = l_pC->ps;
            if (!ps)
                return;

            ps->clear();
            ps->pItemList.clear();
            ps->DeathTime = Device.dwTimeGlobal - 1001;

            m_owner->SetPlayersDefItems(ps);
            m_owner->Money_SetStart(ps);
            m_owner->SpawnPlayer(client->ID, "spectator");
        }
    };
    spectator_spawner tmp_functor;
    tmp_functor.m_owner = this;
    m_server->ForEachClientDoSender(tmp_functor);

    VERIFY(teams.size() >= 2);
    teams[etGreenTeam].score = 0;
    teams[etBlueTeam].score = 0;

    //-------
    // TeamList[etGreenTeam].m_iM_TargetSucceed = 0;
    // TeamList[etBlueTeam].m_iM_TargetSucceed = 0;
    if (m_bFastRestartBefore)
    {
        nextReinforcementTime = Level().timeServer();
    }
    else
    {
        nextReinforcementTime = Level().timeServer() + (Get_ReinforcementTime_msec() / 5);
    }
    ReSpawnArtefacts();
    LoadAnomalySet();
    ReStartRandomAnomaly();
    m_item_respawner.respawn_all_items();
    m_item_respawner.respawn_level_items();
    OnCloseBuyMenuFromAll();
    signal_Syncronize();
}

void game_sv_CaptureTheArtefact::SwapTeams()
{
    struct team_swaper
    {
        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            if (!l_pC || !l_pC->net_Ready || !l_pC->ps)
                return;

            game_PlayerState* ps = l_pC->ps;
            if (ps->team == static_cast<u8>(etGreenTeam))
            {
                ps->team = static_cast<u8>(etBlueTeam);
            }
            else if (ps->team == static_cast<u8>(etBlueTeam))
            {
                ps->team = static_cast<u8>(etGreenTeam);
            }
        }
    };
    team_swaper tmp_functor;
    m_server->ForEachClientDo(tmp_functor);
    teams_swaped = true;
}

void game_sv_CaptureTheArtefact::BalanceTeams()
{
    // calc team count
    s16 MinTeam, MaxTeam;
    u32 NumToMove;
    struct team_counter
    {
        u32 l_teams[2]; // etGreenTeam , etBlueTeam
        team_counter()
        {
            l_teams[0] = 0;
            l_teams[1] = 0;
        }
        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            game_PlayerState* ps = l_pC->ps;
            if (!ps)
                return;
            if (!l_pC->net_Ready)
                return;
            if (ps->IsSkip())
                return;
            if (ps->team == etSpectatorsTeam)
                return;
            R_ASSERT((ps->team == etGreenTeam) || (ps->team == etBlueTeam));
            ++(l_teams[ps->team]);
        };
    };
    team_counter tmp_team_counter;
    m_server->ForEachClientDo(tmp_team_counter);

    if (tmp_team_counter.l_teams[etGreenTeam] == tmp_team_counter.l_teams[etBlueTeam])
        return;

    if (tmp_team_counter.l_teams[etGreenTeam] > tmp_team_counter.l_teams[etBlueTeam])
    {
        MinTeam = etBlueTeam;
        MaxTeam = etGreenTeam;
    }
    else
    {
        MinTeam = etGreenTeam;
        MaxTeam = etBlueTeam;
    };

    NumToMove = (tmp_team_counter.l_teams[MaxTeam] - tmp_team_counter.l_teams[MinTeam]) / 2;

    if (!NumToMove)
        return;
    ///////////////////////////////////////////////////////////////////////
    while (NumToMove)
    {
        ///////// get lowest score player from MaxTeam
        struct lowest_player_searcher
        {
            xrClientData* LowestPlayer;
            s16 LowestScore;
            s16 MaxTeam;
            lowest_player_searcher() : MaxTeam(0)
            {
                LowestPlayer = nullptr;
                LowestScore = 32767;
            }

            void operator()(IClient* client)
            {
                xrClientData* l_pC = static_cast<xrClientData*>(client);
                game_PlayerState* ps = l_pC->ps;
                if (!ps)
                    return;
                if (!l_pC->net_Ready)
                    return;
                if (ps->IsSkip())
                    return;
                if (ps->team != MaxTeam)
                    return;

                if (ps->frags() < LowestScore)
                {
                    LowestScore = ps->frags();
                    LowestPlayer = l_pC;
                }
            }
        };
        lowest_player_searcher tmp_functor;
        tmp_functor.MaxTeam = MaxTeam;
        m_server->ForEachClientDo(tmp_functor);
        R_ASSERT(tmp_functor.LowestPlayer);
        ///////// move player to opposite team
        game_PlayerState* ps = tmp_functor.LowestPlayer->ps;
        ps->team = u8(MinTeam);
        NumToMove--;
    }
}

void game_sv_CaptureTheArtefact::Money_SetStart(game_PlayerState* ps)
{
    if (!ps)
        return;

    ps->money_for_round = 0;

    if (ps->team == etSpectatorsTeam)
        return;

    TeamStruct* pTeamData = GetTeamData(ps->team);
    if (!pTeamData)
        return;

    ps->money_for_round = pTeamData->m_iM_Start;
}

void game_sv_CaptureTheArtefact::OnRoundEnd()
{
    roundStarted = FALSE;
    struct spectator_spawner
    {
        game_sv_CaptureTheArtefact* m_owner;
        xrServer* m_server;
        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            game_PlayerState* ps = l_pC->ps;
            if (!ps)
                return;
            if (ps->IsSkip())
                return;
            if (l_pC->owner && smart_cast<CActor*>(Level().Objects.net_Find(l_pC->owner->ID)))
            {
                m_server->Perform_destroy(l_pC->owner, net_flags(TRUE, TRUE));
            }
            m_owner->SpawnPlayer(l_pC->ID, "spectator");
        };
    };
    spectator_spawner spawner;
    spawner.m_owner = this;
    spawner.m_server = m_server;
    m_server->ForEachClientDoSender(spawner);
    inherited::OnRoundEnd();

    ClearReadyFlagFromAll();
}

void game_sv_CaptureTheArtefact::OnPlayerSelectSkin(NET_Packet& P, ClientID sender)
{
    xrClientData* l_pC = m_server->ID_to_client(sender);
    R_ASSERT2(l_pC, make_string("Client data not found, id = <%d>", sender.value()));
    s8 l_skin;
    P.r_s8(l_skin);
    OnPlayerChangeSkin(l_pC->ID, l_skin);
    signal_Syncronize();
    NET_Packet Px;
    GenerateGameMessage(Px);
    Px.w_u32(GAME_EVENT_PLAYER_GAME_MENU_RESPOND);
    Px.w_u8(PLAYER_CHANGE_SKIN);
    Px.w_s8(l_pC->ps->skin);
    m_server->SendTo(sender, Px, net_flags(TRUE, TRUE));
}
void game_sv_CaptureTheArtefact::OnPlayerSelectTeam(NET_Packet& P, ClientID sender)
{
    xrClientData* l_pC = m_server->ID_to_client(sender);
    R_ASSERT2(l_pC, make_string("Client data not found, id = <%d>", sender.value()));
    s8 prev_team = l_pC->ps->team;
    s8 selectedTeam;
    P.r_s8(selectedTeam);
    OnPlayerChangeTeam(l_pC->ps, selectedTeam);
    signal_Syncronize();
    NET_Packet selTeamRespPacket;
    GenerateGameMessage(selTeamRespPacket);
    selTeamRespPacket.w_u32(GAME_EVENT_PLAYER_GAME_MENU_RESPOND);
    selTeamRespPacket.w_u8(PLAYER_CHANGE_TEAM);
    selTeamRespPacket.w_u8(l_pC->ps->team);
    m_server->SendTo(sender, selTeamRespPacket, net_flags(TRUE, TRUE));
    if (prev_team != selectedTeam)
        KillPlayer(l_pC->ID, l_pC->ps->GameID);
}
void game_sv_CaptureTheArtefact::OnPlayerChangeTeam(game_PlayerState* playerState, s8 team)
{
    VERIFY2(playerState, "game_PlayerState is NULL");
    // search team with minimum players count
    if (team == -1)
    {
        TeamsMap::iterator minTeam = std::min_element(teams.begin(), teams.end(), MinPlayersFunctor());
        // we can solve the problem with team identifiers if we change playerState->team type to ETeam
        playerState->team = static_cast<u8>(minTeam->first);
        minTeam->second.playersCount++;
    }
    else
    {
        playerState->team = team;
    }
    if (playerState->money_for_round == 0)
        Money_SetStart(playerState);
    signal_Syncronize();
}

void game_sv_CaptureTheArtefact::OnPlayerChangeSkin(ClientID id_who, s8 skin)
{
    game_PlayerState* ps_who = get_id(id_who);
    VERIFY2(ps_who, "game_PlayerState object not found");
    ps_who->skin = skin;
    ps_who->resetFlag(GAME_PLAYER_FLAG_SPECTATOR);
    if (skin == -1)
    {
        TeamsMap::iterator tempPlayerTeam = teams.find(static_cast<ETeam>(ps_who->team));
        VERIFY2(tempPlayerTeam != teams.end(), make_string("Player team (%d) not found", ps_who->team));
        TEAM_DATA_LIST::size_type teamIndex = tempPlayerTeam->second.indexOfTeamInList;
        VERIFY2(TeamList.size() > teamIndex, make_string("team index not valid: (%d)", teamIndex));
        s32 maxSkinIndex = static_cast<s32>(TeamList[teamIndex].aSkins.size());
        ps_who->skin = static_cast<u8>(::Random.randI(maxSkinIndex));
    }
    KillPlayer(id_who, ps_who->GameID);
    signal_Syncronize();
}
void game_sv_CaptureTheArtefact::OnPlayerSelectSpectator(NET_Packet& P, ClientID sender)
{
    inherited::OnPlayerSelectSpectator(P, sender);
    xrClientData* pClient = (xrClientData*)m_server->ID_to_client(sender);

    if (!pClient || !pClient->net_Ready)
        return;

    game_PlayerState* ps = pClient->ps;
    if (!ps)
        return;

    ps->team = static_cast<u8>(etSpectatorsTeam);
    signal_Syncronize();
}

void game_sv_CaptureTheArtefact::LoadTeamData(ETeam eteam, const shared_str& caSection)
{
    TeamStruct NewTeam;

    VERIFY2(pSettings->section_exist(caSection), make_string("No %s section found", caSection.c_str()).c_str());

    shared_str m_sBaseWeaponCostSection;
    m_sBaseWeaponCostSection._set("capturetheartefact_base_cost");
    if (!pSettings->section_exist(m_sBaseWeaponCostSection))
    {
        R_ASSERT2(0, "No section for base weapon cost for the Capture The Artefact game !");
        return;
    };
    m_strWeaponsData->Load(m_sBaseWeaponCostSection);

    NewTeam.caSection = caSection;

    // LoadWeaponsForTeam	(caSection, &NewTeam.aWeapons);
    LoadSkinsForTeam(caSection, &NewTeam.aSkins);
    LoadDefItemsForTeam(caSection, /*&NewTeam.aWeapons, */ &NewTeam.aDefaultItems);

    shared_str artefactName;

    VERIFY2(pSettings->line_exist(caSection, "artefact"),
        make_string("Not found \"artefact\" in section %s", caSection.c_str()).c_str());

    artefactName = pSettings->r_string(caSection, "artefact");

    if (pSettings->section_exist(caSection))
    {
        NewTeam.m_iM_Start = GetMoneyAmount(caSection, "money_start");
        NewTeam.m_iM_OnRespawn = GetMoneyAmount(caSection, "money_respawn");
        NewTeam.m_iM_Min = GetMoneyAmount(caSection, "money_min");

        NewTeam.m_iM_KillRival = GetMoneyAmount(caSection, "kill_rival");
        NewTeam.m_iM_KillSelf = GetMoneyAmount(caSection, "kill_self");
        NewTeam.m_iM_KillTeam = GetMoneyAmount(caSection, "kill_team");

        NewTeam.m_iM_TargetRival = GetMoneyAmount(caSection, "target_rival");
        NewTeam.m_iM_TargetTeam = GetMoneyAmount(caSection, "target_team");
        NewTeam.m_iM_TargetSucceed = GetMoneyAmount(caSection, "target_succeed");
        NewTeam.m_iM_TargetSucceedAll = GetMoneyAmount(caSection, "target_succeed_all");
        NewTeam.m_iM_TargetFailed = GetMoneyAmount(caSection, "target_failed");

        NewTeam.m_iM_RoundWin = GetMoneyAmount(caSection, "round_win");
        NewTeam.m_iM_RoundLoose = GetMoneyAmount(caSection, "round_loose");
        NewTeam.m_iM_RoundDraw = GetMoneyAmount(caSection, "round_draw");

        NewTeam.m_iM_RoundWin_Minor = GetMoneyAmount(caSection, "round_win_minor");
        NewTeam.m_iM_RoundLoose_Minor = GetMoneyAmount(caSection, "round_loose_minor");

        NewTeam.m_iM_RivalsWipedOut = GetMoneyAmount(caSection, "rivals_wiped_out");

        NewTeam.m_iM_ClearRunBonus = GetMoneyAmount(caSection, "clear_run_bonus");
        //---------------------------------------------------------------------------
        if (pSettings->line_exist(caSection, "kill_while_invincible"))
            NewTeam.m_fInvinsibleKillModifier = pSettings->r_float(caSection, "kill_while_invincible");
        else
            NewTeam.m_fInvinsibleKillModifier = 0.5f;
    };
    //-------------------------------------------------------------
    TeamList.push_back(NewTeam);
    TEAM_DATA_LIST::size_type newTeamId = TeamList.size() - 1;
    teams.insert(std::make_pair(eteam, MyTeam(newTeamId, 0, caSection, artefactName)));
};

#define CTA_ANOMALY_SET_BASE_NAME "cta_game_anomaly_sets"
#define MAX_ANOMALIES_COUNT 20

bool game_sv_CaptureTheArtefact::LoadAnomaliesItems(LPCSTR ini_set_id, TAnomaliesVector& destination)
{
    VERIFY(ini_set_id);

    CInifile* level_ini_file = Level().pLevel;
    VERIFY2(level_ini_file, "level ini file not initialized");

    if (!level_ini_file->section_exist(CTA_ANOMALY_SET_BASE_NAME))
        return false;

    if (!level_ini_file->line_exist(CTA_ANOMALY_SET_BASE_NAME, ini_set_id))
    {
        Msg("! Warning: \"permanent\" string not found in [%s]", CTA_ANOMALY_SET_BASE_NAME);
        return false;
    }

    LPCSTR anomaly_string = level_ini_file->r_string(CTA_ANOMALY_SET_BASE_NAME, ini_set_id);

    if (!anomaly_string)
        return false;

    u32 items_count = _GetItemCount(anomaly_string);
    if (!items_count)
        return false;

    u32 const str_size = xr_strlen(anomaly_string);
    u32 const buffer_size = (str_size + 1) * sizeof(char);
    PSTR temp_str = static_cast<PSTR>(_alloca(buffer_size));
    for (u32 i = 0; i < items_count; ++i)
    {
        _GetItem(anomaly_string, i, temp_str, buffer_size);
        u16 anomaly_id = GetMinUsedAnomalyID(temp_str);
        if (anomaly_id)
            destination.push_back(std::make_pair(temp_str, anomaly_id));
    }

    if (!destination.size())
        return false;

    return true;
}

void game_sv_CaptureTheArtefact::LoadAnomalySet()
{
    m_AnomaliesPermanent.clear();
    m_AnomalySet.clear();

    CInifile* level_ini_file = Level().pLevel;
    VERIFY2(level_ini_file, "level ini file not initialized");

    string16 set_id_str;
    for (u32 i = 0; i < MAX_ANOMALIES_COUNT; ++i)
    {
        xr_sprintf(set_id_str, "set%d", i);
        if (!level_ini_file->line_exist(CTA_ANOMALY_SET_BASE_NAME, set_id_str))
            continue;

        m_AnomalySet.push_back(std::make_pair(TAnomaliesVector(), u8(0)));

        if (!LoadAnomaliesItems(set_id_str, m_AnomalySet.back().first))
            m_AnomalySet.erase(m_AnomalySet.end() - 1);
    }

    LoadAnomaliesItems("permanent", m_AnomaliesPermanent);
}

void game_sv_CaptureTheArtefact::StopPreviousAnomalies()
{
    typedef TAnomalySet::iterator TAnomIter;
    TAnomIter ie = m_AnomalySet.end();
    for (TAnomIter i = m_AnomalySet.begin(); i != ie; ++i)
    {
        i->second = 0;
    }
}

void game_sv_CaptureTheArtefact::ReStartRandomAnomaly()
{
    typedef TAnomalySet::size_type TAnomSize;
    typedef TAnomalySet::iterator TAnomIter;
    typedef xr_set<TAnomSize> TSAnomsSet;

    if (!m_AnomalySet.size())
        return;

    TSAnomsSet started_set;
    TAnomIter anoms_ie = m_AnomalySet.end();
    TAnomSize pos = 0;
    TAnomSize to_start = 0;

    for (TAnomIter i = m_AnomalySet.begin(); i != anoms_ie; ++i)
    {
        if (i->second)
            started_set.insert(pos);
        ++pos;
    }

    TSAnomsSet::iterator started_ie = started_set.end();
    do
    {
        to_start = ::Random.randI(m_AnomalySet.size());
    } while (started_set.find(to_start) != started_ie);
    VERIFY(m_AnomalySet.size() > to_start);

    StopPreviousAnomalies();
    if (isAnomaliesEnabled())
        m_AnomalySet[to_start].second = 1;

    SendAnomalyStates();
#ifdef DEBUG
    Msg("Anomaly states updated, started set # %d", to_start);
#endif
}

void game_sv_CaptureTheArtefact::AddAnomalyChanges(
    NET_Packet& packet, TAnomaliesVector const& anomalies, CCustomZone::EZoneState state)
{
    typedef TAnomaliesVector::const_iterator TAnomIter;
    TAnomIter ie = anomalies.end();

    for (TAnomIter i = anomalies.begin(); i != ie; ++i)
    {
        NET_Packet temp_packet;
        u_EventGen(temp_packet, GE_ZONE_STATE_CHANGE, i->second);
        temp_packet.w_u8(static_cast<u8>(state));

        VERIFY2((packet.w_tell() + temp_packet.B.count) < NET_PacketSizeLimit, "event packet exceeds size !");
        packet.w_u8(static_cast<u8>(temp_packet.B.count));
        packet.w(&temp_packet.B.data, temp_packet.B.count);
    }
}

void game_sv_CaptureTheArtefact::SendAnomalyStates()
{
    NET_Packet event_pack;
    event_pack.w_begin(M_EVENT_PACK);

    AddAnomalyChanges(event_pack, m_AnomaliesPermanent, CCustomZone::eZoneStateIdle);

    typedef TAnomalySet::iterator TAnomSetIter;
    TAnomSetIter ie = m_AnomalySet.end();

    // this order of for blocks is necessarily, because enabling of zones must be at last
    for (TAnomSetIter i = m_AnomalySet.begin(); i != ie; ++i)
    {
        if (!i->second)
            AddAnomalyChanges(event_pack, i->first, CCustomZone::eZoneStateDisabled);
    }
    for (TAnomSetIter i = m_AnomalySet.begin(); i != ie; ++i)
    {
        if (i->second)
            AddAnomalyChanges(event_pack, i->first, CCustomZone::eZoneStateIdle);
    }
    m_dwLastAnomalyStartTime = Level().timeServer();
    u_EventSend(event_pack);
}

void game_sv_CaptureTheArtefact::CheckAnomalyUpdate(u32 currentTime)
{
    if ((m_dwLastAnomalyStartTime + Get_AnomalySetLengthTime_msec()) <= currentTime)
        ReStartRandomAnomaly();
}

s32 game_sv_CaptureTheArtefact::GetMoneyAmount(const shared_str& caSection, pcstr caMoneyStr)
{
    if (pSettings->line_exist(caSection, caMoneyStr))
        return pSettings->r_s32(caSection, caMoneyStr);
    return 0;
};

void game_sv_CaptureTheArtefact::LoadArtefactRPoints()
{
    string_path fn_game;
    if (FS.exist(fn_game, "$level$", "level.game"))
    {
        IReader* F = FS.r_open(fn_game);
        IReader* O = 0;

        // Load RPoints
        if ((O = F->open_chunk(RPOINT_CHUNK)) != 0)
        {
            for (int id = 0; O->find_chunk(id); ++id)
            {
                RPoint R;
                ETeam team;
                u8 type;
                u16 GameType;

                O->r_fvector3(R.P);
                O->r_fvector3(R.A);
                // in editor there green team has index 1 so we must decrement this value
                team = static_cast<ETeam>(O->r_u8() - 1);
                type = O->r_u8();
                GameType = O->r_u16();
                if (!(GameType & eGameIDCaptureTheArtefact))
                    continue;

                VERIFY2(((team >= 0) && (team < 4)) || (type != rptActorSpawn),
                    "Problem with team indexes. In Editor green team id = 1, blue team id = 2, but in game - 0, 1.");

                // res
                // O->r_u8	();
                switch (type)
                {
                case rptArtefactSpawn:
                {
                    TeamsMap::iterator teamIter = teams.find(team);
                    if (teamIter != teams.end())
                    {
                        teamIter->second.SetArtefactRPoint(R);
                    }
                }
                break;
                };
            }
            O->close();
        }

        FS.r_close(F);
    }
    // verifying initialization of all rpoints
    for (TeamsMap::const_iterator i = teams.begin(); i != teams.end(); i++)
    {
        if (!i->second.rPointInitialized)
        {
            VERIFY2(false, make_string("Not found RPoint for team %d", i->first).c_str());
        }
    }
}

void game_sv_CaptureTheArtefact::LoadSkinsForTeam(const shared_str& caSection, TEAM_SKINS_NAMES* pTeamSkins)
{
    string256 SkinSingleName;
    string4096 Skins;

    // Поле strSectionName должно содержать имя секции
    VERIFY(xr_strcmp(caSection, ""));

    pTeamSkins->clear();

    // Имя поля
    if (!pSettings->line_exist(caSection, "skins"))
        return;

    // Читаем данные этого поля
    xr_strcpy(Skins, pSettings->r_string(caSection, "skins"));
    u32 count = _GetItemCount(Skins);
    // теперь для каждое имя оружия, разделенные запятыми, заносим в массив
    for (u32 i = 0; i < count; ++i)
    {
        _GetItem(Skins, i, SkinSingleName);
        pTeamSkins->push_back(SkinSingleName);
    };
}

void game_sv_CaptureTheArtefact::LoadDefItemsForTeam(const shared_str& caSection, DEF_ITEMS_LIST* pDefItems)
{
    string256 ItemName;
    string4096 DefItems;

    // Поле strSectionName должно содержать имя секции
    VERIFY(xr_strcmp(caSection, ""));

    pDefItems->clear();

    // Имя поля
    if (!pSettings->line_exist(caSection, "default_items"))
        return;

    // Читаем данные этого поля
    xr_strcpy(DefItems, pSettings->r_string(caSection, "default_items"));
    u32 count = _GetItemCount(DefItems);
    // теперь для каждое имя оружия, разделенные запятыми, заносим в массив
    for (u32 i = 0; i < count; ++i)
    {
        _GetItem(DefItems, i, ItemName);
        pDefItems->push_back(u16(m_strWeaponsData->GetItemIdx(ItemName) & 0xffff));
    };
};

void game_sv_CaptureTheArtefact::SpawnWeaponsForActor(CSE_Abstract* pE, game_PlayerState* ps)
{
    CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(pE);
    VERIFY2(pA, "owner not an Actor");
    if (!pA)
        return;

    if (!(ps->team < s16(TeamList.size())))
        return;

    while (ps->pItemList.size())
    {
        u16 ItemID = ps->pItemList.front();
        SpawnWeapon4Actor(
            pA->ID, *m_strWeaponsData->GetItemName(ItemID & 0x00FF), u8((ItemID & 0xFF00) >> 0x08), ps->pItemList);
        // Game().m_WeaponUsageStatistic->OnWeaponBought(ps, *m_strWeaponsData->GetItemName(ItemID& 0x00FF));
        R_ASSERT(ps->pItemList.size());
        ps->pItemList.erase(ps->pItemList.begin());
    }

    Player_AddMoney(ps, ps->LastBuyAcount);
};

KILL_RES game_sv_CaptureTheArtefact::GetKillResult(game_PlayerState* pKiller, game_PlayerState* pVictim)
{
    if (!pKiller || !pVictim)
    {
        return KR_NONE;
    }
    if (pKiller == pVictim)
    {
        return KR_SELF;
    }
    if (pKiller->team == pVictim->team)
    {
        return KR_TEAMMATE;
    }
    return KR_RIVAL;
}

bool game_sv_CaptureTheArtefact::OnKillResult(KILL_RES KillResult, game_PlayerState* pKiller, game_PlayerState* pVictim)
{
    if (!pKiller || !pVictim)
    {
        return false;
    }
    bool res = true;
    TeamStruct* pTeam = GetTeamData(pKiller->team);

    switch (KillResult)
    {
    case KR_NONE: { res = false;
    }
    break;
    case KR_SELF:
    {
        // pKiller->m_iRivalKills		-= 1;
        pKiller->m_iSelfKills++;
        pKiller->m_iKillsInRowMax = 0;
        if (pTeam)
            Player_AddMoney(pKiller, pTeam->m_iM_KillSelf);
        res = false;
    }
    break;
    case KR_RIVAL:
    {
        pKiller->m_iRivalKills++;
        pKiller->m_iKillsInRowCurr++;
        pKiller->m_iKillsInRowMax = _max(pKiller->m_iKillsInRowCurr, pKiller->m_iKillsInRowMax);
        if (pTeam)
        {
            s32 ResMoney = pTeam->m_iM_KillRival;
            if (pKiller->testFlag(GAME_PLAYER_FLAG_INVINCIBLE))
                ResMoney = s32(ResMoney * pTeam->m_fInvinsibleKillModifier);
            Player_AddMoney(pKiller, ResMoney);
        };
        res = true;
    }
    break;
    case KR_TEAMMATE_CRITICAL:
    case KR_TEAMMATE:
    {
        if (pTeam)
        {
            s32 ResMoney = pTeam->m_iM_KillTeam;
            Player_AddMoney(pKiller, ResMoney);
        };
        ++pKiller->m_iTeamKills;
        pKiller->m_iKillsInRowMax = 0;
        // Check for TeamKill
        if (Get_TeamKillPunishment())
        {
            if (pKiller->m_iTeamKills >= Get_TeamKillLimit())
            {
                struct killer_searcher
                {
                    game_PlayerState* pKiller;
                    IClient* ServerClient;
                    bool operator()(IClient* client)
                    {
                        xrClientData* pCL = static_cast<xrClientData*>(client);
                        if (!pCL || pCL == ServerClient)
                            return false;

                        if (!pCL->ps || pCL->ps != pKiller)
                            return false;

                        return true;
                    }
                };
                killer_searcher tmp_predicate;
                tmp_predicate.pKiller = pKiller;
                tmp_predicate.ServerClient = m_server->GetServerClient();
                IClient* tmp_client = m_server->FindClient(tmp_predicate);
                if (tmp_client)
                {
                    LPSTR reason;
                    STRCONCAT(reason, CStringTable().translate("st_kicked_by_server").c_str());
                    m_server->DisconnectClient(tmp_client, reason);
                }
            }
        }
    }
    default: {
    }
    break;
    }
    return res;
}

void game_sv_CaptureTheArtefact::OnPlayerHitted(NET_Packet P)
{
    Set_RankUp_Allowed(true);
    inherited::OnPlayerHitted(P);
    Set_RankUp_Allowed(false);
}

void game_sv_CaptureTheArtefact::OnGiveBonus(KILL_RES KillResult, game_PlayerState* pKiller, game_PlayerState* pVictim,
    KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA)
{
    if (!pKiller)
    {
        return;
    }
    Set_RankUp_Allowed(true);
    switch (KillResult)
    {
    case KR_RIVAL:
    {
        switch (KillType)
        {
        case KT_HIT:
        {
            switch (SpecialKillType)
            {
            case SKT_HEADSHOT:
            {
                Player_AddExperience(pKiller, READ_IF_EXISTS(pSettings, r_float, "mp_bonus_exp", "headshot", 0));
                Player_AddBonusMoney(
                    pKiller, READ_IF_EXISTS(pSettings, r_s32, "mp_bonus_money", "headshot", 0), SKT_HEADSHOT);
            }
            break;
            case SKT_EYESHOT:
            {
                Player_AddExperience(pKiller, READ_IF_EXISTS(pSettings, r_float, "mp_bonus_exp", "eyeshot", 0));
                Player_AddBonusMoney(
                    pKiller, READ_IF_EXISTS(pSettings, r_s32, "mp_bonus_money", "eyeshot", 0), SKT_EYESHOT);
            }
            break;
            case SKT_BACKSTAB:
            {
                Player_AddExperience(pKiller, READ_IF_EXISTS(pSettings, r_float, "mp_bonus_exp", "backstab", 0));
                Player_AddBonusMoney(
                    pKiller, READ_IF_EXISTS(pSettings, r_s32, "mp_bonus_money", "backstab", 0), SKT_BACKSTAB);
            }
            break;
            default:
            {
                if (pWeaponA)
                {
                    switch (pWeaponA->m_tClassID)
                    {
                    case CLSID_OBJECT_W_KNIFE:
                    {
                        Player_AddExperience(
                            pKiller, READ_IF_EXISTS(pSettings, r_float, "mp_bonus_exp", "knife_kill", 0));
                        Player_AddBonusMoney(pKiller,
                            READ_IF_EXISTS(pSettings, r_s32, "mp_bonus_money", "knife_kill", 0), SKT_KNIFEKILL);
                    }
                    break;
                    };
                };
            }
            break;
            };
        }
        break;
        default: {
        }
        break;
        };

        if (pKiller->m_iKillsInRowCurr)
        {
            string64 tmpStr;
            xr_sprintf(tmpStr, "%d_kill_in_row", pKiller->m_iKillsInRowCurr);
            Player_AddBonusMoney(pKiller, READ_IF_EXISTS(pSettings, r_s32, "mp_bonus_money", tmpStr, 0), SKT_KIR,
                u8(pKiller->m_iKillsInRowCurr & 0xff));
        };
    }
    break;
    default: {
    }
    break;
    }
    Set_RankUp_Allowed(false);
}

// copy past :(
void game_sv_CaptureTheArtefact::OnPlayerHitPlayer(u16 id_hitter, u16 id_hitted, NET_Packet& P)
{
    CSE_Abstract* e_hitter = get_entity_from_eid(id_hitter);
    CSE_Abstract* e_hitted = get_entity_from_eid(id_hitted);

    if (!e_hitter || !e_hitted)
        return;
    if (!smart_cast<CSE_ALifeCreatureActor*>(e_hitted))
        return;

    game_PlayerState* ps_hitter = get_eid(id_hitter);
    game_PlayerState* ps_hitted = get_eid(id_hitted);

    if (!ps_hitter || !ps_hitted)
        return;

    SHit HitS;
    HitS.Read_Packet(P);

    HitS.whoID = ps_hitter->GameID;

    OnPlayerHitPlayer_Case(ps_hitter, ps_hitted, &HitS);

    //---------------------------------------
    if (HitS.power > 0)
    {
        ps_hitted->lasthitter = ps_hitter->GameID;
        ps_hitted->lasthitweapon = HitS.weaponID;
    };
    //---------------------------------------
    HitS.Write_Packet(P);
};

void game_sv_CaptureTheArtefact::OnPlayerHitPlayer_Case(
    game_PlayerState* ps_hitter, game_PlayerState* ps_hitted, SHit* pHitS)
{
    // if (pHitS->hit_type != ALife::eHitTypePhysicStrike)
    //{
    if (ps_hitter && ps_hitted)
    {
        if (ps_hitter->team == ps_hitted->team && ps_hitter != ps_hitted)
        {
            pHitS->power *= GetFriendlyFire();
            pHitS->impulse *= (GetFriendlyFire() > 1.0f) ? GetFriendlyFire() : 1.0f;
        }
    }
    if (ps_hitted->testFlag(GAME_PLAYER_FLAG_INVINCIBLE))
    {
        pHitS->power = 0;
        pHitS->impulse = 0;
    }
    //	}
};

void game_sv_CaptureTheArtefact::OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState* ps_killed,
    KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA)
{
    inherited::OnPlayerKillPlayer(ps_killer, ps_killed, KillType, SpecialKillType, pWeaponA);
    ProcessPlayerDeath(ps_killed);
    // ProcessPlayerKill(ps_killer);
    KILL_RES KillRes = GetKillResult(ps_killer, ps_killed);
    bool CanGiveBonus = OnKillResult(KillRes, ps_killer, ps_killed);
    if (CanGiveBonus)
        OnGiveBonus(KillRes, ps_killer, ps_killed, KillType, SpecialKillType, pWeaponA);
    Game().m_WeaponUsageStatistic->OnPlayerKillPlayer(ps_killer, KillType, SpecialKillType);
    signal_Syncronize();
}

void game_sv_CaptureTheArtefact::DropArtefact(
    CSE_ActorMP* aOwner, CSE_ALifeItemArtefact* artefact, Fvector const* dropPosition)
{
    VERIFY2(aOwner, "bad parameter");
    VERIFY2(artefact, "bad parameter");
    NET_Packet P;
    u_EventGen(P, GE_OWNERSHIP_REJECT, aOwner->ID);
    P.w_u16(artefact->ID);
    if (dropPosition)
    {
        P.w_u8(0);
        P.w_vec3(*dropPosition);
    }
    // m_server->SendBroadcast(BroadcastCID, P, net_flags(TRUE, TRUE));*/
    m_server->Process_event_reject(P, m_server->GetServerClient()->ID, 0, aOwner->ID, artefact->ID, true);
}

void game_sv_CaptureTheArtefact::ProcessPlayerDeath(game_PlayerState* playerState)
{
    R_ASSERT(playerState);
    playerState->setFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);
    playerState->resetFlag(GAME_PLAYER_FLAG_READY);
    playerState->m_iDeaths++;
    playerState->m_iKillsInRowCurr = 0;

    TeamStruct* pTeam = GetTeamData(u8(playerState->team));
    VERIFY(pTeam);
    Player_AddMoney(playerState, pTeam->m_iM_ClearRunBonus);
    // here we will set the flag that player not bought items yet...

    xrClientData* l_pC = (xrClientData*)get_client(playerState->GameID);
    if (l_pC)
    {
        TGameIDToBoughtFlag::iterator buyer_iter = m_dead_buyers.find(l_pC->ID);
        if (buyer_iter != m_dead_buyers.end())
            buyer_iter->second = 0;
        else
            m_dead_buyers.insert(std::make_pair(l_pC->ID, 0));
    }
    TeamsMap::iterator te = teams.end();
    TeamsMap::iterator childArtefactTeam = std::find_if(
        teams.begin(), te, [&](const TeamPair& tp) { return SearchOwnerIdFunctor()(tp, playerState->GameID); });
    if (childArtefactTeam != te)
    {
        DropArtefact(childArtefactTeam->second.artefactOwner, childArtefactTeam->second.artefact);
        /*
        VERIFY2(childArtefactTeam->second.artefactOwner, "dead player hasn't an artefact");
        VERIFY2(childArtefactTeam->second.artefact, "trying to reject not existing team artefact");
        NET_Packet				P;
        u_EventGen				(P,GE_OWNERSHIP_REJECT, childArtefactTeam->second.artefactOwner->ID);
        P.w_u16					(childArtefactTeam->second.artefact->ID);
        //m_server->SendBroadcast(BroadcastCID, P, net_flags(TRUE, TRUE));
        m_server->Process_event_reject(P, m_server->GetServerClient()->ID, 0,
            childArtefactTeam->second.artefactOwner->ID,
            childArtefactTeam->second.artefact->ID, true);*/
    }
    Game().m_WeaponUsageStatistic->OnPlayerKilled(playerState);
}

/*void game_sv_CaptureTheArtefact::ProcessPlayerKill(game_PlayerState * playerState)
{
    if (!playerState)
        return;
    playerState->kills++;
}*/

void game_sv_CaptureTheArtefact::ReSpawnArtefacts()
{
    TeamsMap::iterator te = teams.end();
    for (TeamsMap::iterator ti = teams.begin(); ti != te; ti++)
    {
        VERIFY2(!!ti->second.artefactName, make_string("not found artefact class name for team %d", ti->first).c_str());
        CSE_ALifeItemArtefact* tempSvEntity =
            smart_cast<CSE_ALifeItemArtefact*>(spawn_begin(ti->second.artefactName.c_str()));
        tempSvEntity->s_flags.assign(M_SPAWN_OBJECT_LOCAL);
        // MoveArtefactToPoint(tempSvEntity, ti->second.artefactRPoint);
        tempSvEntity->o_Position.set(ti->second.artefactRPoint.P);
        ti->second.artefact =
            smart_cast<CSE_ALifeItemArtefact*>(spawn_end(tempSvEntity, m_server->GetServerClient()->ID));
        VERIFY(ti->second.artefact);
        ti->second.freeArtefactTimeStart = 0;
    }
    signal_Syncronize();
}

// TRUE=allow ownership, FALSE=denied
BOOL game_sv_CaptureTheArtefact::OnTouch(u16 eid_who, u16 eid_target, BOOL bForced)
{
    CSE_ActorMP* e_who = smart_cast<CSE_ActorMP*>(m_server->ID_to_entity(eid_who));

    if (!e_who)
    {
        return TRUE;
    }

    /*VERIFY2(e_who,
        make_string("no actor (id = %id) touches the target (id = %d)").c_str());*/

    xrClientData* xrCData = e_who->owner;
    VERIFY(xrCData);
    game_PlayerState* ps_who = xrCData->ps;
    VERIFY(ps_who);
    // CSE_Abstract *e_what = m_server->ID_to_entity(eid_target);
    /*VERIFY(e_what	); // <- not used because IMHO next code work faster...*/
    TeamsMap::iterator te = teams.end();
    TeamsMap::iterator artefactOfTeam =
        std::find_if(teams.begin(), te, [&](const TeamPair& tp) { return SearchArtefactIdFunctor()(tp, eid_target); });
    if (artefactOfTeam != te)
    {
        CSE_ALifeItemArtefact* tempArtefact = artefactOfTeam->second.artefact;
        RPoint tempRPoint = artefactOfTeam->second.artefactRPoint;
        VERIFY(tempArtefact);
        // searching this owner for determinig if the actor already has an artefact
        /*if (std::find_if(
                teams.begin(),
                te,
                std::bind2nd(SearchOwnerIdFunctor(), e_who->ID)) != te)
        {
            return FALSE;
        }*/

        // if it is myne artefact
        if (artefactOfTeam->first == ps_who->team)
        {
            /*Msg("server artefact position (%2.04f,%2.04f,%2.04f), rpoint position (%2.04f,%2.04f,%2.04f)",
                tempArtefact->o_Position.x, tempArtefact->o_Position.y, tempArtefact->o_Position.z,
                tempRPoint.P.x, tempRPoint.P.y, tempRPoint.P.z);
            Msg("server actor position: (%2.04f, %2.04f, %2.04f)",
                e_who->o_Position.x, e_who->o_Position.y, e_who->o_Position.z);*/
            if (!tempArtefact->o_Position.similar(tempRPoint.P, g_sv_cta_artefactsBaseRadius))
            {
                if (!Get_ActivatedArtefactRet())
                {
                    MoveArtefactToPoint(tempArtefact, tempRPoint);
                    // add to actor some money and exp
                    VERIFY(TeamList.size() >= artefactOfTeam->second.indexOfTeamInList);
                    Player_AddMoney(ps_who, TeamList[artefactOfTeam->second.indexOfTeamInList].m_iM_KillRival);

                    /*Set_RankUp_Allowed(true);
                    Player_AddExperience(ps_who, READ_IF_EXISTS(pSettings, r_float, "mp_bonus_exp","target_succeed",0));
                    Set_RankUp_Allowed(false);*/

                    NET_Packet P;
                    GenerateGameMessage(P);
                    P.w_u32(GAME_EVENT_ARTEFACT_TAKEN);
                    P.w_u8(static_cast<u8>(artefactOfTeam->first));
                    P.w_clientID(xrCData->ID);
                    u_EventSend(P);
                    return FALSE;
                }
                if (std::find_if(teams.begin(), te,
                        [&](const TeamPair& tp) { return SearchOwnerIdFunctor()(tp, e_who->ID); }) != te)
                {
                    return FALSE;
                }
                artefactOfTeam->second.OnPlayerAttachArtefact(e_who);
                return TRUE;
            }
            return FALSE;
        }
        else
        {
            if (std::find_if(
                    teams.begin(), te, [&](const TeamPair& tp) { return SearchOwnerIdFunctor()(tp, e_who->ID); }) != te)
            {
                return FALSE;
            }
            artefactOfTeam->second.OnPlayerAttachArtefact(e_who);
            NET_Packet P;
            GenerateGameMessage(P);
            P.w_u32(GAME_EVENT_ARTEFACT_TAKEN);
            P.w_u8(static_cast<u8>(artefactOfTeam->first));
            P.w_clientID(xrCData->ID);
            u_EventSend(P);

            return TRUE;
        }
    }

    CSE_Abstract* e_entity = m_server->ID_to_entity(eid_target);
    return OnTouchItem(e_who, e_entity);
}

BOOL game_sv_CaptureTheArtefact::OnTouchItem(CSE_ActorMP* actor, CSE_Abstract* item)
{
    VERIFY(actor);
    VERIFY(item);

    if ((item->m_tClassID == CLSID_OBJECT_PLAYERS_BAG) && (item->ID_Parent == 0xffff))
    {
        //-------------------------------
        // move all items from rukzak to player
        if (!item->children.empty())
        {
            NET_Packet EventPack;
            NET_Packet PacketReject;
            NET_Packet PacketTake;

            EventPack.w_begin(M_EVENT_PACK);

            while (!item->children.empty())
            {
                CSE_Abstract* e_child_item = get_entity_from_eid(item->children.back());
                if (e_child_item)
                {
                    if (!OnTouch(actor->ID, e_child_item->ID, FALSE))
                    {
                        NET_Packet P;
                        u_EventGen(P, GE_OWNERSHIP_REJECT, item->ID);
                        P.w_u16(e_child_item->ID);

                        m_server->Process_event_reject(
                            P, m_server->GetServerClient()->ID, 0, item->ID, e_child_item->ID);
                        continue;
                    }
                }

                m_server->Perform_transfer(PacketReject, PacketTake, e_child_item, item, actor);

                EventPack.w_u8(u8(PacketReject.B.count));
                EventPack.w(&PacketReject.B.data, PacketReject.B.count);
                EventPack.w_u8(u8(PacketTake.B.count));
                EventPack.w(&PacketTake.B.data, PacketTake.B.count);
            }
            if (EventPack.B.count > 2)
                u_EventSend(EventPack);
        }
        //-------------------------------
        // destroy the BAG
        DestroyGameItem(item);
        if (isPDAHuntEnabled() && actor->owner && actor->owner->ps)
        {
            Player_AddBonusMoney(
                actor->owner->ps, READ_IF_EXISTS(pSettings, r_s32, "mp_bonus_money", "pda_taken", 0), SKT_PDA);
        };

        //-------------------------------
        return FALSE;
    };
    //---------------------------------------------------------------
    return TRUE;
}

void game_sv_CaptureTheArtefact::OnDetach(u16 eid_who, u16 eid_target)
{
    TeamsMap::iterator te = teams.end();
    TeamsMap::iterator artefactOfTeam =
        std::find_if(teams.begin(), te, [&](const TeamPair& tp) { return SearchArtefactIdFunctor()(tp, eid_target); });

    CSE_ActorMP* e_who = smart_cast<CSE_ActorMP*>(m_server->ID_to_entity(eid_who));
    CSE_Abstract* e_item = m_server->ID_to_entity(eid_target);

    if (!e_who)
    {
        return;
    }
    /*VERIFY2(e_who,
        make_string("failed to get actor entity (id = %d)",eid_who).c_str());*/

    VERIFY2(e_item, make_string("failed to get item entity (id = %d)", eid_target).c_str());

    if (artefactOfTeam != te)
    {
        xrClientData* xrCData = e_who->owner;
        VERIFY(xrCData);

        NET_Packet P;
        GenerateGameMessage(P);
        P.w_u32(GAME_EVENT_ARTEFACT_DROPPED);
        P.w_u8(static_cast<u8>(artefactOfTeam->first));
        P.w_clientID(xrCData->ID);
        u_EventSend(P);

        artefactOfTeam->second.OnPlayerDetachArtefact(e_who);
    }
    OnDetachItem(e_who, e_item);
}

BOOL game_sv_CaptureTheArtefact::OnActivate(u16 eid_who, u16 eid_target)
{
    TeamsMap::iterator te = teams.end();
    TeamsMap::iterator artefactOfTeam =
        std::find_if(teams.begin(), te, [&](const TeamPair& tp) { return SearchArtefactIdFunctor()(tp, eid_target); });

    CSE_ActorMP* e_who = smart_cast<CSE_ActorMP*>(m_server->ID_to_entity(eid_who));
    CSE_Abstract* e_item = m_server->ID_to_entity(eid_target);

    VERIFY2(e_who, make_string("failed to get actor entity (id = %d)", eid_who).c_str());

    VERIFY2(e_item, make_string("failed to get item entity (id = %d)", eid_target).c_str());

    xrClientData* xrCData = e_who->owner;
    VERIFY(xrCData);
    game_PlayerState* ps_who = xrCData->ps;
    VERIFY(ps_who);

    if (artefactOfTeam != te)
    {
        if (ps_who->team == artefactOfTeam->first)
        {
            artefactOfTeam->second.OnPlayerActivateArtefact(eid_who);
            return TRUE;
        }
    }
    return FALSE;
}

void game_sv_CaptureTheArtefact::FillDeathActorRejectItems(CSE_ActorMP* actor, xr_vector<CSE_Abstract*>& to_reject)
{
    R_ASSERT(actor);

    CActor* pActor = smart_cast<CActor*>(Level().Objects.net_Find(actor->ID));
    R_ASSERT(pActor);

    u16 active_slot = pActor->inventory().GetActiveSlot();
    if (active_slot == KNIFE_SLOT)
        active_slot = NO_ACTIVE_SLOT;

    if (active_slot != NO_ACTIVE_SLOT)
    {
        PIItem item = pActor->inventory().ItemFromSlot(active_slot);
        if (!item)
        {
#ifndef MASTER_GOLD
            Msg("! ERROR: item from slot %d is NULL", active_slot);
#endif // #ifndef MASTER_GOLD
            return;
        }

        CSE_Abstract* server_item = m_server->ID_to_entity(item->object_id());
        if (!server_item)
        {
#ifndef MASTER_GOLD
            Msg("! ERROR: server entity is NULL, object ID[%d]", item->object_id());
#endif // #ifndef MASTER_GOLD
            return;
        }

        if (smart_cast<CSE_ALifeItemArtefact*>(server_item))
        {
            return;
        }
// R_ASSERT		(server_item);
#ifdef MP_LOGGING
        Msg("--- SV: to_reject [%d]", server_item->ID);
#endif
        to_reject.push_back(server_item);
    }
}

void game_sv_CaptureTheArtefact::OnDetachItem(CSE_ActorMP* actor, CSE_Abstract* item)
{
    R_ASSERT(actor);
    R_ASSERT(item);
    if (item->m_tClassID == CLSID_OBJECT_PLAYERS_BAG)
    {
        // move all items from player to rukzak
        xr_vector<u16>::const_iterator it_e = actor->children.end();

        xr_vector<CSE_Abstract*> to_transfer;
        xr_vector<CSE_Abstract*> to_destroy;
        xr_vector<CSE_Abstract*> to_reject;
        // may be there is a sense to move next invokation into the ProcessDeath method...
        FillDeathActorRejectItems(actor, to_reject);

        for (xr_vector<u16>::const_iterator it = actor->children.begin(); it != it_e; ++it)
        {
            u16 ItemID = *it;
            CSE_Abstract* e_item = get_entity_from_eid(ItemID);

            R_ASSERT(e_item->ID_Parent == actor->ID);

            if (std::find(to_reject.begin(), to_reject.end(), e_item) != to_reject.end())
                continue;

            if ((e_item->m_tClassID == CLSID_OBJECT_W_KNIFE) || (e_item->m_tClassID == CLSID_DEVICE_TORCH))
            {
                to_destroy.push_back(e_item);
            }
            else if (m_strWeaponsData->GetItemIdx(e_item->s_name) != u32(-1))
            {
                if (!smart_cast<CSE_ALifeItemCustomOutfit*>(e_item))
                {
                    to_transfer.push_back(e_item);
                }
            }
        }

        xr_vector<CSE_Abstract*>::const_iterator tr_it_e = to_transfer.end();

        NET_Packet EventPack;
        NET_Packet PacketReject;
        NET_Packet PacketTake;
        EventPack.w_begin(M_EVENT_PACK);

        for (xr_vector<CSE_Abstract*>::const_iterator tr_it = to_transfer.begin(); tr_it != tr_it_e; ++tr_it)
        {
            m_server->Perform_transfer(PacketReject, PacketTake, *tr_it, actor, item);
            EventPack.w_u8(u8(PacketReject.B.count));
            EventPack.w(&PacketReject.B.data, PacketReject.B.count);
            EventPack.w_u8(u8(PacketTake.B.count));
            EventPack.w(&PacketTake.B.data, PacketTake.B.count);
        }

        if (EventPack.B.count > 2)
            u_EventSend(EventPack);
        for (const auto& item : to_destroy)
            DestroyGameItem(item);
        for (const auto& item : to_reject)
            RejectGameItem(item);
    };
}

void game_sv_CaptureTheArtefact::MoveArtefactToPoint(CSE_ALifeItemArtefact* artefact, RPoint const& toPoint)
{
    VERIFY(artefact);
    // artefact->o_Position.set(toPoint.P);

    VERIFY(artefact->cast_inventory_item());
    artefact->cast_inventory_item()->State.position.set(toPoint.P); // settings position to server object

    CArtefact* OArtefact = smart_cast<CArtefact*>(Level().Objects.net_Find(artefact->ID));

    R_ASSERT2(OArtefact,
        make_string("artefact not found. artefact_id = [%d]. CTA:MoveArtefactToPoint()", artefact->ID).c_str());

    OArtefact->StopActivation();
    OArtefact->MoveTo(toPoint.P); // to server client object

    NET_Packet MovePacket;
    MovePacket.w_begin(M_MOVE_ARTEFACTS);
    MovePacket.w_u8(1);
    MovePacket.w_u16(artefact->ID);
    MovePacket.w_vec3(toPoint.P);
    m_server->SendBroadcast(BroadcastCID, MovePacket,
        net_flags(TRUE, TRUE)); // and to all clients, because it will freeze and will not send updates...
}

void game_sv_CaptureTheArtefact::MoveLifeActors()
{
    u32 playersCount = get_players_count();
    if (!playersCount)
    {
        return;
    }

    struct players_teleporter
    {
        NET_Packet tempPacket;
        u8 lifeActors;
        players_teleporter()
        {
            tempPacket.write_start();
            lifeActors = 0;
        }
        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            if (!l_pC->ps)
                return;

            if (!l_pC->net_Ready || l_pC->ps->IsSkip())
                return;

            VERIFY(l_pC && l_pC->ps && l_pC->owner);

            if (l_pC->ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
                return;
            VERIFY2(tempPacket.w_tell() < (NET_PacketSizeLimit - 256), "max packet size exceeds !");
            tempPacket.w_u16(l_pC->owner->ID);
            tempPacket.w_vec3(l_pC->owner->o_Position);
            tempPacket.w_vec3(l_pC->owner->o_Angle);
            ++lifeActors;
        }
    };
    players_teleporter tmp_functor;
    m_server->ForEachClientDo(tmp_functor);

    NET_Packet MovePacket;
    MovePacket.w_begin(M_MOVE_PLAYERS);
    MovePacket.w_u8(tmp_functor.lifeActors);
    MovePacket.w(tmp_functor.tempPacket.B.data, tmp_functor.tempPacket.B.count);
    m_server->SendBroadcast(BroadcastCID, MovePacket, net_flags(TRUE, TRUE));
}

void game_sv_CaptureTheArtefact::RespawnClient(xrClientData const* pclient)
{
    if (pclient->ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
#ifndef MASTER_GOLD
        VERIFY(pclient->ps);
        Msg("---Respawning dead player [%s]", pclient->ps->getName());
#endif // #ifndef MASTER_GOLD
        RespawnPlayer(pclient->ID, true);
        VERIFY(pclient->ps);
        TGameIDToBoughtFlag::const_iterator buyer_iter = m_dead_buyers.find(pclient->ID);
        int buy_value = 0;
        if (buyer_iter != m_dead_buyers.end())
        {
            buy_value = buyer_iter->second;
        }
        if (buy_value == 0)
        {
            ClearPlayerItems(pclient->ps);
            SetPlayersDefItems(pclient->ps);
        }
        SpawnWeaponsForActor(pclient->owner, pclient->ps);
        pclient->ps->setFlag(GAME_PLAYER_FLAG_READY);
        signal_Syncronize();
    }
}
void game_sv_CaptureTheArtefact::RespawnDeadPlayers()
{
    struct deadplayers_respawner
    {
        game_sv_CaptureTheArtefact* m_owner;
        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            // R_VERIFY2(l_pC && l_pC->ps && l_pC->owner, make_string("client #%d has problem with states", i).c_str());
            if (!l_pC || !l_pC->ps || !l_pC->owner)
                return;

            if (l_pC->ps->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
                return;

            if (m_owner->CheckIfPlayerInBuyMenu(l_pC))
                m_owner->SetReadyToSpawnPlayer(l_pC);
            else
                m_owner->RespawnClient(l_pC);
        }
    };
    deadplayers_respawner tmp_functor;
    tmp_functor.m_owner = this;
    m_server->ForEachClientDoSender(tmp_functor);
}

void game_sv_CaptureTheArtefact::OnObjectEnterTeamBase(u16 id, u16 zone_team)
{
    CSE_Abstract* e_who = m_server->ID_to_entity(id);
    VERIFY(e_who);
    CSE_ALifeCreatureActor* eActor = smart_cast<CSE_ALifeCreatureActor*>(e_who);
    if (eActor)
    {
        game_PlayerState* ps = eActor->owner->ps;
        if (ps && (ps->team == zone_team))
            ps->setFlag(GAME_PLAYER_FLAG_ONBASE);
        signal_Syncronize();
    }
}

void game_sv_CaptureTheArtefact::OnObjectLeaveTeamBase(u16 id, u16 zone_team)
{
    CSE_Abstract* e_who = m_server->ID_to_entity(id);
    if (!e_who)
        return;

    CSE_ALifeCreatureActor* eActor = smart_cast<CSE_ALifeCreatureActor*>(e_who);
    if (eActor)
    {
        game_PlayerState* ps = eActor->owner->ps;
        if (ps && (ps->team == zone_team))
            ps->resetFlag(GAME_PLAYER_FLAG_ONBASE);
        ResetInvincibility(eActor->owner->ID);
        signal_Syncronize();
    }
}

void game_sv_CaptureTheArtefact::ActorDeliverArtefactOnBase(CSE_ActorMP* actor, ETeam actorTeam, ETeam teamOfArtefact)
{
    VERIFY(actor);
    xrClientData* xrCData = actor->owner;
    game_PlayerState* ps = xrCData->ps;
    VERIFY2(xrCData, "client data for actor sv object not found");
    VERIFY2(ps, "player state of client is NULL");
    TeamsMap::iterator te = teams.end();
    TeamsMap::iterator artefactOfTeam = teams.find(teamOfArtefact);
    VERIFY2(artefactOfTeam != te, make_string("artefact owner team (%d) not found", teamOfArtefact));
    VERIFY(artefactOfTeam->second.artefact);

    DropArtefact(actor, artefactOfTeam->second.artefact, &artefactOfTeam->second.artefactRPoint.P);

    NET_Packet P;
    GenerateGameMessage(P);
    P.w_u32(GAME_EVENT_ARTEFACT_ONBASE);
    P.w_u8(static_cast<u8>(actorTeam));
    P.w_u16(xrCData->ps->GameID);
    u_EventSend(P);

    R_ASSERT2(teams.find(actorTeam) != teams.end(), "actor team not found");

    // i hope TEAM_DATA_LIST has random access iteration...
    TEAM_DATA_LIST::const_iterator teamIter = TeamList.begin() + teams[actorTeam].indexOfTeamInList;

    R_ASSERT2(
        teamIter != TeamList.end(), make_string("deliver artefact team (%d) not found in TeamList", actorTeam).c_str());

    Player_AddMoney(ps, teamIter->m_iM_TargetSucceed);
    ps->af_count++;
    teams[actorTeam].score++;

    Set_RankUp_Allowed(true);
    Player_AddExperience(ps, READ_IF_EXISTS(pSettings, r_float, "mp_bonus_exp", "target_succeed", 0));

    // Add money to players in this team
    struct bonusmoney_adder
    {
        game_sv_CaptureTheArtefact* m_owner;
        game_PlayerState* ps;
        s32 m_iM_TargetSucceedAll;

        void operator()(IClient* client)
        {
            // init
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            game_PlayerState* pstate = l_pC->ps;
            if (!pstate || !l_pC->net_Ready || pstate->IsSkip() || pstate == ps)
                return;
            if (pstate->team == ps->team)
            {
                m_owner->Player_AddMoney(pstate, m_iM_TargetSucceedAll);
                m_owner->Player_AddExperience(
                    pstate, READ_IF_EXISTS(pSettings, r_float, "mp_bonus_exp", "target_succeed_all", 0));
            }
            else
            {
                m_owner->Player_AddExperience(pstate, 0.f);
            }
        }
    };
    bonusmoney_adder tmp_functor;
    tmp_functor.m_owner = this;
    tmp_functor.ps = ps;
    tmp_functor.m_iM_TargetSucceedAll = teamIter->m_iM_TargetSucceedAll;
    m_server->ForEachClientDo(tmp_functor);

    Set_RankUp_Allowed(false);

    signal_Syncronize();
    Game().m_WeaponUsageStatistic->OnPlayerBringArtefact(ps);
    AskAllToUpdateStatistics();
    StartNewRound();
}

void game_sv_CaptureTheArtefact::StartNewRound()
{
    fastdelegate::FastDelegate1<IClient*, void> tmp_functor;
    tmp_functor.bind(this, &game_sv_CaptureTheArtefact::PrepareClientForNewRound);
    m_server->ForEachClientDoSender(tmp_functor);
    MoveLifeActors();
    RenewAllActorsHealth();
    RespawnDeadPlayers();
    m_item_respawner.respawn_level_items();
    VERIFY(TeamList.size() >= 2);
#ifndef MASTER_GOLD
    Msg("---Starting new round, scores: [ %d : %d ]", teams[etGreenTeam].score, teams[etBlueTeam].score);
#endif // #ifndef MASTER_GOLD
}

void game_sv_CaptureTheArtefact::PrepareClientForNewRound(IClient* client)
{
    xrClientData* clientData = static_cast<xrClientData*>(client);
    // VERIFY2(clientData && clientData->ps && clientData->owner, "bad client data");
    if (!clientData || !clientData->ps || !clientData->owner)
    {
        return;
    }
    game_PlayerState* ps = clientData->ps;
    if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        // some actions
        return;
    }
    NET_Packet P;
    u_EventGen(P, GE_ACTOR_MAX_POWER, clientData->owner->ID);
    m_server->SendTo(clientData->ID, P, net_flags(TRUE, TRUE));
    // u_EventGen(P, GE_ACTOR_MAX_HEALTH, clientData->owner->ID);
    // m_server->SendTo(clientData->ID,P,net_flags(TRUE,TRUE));
    assign_RP(static_cast<CSE_ALifeCreatureActor*>(clientData->owner), ps);
}

void game_sv_CaptureTheArtefact::CheckForArtefactReturning(u32 currentTime)
{
    TeamsMap::iterator te = teams.end();
    TeamsMap::iterator team_iter;
    for (TeamsMap::iterator ti = teams.begin(); ti != te; ++ti)
    {
        CSE_ActorMP* owner_actor = ti->second.artefactOwner;
        CSE_ALifeItemArtefact* artefact = ti->second.artefact;
        VERIFY(artefact);

        if (owner_actor)
            continue;

        Fvector3& artefact_rpoint = ti->second.artefactRPoint.P;
        if (artefact->o_Position.similar(artefact_rpoint, g_sv_cta_artefactsBaseRadius))
            continue;

        if (ti->second.IsArtefactActivated())
        {
            /*if ((currentTime - ti->second.activationArtefactTimeStart) >=
                Get_ActivatedArtefactRetTime_msec())
            {*/
            MoveArtefactToPoint(artefact, ti->second.artefactRPoint);
            CSE_ActorMP* e_who = smart_cast<CSE_ActorMP*>(m_server->ID_to_entity(ti->second.last_activator_id));
            ti->second.DeactivateArtefact();
            if (e_who)
            {
                // add some money and exp for returning artefact on base
                xrClientData* xrCData = e_who->owner;
                VERIFY(xrCData);
                game_PlayerState* ps_who = xrCData->ps;
                VERIFY(ps_who);

                Player_AddMoney(ps_who, TeamList[ps_who->team].m_iM_KillRival);

                /*Set_RankUp_Allowed(true);
                Player_AddExperience(ps_who, READ_IF_EXISTS(pSettings, r_float, "mp_bonus_exp","target_succeed",0));
                Set_RankUp_Allowed(false);*/

                NET_Packet P;
                GenerateGameMessage(P);
                P.w_u32(GAME_EVENT_ARTEFACT_TAKEN);
                P.w_u8(static_cast<u8>(ps_who->team));
                P.w_clientID(xrCData->ID);
                u_EventSend(P);
            }
            //}
            continue;
        }
        if (!ti->second.freeArtefactTimeStart ||
            ((currentTime - ti->second.freeArtefactTimeStart) >= Get_ArtefactReturningTime_msec()))
        {
            MoveArtefactToPoint(artefact, ti->second.artefactRPoint);
            ti->second.freeArtefactTimeStart = currentTime;
        }
    }
}

void game_sv_CaptureTheArtefact::CheckForArtefactDelivering()
{
    TeamsMap::iterator te = teams.end();
    TeamsMap::iterator myTeamIter;
    for (TeamsMap::iterator ti = teams.begin(); ti != te; ++ti)
    {
        CSE_ActorMP* tempActor = ti->second.artefactOwner;
        if (!tempActor)
        {
            continue;
        }
        xrClientData* xrCData = tempActor->owner;
        VERIFY2(xrCData, "client data for actor sv object not found");
        if (!xrCData)
        {
#ifdef MP_LOGGING
            Msg("! WARNING: bad actor [%d] tries to deliver artefact", tempActor->ID);
#endif //#ifdef MP_LOGGING
            continue;
        }
        if (!xrCData->net_Ready)
        {
            continue;
        }
        VERIFY2(xrCData->ps, "player state in client data is NULL");
        ETeam artefactOfTeam = static_cast<ETeam>(xrCData->ps->team);
        myTeamIter = teams.find(artefactOfTeam);
        VERIFY2(myTeamIter != teams.end(), make_string("artefact team (%d) not found", artefactOfTeam).c_str());
        // if some one alredy took your artefact or own artefact not on base, continue
        Fvector3& myArtPoint = myTeamIter->second.artefactRPoint.P;
        if ((myTeamIter->second.artefactOwner) ||
            (!myTeamIter->second.artefact->o_Position.similar(myArtPoint, g_sv_cta_artefactsBaseRadius)))
        {
            continue;
        }
        if (tempActor->o_Position.similar(myTeamIter->second.artefactRPoint.P, g_sv_cta_artefactsBaseRadius))
        {
            ActorDeliverArtefactOnBase(tempActor, artefactOfTeam, ti->first);
        }
    }
}

BOOL game_sv_CaptureTheArtefact::CheckForRoundEnd()
{
    VERIFY(TeamList.size() >= 2);
    if (m_dwWarmUp_CurTime != 0 || m_bInWarmUp)
        return FALSE;
    if ((teams[etGreenTeam].score >= Get_ScoreLimit()) || (teams[etBlueTeam].score >= Get_ScoreLimit()))
    {
        round_end_reason = eRoundEnd_ArtrefactLimit;
        return TRUE;
    }
    if (!GetTimeLimit())
        return FALSE;
    if ((Level().timeServer() - StartTime()) > u32(GetTimeLimit() * 60000))
    {
        if (teams[etGreenTeam].score != teams[etBlueTeam].score)
        {
            round_end_reason = eRoundEnd_TimeLimit;
            return TRUE;
        }
    }
    return FALSE;
}

bool game_sv_CaptureTheArtefact::ResetInvincibility(ClientID const clientId)
{
    xrClientData* tempClient = m_server->ID_to_client(clientId);
    if (!tempClient) // when the time comes, we'll delete this element in collection
        return false;

    if (!tempClient->ps)
        return false;

    if (!tempClient->ps->testFlag(GAME_PLAYER_FLAG_INVINCIBLE))
        return false;

    tempClient->ps->resetFlag(GAME_PLAYER_FLAG_INVINCIBLE);
    return true;
}

void game_sv_CaptureTheArtefact::ResetTimeoutInvincibility(u32 currentTime)
{
    InvincibilityTimeouts::iterator ii = m_invTimeouts.begin();
    InvincibilityTimeouts::iterator iie = m_invTimeouts.end();
    bool resetted = false;

    for (; ii != iie; ++ii)
    {
        if ((currentTime >= ii->second) && (ii->second != 0))
        {
            resetted = ResetInvincibility(ii->first);
            ii->second = 0;
        }
    }

    if (resetted)
        signal_Syncronize();
}

void game_sv_CaptureTheArtefact::RespawnPlayer(ClientID id_who, bool NoSpectator)
{
    inherited::RespawnPlayer(id_who, NoSpectator);

    xrClientData* xrCData = m_server->ID_to_client(id_who);
    VERIFY(xrCData->ps);
    game_PlayerState* ps = xrCData->ps;
    CSE_Abstract* pOwner = xrCData->owner;
    CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(pOwner);

    if (pA)
    {
        TeamStruct* pTeamData = GetTeamData(ps->team);
        if (pTeamData)
            Player_AddMoney(ps, pTeamData->m_iM_OnRespawn);
        ps->setFlag(GAME_PLAYER_FLAG_INVINCIBLE);

        u32 invLostTime = Level().timeServer() + Get_InvincibilityTime_msec();
        InvincibilityTimeouts::iterator ti = m_invTimeouts.find(id_who);

        if (ti == m_invTimeouts.end())
        {
            m_invTimeouts.insert(std::make_pair(id_who, invLostTime));
        }
        else
        {
            ti->second = invLostTime;
        }
        SpawnWeapon4Actor(pA->ID, "mp_players_rukzak", 0, ps->pItemList);
    }
}

BOOL game_sv_CaptureTheArtefact::OnPreCreate(CSE_Abstract* E) { return inherited::OnPreCreate(E); }
void game_sv_CaptureTheArtefact::OnCreate(u16 eid_who) { inherited::OnCreate(eid_who); }
void game_sv_CaptureTheArtefact::OnDestroyObject(u16 eid_who)
{
    if (eid_who == m_dwSM_CurViewEntity && m_bSpectatorMode)
    {
        SM_SwitchOnNextActivePlayer();
    };
    inherited::OnDestroyObject(eid_who);
    m_item_respawner.check_to_delete(eid_who);
}

void game_sv_CaptureTheArtefact::OnPostCreate(u16 id_who)
{
    inherited::OnPostCreate(id_who);
    CSE_Abstract* entity = get_entity_from_eid(id_who);
    if (!entity)
        return;

    CSE_ALifeCustomZone* custom_zone = smart_cast<CSE_ALifeCustomZone*>(entity);
    if (!custom_zone)
        return;

    LPCSTR zone_name = custom_zone->name_replace();
    if (!zone_name)
        return;

    TGIDCPair id_pair = std::make_pair(id_who, u8(0));

    m_AnomalyIds.insert(std::make_pair(zone_name, id_pair));
}

void game_sv_CaptureTheArtefact::ReadOptions(shared_str& options)
{
    inherited::ReadOptions(options);

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    g_sv_dm_bAnomaliesEnabled = (get_option_i(*options, "ans", (isAnomaliesEnabled() ? 1 : 0)) != 0);
    g_sv_dm_dwAnomalySetLengthTime = get_option_i(*options, "anslen", g_sv_dm_dwAnomalySetLengthTime); // in (min)
    g_sv_dm_bPDAHunt = (get_option_i(*options, "pdahunt", (isPDAHuntEnabled() ? 1 : 0)) != 0);
    g_sv_dm_bDamageBlockIndicators = (get_option_i(*options, "dmbi", (g_sv_dm_bDamageBlockIndicators ? 1 : 0)) != 0);
    g_sv_dm_dwWarmUp_MaxTime = get_option_i(*options, "warmup", g_sv_dm_dwWarmUp_MaxTime);
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    //-------------------------------------------------------------------------
    g_sv_tdm_bAutoTeamBalance = get_option_i(*options, "abalance", (g_sv_tdm_bAutoTeamBalance ? 1 : 0)) != 0;
    g_sv_tdm_bAutoTeamSwap = get_option_i(*options, "aswap", (g_sv_tdm_bAutoTeamSwap ? 1 : 0)) != 0;
    g_sv_tdm_bFriendlyIndicators = get_option_i(*options, "fi", (g_sv_tdm_bFriendlyIndicators ? 1 : 0)) != 0;
    g_sv_tdm_bFriendlyNames = get_option_i(*options, "fn", (g_sv_tdm_bFriendlyNames ? 1 : 0)) != 0;

    float fFF = get_option_f(*options, "ffire", g_sv_tdm_fFriendlyFireModifier);
    g_sv_tdm_fFriendlyFireModifier = fFF;
    //-------------------------------------------------------------------------
    g_sv_ah_dwArtefactsNum = get_option_i(*options, "anum", g_sv_ah_dwArtefactsNum);
    //----------------------------------------------------------------------------
    g_sv_ah_iReinforcementTime = get_option_i(*options, "reinf", g_sv_ah_iReinforcementTime);
    if (g_sv_ah_iReinforcementTime <= 0)
        g_sv_ah_iReinforcementTime = 1;
    //----------------------------------------------------------------------------
    g_sv_cta_dwInvincibleTime = get_option_i(*options, "dmgblock", g_sv_cta_dwInvincibleTime); // in (sec)
    g_sv_cta_artefactReturningTime = get_option_i(*options, "artrettime", g_sv_cta_artefactReturningTime); // in (sec)
    g_sv_cta_activatedArtefactRet = get_option_i(*options, "actret", g_sv_cta_activatedArtefactRet); // in (sec)

    m_bSpectatorMode = false;
    if (!GEnv.isDedicatedServer && (get_option_i(*options, "spectr", -1) != -1))
    {
        m_bSpectatorMode = true;
        m_dwSM_SwitchDelta = get_option_i(*options, "spectr", 0) * 1000;
        if (m_dwSM_SwitchDelta < 1000)
            m_dwSM_SwitchDelta = 1000;
    };
}

u16 game_sv_CaptureTheArtefact::GetMinUsedAnomalyID(LPCSTR zone_name)
{
    typedef TMultiMap::iterator TMultiMIter;

    TMultiMIter lower_iter = m_AnomalyIds.lower_bound(zone_name);
    TMultiMIter upper_iter = m_AnomalyIds.upper_bound(zone_name);

    if (lower_iter == m_AnomalyIds.end())
        return 0;

    u8 min_count = lower_iter->second.second;
    TMultiMIter min_iter = lower_iter;

    while (lower_iter != upper_iter)
    {
        if (lower_iter->second.second < min_count)
        {
            min_iter = lower_iter;
            min_count = lower_iter->second.second;
        }
        ++lower_iter;
    }
    ++min_iter->second.second;

    return min_iter->second.first;
}

void game_sv_CaptureTheArtefact::OnPlayerBuySpawn(ClientID sender)
{
    xrClientData* xrCData = m_server->ID_to_client(sender);
    if (!xrCData || !xrCData->owner)
        return;

    if (!xrCData->ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
        return;

    if (xrCData->ps->m_bPayForSpawn)
        return;

    xrCData->ps->m_bPayForSpawn = true;
    Player_AddMoney(xrCData->ps, m_iMoney_for_BuySpawn);
    OnPlayerReady(sender);
    if (!xrCData->ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        xrCData->ps->m_bPayForSpawn = false;
    }
}

void game_sv_CaptureTheArtefact::ClearReadyFlagFromAll()
{
    struct readyclearer_from_all
    {
        void operator()(IClient* client)
        {
            xrClientData* tmp_client = static_cast<xrClientData*>(client);
            if (!tmp_client->ps)
                return;

            tmp_client->ps->resetFlag(GAME_PLAYER_FLAG_READY + GAME_PLAYER_FLAG_VERY_VERY_DEAD);
        }
    };
    readyclearer_from_all tmp_functor;
    m_server->ForEachClientDo(tmp_functor);
}

void game_sv_CaptureTheArtefact::WriteGameState(CInifile& ini, LPCSTR sect, bool bRoundResult)
{
    inherited::WriteGameState(ini, sect, bRoundResult);
    ini.w_u32(sect, "team_0_score", teams[etGreenTeam].score);
    ini.w_u32(sect, "team_1_score", teams[etBlueTeam].score);
    ini.w_s32(sect, "timelimit_mins", GetTimeLimit());
    ini.w_u32(sect, "artefacts_limit", Get_ScoreLimit());
    ini.w_string(sect, "anomalies", isAnomaliesEnabled() ? "true" : "false");
}

bool game_sv_CaptureTheArtefact::Player_Check_Rank(game_PlayerState* ps)
{
    if (ps->rank == m_aRanks.size() - 1)
        return false;

    int NextExp = m_aRanks[ps->rank + 1].m_iTerms[0];
    if ((ps->experience_Real + ps->experience_New) < NextExp)
        return false;

    u32 const next_rank = ps->rank + 1;
    u32 max_art_count = std::max(teams[etGreenTeam].score, teams[etBlueTeam].score);

    if (next_rank > (max_art_count * g_sv_cta_rankUpToArtsCountDiv))
    {
        ps->experience_New = NextExp - ps->experience_Real;
        return false;
    }

    return true;
}
