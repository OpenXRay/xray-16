#pragma once

#include "game_base_space.h"
#include "alife_space.h"
#include "gametype_chooser.h"
#include "player_account.h"
#include "xrCommon/xr_deque.h"
#include "xrCommon/xr_vector.h"
#include "xrEngine/EngineAPI.h"

#pragma pack(push, 1)

struct game_PlayerState; // fw
class NET_Packet;

struct RPoint
{
    Fvector P;
    Fvector A;
    u32 TimeToUnfreeze;
    bool bBlocked;
    u16 BlockedByID;
    u32 BlockTime;
    RPoint() : BlockedByID(0), BlockTime(0)
    {
        P.set(.0f, 0.f, .0f);
        A.set(.0f, 0.f, .0f);
        TimeToUnfreeze = 0;
        bBlocked = false;
    }

    bool operator==(const u16& ID) const { return (bBlocked && BlockedByID == ID); }
};

struct Bonus_Money_Struct
{
    s32 Money;
    u8 Reason;
    u8 Kills;
    Bonus_Money_Struct(s32 M, u8 R, u8 K)
    {
        Money = M;
        Reason = R;
        Kills = K;
    }
    Bonus_Money_Struct()
    {
        Money = 0;
        Reason = 0;
        Kills = 0;
    }
};

struct game_PlayerState
{
    // string64	name;
    u8 team;

    // for statistics
    s16 m_iRivalKills;
    s16 m_iSelfKills;
    s16 m_iTeamKills;
    s16 m_iKillsInRowCurr;
    s16 m_iKillsInRowMax;
    s16 m_iDeaths;
    // money that player have at the current moment
    s32 money_for_round;

    float experience_Real;
    float experience_New;

    // count delivered artefacts (in CTA and AH)
    u8 rank;
    u8 af_count;
    u16 flags__;

    u16 ping;

    u16 GameID;

    u16 lasthitter;
    u16 lasthitweapon;
    s8 skin;
    u32 RespawnTime;
    u32 DeathTime;
    s16 money_delta;
    u8 m_bCurrentVoteAgreed;
    using OLD_GAME_ID = xr_deque<u16>;
    OLD_GAME_ID mOldIDs;
    s32 money_added;
    using MONEY_BONUS = xr_vector<Bonus_Money_Struct>;
    MONEY_BONUS m_aBonusMoney;
    bool m_bPayForSpawn;
    u32 m_online_time;
    player_account m_account;

    shared_str m_player_ip;
    shared_str m_player_digest;

    // if account_info == NULL then constructor call load_account method.
    // so it MUST be use ONLY for local_player !
    explicit game_PlayerState(NET_Packet* account_info);
    ~game_PlayerState();

    virtual void clear();
    bool testFlag(u16 f) const;
    void setFlag(u16 f);
    void resetFlag(u16 f);
    LPCSTR getName() const { return m_account.name().c_str(); }
    // void	setName					(LPCSTR s){xr_strcpy(name,s);}
    void SetGameID(u16 NewID);
    bool HasOldID(u16 ID);
    bool IsSkip() const { return testFlag(GAME_PLAYER_FLAG_SKIP); }
    s16 frags() const { return m_iRivalKills - m_iSelfKills - m_iTeamKills; }
#ifndef AI_COMPILER
    virtual void net_Export(NET_Packet& P, BOOL Full = FALSE);
    virtual void net_Import(NET_Packet& P);
    static void skip_Import(NET_Packet& P);
#endif
    //---------------------------------------

    using PLAYER_ITEMS_LIST = xr_vector<u16>;

    PLAYER_ITEMS_LIST pItemList;

    using SPAWN_POINTS_LIST = xr_vector<s16>;

    SPAWN_POINTS_LIST pSpawnPointsList;
    s16 m_s16LastSRoint;

    s32 LastBuyAcount;
    bool m_bClearRun;
};

struct game_TeamState
{
    int score;
    u16 num_targets;

    game_TeamState();
};

// [15.11.07] Alexander Maniluk: added next enumeratuion to solve problem with team identifiers
enum ETeam
{
    etGreenTeam = 0x00,
    etBlueTeam = 0X01,
    etSpectatorsTeam = 0x02
};
//--------------

#pragma pack(pop)

class IGameState
{
public:
    virtual ~IGameState() = 0;
    virtual EGameIDs Type() const = 0;
    virtual u16 Phase() const = 0;
    virtual s32 Round() const = 0;
    virtual u32 StartTime() const = 0;
    virtual void Create(shared_str& options) = 0;
    virtual const char* type_name() const = 0;
    virtual game_PlayerState* createPlayerState(NET_Packet* accountInfo) = 0;
    virtual ALife::_TIME_ID GetStartGameTime() = 0;
    virtual ALife::_TIME_ID GetGameTime() = 0;
    virtual float GetGameTimeFactor() = 0;
    virtual void SetGameTimeFactor(const float value) = 0;
    virtual ALife::_TIME_ID GetEnvironmentGameTime() = 0;
    virtual float GetEnvironmentGameTimeFactor() = 0;
    virtual void SetEnvironmentGameTimeFactor(const float value) = 0;
    virtual void SetGameTimeFactor(ALife::_TIME_ID gameTime, const float timeFactor) = 0;
    virtual void SetEnvironmentGameTimeFactor(ALife::_TIME_ID gameTime, const float timeFactor) = 0;
};

IC IGameState::~IGameState() {}
class game_GameState : public FactoryObjectBase, public virtual IGameState
{
protected:
    EGameIDs m_type;
    u16 m_phase;
    s32 m_round;
    u32 m_start_time;

    u32 m_round_start_time;
    string64 m_round_start_time_str;

protected:
    virtual void switch_Phase(u32 new_phase);
    virtual void OnSwitchPhase(u32 old_phase, u32 new_phase){};

public:
    game_GameState();
    virtual ~game_GameState() {}
    // IGameState
    virtual EGameIDs Type() const override { return m_type; }
    virtual u16 Phase() const override { return m_phase; }
    virtual s32 Round() const override { return m_round; }
    virtual u32 StartTime() const override { return m_start_time; }
    virtual void Create(shared_str& options) override {}
    virtual const char* type_name() const override { return "base game"; }
    virtual game_PlayerState* createPlayerState(NET_Packet* accountInfo) override
    {
        return new game_PlayerState(accountInfo);
    }
    virtual ALife::_TIME_ID GetStartGameTime() override;
    virtual ALife::_TIME_ID GetGameTime() override;
    virtual float GetGameTimeFactor() override;
    virtual void SetGameTimeFactor(const float value) override;
    virtual ALife::_TIME_ID GetEnvironmentGameTime() override;
    virtual float GetEnvironmentGameTimeFactor() override;
    virtual void SetEnvironmentGameTimeFactor(const float value) override;
    virtual void SetGameTimeFactor(ALife::_TIME_ID GameTime, const float fTimeFactor) override;
    virtual void SetEnvironmentGameTimeFactor(ALife::_TIME_ID GameTime, const float fTimeFactor) override;
    // ~IGameState
    // for scripting enhancement
    static CLASS_ID getCLASS_ID(LPCSTR game_type_name, bool bServer);

    // moved from game_sv_base (time routines)
private:
    // scripts
    u64 m_qwStartProcessorTime;
    u64 m_qwStartGameTime;
    float m_fTimeFactor;
    //-------------------------------------------------------
    u64 m_qwEStartProcessorTime;
    u64 m_qwEStartGameTime;
    float m_fETimeFactor;
};
