#include "StdAfx.h"
#include "game_sv_teamdeathmatch.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrServer.h"
#include "Level.h"
#include "game_cl_mp.h"
#include "string_table.h"
#include "clsid_game.h"
#include <functional>
#include "xrNetServer/NET_Messages.h"

#include "ui/UIBuyWndShared.h"

//-------------------------------------------------------
extern s32 g_sv_dm_dwFragLimit;
extern BOOL g_sv_dm_bPDAHunt;
//-------------------------------------------------------
BOOL g_sv_tdm_bAutoTeamBalance = FALSE;
BOOL g_sv_tdm_bAutoTeamSwap = TRUE;
BOOL g_sv_tdm_bFriendlyIndicators = FALSE;
BOOL g_sv_tdm_bFriendlyNames = FALSE;
float g_sv_tdm_fFriendlyFireModifier = 1.0f;
//-------------------------------------------------------
int g_sv_tdm_iTeamKillLimit = 3;
int g_sv_tdm_bTeamKillPunishment = TRUE;
//-------------------------------------------------------
BOOL game_sv_TeamDeathmatch::isFriendlyFireEnabled() { return (int(g_sv_tdm_fFriendlyFireModifier * 100.0f) > 0); };
float game_sv_TeamDeathmatch::GetFriendlyFire()
{
    return (int(g_sv_tdm_fFriendlyFireModifier * 100.0f) > 0) ? g_sv_tdm_fFriendlyFireModifier : 0.0f;
};
BOOL game_sv_TeamDeathmatch::Get_AutoTeamBalance() { return g_sv_tdm_bAutoTeamBalance; };
BOOL game_sv_TeamDeathmatch::Get_AutoTeamSwap() { return g_sv_tdm_bAutoTeamSwap; };
BOOL game_sv_TeamDeathmatch::Get_FriendlyIndicators() { return g_sv_tdm_bFriendlyIndicators; };
BOOL game_sv_TeamDeathmatch::Get_FriendlyNames() { return g_sv_tdm_bFriendlyNames; };
int game_sv_TeamDeathmatch::Get_TeamKillLimit() { return g_sv_tdm_iTeamKillLimit; };
BOOL game_sv_TeamDeathmatch::Get_TeamKillPunishment() { return g_sv_tdm_bTeamKillPunishment; };
//-------------------------------------------------------
void game_sv_TeamDeathmatch::Create(shared_str& options)
{
    inherited::Create(options);
    R_ASSERT2(rpoints[0].size(), "rpoints for specators not found");

    switch_Phase(GAME_PHASE_PENDING);
    game_TeamState td;
    td.score = 0;
    td.num_targets = 0;
    teams.push_back(td);
    teams.push_back(td);
    //-----------------------------------------------------------
    teams_swaped = false;
    round_end_reason = eRoundEnd_Force;
}

void game_sv_TeamDeathmatch::net_Export_State(NET_Packet& P, ClientID to)
{
    inherited::net_Export_State(P, to);
    P.w_u8(u8(Get_FriendlyIndicators()));
    P.w_u8(u8(Get_FriendlyNames()));
}

u8 game_sv_TeamDeathmatch::AutoTeam()
{
    struct TeamPlayersCalculator
    {
        u32 m_teams[2];
        TeamPlayersCalculator()
        {
            m_teams[0] = 0;
            m_teams[1] = 0;
        }
        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            game_PlayerState* ps = l_pC->ps;
            if (!ps)
                return;
            if (!l_pC->net_Ready)
                return;
            if (ps->IsSkip() || ps->team == 0 || ps->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
                return;

            if (ps->team >= 1)
                ++(m_teams[ps->team - 1]);
        }
    };
    TeamPlayersCalculator team_calculator;
    m_server->ForEachClientDo(team_calculator);

    return (team_calculator.m_teams[0] > team_calculator.m_teams[1]) ? 2 : 1;
}

