#pragma once

#include "game_sv_mp.h"
#include "inventory_space.h"
#include "xrCore/client_id.h"
#include "Hit.h"
#include "xrEngine/pure_relcase.h"

class IClient;

class game_sv_Deathmatch : public game_sv_mp, private pure_relcase
{
    friend class pure_relcase;
    typedef game_sv_mp inherited;

protected:
    struct RPointData
    {
        u32 PointID;
        float MinEnemyDist;
        bool bFreezed;

        RPointData(u32 ID, float Dist, bool Freezed) : PointID(ID), MinEnemyDist(Dist), bFreezed(Freezed){};
        IC bool operator<(const RPointData& x) const
        {
            if (bFreezed && !x.bFreezed)
                return false;
            if (!bFreezed && x.bFreezed)
                return true;
            return MinEnemyDist < x.MinEnemyDist;
        };
    };
    xr_vector<u32> m_vFreeRPoints[TEAM_COUNT];
    u32 m_dwLastRPoints[TEAM_COUNT];

    BOOL m_delayedRoundEnd;
    u32 m_roundEndDelay;

    BOOL m_delayedTeamEliminated;
    u32 m_TeamEliminatedDelay;

    shared_str m_sBaseWeaponCostSection;

    xr_vector<game_TeamState> teams; // dm,tdm,ah

    LPCSTR pWinnigPlayerName;

    virtual void ReadOptions(shared_str& options);
    virtual void ConsoleCommands_Create();
    virtual void ConsoleCommands_Clear();
    /////////////////////////////////////////////////////////////
    using ANOMALIES = xr_vector<xr_string>;
    using ANOMALY_SETS = xr_vector<ANOMALIES>;

    ANOMALIES m_AnomaliesPermanent;
    ANOMALY_SETS m_AnomalySetsList;
    xr_vector<u8> m_AnomalySetID;
    u32 m_dwLastAnomalySetID;
    u32 m_dwLastAnomalyStartTime;

    using ANOMALIES_ID = xr_vector<u16>;
    using ANOMALY_SETS_ID = xr_vector<ANOMALIES_ID>;

    ANOMALY_SETS_ID m_AnomalyIDSetsList;

    bool m_bSpectatorMode;
    u32 m_dwSM_SwitchDelta;
    u32 m_dwSM_LastSwitchTime;
    u32 m_dwSM_CurViewEntity;
    IGameObject* m_pSM_CurViewEntity;
    void SM_SwitchOnNextActivePlayer();
    void SM_SwitchOnPlayer(IGameObject* pNewObject);

    BOOL Is_Anomaly_InLists(CSE_Abstract* E);

protected:
    virtual bool checkForTimeLimit();
    virtual bool checkForFragLimit();
    virtual bool checkForRoundStart();
    virtual bool checkForRoundEnd();
    virtual bool check_for_Anomalies();
    virtual void check_for_WarmUp();

    void Send_Anomaly_States(ClientID id_who);
    void Send_EventPack_for_AnomalySet(u32 AnomalySet, u8 Event);

    virtual void OnPlayerBuyFinished(ClientID id_who, NET_Packet& P);

    virtual void CheckItem(game_PlayerState* ps, PIItem pItem, xr_vector<s16>* pItemsDesired,
        xr_vector<u16>* pItemsToDelete, bool ExactMatch);
    virtual bool HasChampion();

    virtual void check_Player_for_Invincibility(game_PlayerState* ps);

    virtual void Check_ForClearRun(game_PlayerState* ps);
    virtual void FillDeathActorRejectItems(CSE_ActorMP* actor, xr_vector<CSE_Abstract*>& to_reject);

    u32 m_dwWarmUp_CurTime;
    bool m_bInWarmUp;

    void __stdcall net_Relcase(IGameObject* O);

public:
    game_sv_Deathmatch();
    virtual ~game_sv_Deathmatch();
    virtual void Create(shared_str& options);

    virtual LPCSTR type_name() const { return "deathmatch"; };
    virtual void net_Export_State(NET_Packet& P, ClientID id_to);

    virtual void OnEvent(NET_Packet& tNetPacket, u16 type, u32 time, ClientID sender);

    virtual void OnTeamScore(u32 /**team**/, bool); // команда выиграла
    virtual void OnTeamsInDraw(){}; // ничья

    // Events
    virtual void OnRoundStart(); // старт раунда
    virtual void OnRoundEnd(); // round_end_reason							// конец раунда
    virtual void OnDelayedRoundEnd(ERoundEnd_Result reason);
    virtual void OnDelayedTeamEliminated();

    virtual void OnPlayerHitPlayer(u16 id_hitter, u16 id_hitted, NET_Packet& P); //игрок получил Hit
    virtual void OnPlayerHitPlayer_Case(game_PlayerState* ps_hitter, game_PlayerState* ps_hitted, SHit* pHitS);

