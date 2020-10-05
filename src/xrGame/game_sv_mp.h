#pragma once
#include "game_sv_base.h"
#include "game_sv_mp_team.h"
#include "game_base_kill_type.h"
#include "game_base_menu_events.h"
#include "actor_mp_server.h"
#include "cdkey_ban_list.h"

class CItemMgr;
class xrClientData;
#define VOTE_LENGTH_TIME 1
#define VOTE_QUOTA 0.51f

#define MAX_TERMS 2
struct Rank_Struct
{
    shared_str m_sTitle;
    int m_iTerms[MAX_TERMS];
    int m_iBonusMoney;
    xr_vector<float> m_aRankDiff_ExpBonus;

    Rank_Struct()
    {
        m_sTitle = NULL;
        ZeroMemory(m_iTerms, sizeof(m_iTerms));
        m_iBonusMoney = 0;
        m_aRankDiff_ExpBonus.clear();
    };
};

class game_sv_mp : public game_sv_GameState
{
    friend void game_sv_mp_script_register(lua_State* luaState);

    using inherited = game_sv_GameState;

protected:
    //список трупов для удаления
    using CORPSE_LIST = xr_deque<u16>;

    CORPSE_LIST m_CorpseList;

    using RANKS_LIST = xr_vector<Rank_Struct>;

    RANKS_LIST m_aRanks;
    bool m_bRankUp_Allowed;

    TEAM_DATA_LIST TeamList;
    CItemMgr* m_strWeaponsData;

    //-------------------------------------------------------
    bool m_bVotingActive;
    bool m_bVotingReal;
    u32 m_uVoteStartTime;
    shared_str m_pVoteCommand;
    shared_str m_voting_string; // for sending to clients...
    shared_str m_started_player;

    virtual void LoadRanks();
    virtual bool Player_Check_Rank(game_PlayerState* ps);
    virtual void Player_Rank_Up(game_PlayerState* ps);
    virtual bool Player_RankUp_Allowed() { return m_bRankUp_Allowed; };
    virtual void Set_RankUp_Allowed(bool RUA) { m_bRankUp_Allowed = RUA; };
    virtual void UpdatePlayersMoney();
    virtual void DestroyAllPlayerItems(ClientID id_who); // except rukzak and artefact :)

    u8 m_u8SpectatorModes;

    cdkey_ban_list m_cdkey_ban_list;

protected:
    virtual void SendPlayerKilledMessage(
        u16 KilledID, KILL_TYPE KillType, u16 KillerID, u16 WeaponID, SPECIAL_KILL_TYPE SpecialKill);
    virtual void RespawnPlayer(ClientID id_who, bool NoSpectator);
    virtual void SetSkin(CSE_Abstract* E, u16 Team, u16 ID);
    bool GetPosAngleFromActor(ClientID id, Fvector& Pos, Fvector& Angle);
    void AllowDeadBodyRemove(ClientID id, u16 GameID);
    void SpawnWeapon4Actor(u16 actorId, LPCSTR N, u8 Addons, game_PlayerState::PLAYER_ITEMS_LIST& playerItems);
    virtual bool CanChargeFreeAmmo(char const* ammo_section) { return false; };
    // void				SpawnWeaponForActor		(u16 actorId,  LPCSTR N, bool isScope, bool isGrenadeLauncher, bool
    // isSilencer);
    void SetCanOpenBuyMenu(ClientID id);

    // spawning weapons
    typedef std::pair<shared_str, u16> ammo_diff_t;
    virtual void SetAmmoForWeapon(CSE_ALifeItemWeapon* weapon, u8 Addons,
        game_PlayerState::PLAYER_ITEMS_LIST& playerItems, ammo_diff_t& ammo_diff);
    void ChargeAmmo(CSE_ALifeItemWeapon* weapon, LPCSTR ammo_string, game_PlayerState::PLAYER_ITEMS_LIST& playerItems,
        ammo_diff_t& ammo_diff);
    void ChargeGrenades(
        CSE_ALifeItemWeapon* weapon, LPCSTR grenades_string, game_PlayerState::PLAYER_ITEMS_LIST& playerItems);
    void SpawnAmmoDifference(u16 actorId, ammo_diff_t const& ammo_diff);
    // ----------------

    //	virtual		bool				GetTeamItem_ByID		(WeaponDataStruct** pRes, TEAM_WPN_LIST* pWpnList, u16
    // ItemID);
    //	virtual		bool				GetTeamItem_ByName		(WeaponDataStruct** pRes,TEAM_WPN_LIST* pWpnList, LPCSTR
    // ItemName);

    virtual void Player_AddBonusMoney(game_PlayerState* ps, s32 MoneyAmount, SPECIAL_KILL_TYPE Reason, u8 Kill = 0);

    u8 SpectatorModes_Pack();
    void SpectatorModes_UnPack(u8 SpectrModesPacked);

    // [14.11.07] Alexander Maniluk: added this method for renewing health and power of all actors in game.
    // This method sets the health of all CActor objects (on server site) to 1.0, after that server Update will
    // send to all clients new states of health.
    void RenewAllActorsHealth();