u32 game_sv_TeamDeathmatch::GetPlayersCountInTeams(u8 team)
{
    struct team_players_calculator
    {
        u8 team;
        u32 count;
        void operator()(IClient* client)
        {
            xrClientData* tmp_client = static_cast<xrClientData*>(client);
            game_PlayerState* ps = tmp_client->ps;
            if (!ps)
                return;
            if (tmp_client->net_Ready)
                return;
            if (ps->IsSkip() || ps->team == 0 || ps->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
                return;
            if (ps->team >= team)
                ++count;
        }
    };
    team_players_calculator tmp_functor;
    tmp_functor.team = team;
    tmp_functor.count = 0;
    m_server->ForEachClientDo(tmp_functor);
    return tmp_functor.count;
};

bool game_sv_TeamDeathmatch::TeamSizeEqual() { return GetPlayersCountInTeams(1) == GetPlayersCountInTeams(2); }
struct lowest_player_functor // for autoteam balance
{
    s16 lowest_score;
    s16 MaxTeam;
    xrClientData* lowest_player;
    lowest_player_functor() : MaxTeam(0)
    {
        lowest_score = 32767;
        lowest_player = nullptr;
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
        if (ps->team - 1 != MaxTeam)
            return;

        if (ps->frags() < lowest_score)
        {
            lowest_score = ps->frags();
            lowest_player = l_pC;
        };
    }
};

void game_sv_TeamDeathmatch::AutoBalanceTeams()
{
    if (!Get_AutoTeamBalance())
        return;
    // calc team count
    s16 MinTeam, MaxTeam;
    u32 NumToMove;
    struct team_counter_functor
    {
        u8 l_teams[2];
        team_counter_functor()
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
            if (ps->team >= 1)
                ++(l_teams[ps->team - 1]);
        }
    };
    team_counter_functor tmp_functor;
    m_server->ForEachClientDo(tmp_functor);

    if (tmp_functor.l_teams[0] == tmp_functor.l_teams[1])
        return;

    if (tmp_functor.l_teams[0] > tmp_functor.l_teams[1])
    {
        MinTeam = 1;
        MaxTeam = 0;
    }
    else
    {
        MinTeam = 0;
        MaxTeam = 1;
    };

    NumToMove = (tmp_functor.l_teams[MaxTeam] - tmp_functor.l_teams[MinTeam]) / 2;
    if (!NumToMove)
        return;
    ///////////////////////////////////////////////////////////////////////
    while (NumToMove)
    {
        ///////// get lowest score player from MaxTeam
        lowest_player_functor autob_functor;
        autob_functor.MaxTeam = MaxTeam;
        m_server->ForEachClientDo(autob_functor);
        R_ASSERT(autob_functor.lowest_player && autob_functor.lowest_player->ps);
        game_PlayerState* ps = autob_functor.lowest_player->ps;
        ps->team = u8((MinTeam + 1) & 0x00ff);
        NumToMove--;
    }
};

void game_sv_TeamDeathmatch::OnRoundStart()
{
    if ((round_end_reason != eRoundEnd_Force) && (round_end_reason != eRoundEnd_GameRestartedFast))
    {
        AutoBalanceTeams();
        AutoSwapTeams();
    }
    if (round_end_reason == eRoundEnd_GameRestarted)
    {
        teams_swaped = false;
    }
    inherited::OnRoundStart();
};

void game_sv_TeamDeathmatch::OnRoundEnd()
{
    inherited::OnRoundEnd();
    if (m_bMapNeedRotation)
    {
        if (Get_AutoTeamSwap() && !teams_swaped)
        {
            m_bMapNeedRotation = false;
        }
    }
};

void game_sv_TeamDeathmatch::OnPlayerConnect(ClientID id_who)
{
    inherited::OnPlayerConnect(id_who);

    xrClientData* xrCData = m_server->ID_to_client(id_who);
    game_PlayerState* ps_who = get_id(id_who);
    //	LPCSTR	options				=	get_name_id	(id_who);
    ps_who->team = AutoTeam(); // u8(get_option_i(options,"team",AutoTeam()));

    if (ps_who->IsSkip())
        return;

    if (!xrCData->flags.bReconnect)
        Money_SetStart(id_who);
    SetPlayersDefItems(ps_who);
}

