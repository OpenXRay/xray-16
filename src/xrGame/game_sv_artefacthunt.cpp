#include "StdAfx.h"
#include "game_sv_artefacthunt.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrServer.h"
#include "Level.h"
#include "Common/LevelGameDef.h"
#include "Actor.h"
#include "game_cl_base.h"
#include "Inventory.h"
#include "Artefact.h"
#include "MPPlayersBag.h"
#include "WeaponKnife.h"
#include "ui/UIBuyWndShared.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "xrNetServer/NET_Messages.h"

BOOL g_SV_Force_Artefact_Spawn = FALSE;

#ifdef DEBUG
#include "debug_renderer.h"
#endif
//-------------------------------------------------------
extern s32 g_sv_dm_dwFragLimit;
extern s32 g_sv_dm_dwTimeLimit;
extern BOOL g_sv_dm_bAnomaliesEnabled;
extern BOOL g_sv_tdm_bAutoTeamSwap;
//-------------------------------------------------------
u32 g_sv_ah_dwArtefactRespawnDelta = 30;
int g_sv_ah_dwArtefactsNum = 10;
u32 g_sv_ah_dwArtefactStayTime = 3;
int g_sv_ah_iReinforcementTime = 15; // 0 - Immediate, -1 - after artefact spawn , other - reinforcement
BOOL g_sv_ah_bBearerCantSprint = FALSE;
BOOL g_sv_ah_bShildedBases = TRUE;
BOOL g_sv_ah_bAfReturnPlayersToBases = TRUE;
//-------------------------------------------------------
int game_sv_ArtefactHunt::Get_ArtefactsCount() { return g_sv_ah_dwArtefactsNum; };
u32 game_sv_ArtefactHunt::Get_ArtefactsRespawnDelta() { return g_sv_ah_dwArtefactRespawnDelta; };
u32 game_sv_ArtefactHunt::Get_ArtefactsStayTime() { return g_sv_ah_dwArtefactStayTime; };
int game_sv_ArtefactHunt::Get_ReinforcementTime() { return g_sv_ah_iReinforcementTime; };
BOOL game_sv_ArtefactHunt::Get_BearerCantSprint() { return g_sv_ah_bBearerCantSprint; }
BOOL game_sv_ArtefactHunt::Get_ShieldedBases() { return g_sv_ah_bShildedBases; };
BOOL game_sv_ArtefactHunt::Get_ReturnPlayers() { return g_sv_ah_bAfReturnPlayersToBases; };
//-------------------------------------------------------
void game_sv_ArtefactHunt::Create(shared_str& options)
{
    g_SV_Force_Artefact_Spawn = FALSE;
    inherited::Create(options);

    m_delayedRoundEnd = false;
    m_delayedTeamEliminated = false;
    m_eAState = NONE;
    //---------------------------------------------------
    // loading respawn points for artefacts
    //.	m_LastRespawnPointID = 0;
    //.	ArtefactsRPoints_ID.clear();
    Artefact_rpoints.clear();

    string_path fn_game;
    if (FS.exist(fn_game, "$level$", "level.game"))
    {
        IReader* F = FS.r_open(fn_game);
        IReader* O = 0;

        // Load RPoints
        if (0 != (O = F->open_chunk(RPOINT_CHUNK)))
        {
            for (int id = 0; O->find_chunk(id); ++id)
            {
                RPoint R;
                u8 team;
                u16 type;

                O->r_fvector3(R.P);
                O->r_fvector3(R.A);
                team = O->r_u8();
                VERIFY(team >= 0 && team < 4);
                type = O->r_u8();
                u16 GameType = O->r_u16();
                switch (type)
                {
                case rptArtefactSpawn:
                {
                    if (GameType & eGameIDArtefactHunt)
                    {
                        Artefact_rpoints.push_back(R);
                    }
                }
                break;
                };
            }
            O->close();
        }

        FS.r_close(F);
    }
    R_ASSERT2(!Artefact_rpoints.empty(), "No points to spawn ARTEFACT");
    //---------------------------------------------------------------
    Artefact_PrepareForSpawn();

    m_ArtefactsSpawnedTotal = 0;
    //---------------------------------------------------------------
    artefactBearerID = 0;
    m_iAfBearerMenaceID = 0;
    teamInPossession = 0;
    m_dwArtefactID = 0;

    bNoLostMessage = false;
    m_bArtefactWasBringedToBase = true;

    m_bSwapBases = false;
    //---------------------------------------------------------------
    m_iMoney_for_BuySpawn = READ_IF_EXISTS(pSettings, r_s32, "artefacthunt_gamedata", "spawn_cost", -10000);
    //---------------------------------------------------------------
    Set_RankUp_Allowed(false);
    ArtefactChooserRandom.seed(u32(CPU::QPC() & 0xffffffff));
}

void game_sv_ArtefactHunt::OnRoundStart()
{
    inherited::OnRoundStart();

    m_delayedRoundEnd = false;
    m_delayedTeamEliminated = false;

    m_ArtefactsSpawnedTotal = 0;

    m_dwNextReinforcementTime = Level().timeServer();

    Artefact_PrepareForSpawn();
    m_item_respawner.respawn_level_items();
    //-------------------------------------------------------
    if (Get_ReinforcementTime() == -1)
    {
        RespawnAllNotAlivePlayers();
    };
}

KILL_RES game_sv_ArtefactHunt::GetKillResult(game_PlayerState* pKiller, game_PlayerState* pVictim)
{
    KILL_RES Res = inherited::GetKillResult(pKiller, pVictim);
    switch (Res)
    {
    case KR_TEAMMATE:
    {
        if (pVictim->GameID == artefactBearerID)
            Res = KR_TEAMMATE_CRITICAL;
    }
    break;
    case KR_RIVAL:
    {
        if (pVictim->GameID == artefactBearerID)
            Res = KR_RIVAL_CRITICAL;
    }
    break;
    default: {
    }
    break;
    };
    return Res;
};

bool game_sv_ArtefactHunt::OnKillResult(KILL_RES KillResult, game_PlayerState* pKiller, game_PlayerState* pVictim)
{
    bool res = true;
    TeamStruct* pTeam = GetTeamData(u8(pKiller->team));
    switch (KillResult)
    {
    case KR_TEAMMATE_CRITICAL:
    {
        //.			pKiller->kills -= 1;
        pKiller->m_iTeamKills++;
        if (pTeam)
            Player_AddMoney(pKiller, pTeam->m_iM_TargetTeam);
        res = false;
    }
    break;
    case KR_RIVAL_CRITICAL:
    {
        //.			pKiller->kills += 1;
        pKiller->m_iRivalKills++;
        pKiller->m_iKillsInRowCurr++;
        pKiller->m_iKillsInRowMax = _max(pKiller->m_iKillsInRowCurr, pKiller->m_iKillsInRowMax);
        if (pTeam)
        {
            u32 ResMoney = pTeam->m_iM_TargetRival;
            if (pKiller->testFlag(GAME_PLAYER_FLAG_INVINCIBLE))
                ResMoney = s32(ResMoney * pTeam->m_fInvinsibleKillModifier);
            Player_AddMoney(pKiller, ResMoney);
        };
        res = true;
    }
    break;
    default: { res = inherited::OnKillResult(KillResult, pKiller, pVictim);
    }
    break;
    }
    return res;
}

