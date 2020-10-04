#pragma once

#include "game_sv_mp.h"
#include "actor_mp_server.h"
#include "xrServer.h"
#include "xrServer_Object_Base.h"
#include "CustomZone.h"

class game_sv_CaptureTheArtefact : public game_sv_mp
{
private:
    typedef game_sv_mp inherited;

    struct MyTeam
    {
        TEAM_DATA_LIST::size_type indexOfTeamInList;
        u16 playersCount;
        s32 score;
        // warning teamName must be <= 256 bytes !
        shared_str teamName;
        bool rPointInitialized;
        bool artefactActivated;

        RPoint artefactRPoint;
        shared_str artefactName;
        CSE_ALifeItemArtefact* artefact;
        CSE_ActorMP* artefactOwner;
        u32 freeArtefactTimeStart;
        u16 last_activator_id;

        u32 activationArtefactTimeStart;

        MyTeam();
        MyTeam(const MyTeam& clone);
        MyTeam(TEAM_DATA_LIST::size_type indexInTeamList, u16 pCount, const shared_str& tName, const shared_str& aName);
        void SetArtefactRPoint(const RPoint& rpoint);
        void OnPlayerAttachArtefact(CSE_ActorMP* newArtefactOwner);
        void OnPlayerDetachArtefact(CSE_ActorMP* oldArtefactOwner);
        void OnPlayerActivateArtefact(u16 eid_who);
        bool IsArtefactActivated();
        void DeactivateArtefact();
        CSE_ActorMP* GetArtefactOwner() const;
    };
    typedef std::pair<ETeam, MyTeam> TeamPair;
    // For balancing team players count
    struct MinPlayersFunctor
    {
        bool operator()(const TeamPair& left, const TeamPair& right) const;
    };
    struct SearchArtefactIdFunctor
    {
        bool operator()(const TeamPair& tr, u16 artefactId) const;
    };
    struct SearchOwnerIdFunctor
    {
        bool operator()(const TeamPair& tr, u16 actorId) const;
    };

    typedef xr_map<ETeam, MyTeam> TeamsMap;
    TeamsMap teams;

    // todo: transmit work with anomalies into other class...
    //----------------------------------------------------
    typedef std::pair<xr_string, u16> TNameGameIDAnomalyPair;
    typedef xr_vector<TNameGameIDAnomalyPair> TAnomaliesVector;

    typedef std::pair<TAnomaliesVector, u8> TAnomalyStartedPair;
    typedef xr_vector<TAnomalyStartedPair> TAnomalySet;

    typedef std::pair<u16, u8> TGIDCPair; // GameIDCountPair
    typedef xr_multimap<xr_string, TGIDCPair> TMultiMap;

    typedef xr_map<ClientID, int>
        TGameIDToBoughtFlag; // this map shows what player already bought items when he was dead...

    TAnomaliesVector m_AnomaliesPermanent;
    TAnomalySet m_AnomalySet;
    TMultiMap m_AnomalyIds;

    u32 m_dwLastAnomalyStartTime;
    s32 m_iMoney_for_BuySpawn;

    TGameIDToBoughtFlag m_dead_buyers;
    bool m_bSpectatorMode;

    u32 m_dwWarmUp_CurTime;
    bool m_bInWarmUp;

    u32 m_dwSM_SwitchDelta;
    u32 m_dwSM_LastSwitchTime;
    u32 m_dwSM_CurViewEntity;
    IGameObject* m_pSM_CurViewEntity;
    // static const float			spectr_cam_inert_value;
    // float							prev_cam_inert_value;
    void SM_SwitchOnNextActivePlayer();
    void SM_SwitchOnPlayer(IGameObject* pNewObject);
    void SM_CheckViewSwitching();

    void LoadAnomalySet();
    bool LoadAnomaliesItems(LPCSTR ini_set_id, TAnomaliesVector& dest_vector);

    void StopPreviousAnomalies();
    void ReStartRandomAnomaly();
    void AddAnomalyChanges(NET_Packet& packet, TAnomaliesVector const& anomalies, CCustomZone::EZoneState state);