void game_sv_TeamDeathmatch::OnPlayerConnectFinished(ClientID id_who)
{
    xrClientData* xrCData = m_server->ID_to_client(id_who);
    VERIFY(xrCData && xrCData->ps);

    NET_Packet P;
    GenerateGameMessage(P);
    P.w_u32(GAME_EVENT_PLAYER_CONNECTED);
    P.w_clientID(id_who);
    xrCData->ps->team = 1;
    xrCData->ps->m_iTeamKills = 0;
    xrCData->ps->setFlag(GAME_PLAYER_FLAG_SPECTATOR);
    xrCData->ps->setFlag(GAME_PLAYER_FLAG_READY);
    xrCData->ps->net_Export(P, true);
    u_EventSend(P);

    // Send Message About Client join Team
    GenerateGameMessage(P);
    P.w_u32(GAME_EVENT_PLAYER_JOIN_TEAM);
    P.w_stringZ(xrCData->ps->getName());
    P.w_u16(xrCData->ps->team);
    u_EventSend(P);

    SpawnPlayer(id_who, "spectator");
    Send_Anomaly_States(id_who);

    xrCData->net_Ready = TRUE;
};

void game_sv_TeamDeathmatch::OnPlayerSelectTeam(NET_Packet& P, ClientID sender)
{
    xrClientData* l_pC = m_server->ID_to_client(sender);
    s16 l_team;
    P.r_s16(l_team);
    OnPlayerChangeTeam(l_pC->ID, l_team);
    //-------------------------------------------------
};

void game_sv_TeamDeathmatch::OnPlayerChangeTeam(ClientID id_who, s16 team)
{
    game_PlayerState* ps_who = get_id(id_who);
    if (!ps_who)
        return;
    if (!team)
    {
        if (!ps_who->team)
            team = AutoTeam();
        else if (TeamSizeEqual())
        {
            team = ps_who->team;
        }
        else
        {
            team = AutoTeam();
        }
    };
    //-----------------------------------------------------
    NET_Packet Px;
    GenerateGameMessage(Px);
    Px.w_u32(GAME_EVENT_PLAYER_GAME_MENU_RESPOND);
    Px.w_u8(PLAYER_CHANGE_TEAM);
    Px.w_s16(team);
    m_server->SendTo(id_who, Px, net_flags(TRUE, TRUE));
    //-----------------------------------------------------
    if (ps_who->team == team)
        return;
    //-----------------------------------------------------
    KillPlayer(id_who, ps_who->GameID);
    //-----------------------------------------------------
    ps_who->setFlag(GAME_PLAYER_FLAG_SPECTATOR);
    //-----------------------------------------------------
    s16 OldTeam = ps_who->team;
    ps_who->team = u8(team & 0x00ff);
    TeamStruct* pTS = GetTeamData(team);
    if (pTS)
    {
        if ((ps_who->money_for_round < pTS->m_iM_Start) || (OldTeam == 0))
            Money_SetStart(id_who);
    }

    /////////////////////////////////////////////////////////
    // Send Switch team message
    NET_Packet P;
    //	P.w_begin			(M_GAMEMESSAGE);
    GenerateGameMessage(P);
    P.w_u32(PLAYER_CHANGE_TEAM);
    P.w_u16(ps_who->GameID);
    P.w_u16(ps_who->team);
    P.w_u16(team);
    u_EventSend(P);
    /////////////////////////////////////////////////////////

    SetPlayersDefItems(ps_who);
}

void game_sv_TeamDeathmatch::OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState* ps_killed,
    KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA)
{
    s16 OldKillsKiller = 0;
    s16 OldKillsVictim = 0;

    if (ps_killer)
    {
        //.		OldKillsKiller = ps_killer->kills;
        OldKillsKiller = ps_killer->frags();
    }

    if (ps_killed)
    {
        //.		OldKillsVictim = ps_killed->kills;
        OldKillsVictim = ps_killed->frags();
    }

    inherited::OnPlayerKillPlayer(ps_killer, ps_killed, KillType, SpecialKillType, pWeaponA);

    UpdateTeamScore(ps_killer, OldKillsKiller);

    if (ps_killer != ps_killed)
        UpdateTeamScore(ps_killed, OldKillsVictim);

    //-------------------------------------------------------------------
    if (ps_killed && ps_killer)
    {
        if (ps_killed != ps_killer && ps_killer->team == ps_killed->team)
        {
            //.			ps_killer->m_iTeamKills++;

            // Check for TeamKill
            if (Get_TeamKillPunishment())
            {
                if (ps_killer->m_iTeamKills >= Get_TeamKillLimit())
                {
                    struct player_state_searcher
                    {
                        game_PlayerState* ps_killer;
                        IClient* server_client;

                        bool operator()(IClient* client)
                        {
                            xrClientData* pCL = (xrClientData*)client;
                            if (!pCL || pCL == server_client)
                                return false;
                            if (!pCL->ps || pCL->ps != ps_killer)
                                return false;
                            return true;
                        }
                    };
                    player_state_searcher tmp_predicate;
                    tmp_predicate.ps_killer = ps_killer;
                    tmp_predicate.server_client = m_server->GetServerClient();
                    xrClientData* tmp_client = static_cast<xrClientData*>(m_server->FindClient(tmp_predicate));
                    if (tmp_client)
                    {
#ifdef DEBUG
                        Msg("--- Kicking player %s", tmp_client->ps->getName());
#endif
                        pstr reason;
                        STRCONCAT(reason, StringTable().translate("st_kicked_by_server").c_str());
                        m_server->DisconnectClient(tmp_client, reason);
                    }
                }
            }
        }
    }
}