void game_sv_ArtefactHunt::OnGiveBonus(KILL_RES KillResult, game_PlayerState* pKiller, game_PlayerState* pVictim,
    KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA)
{
    if (!pKiller)
        return;
    switch (KillResult)
    {
    case KR_RIVAL:
    {
        if (pVictim->GameID == m_iAfBearerMenaceID)
            Player_AddExperience(pKiller, READ_IF_EXISTS(pSettings, r_float, "mp_bonus_exp", "assist_kill", 0));

        inherited::OnGiveBonus(KR_RIVAL, pKiller, pVictim, KillType, SpecialKillType, pWeaponA);
    }
    break;
    case KR_RIVAL_CRITICAL: { inherited::OnGiveBonus(KR_RIVAL, pKiller, pVictim, KillType, SpecialKillType, pWeaponA);
    }
    break;
    default: { inherited::OnGiveBonus(KillResult, pKiller, pVictim, KillType, SpecialKillType, pWeaponA);
    }
    break;
    }
}

void game_sv_ArtefactHunt::OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState* ps_killed,
    KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA)
{
    inherited::OnPlayerKillPlayer(ps_killer, ps_killed, KillType, SpecialKillType, pWeaponA);

    if (ps_killed && ps_killed->GameID == m_iAfBearerMenaceID)
        m_iAfBearerMenaceID = 0;
};

void game_sv_ArtefactHunt::OnPlayerReady(ClientID id)
{
    xrClientData* xrCData = m_server->ID_to_client(id);
    if (!xrCData || !xrCData->owner)
        return;
    //	if	(GAME_PHASE_INPROGRESS == phase) return;
    switch (m_phase)
    {
    case GAME_PHASE_INPROGRESS:
    {
        CSE_Abstract* pOwner = xrCData->owner;
        CSE_Spectator* pS = smart_cast<CSE_Spectator*>(pOwner);

        if (pS && (Get_ReinforcementTime() != 0 && !xrCData->ps->m_bPayForSpawn) && (m_dwWarmUp_CurTime == 0))
        {
            return;
        }
    }
    break;
    };
    inherited::OnPlayerReady(id);
}

void game_sv_ArtefactHunt::OnPlayerBuySpawn(ClientID sender)
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
};