    void SendAnomalyStates();
    void CheckAnomalyUpdate(u32 current_time);
    void CheckForWarmap(u32 current_time);

    u16 GetMinUsedAnomalyID(LPCSTR zone_name);
    //----------------------------------------------------

    void LoadTeamData(ETeam eteam, const shared_str& caSection);
    void LoadArtefactRPoints();

    s32 GetMoneyAmount(const shared_str& caSection, pcstr caMoneyStr);
    void OnPlayerChangeSkin(ClientID id_who, s8 skin);
    void OnPlayerChangeTeam(game_PlayerState* playerState, s8 team);
    void ProcessPlayerDeath(game_PlayerState* playerState);
    // void ProcessPlayerKill(game_PlayerState *playerState);
    void Money_SetStart(game_PlayerState* ps);

    void ReSpawnArtefacts();
    void MoveArtefactToPoint(CSE_ALifeItemArtefact* artefact, RPoint const& toPoint);
    void MoveLifeActors();
    void RespawnDeadPlayers();
    void RespawnClient(xrClientData const* pclient);
    void __stdcall PrepareClientForNewRound(IClient* client);
    void BalanceTeams();
    void ClearReadyFlagFromAll();

    enum buyMenuPlayerState
    {
        buyMenuPlayerClosesBuyMenu = 0, // this value set in OnCloseBuyMenu
        buyMenuPlayerOpensBuyMenu = 1, // this value set in OnPlayerOpenBuyMenu
        buyMenuPlayerReadyToSpawn = 2 // this value set in RespawnDeadPlayers
    };

    typedef AssociativeVector<xrClientData const*, buyMenuPlayerState> TBuyMenuPlayerStates;
    TBuyMenuPlayerStates m_buyMenuPlayerStates;
    virtual void OnPlayerOpenBuyMenu(xrClientData const* pclient); // this method invokes only if player dead
    virtual void OnPlayerCloseBuyMenu(
        xrClientData const* pclient); // if client state buyMenuPlayerReadyToSpawn respawn player
    void OnCloseBuyMenuFromAll(); // just clears buy menu player states associative vector
    bool CheckIfPlayerInBuyMenu(xrClientData const* pclient);
    void SetReadyToSpawnPlayer(xrClientData const* pclient);

    void OnPlayerBuyFinished(ClientID id_who, NET_Packet& P);
    // void DestroyAllPlayerItems(ClientID id_who);	//except rukzak and artefact :)

    // void DestroyGameItem(CSE_Abstract* entity);
    // void RejectGameItem(CSE_Abstract* entity);

    BOOL OnTouchItem(CSE_ActorMP* actor, CSE_Abstract* item);
    void OnDetachItem(CSE_ActorMP* actor, CSE_Abstract* item);

    void OnObjectEnterTeamBase(u16 id, u16 zone_team);
    void OnObjectLeaveTeamBase(u16 id, u16 zone_team);

    /// Moves and prepears all player for new round (invokes
    /// PrepareActorForNewRound, MoveActorToPoint.
    void StartNewRound();
    void ActorDeliverArtefactOnBase(CSE_ActorMP* actor, ETeam actorTeam, ETeam teamOfArtefact);
    void DropArtefact(CSE_ActorMP* aOwner, CSE_ALifeItemArtefact* artefact, Fvector const* dropPosition = NULL);
    void ReturnArtefactToBase();
    void CheckForArtefactDelivering();
    void CheckForArtefactReturning(u32 currentTime);
    BOOL CheckForAllPlayersReady();
    bool CheckForRoundStart();
    BOOL CheckForRoundEnd();

    KILL_RES GetKillResult(game_PlayerState* pKiller, game_PlayerState* pVictim);
    bool OnKillResult(KILL_RES KillResult, game_PlayerState* pKiller, game_PlayerState* pVictim);
    void OnGiveBonus(KILL_RES KillResult, game_PlayerState* pKiller, game_PlayerState* pVictim, KILL_TYPE KillType,
        SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA);

    BOOL roundStarted;
    u32 nextReinforcementTime;
    u32 currentTime;
    bool teams_swaped;