void game_sv_TeamDeathmatch::UpdateTeamScore(game_PlayerState* ps_killer, s16 OldKills)
{
    if (!ps_killer)
        return;
    SetTeamScore(ps_killer->team - 1, GetTeamScore(ps_killer->team - 1) + ps_killer->frags() - OldKills);
}

KILL_RES game_sv_TeamDeathmatch::GetKillResult(game_PlayerState* pKiller, game_PlayerState* pVictim)
{
    KILL_RES Res = inherited::GetKillResult(pKiller, pVictim);
    switch (Res)
    {
    case KR_RIVAL:
    {
        if (pKiller->team == pVictim->team)
            Res = KR_TEAMMATE;
    }
    break;
    default: {
    }
    break;
    };
    return Res;
};

bool game_sv_TeamDeathmatch::OnKillResult(KILL_RES KillResult, game_PlayerState* pKiller, game_PlayerState* pVictim)
{
    bool res = true;
    TeamStruct* pTeam = GetTeamData(u8(pKiller->team));
    switch (KillResult)
    {
    case KR_TEAMMATE:
    {
        //.			pKiller->kills -= 1;
        pKiller->m_iTeamKills++;
        if (pTeam)
            Player_AddMoney(pKiller, pTeam->m_iM_KillTeam);
        res = false;
    }
    break;
    default: { res = inherited::OnKillResult(KillResult, pKiller, pVictim);
    }
    break;
    }
    return res;
};

bool game_sv_TeamDeathmatch::checkForFragLimit()
{
    if (g_sv_dm_dwFragLimit && ((teams[0].score >= g_sv_dm_dwFragLimit) || (teams[1].score >= g_sv_dm_dwFragLimit)))
    {
        OnFraglimitExceed();
        return true;
    };
    return false;
}

u32 game_sv_TeamDeathmatch::RP_2_Use(CSE_Abstract* E)
{
    CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(E);
    if (!pA)
        return 0;

    u32 Team = u32(pA->g_team());
    return (rpoints[Team].size()) ? Team : 0;
};

void game_sv_TeamDeathmatch::OnPlayerHitPlayer_Case(
    game_PlayerState* ps_hitter, game_PlayerState* ps_hitted, SHit* pHitS)
{
    if (pHitS->hit_type != ALife::eHitTypePhysicStrike)
    {
        if (ps_hitter && ps_hitted)
        {
            if (ps_hitter->team == ps_hitted->team && ps_hitter != ps_hitted)
            {
                pHitS->power *= GetFriendlyFire();
                pHitS->impulse *= (GetFriendlyFire() > 1.0f) ? GetFriendlyFire() : 1.0f;
            }
        }
    }
    inherited::OnPlayerHitPlayer_Case(ps_hitter, ps_hitted, pHitS);
};

void game_sv_TeamDeathmatch::OnPlayerHitPlayer(u16 id_hitter, u16 id_hitted, NET_Packet& P)
{
    inherited::OnPlayerHitPlayer(id_hitter, id_hitted, P);
};