bool game_sv_ArtefactHunt::assign_rp_tmp(game_PlayerState* ps_who, xr_vector<RPoint>& rps, xr_vector<u32>& dest,
    xr_vector<u32>& rpIDEnemy, xr_vector<ClientID>& EnemyIt, bool force_find)
{
    struct rpoints_blocker
    {
        game_sv_ArtefactHunt* m_owner;
        game_PlayerState* ps_who;
        RPoint* rp;
        u32 rpoint_number;
        xr_vector<u32>* rpIDEnemy;
        xr_vector<ClientID>* EnemyIt;
        bool ffind;
        bool blocked;
        bool teams_not_empty;

        void operator()(IClient* client)
        {
            if (blocked)
                return;
            xrClientData* tmp_client = static_cast<xrClientData*>(client);
            game_PlayerState* tmp_ps = tmp_client->ps;
            if (!tmp_ps)
                return;
            if (tmp_ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
                return;
            IGameObject* tmp_player = Level().Objects.net_Find(tmp_ps->GameID);
            if (!tmp_player)
                return;

            if (rp->P.distance_to(tmp_player->Position()) <= 0.4f && !ffind)
            {
                blocked = true;
                if ((ps_who->team != tmp_ps->team) && teams_not_empty)
                {
                    rpIDEnemy->push_back(rpoint_number);
                    EnemyIt->push_back(client->ID);
                }
            }
        }
    };
    dest.clear();

    rpoints_blocker tmp_blocker;
    tmp_blocker.m_owner = this;
    tmp_blocker.ps_who = ps_who;
    tmp_blocker.teams_not_empty = !teams.empty();
    tmp_blocker.ffind = force_find;
    tmp_blocker.rpIDEnemy = &rpIDEnemy;
    tmp_blocker.EnemyIt = &EnemyIt;

    for (u32 p = 0; p < rps.size(); ++p)
    {
        RPoint rp = rps[p];

        tmp_blocker.rp = &rp;
        tmp_blocker.rpoint_number = p;
        tmp_blocker.blocked = false;

        m_server->ForEachClientDo(tmp_blocker);

        if (tmp_blocker.blocked || rp.bBlocked)
        {
            continue;
        };
        dest.push_back(p);
    }

    if (force_find && dest.size() == 0)
        for (u32 i = 0; i < rps.size(); ++i)
            dest.push_back(i);

    return dest.size() > 0;
}

void game_sv_ArtefactHunt::assign_RP(CSE_Abstract* E, game_PlayerState* ps_who)
{
    CSE_Spectator* pSpectator = smart_cast<CSE_Spectator*>(E);
    if (pSpectator)
    {
        inherited::assign_RP(E, ps_who);
        return;
    };

    CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(E);
    if (!pA)
    {
        inherited::assign_RP(E, ps_who);
        return;
    };
    //-----------------------------------------------------------------------------
    u32 Team = RP_2_Use(E);
#ifdef DEBUG
    Msg("--- ArtefactHunt RPoint for %s uses team %d", ps_who->getName(), Team);
#endif // #ifdef DEBUG
    R_ASSERT(rpoints[Team].size());

    xr_vector<RPoint>& rps = rpoints[Team];
    xr_vector<u32> rpID;
    xr_vector<u32> rpIDEnemy;
    xr_vector<ClientID> EnemyIt;

    if (!assign_rp_tmp(ps_who, rps, rpID, rpIDEnemy, EnemyIt, true))
    {
#ifdef DEBUG
        Msg("--- No free Arthfacthunt rpoints found, trying to find rpoints with enemies!");
#endif // #ifdef DEBUG
        assign_rp_tmp(ps_who, rps, rpID, rpIDEnemy, EnemyIt, false);
    }

    if (rpID.empty() && !rpIDEnemy.empty())
    {
        u32 PointID = ::Random.randI(rpIDEnemy.size());
        ;
        RPoint& r = rps[rpIDEnemy[PointID]];
        SetRP(E, &r);
        //---------------------------------------------------------------------
        game_PlayerState* PSE = static_cast<xrClientData*>(m_server->GetClientByID(EnemyIt[PointID]))->ps;
        R_ASSERT2(PSE, "Where is Enemy!!!");
        CGameObject* pPlayer = smart_cast<CGameObject*>(Level().Objects.net_Find(PSE->GameID));
        R_ASSERT2(pPlayer, "Where is Enemy Object!!!");

        NET_Packet P;
        pPlayer->u_EventGen(P, GE_GAME_EVENT, pPlayer->ID());
        P.w_u16(GAME_EVENT_PLAYER_KILL);
        P.w_u16(u16(pPlayer->ID()));
        pPlayer->u_EventSend(P);
    }
    else
    {
        R_ASSERT2(rpID.size() > 0, "No free Respawn Points!");
        u32 PointID = ::Random.randI(rpID.size());
        RPoint& r = rps[rpID[PointID]];
        SetRP(E, &r);
    }
};

void game_sv_ArtefactHunt::SetRP(CSE_Abstract* E, RPoint* pRP)
{
    E->o_Position.set(pRP->P);
    E->o_Angle.set(pRP->A);

    pRP->bBlocked = true;
    pRP->BlockedByID = E->ID;
    pRP->BlockTime = Level().timeServer();

    if (std::find(rpointsBlocked.begin(), rpointsBlocked.end(), pRP) == rpointsBlocked.end())
    {
        rpointsBlocked.push_back(pRP);
    }
}

struct RemoveBlockedRPointPredicate
{
    bool operator()(RPoint* rp) const
    {
        if (!rp->bBlocked)
            return true;

        if (rp->BlockTime + 1000 < Level().timeServer())
        {
            rp->bBlocked = false;
            return true;
        }
        IGameObject* pPlayer = Level().Objects.net_Find(rp->BlockedByID);
        if (!pPlayer || (rp->P.distance_to(pPlayer->Position()) <= 0.4f))
        {
            rp->bBlocked = false;
            return true;
        }
        return false;
    }
};

void game_sv_ArtefactHunt::CheckRPUnblock()
{
    if (rpointsBlocked.empty())
        return;
    rpointsBlocked.erase(std::remove_if(rpointsBlocked.begin(), rpointsBlocked.end(), RemoveBlockedRPointPredicate()),
        rpointsBlocked.end());
    /*for (u32 b=0; b<rpointsBlocked.size(); )
    {
        RPoint* pRP = rpointsBlocked[b];
        if (!pRP->bBlocked || pRP->BlockTime+1000 < Level().timeServer())
        {
            pRP->bBlocked			= false;
            rpointsBlocked.erase	(rpointsBlocked.begin()+b);
            continue;
        };
        IGameObject* pPlayer = Level().Objects.net_Find(pRP->BlockedByID);
        if (!pPlayer || pRP->P.distance_to(pPlayer->Position())<=0.4f)
        {
            pRP->bBlocked = false;
            continue;
        };
        b++;
    }*/
};

u32 game_sv_ArtefactHunt::RP_2_Use(CSE_Abstract* E)
{
    CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(E);
    if (!pA)
        return 0;

    return u32(pA->g_team());
};

void game_sv_ArtefactHunt::LoadTeams()
{
    m_sBaseWeaponCostSection._set("artefacthunt_base_cost");
    if (!pSettings->section_exist(m_sBaseWeaponCostSection))
    {
        R_ASSERT2(0, "No section for base weapon cost for this type of the Game!");
        return;
    };
    m_strWeaponsData->Load(m_sBaseWeaponCostSection);

    LoadTeamData("artefacthunt_team0");
    LoadTeamData("artefacthunt_team1");
    LoadTeamData("artefacthunt_team2");
};

BOOL game_sv_ArtefactHunt::OnTouch(u16 eid_who, u16 eid_what, BOOL bForced)
{
    CSE_Abstract* e_who = m_server->ID_to_entity(eid_who);
    VERIFY(e_who);
    CSE_Abstract* e_what = m_server->ID_to_entity(eid_what);
    VERIFY(e_what);

    CSE_ALifeCreatureActor* A = smart_cast<CSE_ALifeCreatureActor*>(e_who);
    if (A)
    {
        CSE_ALifeItemArtefact* pIArtefact = smart_cast<CSE_ALifeItemArtefact*>(e_what);
        if (pIArtefact)
        {
            artefactBearerID = eid_who;
            m_iAfBearerMenaceID = 0;
            teamInPossession = A->g_team();
            signal_Syncronize();

            m_eAState = IN_POSSESSION;
            xrClientData* xrCData = e_who->owner;
            game_PlayerState* ps_who = xrCData->ps;
            if (ps_who)
            {
                NET_Packet P;
                // P.w_begin			(M_GAMEMESSAGE);
                GenerateGameMessage(P);
                P.w_u32(GAME_EVENT_ARTEFACT_TAKEN);
                P.w_u16(ps_who->GameID);
                P.w_u16(ps_who->team);
                u_EventSend(P);
                //-- Artefact is taken for first time
                if (!m_bArtefactWasTaken)
                {
                    m_bArtefactWasTaken = true;
                    TeamStruct* pTeam = GetTeamData(u8(ps_who->team));
                    if (pTeam)
                    {
                        struct experience_adder
                        {
                            game_PlayerState* ps_who;
                            game_sv_mp* m_owner;
                            void operator()(IClient* client)
                            {
                                xrClientData* l_pC = static_cast<xrClientData*>(client);
                                game_PlayerState* pstate = l_pC->ps;
                                if (!pstate)
                                    return;

                                if (!l_pC->net_Ready || pstate->IsSkip() || pstate->team != ps_who->team)
                                    return;

                                m_owner->Player_AddExperience(
                                    pstate, READ_IF_EXISTS(pSettings, r_float, "mp_bonus_exp", "af_first_take_all", 0));
                            }
                        };
                        experience_adder exp_adder;
                        exp_adder.m_owner = this;
                        exp_adder.ps_who = ps_who;
                        m_server->ForEachClientDo(exp_adder);
                    }
                }
            };
            return TRUE;
        };

        // Actor touches something
        CSE_ALifeItemWeapon* W = smart_cast<CSE_ALifeItemWeapon*>(e_what);
        if (W)
        {
            //---------------------------------------------------------------
            if (IsBuyableItem(*e_what->s_name))
                return TRUE;
            //---------------------------------------------------------------
        }
    }
    return inherited::OnTouch(eid_who, eid_what, bForced);
};

void game_sv_ArtefactHunt::OnDetach(u16 eid_who, u16 eid_what)
{
    CSE_Abstract* e_who = m_server->ID_to_entity(eid_who);
    VERIFY(e_who);
    CSE_Abstract* e_what = m_server->ID_to_entity(eid_what);
    VERIFY(e_what);

    CSE_ALifeCreatureActor* A = smart_cast<CSE_ALifeCreatureActor*>(e_who);
    if (A)
    {
        CSE_ALifeItemArtefact* pIArtefact = smart_cast<CSE_ALifeItemArtefact*>(e_what);
        if (pIArtefact)
        {
            artefactBearerID = 0;
            m_iAfBearerMenaceID = 0;
            teamInPossession = 0;
            signal_Syncronize();
            m_eAState = ON_FIELD;
            m_bArtefactWasDropped = true;

            xrClientData* xrCData = e_who->owner;
            game_PlayerState* ps_who = xrCData->ps;

            if (ps_who && !bNoLostMessage)
            {
                NET_Packet P;
                GenerateGameMessage(P);
                P.w_u32(GAME_EVENT_ARTEFACT_DROPPED);
                P.w_u16(ps_who->GameID);
                P.w_u16(ps_who->team);
                u_EventSend(P);
            };
            Artefact_PrepareForRemove();
        };
    }
    inherited::OnDetach(eid_who, eid_what);
};

void game_sv_ArtefactHunt::OnObjectEnterTeamBase(u16 id, u16 zone_team)
{
    CSE_Abstract* e_who = m_server->ID_to_entity(id);
    VERIFY(e_who);
    CSE_ALifeCreatureActor* eActor = smart_cast<CSE_ALifeCreatureActor*>(e_who);
    if (eActor)
    {
        if (eActor->g_team() == zone_team)
        {
            game_PlayerState* ps = eActor->owner->ps;
            if (ps)
                ps->setFlag(GAME_PLAYER_FLAG_ONBASE);

            signal_Syncronize();

            xr_vector<u16>& C = eActor->children;
            xr_vector<u16>::iterator c = std::find(C.begin(), C.end(), m_dwArtefactID);
            if (C.end() != c)
            {
                OnArtefactOnBase(eActor->owner->ID);
                Game().m_WeaponUsageStatistic->OnPlayerBringArtefact(ps);
            };
        }
    };
};

void game_sv_ArtefactHunt::OnObjectLeaveTeamBase(u16 id, u16 zone_team)
{
    CSE_Abstract* e_who = m_server->ID_to_entity(id);
    if (!e_who)
        return;

    //	CSE_Abstract*		e_zone	= m_server->ID_to_entity(id_zone);	VERIFY(e_zone	);

    CSE_ALifeCreatureActor* eActor = smart_cast<CSE_ALifeCreatureActor*>(e_who);
    //	CSE_ALifeTeamBaseZone*	eZoneBase = smart_cast<CSE_ALifeTeamBaseZone*> (e_zone);
    if (eActor /*&& eZoneBase*/)
    {
        if (eActor->g_team() == zone_team)
        {
            game_PlayerState* ps = eActor->owner->ps;
            if (ps)
                ps->resetFlag(GAME_PLAYER_FLAG_ONBASE);

            signal_Syncronize();
        }
    };
};

void game_sv_ArtefactHunt::OnArtefactOnBase(ClientID id_who)
{
    if (Get_ReinforcementTime() == -1 || Get_ReturnPlayers())
    {
        MoveAllAlivePlayers();
    };
    if (Get_ReinforcementTime() > 0 || Get_ReinforcementTime() == -1)
    {
        RespawnAllNotAlivePlayers();
    };
    m_item_respawner.respawn_level_items();
    //-----------------------------------------------------------
    game_PlayerState* ps = get_id(id_who);
    if (!ps)
        return;
    //-----------------------------------------------
    // add player's points

    Set_RankUp_Allowed(true);
    TeamStruct* pTeam = GetTeamData(u8(ps->team));
    if (pTeam)
    {
        Player_AddMoney(ps, pTeam->m_iM_TargetSucceed);
        Player_AddExperience(ps, READ_IF_EXISTS(pSettings, r_float, "mp_bonus_exp", "target_succeed", 0));
        ps->af_count++;

        struct money_for_team_adder
        {
            game_sv_mp* m_owner;
            game_PlayerState* ps;
            TeamStruct* pTeam;
            bool m_bArtefactWasDropped;
            void operator()(IClient* client)
            {
                xrClientData* l_pC = static_cast<xrClientData*>(client);
                game_PlayerState* pstate = l_pC->ps;
                if (!pstate)
                    return;

                if (!l_pC->net_Ready || pstate->IsSkip() || pstate == ps)
                    return;
                if (pstate->team == ps->team)
                {
                    m_owner->Player_AddMoney(pstate, pTeam->m_iM_TargetSucceedAll);
                    m_owner->Player_AddExperience(
                        pstate, READ_IF_EXISTS(pSettings, r_float, "mp_bonus_exp", "target_succeed_all", 0));
                }
                else
                {
                    m_owner->Player_AddMoney(pstate, pTeam->m_iM_TargetFailed);
                    if (!m_bArtefactWasDropped)
                        pstate->experience_New *=
                            READ_IF_EXISTS(pSettings, r_float, "mp_bonus_exp", "target_failed_all_mul", 1.0f);
                };
                m_owner->Player_AddExperience(pstate, 0);
                m_owner->Player_ExperienceFin(pstate);
            }
        };
        money_for_team_adder tmp_functor;
        tmp_functor.m_owner = this;
        tmp_functor.ps = ps;
        tmp_functor.pTeam = pTeam;
        tmp_functor.m_bArtefactWasDropped = m_bArtefactWasDropped;
        m_server->ForEachClientDo(tmp_functor);
    }
    Set_RankUp_Allowed(false);

    //	teams[ps->team-1].score++;
    SetTeamScore(ps->team - 1, GetTeamScore(ps->team - 1) + 1);
    //-----------------------------------------------
    bNoLostMessage = true;
    //-----------------------------------------------
    // remove artefact from player
    NET_Packet P;
    P.w_begin(M_EVENT);
    P.w_u32(Device.dwTimeGlobal);
    P.w_u16(GE_DESTROY);
    P.w_u16(m_dwArtefactID);

    Level().Send(P, net_flags(TRUE, TRUE));
    //-----------------------------------------------
    bNoLostMessage = false;
    //-----------------------------------------------
    //	P.w_begin			(M_GAMEMESSAGE);
    GenerateGameMessage(P);
    P.w_u32(GAME_EVENT_ARTEFACT_ONBASE);
    P.w_u16(ps->GameID);
    P.w_u16(ps->team);
    u_EventSend(P);
    //-----------------------------------------------
    CActor* pActor = smart_cast<CActor*>(Level().Objects.net_Find(ps->GameID));
    if (pActor)
    {
        pActor->SetfHealth(pActor->GetMaxHealth());
        //-------------------------------------------
        u_EventGen(P, GE_ACTOR_MAX_POWER, ps->GameID);
        m_server->SendTo(id_who, P, net_flags(TRUE, TRUE));
    };
    //-----------------------------------------------
    signal_Syncronize();
    AskAllToUpdateStatistics();
    //-----------------------------------------------
    Artefact_PrepareForSpawn();
    //-----------------------------------------------
    m_bArtefactWasBringedToBase = true;
};

void game_sv_ArtefactHunt::SpawnArtefact()
{
    //	if (OnClient()) return;

    CSE_Abstract* E = NULL;
    if (pSettings->line_exist("artefacthunt_gamedata", "artefact"))
        E = spawn_begin(pSettings->r_string("artefacthunt_gamedata", "artefact"));
    else
        return;

    E->s_flags.assign(M_SPAWN_OBJECT_LOCAL); // flags

    Assign_Artefact_RPoint(E);

    CSE_Abstract* af = spawn_end(E, m_server->GetServerClient()->ID);
    m_dwArtefactID = af->ID;
    //-----------------------------------------------
    NET_Packet P;
    GenerateGameMessage(P);
    P.w_u32(GAME_EVENT_ARTEFACT_SPAWNED);
    u_EventSend(P);
    //-----------------------------------------------
    m_eAState = ON_FIELD;

    Artefact_PrepareForRemove();

    signal_Syncronize();
    //-------------------------------------------------
    if (g_sv_dm_bAnomaliesEnabled)
        StartAnomalies();
    //-------------------------------------------------
    m_bArtefactWasTaken = false;
    m_bArtefactWasDropped = false;
};

void game_sv_ArtefactHunt::RemoveArtefact()
{
    if (m_dwArtefactID != 0)
    {
        NET_Packet P;
        //-----------------------------------------------
        GenerateGameMessage(P);
        P.w_u32(GAME_EVENT_ARTEFACT_DESTROYED);
        P.w_u16(m_dwArtefactID);
        u_EventSend(P);
        //-----------------------------------------------
        u_EventGen(P, GE_DESTROY, m_dwArtefactID);
        Level().Send(P, net_flags(TRUE, TRUE));
        //-----------------------------------------------
    };
    Artefact_PrepareForSpawn();
};

void game_sv_ArtefactHunt::Update()
{
    inherited::Update();

    switch (Phase())
    {
    case GAME_PHASE_TEAM1_ELIMINATED:
    case GAME_PHASE_TEAM2_ELIMINATED:
    {
        if (m_delayedTeamEliminated && m_TeamEliminatedDelay < Device.TimerAsync())
        {
            switch_Phase(GAME_PHASE_INPROGRESS);
            if (Get_ReturnPlayers())
            {
                MoveAllAlivePlayers();
            }
            RespawnAllNotAlivePlayers();
        };
    }
    break;
    case GAME_PHASE_PENDING: {
    }
    break;
    case GAME_PHASE_INPROGRESS:
    {
        UpdatePlayersNotSendedMoveRespond();
        //---------------------------------------------------
        CheckRPUnblock();
        //---------------------------------------------------
        if (m_dwWarmUp_CurTime == 0)
        {
            //---------------------------------------------------
            if (Get_ReinforcementTime() > 0)
            {
                u32 CurTime = Level().timeServer();
                if (m_dwNextReinforcementTime < CurTime)
                {
                    RespawnAllNotAlivePlayers();
                    m_dwNextReinforcementTime = CurTime + Get_ReinforcementTime() * 1000;
                }
            };
            //---------------------------------------------------
            if (Get_ReinforcementTime() == -1 && m_dwArtefactID != 0)
            {
                CheckForAnyAlivePlayer();
                CheckForTeamElimination();
            };
            //---------------------------------------------------
            CheckForTeamWin();
        }
        //---------------------------------------------------
        if (Artefact_NeedToSpawn())
            return;
        if (Artefact_NeedToRemove())
            return;
        if (Artefact_MissCheck())
            return;
    }
    break;
    }
}

bool game_sv_ArtefactHunt::ArtefactSpawn_Allowed()
{
    if (g_SV_Force_Artefact_Spawn)
        return true;
    ///	return true;
    // Check if all players ready
    struct all_players_ready_cond
    {
        u32 TeamAlived[2];
        all_players_ready_cond()
        {
            TeamAlived[0] = 0;
            TeamAlived[1] = 0;
        }
        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            game_PlayerState* ps = l_pC->ps;

            if (!ps)
                return;

            if (!ps->team)
                return;

            if (!l_pC->net_Ready || ps->IsSkip() || ps->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
                return;
            else
            {
                VERIFY2(((ps->team - 1) < 2) && ((ps->team - 1) >= 0), make_string("ps->team = %d", ps->team).c_str());
                TeamAlived[ps->team - 1]++;
            }
        }
    };
    all_players_ready_cond tmp_functor;
    m_server->ForEachClientDo(tmp_functor);
    if (tmp_functor.TeamAlived[0] == 0 || tmp_functor.TeamAlived[1] == 0)
        return FALSE;

    return TRUE;
};

void game_sv_ArtefactHunt::OnCreate(u16 id_who)
{
    inherited::OnCreate(id_who);

    CSE_Abstract* pEntity = get_entity_from_eid(id_who);
    if (!pEntity)
        return;
    CSE_ALifeItemArtefact* pIArtefact = smart_cast<CSE_ALifeItemArtefact*>(pEntity);
    if (pIArtefact)
        m_dwArtefactID = pIArtefact->ID;
    CSE_ALifeTeamBaseZone* pTeamBase = smart_cast<CSE_ALifeTeamBaseZone*>(pEntity);
    if (pTeamBase && m_bSwapBases)
    {
        pTeamBase->m_team = 3 - pTeamBase->m_team;
    }
};

void game_sv_ArtefactHunt::Assign_Artefact_RPoint(CSE_Abstract* E)
{
    R_ASSERT(E);
    const xr_vector<RPoint>& rp = Artefact_rpoints;
    //.	xr_vector<u8>&	rpID		= ArtefactsRPoints_ID;
    RPoint r;
    /*
        if (rpID.empty())
        {
            for (u8 i=0; i<rp.size(); i++)
            {
                if (m_LastRespawnPointID == i) continue;
                rpID.push_back(i);
            }
        };

        u8 ID = u8(::Random.randI((int)rpID.size()));
        m_LastRespawnPointID = rpID[ID];
        r	= rp[m_LastRespawnPointID];
        rpID.erase(rpID.begin()+ID);
    */
    u32 ID = ArtefactChooserRandom.randI((int)rp.size());
#ifndef MASTER_GOLD
    Msg("---select artefact RPoint [%d]", ID);
#endif // #ifndef MASTER_GOLD
    r = rp[ID];
    E->o_Position.set(r.P);
    E->o_Angle.set(r.A);
};

void game_sv_ArtefactHunt::OnTimelimitExceed()
{
    if (GetTeamScore(0) == GetTeamScore(1))
        return;
    u8 winning_team = (GetTeamScore(0) < GetTeamScore(1)) ? 1 : 0;
    OnTeamScore(winning_team, false);
    m_phase = u16((winning_team) ? GAME_PHASE_TEAM2_SCORES : GAME_PHASE_TEAM1_SCORES);
    switch_Phase(m_phase);

    OnDelayedRoundEnd(eRoundEnd_TimeLimit); // "Team Final Score"
};

void game_sv_ArtefactHunt::net_Export_State(NET_Packet& P, ClientID id_to)
{
    inherited::net_Export_State(P, id_to);
    P.w_u8(u8(Get_ArtefactsCount()));
    P.w_u16(artefactBearerID);
    P.w_u8(teamInPossession);
    P.w_u16(m_dwArtefactID);
    P.w_u8((u8)Get_BearerCantSprint());

    P.w_s32(Get_ReinforcementTime());
    if (Get_ReinforcementTime() > 0)
    {
        u32 CurTime = Level().timeServer();
        u32 dTime = m_dwNextReinforcementTime - CurTime;

        P.w_s32(dTime);
    }
};

void game_sv_ArtefactHunt::Artefact_PrepareForSpawn()
{
    m_dwArtefactID = 0;

    m_eAState = NOARTEFACT;

    m_dwArtefactSpawnTime = Device.dwTimeGlobal + Get_ArtefactsRespawnDelta() * 1000;

    artefactBearerID = 0;
    m_iAfBearerMenaceID = 0;
    teamInPossession = 0;

    signal_Syncronize();
};

void game_sv_ArtefactHunt::Artefact_PrepareForRemove()
{
    m_dwArtefactRemoveTime = Device.dwTimeGlobal + Get_ArtefactsStayTime() * 60000;
    m_dwArtefactSpawnTime = 0;
};

bool game_sv_ArtefactHunt::Artefact_NeedToSpawn()
{
    if (m_eAState == ON_FIELD || m_eAState == IN_POSSESSION)
        return false;

    if (m_dwArtefactID != 0)
        return false;

    VERIFY(m_dwArtefactID == 0);

    if (m_dwArtefactSpawnTime < Device.dwTimeGlobal)
    {
        if (ArtefactSpawn_Allowed() || 0 != m_ArtefactsSpawnedTotal)
        {
            m_dwArtefactSpawnTime = 0;
            // time to spawn Artefact;
            SpawnArtefact();
            return true;
        };
    };
    return false;
};

bool game_sv_ArtefactHunt::Artefact_NeedToRemove()
{
    if (m_eAState == IN_POSSESSION)
        return false;
    if (m_eAState == NOARTEFACT)
        return false;

    if (Get_ArtefactsStayTime() == 0)
        return false;

    if (m_dwArtefactRemoveTime < Device.dwTimeGlobal)
    {
        //		VERIFY (m_eAState == ON_FIELD);
        RemoveArtefact();
        return true;
    };
    return false;
}

bool game_sv_ArtefactHunt::Artefact_MissCheck()
{
    if (m_eAState == NONE)
        return false;

    if (m_dwArtefactID != 0)
    {
        CSE_Abstract* E = get_entity_from_eid(m_dwArtefactID);
        if (!E)
        {
            Artefact_PrepareForSpawn();
            return true;
        };
    };
    return false;
}

void game_sv_ArtefactHunt::RespawnAllNotAlivePlayers()
{
    struct not_alive_players_respawner
    {
        game_sv_ArtefactHunt* m_owner;
        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            game_PlayerState* ps = l_pC->ps;
            if (!ps)
                return;

            if (!l_pC->net_Ready || ps->IsSkip())
                return;
            if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD) && !ps->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
            {
                m_owner->RespawnPlayer(l_pC->ID, true);
                m_owner->SpawnWeaponsForActor(l_pC->owner, ps);
                m_owner->Check_ForClearRun(ps);
            };
        }
    };
    not_alive_players_respawner tmp_functor;
    tmp_functor.m_owner = this;

    m_server->ForEachClientDoSender(tmp_functor);

    signal_Syncronize();

    m_dwNextReinforcementTime = Level().timeServer() + Get_ReinforcementTime() * 1000;
};