    virtual void FillDeathActorRejectItems(CSE_ActorMP* actor, xr_vector<CSE_Abstract*>& to_reject){};

public:
    game_sv_mp();
    virtual ~game_sv_mp();
    virtual void Create(shared_str& options);
    virtual void OnPlayerConnect(ClientID id_who);
    virtual void OnPlayerDisconnect(ClientID id_who, pstr Name, u16 GameID);
    virtual BOOL OnTouch(u16 eid_who, u16 eid_target, BOOL bForced = FALSE)
    {
        return true;
    }; // TRUE=allow ownership, FALSE=denied
    virtual void OnDetach(u16 eid_who, u16 eid_target){};
    virtual void OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState* ps_killed, KILL_TYPE KillType,
        SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA){};
    virtual void OnPlayerKilled(NET_Packet P);
    virtual bool CheckTeams() { return false; };
    virtual void OnPlayerHitted(NET_Packet P);
    virtual void OnPlayerEnteredGame(ClientID id_who);

    virtual void OnDestroyObject(u16 eid_who);

    virtual void net_Export_State(NET_Packet& P, ClientID id_to);

    virtual void OnRoundStart(); // старт раунда
    virtual void OnRoundEnd(); // round_end_reason							// конец раунда
    virtual bool OnNextMap();
    virtual void OnPrevMap();

    virtual void OnVoteStart(LPCSTR VoteCommand, ClientID sender);
    void SendActiveVotingTo(ClientID const& receiver);
    virtual bool IsVotingActive() { return m_bVotingActive; };
    virtual void SetVotingActive(bool Active) { m_bVotingActive = Active; };
    virtual void UpdateVote();
    virtual void OnVoteYes(ClientID sender);
    virtual void OnVoteNo(ClientID sender);
    virtual void OnVoteStop();

    virtual void OnPlayerChangeName(NET_Packet& P, ClientID sender);
    virtual void OnPlayerSpeechMessage(NET_Packet& P, ClientID sender);
    virtual void OnPlayerGameMenu(NET_Packet& P, ClientID sender);

    virtual void OnPlayerSelectSpectator(NET_Packet& P, ClientID sender);
    virtual void OnPlayerSelectTeam(NET_Packet& P, ClientID sender){};
    virtual void OnPlayerSelectSkin(NET_Packet& P, ClientID sender){};
    virtual void OnPlayerBuySpawn(ClientID sender){};

    virtual void OnEvent(NET_Packet& tNetPacket, u16 type, u32 time, ClientID sender);
    virtual void Update();
    void KillPlayer(ClientID id_who, u16 GameID);
    virtual BOOL CanHaveFriendlyFire() { return TRUE; };
    virtual void ClearPlayerState(game_PlayerState* ps);
    virtual void ClearPlayerItems(game_PlayerState* ps);
    virtual void SetPlayersDefItems(game_PlayerState* ps);

    virtual void ReadOptions(shared_str& options);
    virtual void ConsoleCommands_Create();
    virtual void ConsoleCommands_Clear();

    virtual u32 GetTeamCount() { return TeamList.size(); };
    TeamStruct* GetTeamData(u32 Team);

    virtual u8 GetSpectatorModes() { return m_u8SpectatorModes; };
    virtual u32 GetNumTeams() { return 0; };
    virtual void DumpOnlineStatistic();
    void DestroyGameItem(CSE_Abstract* entity);
    void RejectGameItem(CSE_Abstract* entity);

    void DumpRoundStatisticsAsync();
    bool CheckStatisticsReady();
    void DumpRoundStatistics();

    void StartToDumpStatistics(); // creates file name for statistics..
    string_path round_statistics_dump_fn;
    void FinishToDumpStatistics();
    void StopToDumpStatistics(); // removes file
    void AskAllToUpdateStatistics();

    struct async_statistics_collector
    {
        typedef AssociativeVector<ClientID, bool> responses_t;
        responses_t async_responses;
        void operator()(IClient* client);
        bool all_ready() const;
        void set_responded(ClientID clientID);
    };
    async_statistics_collector m_async_stats;
    u32 m_async_stats_request_time;

    void SvSendChatMessage(LPCSTR str);
    bool IsPlayerBanned(char const* hexstr_digest, shared_str& by_who);
    IClient* BanPlayer(ClientID const& client_id, s32 ban_time_sec, xrClientData* initiator);
    void BanPlayerDirectly(char const* client_hex_digest, s32 ban_time_sec, xrClientData* initiator);
    void UnBanPlayer(size_t banned_player_index);
    void PrintBanList(char const* filter);

protected:
    virtual void WriteGameState(CInifile& ini, LPCSTR sect, bool bRoundResult);
    bool CheckPlayerMapName(ClientID const& clientID, NET_Packet& P);
    void ReconnectPlayer(ClientID const& clientID);

    virtual void OnPlayerOpenBuyMenu(xrClientData const* pclient){}; // this method invokes only if player dead
    virtual void OnPlayerCloseBuyMenu(
        xrClientData const* pclient){}; // if client state buyMenuPlayerReadyToSpawn respawn player

    s32 ExcludeBanTimeFromVoteStr(char const* vote_string, char* new_vote_str, u32 new_vote_str_size);

public:
    virtual void WritePlayerStats(CInifile& ini, LPCSTR sect, xrClientData* pCl);
    virtual void Player_AddExperience(game_PlayerState* ps, float Exp);
    virtual void Player_ExperienceFin(game_PlayerState* ps);
    virtual void Player_AddMoney(game_PlayerState* ps, s32 MoneyAmount);
    void SpawnPlayer(ClientID id, LPCSTR N);
};