void game_sv_TeamDeathmatch::LoadTeams()
{
    m_sBaseWeaponCostSection._set("teamdeathmatch_base_cost");
    if (!pSettings->section_exist(m_sBaseWeaponCostSection))
    {
        R_ASSERT2(0, "No section for base weapon cost for this type of the Game!");
        return;
    };
    m_strWeaponsData->Load(m_sBaseWeaponCostSection);

    LoadTeamData("teamdeathmatch_team0");
    LoadTeamData("teamdeathmatch_team1");
    LoadTeamData("teamdeathmatch_team2");
};

void game_sv_TeamDeathmatch::Update()
{
    inherited::Update();
    switch (Phase())
    {
    case GAME_PHASE_TEAM1_SCORES:
    case GAME_PHASE_TEAM2_SCORES:
    case GAME_PHASE_TEAMS_IN_A_DRAW:
    {
        if (m_delayedRoundEnd && m_roundEndDelay < Device.TimerAsync())
        {
            OnRoundEnd(); // eRoundEnd_Finish
        }
    }
    break;
    };
}
extern int g_sv_Skip_Winner_Waiting;
bool game_sv_TeamDeathmatch::HasChampion() { return (GetTeamScore(0) != GetTeamScore(1) || g_sv_Skip_Winner_Waiting); }
void game_sv_TeamDeathmatch::OnTimelimitExceed()
{
    u8 winning_team = (GetTeamScore(0) < GetTeamScore(1)) ? 1 : 0;
    OnTeamScore(winning_team, false);
    m_phase = u16((winning_team) ? GAME_PHASE_TEAM2_SCORES : GAME_PHASE_TEAM1_SCORES);
    switch_Phase(m_phase);

    OnDelayedRoundEnd(eRoundEnd_TimeLimit); //"TIME_limit"
}
void game_sv_TeamDeathmatch::OnFraglimitExceed()
{
    u8 winning_team = (GetTeamScore(0) < GetTeamScore(1)) ? 1 : 0;
    OnTeamScore(winning_team, false);
    m_phase = u16((winning_team) ? GAME_PHASE_TEAM2_SCORES : GAME_PHASE_TEAM1_SCORES);
    switch_Phase(m_phase);

    OnDelayedRoundEnd(eRoundEnd_FragLimit); //"FRAG_limit"
}
//-----------------------------------------------
void game_sv_TeamDeathmatch::ReadOptions(shared_str& options)
{
    inherited::ReadOptions(options);
    //-------------------------------
    g_sv_tdm_bAutoTeamBalance = get_option_i(*options, "abalance", (g_sv_tdm_bAutoTeamBalance ? 1 : 0)) != 0;
    g_sv_tdm_bAutoTeamSwap = get_option_i(*options, "aswap", (g_sv_tdm_bAutoTeamSwap ? 1 : 0)) != 0;
    g_sv_tdm_bFriendlyIndicators = get_option_i(*options, "fi", (g_sv_tdm_bFriendlyIndicators ? 1 : 0)) != 0;
    g_sv_tdm_bFriendlyNames = get_option_i(*options, "fn", (g_sv_tdm_bFriendlyNames ? 1 : 0)) != 0;

    float fFF = get_option_f(*options, "ffire", g_sv_tdm_fFriendlyFireModifier);
    g_sv_tdm_fFriendlyFireModifier = fFF;
}

static bool g_bConsoleCommandsCreated_TDM = false;
void game_sv_TeamDeathmatch::ConsoleCommands_Create(){};

void game_sv_TeamDeathmatch::ConsoleCommands_Clear() { inherited::ConsoleCommands_Clear(); };
void game_sv_TeamDeathmatch::AutoSwapTeams()
{
    if (!Get_AutoTeamSwap())
        return;

    struct auto_team_swaper
    {
        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            if (!l_pC || !l_pC->net_Ready || !l_pC->ps)
                return;
            game_PlayerState* ps = l_pC->ps;
            if (!ps || ps->IsSkip())
                return;

            if (ps->team != 0)
                ps->team = (ps->team == 1) ? 2 : 1;
        }
    };
    auto_team_swaper tmp_functor;
    m_server->ForEachClientDo(tmp_functor);
    teams_swaped = true;
}

void game_sv_TeamDeathmatch::WriteGameState(CInifile& ini, LPCSTR sect, bool bRoundResult)
{
    inherited::WriteGameState(ini, sect, bRoundResult);

    for (u32 i = 0; i < teams.size(); ++i)
    {
        string16 buf_name;
        xr_sprintf(buf_name, "team_%d_score", i);
        ini.w_u32(sect, buf_name, GetTeamScore(i));
    }
}