void game_sv_ArtefactHunt::CheckForAnyAlivePlayer()
{
    struct alife_player_searcher
    {
        bool operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            game_PlayerState* ps = l_pC->ps;
            if (!l_pC->net_Ready || ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD) || ps->IsSkip())
                return false;
            return true;
        }
    };
    alife_player_searcher tmp_predicate;
    IClient* tmp_client = m_server->FindClient(tmp_predicate);
    if (tmp_client)
        return;
    // no alive players found
    RespawnAllNotAlivePlayers();
}

bool game_sv_ArtefactHunt::CheckAlivePlayersInTeam(s16 Team)
{
    struct alife_players_counter_in_team
    {
        u32 cnt_alive;
        u32 cnt_exist;
        s16 Team;
        alife_players_counter_in_team() : Team(0)
        {
            cnt_alive = 0;
            cnt_exist = 0;
        }

        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            if (!l_pC->net_Ready)
                return;
            game_PlayerState* ps = l_pC->ps;
            if (!ps)
                return;

            if (ps->IsSkip() || ps->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
                return;
            if (ps->team != Team)
                return;
            cnt_exist++;
            if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
                return;
            cnt_alive++;
        };
    };
    alife_players_counter_in_team tmp_functor;
    tmp_functor.Team = Team;
    m_server->ForEachClientDo(tmp_functor);

    if (tmp_functor.cnt_exist == 0)
        return true;
    return tmp_functor.cnt_alive != 0;
};