    virtual BOOL OnTouch(u16 eid_who, u16 eid_what, BOOL bForced = FALSE);
    virtual void OnDetach(u16 eid_who, u16 eid_what);

    virtual BOOL OnPreCreate(CSE_Abstract* E);
    virtual void OnCreate(u16 eid_who);
    virtual void OnPostCreate(u16 id_who);

    virtual void OnPlayerConnect(ClientID id_who);
    virtual void OnPlayerConnectFinished(ClientID id_who);
    virtual void OnPlayerDisconnect(ClientID id_who, pstr Name, u16 GameID);
    virtual void OnPlayerReady(ClientID id_who);
    virtual KILL_RES GetKillResult(game_PlayerState* pKiller, game_PlayerState* pVictim);
    virtual bool OnKillResult(KILL_RES KillResult, game_PlayerState* pKiller, game_PlayerState* pVictim);
    virtual void OnGiveBonus(KILL_RES KillResult, game_PlayerState* pKiller, game_PlayerState* pVictim,
        KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA);
    virtual void Processing_Victim(game_PlayerState* pVictim, game_PlayerState* pKiller);
    virtual void Victim_Exp(game_PlayerState* pVictim);
    virtual bool CheckTeams() { return false; };
    virtual void OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState* ps_killed, KILL_TYPE KillType,
        SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA);

    virtual void OnPlayer_Sell_Item(ClientID id_who, NET_Packet& P);

    virtual void OnPlayerSelectSkin(NET_Packet& P, ClientID sender);
    virtual void OnPlayerChangeSkin(ClientID id_who, s8 skin);

    virtual void OnFraglimitExceed();
    virtual void OnTimelimitExceed();
    void OnPlayerScores();
    virtual void OnDestroyObject(u16 eid_who);
    virtual void OnPlayerFire(ClientID id_who, NET_Packet& P);
    // Main
    virtual void Update();
    BOOL AllPlayers_Ready();

    virtual void assign_RP(CSE_Abstract* E, game_PlayerState* ps_who);
    virtual u32 RP_2_Use(CSE_Abstract* E);

#ifdef DEBUG
    virtual void OnRender();
#endif

    virtual void SetSkin(CSE_Abstract* E, u16 Team, u16 ID); //	{};

    virtual void SpawnWeaponsForActor(CSE_Abstract* pE, game_PlayerState* ps);

    virtual void LoadTeams();
    virtual void LoadTeamData(const shared_str& caSection);
    virtual void LoadSkinsForTeam(const shared_str& caSection, TEAM_SKINS_NAMES* pTeamSkins);
    virtual void LoadDefItemsForTeam(
        const shared_str& caSection, /*TEAM_WPN_LIST *pWpnList,*/ DEF_ITEMS_LIST* pDefItems);

    virtual pcstr GetAnomalySetBaseName() { return "deathmatch_game_anomaly_sets"; };
    virtual void LoadAnomalySets();

    void LoadItemRespawns();

    virtual void StartAnomalies(int AnomalySet = -1);

    virtual bool IsBuyableItem(LPCSTR ItemName);
    void RemoveItemFromActor(CSE_Abstract* pItem);
    //----- Money routines -----------------------------------------------------------------
    virtual void Money_SetStart(ClientID id_who);
    virtual s32 GetMoneyAmount(const shared_str& caSection, pcstr caMoneyStr);
    int GetTeamScore(u32 idx);
    void SetTeamScore(u32 idx, int val);
    game_PlayerState* GetWinningPlayer();
    virtual BOOL CanHaveFriendlyFire() { return FALSE; }
    virtual void RespawnPlayer(ClientID id_who, bool NoSpectator);
    virtual void check_InvinciblePlayers();
    virtual void check_ForceRespawn();
    virtual void on_death(CSE_Abstract* e_dest, CSE_Abstract* e_src);
    //---------------------------------------------------------------------------------------------------
    virtual BOOL IsDamageBlockIndEnabled();
    virtual s32 GetTimeLimit();
    virtual s32 GetFragLimit();
    virtual u32 GetDMBLimit();
    virtual u32 GetForceRespawn();
    virtual u32 GetWarmUpTime();
    virtual BOOL IsAnomaliesEnabled();
    virtual u32 GetAnomaliesTime();

    virtual u32 GetNumTeams() { return teams.size(); };
    // adtitional methods for predicates
    void __stdcall RespawnPlayerAsSpectator(IClient* client);

protected:
    virtual void WriteGameState(CInifile& ini, LPCSTR sect, bool bRoundResult);
    shared_str m_not_free_ammo_str;
    virtual bool CanChargeFreeAmmo(char const* ammo_section);
};