BOOL game_sv_TeamDeathmatch::OnTouchItem(CSE_ActorMP* actor, CSE_Abstract* item)
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
        if (g_sv_dm_bPDAHunt && actor->owner && actor->owner->ps)
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

void game_sv_TeamDeathmatch::OnDetachItem(CSE_ActorMP* actor, CSE_Abstract* item)
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

        NET_Packet EventPack;
        NET_Packet PacketReject;
        NET_Packet PacketTake;
        EventPack.w_begin(M_EVENT_PACK);

        for (const auto& it : to_transfer)
        {
            m_server->Perform_transfer(PacketReject, PacketTake, it, actor, item);
            EventPack.w_u8(u8(PacketReject.B.count));
            EventPack.w(&PacketReject.B.data, PacketReject.B.count);
            EventPack.w_u8(u8(PacketTake.B.count));
            EventPack.w(&PacketTake.B.data, PacketTake.B.count);
        }

        if (EventPack.B.count > 2)
            u_EventSend(EventPack);

        for (auto& it : to_destroy)
            DestroyGameItem(it);

        for (auto& it : to_reject)
            DestroyGameItem(it);
    }
}

BOOL game_sv_TeamDeathmatch::OnTouch(u16 eid_who, u16 eid_what, BOOL bForced)
{
    CSE_ActorMP* e_who = smart_cast<CSE_ActorMP*>(m_server->ID_to_entity(eid_who));
    if (!e_who)
        return FALSE;

    CSE_Abstract* e_entity = m_server->ID_to_entity(eid_what);
    if (!e_entity)
        return FALSE;

    return OnTouchItem(e_who, e_entity);
}

void game_sv_TeamDeathmatch::OnDetach(u16 eid_who, u16 eid_what)
{
    CSE_ActorMP* e_who = smart_cast<CSE_ActorMP*>(m_server->ID_to_entity(eid_who));
    if (!e_who)
        return;

    CSE_Abstract* e_entity = m_server->ID_to_entity(eid_what);
    if (!e_entity)
        return;

    OnDetachItem(e_who, e_entity);
}

void game_sv_TeamDeathmatch::OnObjectEnterTeamBase(u16 id, u16 zone_team)
{
    CSE_Abstract* e_who = m_server->ID_to_entity(id);
    VERIFY(e_who);
    CSE_ALifeCreatureActor* eActor = smart_cast<CSE_ALifeCreatureActor*>(e_who);
    if (eActor)
    {
        game_cl_mp* tmp_cl_game = smart_cast<game_cl_mp*>(&Game());
        s16 mteam = tmp_cl_game->ModifyTeam(s16(zone_team));
        game_PlayerState* ps = eActor->owner->ps;
        if (ps && (ps->team == mteam))
        {
            ps->setFlag(GAME_PLAYER_FLAG_ONBASE);
            signal_Syncronize();
        }
    }
}

void game_sv_TeamDeathmatch::OnObjectLeaveTeamBase(u16 id, u16 zone_team)
{
    CSE_Abstract* e_who = m_server->ID_to_entity(id);
    if (!e_who)
        return;

    CSE_ALifeCreatureActor* eActor = smart_cast<CSE_ALifeCreatureActor*>(e_who);
    if (eActor)
    {
        game_cl_mp* tmp_cl_game = smart_cast<game_cl_mp*>(&Game());
        s16 mteam = tmp_cl_game->ModifyTeam(s16(zone_team));
        game_PlayerState* ps = eActor->owner->ps;
        if (ps && (ps->team == mteam))
        {
            ps->resetFlag(GAME_PLAYER_FLAG_ONBASE);
            signal_Syncronize();
        }
    }
}

void game_sv_TeamDeathmatch::RespawnPlayer(ClientID id_who, bool NoSpectator)
{
    inherited::RespawnPlayer(id_who, NoSpectator);
    xrClientData* xrCData = (xrClientData*)m_server->ID_to_client(id_who);
    VERIFY(xrCData);
    game_PlayerState* ps = xrCData->ps;
    VERIFY(ps);
    ps->resetFlag(GAME_PLAYER_FLAG_ONBASE);
}