void game_sv_ArtefactHunt::MoveAllAlivePlayers()
{
    struct alife_players_teleporter
    {
        u8 AliveCount;
        NET_Packet tmpP;
        xrServer* m_server;
        game_sv_ArtefactHunt* m_owner;

        alife_players_teleporter()
            : m_server(nullptr), m_owner(nullptr)
        {
            AliveCount = 0;
            tmpP.B.count = 0;
        }

        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            game_PlayerState* ps = l_pC->ps;
            if (!ps)
                return;

            if (!l_pC->net_Ready || ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD) || ps->IsSkip())
                return;
            CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(l_pC->owner);
            CActor* pActor = smart_cast<CActor*>(Level().Objects.net_Find(ps->GameID));
            if (!pA || !pActor)
                return;

            if (!ps->testFlag(GAME_PLAYER_FLAG_ONBASE))
                m_owner->assign_RP(l_pC->owner, ps);
            //-----------------------------------------------
            Fvector Pos = pA->o_Position;
            Fvector Angle = pA->o_Angle;
            //		pA->o_Position	= Pos;
            //		pA->o_Angle		= Angle;
            //------------------------------------------------
            pActor->SetfHealth(pActor->GetMaxHealth());
            pActor->MoveActor(Pos, Angle);
            pActor->StopAnyMove();
            //------------------------------------------------
            NET_Packet P;
            m_owner->u_EventGen(P, GE_ACTOR_MAX_POWER, ps->GameID);
            m_server->SendTo(l_pC->ID, P, net_flags(TRUE, TRUE));
            //------------------------------------------------
            P.B.count = 0;
            tmpP.w_u16(pA->ID);
            tmpP.w_vec3(pA->o_Position);
            tmpP.w_vec3(pA->o_Angle);
            //------------------------------------------------
            AliveCount++;
            l_pC->net_PassUpdates = FALSE;
            l_pC->net_LastMoveUpdateTime = Level().timeServer();
        }
    };
    alife_players_teleporter tmp_functor;
    tmp_functor.m_server = m_server;
    tmp_functor.m_owner = this;
    m_server->ForEachClientDoSender(tmp_functor);

    if (tmp_functor.AliveCount == 0)
        return;

    NET_Packet MovePacket;
    MovePacket.w_begin(M_MOVE_PLAYERS);
    MovePacket.w_u8(tmp_functor.AliveCount);
    MovePacket.w(&tmp_functor.tmpP.B.data, tmp_functor.tmpP.B.count);

    m_server->SendBroadcast(BroadcastCID, MovePacket, net_flags(TRUE, TRUE));
};