    typedef AssociativeVector<ClientID, u32> InvincibilityTimeouts;
    InvincibilityTimeouts m_invTimeouts;
    void ResetTimeoutInvincibility(u32 currentTime);
    bool ResetInvincibility(ClientID const clientId);

    BOOL isFriendlyFireEnabled();
    float GetFriendlyFire();
    int Get_TeamKillLimit();
    BOOL Get_TeamKillPunishment();
    BOOL Get_FriendlyIndicators();
    BOOL Get_FriendlyNames();
    int Get_ReinforcementTime_msec();
    u32 GetWarmUpTime();
    s32 GetTimeLimit();

    BOOL isAnomaliesEnabled();
    BOOL isPDAHuntEnabled();
    u32 Get_InvincibilityTime_msec();
    u32 Get_AnomalySetLengthTime_msec();
    u32 Get_ArtefactReturningTime_msec();
    u32 Get_ActivatedArtefactRet();
    u32 Get_PlayerScoresDelayTime_msec();
    s32 Get_ScoreLimit();
    BOOL Get_BearerCanSprint();

protected:
    virtual void ReadOptions(shared_str& options);
    virtual void FillDeathActorRejectItems(CSE_ActorMP* actor, xr_vector<CSE_Abstract*>& to_reject);
    shared_str m_not_free_ammo_str;
    virtual bool CanChargeFreeAmmo(char const* ammo_section);
    virtual void WriteGameState(CInifile& ini, LPCSTR sect, bool bRoundResult);

public:
    game_sv_CaptureTheArtefact();
    virtual ~game_sv_CaptureTheArtefact();

    virtual LPCSTR type_name() const;
    virtual void Create(shared_str& options);
    virtual void OnPlayerConnect(ClientID id_who);
    virtual void OnPlayerDisconnect(ClientID id_who, pstr Name, u16 GameID);
    virtual void OnPlayerConnectFinished(ClientID id_who);
    virtual void OnPlayerHitted(NET_Packet P);

    virtual void OnPlayerReady(ClientID id_who);

    virtual void OnPlayerSelectSkin(NET_Packet& P, ClientID sender);
    virtual void OnPlayerSelectTeam(NET_Packet& P, ClientID sender);
    virtual void OnPlayerSelectSpectator(NET_Packet& P, ClientID sender);
    virtual void OnRoundStart();
    virtual void OnRoundEnd();

    virtual BOOL OnPreCreate(CSE_Abstract* E);
    virtual void OnCreate(u16 eid_who);
    virtual void OnPostCreate(u16 id_who);
    virtual void OnDestroyObject(u16 eid_who);

    virtual void Update();

    virtual void net_Export_State(NET_Packet& P, ClientID id_to);
    virtual void net_Export_Update(NET_Packet& P, ClientID id_to, ClientID id);

    virtual void LoadSkinsForTeam(const shared_str& caSection, TEAM_SKINS_NAMES* pTeamSkins);
    virtual void LoadDefItemsForTeam(const shared_str& caSection, DEF_ITEMS_LIST* pDefItems);
    virtual void SpawnWeaponsForActor(CSE_Abstract* pE, game_PlayerState* ps);
    virtual void OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState* ps_killed, KILL_TYPE KillType,
        SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA);

    virtual void OnPlayerHitPlayer(u16 id_hitter, u16 id_hitted, NET_Packet& P);
    virtual void OnPlayerHitPlayer_Case(game_PlayerState* ps_hitter, game_PlayerState* ps_hitted, SHit* pHitS);

    virtual BOOL OnTouch(u16 eid_who, u16 eid_target, BOOL bForced = FALSE); // TRUE=allow ownership, FALSE=denied
    virtual void OnDetach(u16 eid_who, u16 eid_target);
    virtual BOOL OnActivate(u16 eid_who, u16 eid_target);
    virtual void OnEvent(NET_Packet& tNetPacket, u16 type, u32 time, ClientID sender);
    virtual void RespawnPlayer(ClientID id_who, bool NoSpectator);
    virtual void OnPlayerBuySpawn(ClientID sender);
    virtual bool Player_Check_Rank(game_PlayerState* ps);

    void SwapTeams();
};