void game_sv_ArtefactHunt::UpdatePlayersNotSendedMoveRespond()
{
    struct player_not_sended_move_resp
    {
        bool operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            if (!l_pC)
                return false;
            game_PlayerState* ps = l_pC->ps;
            if (!l_pC->net_Ready || ps->IsSkip())
                return false;
            if (l_pC->net_PassUpdates)
                return false;
            if (l_pC->net_LastMoveUpdateTime > Level().timeServer() - 1000)
                return false;
            return true;
        }
    };
    player_not_sended_move_resp tmp_functor;
    xrClientData* l_pC = static_cast<xrClientData*>(m_server->FindClient(tmp_functor));
    if (l_pC)
    {
        ReplicatePlayersStateToPlayer(l_pC->ID);
        l_pC->net_PassUpdates = FALSE;
        l_pC->net_LastMoveUpdateTime = Level().timeServer();
    }
};

void game_sv_ArtefactHunt::ReplicatePlayersStateToPlayer(ClientID CID)
{
    struct player_replicator
    {
        u8 AliveCount;
        NET_Packet tmpP;
        player_replicator()
        {
            AliveCount = 0;
            tmpP.B.count = 0;
        }

        void operator()(IClient* client)
        {
            xrClientData* l_pC = static_cast<xrClientData*>(client);
            game_PlayerState* ps = l_pC->ps;
            if (!ps)
                return;

            if (!l_pC->net_Ready || ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD) || ps->IsSkip())
                return;
            CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(l_pC->owner);
            if (!pA)
                return;
            //-----------------------------------------------
            NET_Packet P;
            P.B.count = 0;
            tmpP.w_u16(pA->ID);
            tmpP.w_vec3(pA->o_Position);
            tmpP.w_vec3(pA->o_Angle);
            //------------------------------------------------
            AliveCount++;
        };
    };
    player_replicator tmp_functor;
    m_server->ForEachClientDo(tmp_functor);

    NET_Packet MovePacket;
    MovePacket.w_begin(M_MOVE_PLAYERS);
    MovePacket.w_u8(tmp_functor.AliveCount);
    MovePacket.w(&tmp_functor.tmpP.B.data, tmp_functor.tmpP.B.count);

    m_server->SendTo(CID, MovePacket, net_flags(TRUE, TRUE));
};

void game_sv_ArtefactHunt::CheckForTeamElimination()
{
    u8 WinTeam = 0;
    if (!CheckAlivePlayersInTeam(1))
        WinTeam = 2;
    else if (!CheckAlivePlayersInTeam(2))
        WinTeam = 1;
    if (!WinTeam)
        return;

    SetTeamScore(WinTeam - 1, GetTeamScore(WinTeam - 1) + 1);
    //			OnTeamScore(ps_killer->team, false);
    //-----------------------------------------------------------------------------
    TeamStruct* pWTeam = GetTeamData(WinTeam);
    if (pWTeam)
    {
        struct money_adder
        {
            game_sv_ArtefactHunt* m_owner;
            TeamStruct* pWTeam;
            u8 WinTeam;
            void operator()(IClient* client)
            {
                // init
                xrClientData* l_pC = static_cast<xrClientData*>(client);
                game_PlayerState* pstate = l_pC->ps;
                if (!pstate)
                    return;
                if (!l_pC->net_Ready || pstate->IsSkip() || pstate->team != WinTeam)
                    return;
                m_owner->Player_AddMoney(pstate, pWTeam->m_iM_RivalsWipedOut);
            };
        };
        money_adder tmp_functor;
        tmp_functor.m_owner = this;
        tmp_functor.pWTeam = pWTeam;
        tmp_functor.WinTeam = WinTeam;
        m_server->ForEachClientDo(tmp_functor);
    };
    //-----------------------------------------------------------------------------
    m_phase = u16((WinTeam == 1) ? GAME_PHASE_TEAM2_ELIMINATED : GAME_PHASE_TEAM1_ELIMINATED);
    switch_Phase(m_phase);

    OnDelayedTeamEliminated(); // OnDelayedRoundEnd( eRoundEnd_FragLimit ); //??   "Team Eliminated"
    RemoveArtefact();
}

extern int g_sv_Skip_Winner_Waiting;
void game_sv_ArtefactHunt::CheckForTeamWin()
{
    u8 WinTeam = 0;
    if (GetTeamScore(0) >= Get_ArtefactsCount())
        WinTeam = 1;
    else if (GetTeamScore(1) >= Get_ArtefactsCount())
        WinTeam = 2;
    if (!WinTeam)
    {
        if (!GetTimeLimit())
            return;
        if (GetTimeLimit() && ((Level().timeServer() - StartTime())) > u32(GetTimeLimit() * 60000))
        {
            int Team1 = GetTeamScore(0);
            int Team2 = GetTeamScore(1);
            if (Team1 == Team2)
            {
                if (!g_sv_Skip_Winner_Waiting)
                {
                    return;
                };
                WinTeam = 1;
            }
            else
            {
                WinTeam = (Team1 > Team2) ? 1 : 2;
            }
        }
        else
        {
            return;
        }
    };

    OnTeamScore(WinTeam, false);
    m_phase = u16((WinTeam == 2) ? GAME_PHASE_TEAM2_SCORES : GAME_PHASE_TEAM1_SCORES);
    switch_Phase(m_phase);

    OnDelayedRoundEnd(eRoundEnd_ArtrefactLimit); //"Team Final Score"
}

void game_sv_ArtefactHunt::check_Player_for_Invincibility(game_PlayerState* ps)
{
    if (!ps)
        return;
    if (Get_ShieldedBases() && ps->testFlag(GAME_PLAYER_FLAG_ONBASE))
        ps->setFlag(GAME_PLAYER_FLAG_INVINCIBLE);
    else
        inherited::check_Player_for_Invincibility(ps);
};

void game_sv_ArtefactHunt::Check_ForClearRun(game_PlayerState* ps)
{
    if (!ps)
        return;
    /*if (!ps->m_bClearRun)
    {
        ps->m_bClearRun = true;
        return;
    };*/
    TeamStruct* pTeam = GetTeamData(u8(ps->team));
    if (!pTeam)
        return;

    Player_AddMoney(ps, pTeam->m_iM_ClearRunBonus);
};

void game_sv_ArtefactHunt::ReadOptions(shared_str& options)
{
    inherited::ReadOptions(options);
    //-------------------------------
    g_sv_ah_dwArtefactRespawnDelta = get_option_i(*options, "ardelta", g_sv_ah_dwArtefactRespawnDelta); // sec
    g_sv_ah_dwArtefactsNum = get_option_i(*options, "anum", g_sv_ah_dwArtefactsNum);
    g_sv_ah_dwArtefactStayTime = get_option_i(*options, "astime", g_sv_ah_dwArtefactStayTime);
    g_sv_dm_dwFragLimit = 0;
    //----------------------------------------------------------------------------
    g_sv_ah_iReinforcementTime = get_option_i(*options, "reinf", g_sv_ah_iReinforcementTime);
    if (g_sv_ah_iReinforcementTime < 0)
        g_sv_ah_iReinforcementTime = -1;
    //----------------------------------------------------------------------------
}

static bool g_bConsoleCommandsCreated_AHUNT = false;
void game_sv_ArtefactHunt::ConsoleCommands_Create(){};

void game_sv_ArtefactHunt::ConsoleCommands_Clear(){};

//  [7/5/2005]
#ifdef DEBUG

extern Flags32 dbg_net_Draw_Flags;

void game_sv_ArtefactHunt::OnRender()
{
    if (dbg_net_Draw_Flags.test(dbg_draw_rp))
    {
        Fmatrix T;
        T.identity();
        Fvector V0, V1;
        for (u32 i = 0; i < Artefact_rpoints.size(); i++)
        {
            RPoint rp = Artefact_rpoints[i];
            V1 = V0 = rp.P;
            V1.y += 1.0f;

            T.identity();
            Level().debug_renderer().draw_line(Fidentity, V0, V1, color_xrgb(0, 255, 255));

            float r = .4f;
            T.identity();
            T.scale(r, r / 2, r);
            T.translate_add(rp.P);
            Level().debug_renderer().draw_ellipse(T, color_xrgb(0, 255, 255));
        }
    };
    inherited::OnRender();
}
#endif
//  [7/5/2005]

//  [7/29/2005]
bool game_sv_ArtefactHunt::Player_Check_Rank(game_PlayerState* ps)
{
    if (!inherited::Player_Check_Rank(ps))
        return false;
    if (ps->af_count < m_aRanks[ps->rank + 1].m_iTerms[1])
        return false;
    return true;
}
//  [7/29/2005]

void game_sv_ArtefactHunt::OnPlayerHitPlayer_Case(game_PlayerState* ps_hitter, game_PlayerState* ps_hitted, SHit* pHitS)
{
    if (pHitS->hit_type != ALife::eHitTypePhysicStrike)
    {
        if (ps_hitted->testFlag(GAME_PLAYER_FLAG_ONBASE) && Get_ShieldedBases())
        {
            pHitS->power = 0;
            pHitS->impulse = 0;
        }
    }
    inherited::OnPlayerHitPlayer_Case(ps_hitter, ps_hitted, pHitS);
};

void game_sv_ArtefactHunt::OnPlayerHitPlayer(u16 id_hitter, u16 id_hitted, NET_Packet& P)
{
    inherited::OnPlayerHitPlayer(id_hitter, id_hitted, P);

    game_PlayerState* ps_hitter = get_eid(id_hitter);
    game_PlayerState* ps_hitted = get_eid(id_hitted);

    if (!ps_hitter || !ps_hitted)
        return;
    if (ps_hitter->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD) || ps_hitted->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
        return;
    if (ps_hitted->team == ps_hitter->team)
        return;

    if (ps_hitted->GameID == artefactBearerID)
        m_iAfBearerMenaceID = ps_hitter->GameID;
};

void game_sv_ArtefactHunt::SwapTeams()
{
    BOOL old_team_swap = g_sv_tdm_bAutoTeamSwap;
    // swap rpoints
    /*xr_vector<RPoint> tmpRPoints;
    tmpRPoints = rpoints[1];
    rpoints[1] = rpoints[2];
    rpoints[2] = tmpRPoints;
    //swap bases
    m_bSwapBases = !m_bSwapBases;*/
    g_sv_tdm_bAutoTeamSwap = TRUE;
    AutoSwapTeams();
    g_sv_tdm_bAutoTeamSwap = old_team_swap;
};

void game_sv_ArtefactHunt::WriteGameState(CInifile& ini, LPCSTR sect, bool bRoundResult)
{
    inherited::WriteGameState(ini, sect, bRoundResult);

    ini.w_u32(sect, "artefacts_limit", Get_ArtefactsCount());
}

/*void game_sv_ArtefactHunt::DestroyAllPlayerItems(ClientID id_who)	//except rukzak
{
    xrClientData* xrCData = m_server->ID_to_client(id_who);

    VERIFY2(xrCData,
        make_string("client (ClientID = 0x%08x) not found", id_who.value()).c_str());
    VERIFY(xrCData->ps);
    game_PlayerState*	ps	=	xrCData->ps;
#ifndef MASTER_GOLD
    Msg("---Destroying player [%s] items before spawning new bought items.", ps->getName());
#endif // #ifndef MASTER_GOLD

    CActor* pActor = smart_cast<CActor*>(Level().Objects.net_Find(ps->GameID));
    if (!pActor)
        return;

    TIItemContainer::const_iterator iie = pActor->inventory().m_all.end();
    for (TIItemContainer::const_iterator ii = pActor->inventory().m_all.begin();
        ii != iie; ++ii)
    {
        VERIFY(*ii);
        u16 object_id = (*ii)->object().ID();
        CSE_Abstract* tempEntity = m_server->ID_to_entity(object_id);
        VERIFY(tempEntity);

        if (smart_cast<CMPPlayersBag*>(*ii))
            continue;

        CArtefact*	temp_artefact = smart_cast<CArtefact*>(*ii);
        if (temp_artefact)
            continue;

        DestroyGameItem(tempEntity);
    }
}*/
