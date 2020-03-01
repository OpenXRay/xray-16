#pragma once
#include "Level_Bullet_Manager.h"
#include "game_base_kill_type.h"
#include "xrCore/xrstring.h"
#include "xrCore/buffer_vector.h"
#include "xrCommon/xr_vector.h"
#include "xrCore/Threading/Lock.hpp" // XXX: Try to get rid of this compile-time dependency

#define STAT_TEAM_COUNT 3

struct game_PlayerState;

struct BulletData
{
    shared_str FirerName;
    shared_str WeaponName;

    u16 HitRefCount;
    u16 HitResponds;

    bool Removed;

    SBullet Bullet;
    bool operator==(u32 BulletID) { return BulletID == Bullet.m_dwID; };
    bool operator!=(u32 BulletID) { return BulletID != Bullet.m_dwID; };
    BulletData(shared_str FName, shared_str WName, SBullet* pBullet);
};

using ABULLETS = xr_vector<BulletData>;
using ABULLETS_it = ABULLETS::iterator;

struct victims_table
{
    static u32 const header_count_size;
    typedef buffer_vector<shared_str> victims_table_t;
    victims_table_t& m_data;
    victims_table(victims_table_t& storage) : m_data(storage){};
    victims_table& operator=(victims_table const& other)
    {
        m_data = other.m_data;
        return *this;
    }
    u8 get_id_by_name(shared_str const& player_name) const;
    shared_str get_name_by_id(u8 id) const;
    bool add_name(shared_str const& player_name);

    void net_save(NET_Packet* P);
    void net_load(NET_Packet* P);
};

struct bone_table
{
    static u32 const header_count_size;
    typedef buffer_vector<std::pair<shared_str, s16>> bone_table_t;
    bone_table_t& m_data;
    bone_table(bone_table_t& storage) : m_data(storage){};
    bone_table& operator=(bone_table const& other)
    {
        m_data = other.m_data;
        return *this;
    }
    shared_str get_name_by_id(s16 id) const;
    bool add_bone(shared_str const& bone_name, s16 bone_id);

    void net_save(NET_Packet* P);
    void net_load(NET_Packet* P);
};

struct HitData
{
    Fvector Pos0;
    Fvector Pos1;

    s16 BoneID;
    shared_str BoneName;
    u16 TargetID;
    shared_str TargetName;
    u32 BulletID;
    bool Deadly;
    u8 count;

    bool Completed;

    void net_save(NET_Packet* P, victims_table const& vt, bone_table const& bt);
    void net_load(NET_Packet* P, victims_table const& vt, bone_table const& bt);
    static const u32 net_packet_size;

    bool operator==(u32 ID) { return ID == BulletID; };
    bool operator!=(u32 ID) { return ID != BulletID; };
    //-----------------------------------------------------------
    void Write(FILE* pFile);
    void WriteLtx(CInifile& ini, LPCSTR sect, LPCSTR perfix);
};

using HITS_VEC = xr_vector<HitData>;
using HITS_VEC_it = HITS_VEC::iterator;

#define MAX_BASKET 34
struct Weapon_Statistic
{
    static u32 const net_packet_size;
    shared_str WName;
    shared_str InvName;
    u32 NumBought;
    //---------------------------
    u32 m_dwRoundsFired, m_dwRoundsFired_d;
    u32 m_dwBulletsFired, m_dwBulletsFired_d;
    u32 m_dwHitsScored, m_dwHitsScored_d;
    u32 m_dwKillsScored, m_dwKillsScored_d;
    u16 m_explosion_kills;
    u16 m_bleed_kills;
    //---------------------------
    u32 m_Basket[STAT_TEAM_COUNT][MAX_BASKET];

    // u32				m_dwNumCompleted;
    HITS_VEC m_Hits;
    void add_hit(HitData const& hit);

    Weapon_Statistic(LPCSTR Name);
    ~Weapon_Statistic();

    void net_save(NET_Packet* P, victims_table const& vt, bone_table const& bt);
    void net_load(NET_Packet* P, victims_table const& vt, bone_table const& bt);

    bool FindHit(u32 BulletID, HITS_VEC_it& Hit_it);
    bool operator==(LPCSTR name)
    {
        int res = xr_strcmp(WName.c_str(), name);
        return res == 0;
    }
    //-----------------------------------------------------------
    void Write(FILE* pFile);
    void WriteLtx(CInifile& ini, LPCSTR sect);
};

using WEAPON_STATS = xr_vector<Weapon_Statistic>;
using WEAPON_STATS_it = WEAPON_STATS::iterator;

struct Player_Statistic
{
    shared_str PName;
    shared_str PDigest;
    u32 PID;

    u32 m_dwTotalShots;
    u32 m_dwTotalShots_d;
    //-----------------------------------------------
    u32 last_alive_update_time;
    u32 m_dwTotalAliveTime[STAT_TEAM_COUNT];
    s32 m_dwTotalMoneyRound[STAT_TEAM_COUNT];
    u32 m_dwNumRespawned[STAT_TEAM_COUNT];
    u8 m_dwArtefacts[STAT_TEAM_COUNT];

    u32 m_dwSpecialKills[4]; // headshot, backstab, knifekill, eyeshot

    u8 m_dwCurrentTeam;

    WEAPON_STATS aWeaponStats;
    //-----------------------------------------------
    u32 m_dwCurMoneyRoundDelta;

    Player_Statistic(LPCSTR Name);
    ~Player_Statistic();

    WEAPON_STATS_it FindPlayersWeapon(LPCSTR WeaponName);

    void net_save(NET_Packet* P);
    void net_load(NET_Packet* P);

    bool operator==(LPCSTR name)
    {
        int res = xr_strcmp(PName.c_str(), name);
        return res == 0;
    }
    //-----------------------------------------------------------
    void Write(FILE* pFile);
    void WriteLtx(CInifile& ini, LPCSTR sect);

    u32 create_victims_table(victims_table& victims_table); // retutns size in bytes of table
    u32 create_bone_table(bone_table& bone_table); // retutns size in bytes of table
};

using PLAYERS_STATS = xr_vector<Player_Statistic>;
using PLAYERS_STATS_it = PLAYERS_STATS::iterator;

struct Bullet_Check_Request
{
    u32 BulletID;
    s16 BoneID;
    bool Result;
    bool Processed;
    Bullet_Check_Request() : BulletID(0), BoneID(0), Result(false), Processed(false) {}
    Bullet_Check_Request(u32 ID, s16 BID) : BulletID(ID), BoneID(BID), Result(false), Processed(false) {}
};

using BChR = xr_vector<Bullet_Check_Request>;

struct Bullet_Check_Array
{
    u32 SenderID;

    BChR Requests;
    u8 NumTrue;
    u8 NumFalse;

    bool operator==(u32 ID) { return ID == SenderID; }
    bool operator!=(u32 ID) { return ID != SenderID; }
    Bullet_Check_Array(u32 ID) : SenderID(ID)
    {
        Requests.clear();
        NumTrue = 0;
        NumFalse = 0;
    };
    ~Bullet_Check_Array() { Requests.clear(); };
};

using BChA = xr_vector<Bullet_Check_Array>;

struct WeaponUsageStatistic
{
    bool m_bCollectStatistic;
    bool CollectData() { return m_bCollectStatistic; };
    void SetCollectData(bool Collect);
    //-----------------------------------------------
    ABULLETS ActiveBullets;
    //-----------------------------------------------
    PLAYERS_STATS aPlayersStatistic;
    //-----------------------------------------------
    u32 m_dwTotalPlayersAliveTime[STAT_TEAM_COUNT];
    s32 m_dwTotalPlayersMoneyRound[STAT_TEAM_COUNT];
    u32 m_dwTotalNumRespawns[STAT_TEAM_COUNT];
    //-----------------------------------------------
    u32 m_dwLastUpdateTime;
    u32 m_dwUpdateTimeDelta;
    //-----------------------------------------------
    WeaponUsageStatistic();
    ~WeaponUsageStatistic();

    void Clear();

    PLAYERS_STATS_it FindPlayer(LPCSTR PlayerName);
    bool GetPlayer(LPCSTR PlayerName, PLAYERS_STATS_it& pPlayerI);
    void ChangePlayerName(LPCSTR from, LPCSTR to);
    u8 ConvertToTeamIndex(s16 team);

    bool FindBullet(u32 BulletID, ABULLETS_it& Bullet_it);
    void RemoveBullet(ABULLETS_it& Bullet_it);
    //-----------------------------------------------
    void OnWeaponBought(game_PlayerState* ps, LPCSTR WeaponName);
    void OnBullet_Fire(SBullet* pBullet, const CCartridge& cartridge);
    virtual void OnBullet_Hit(SBullet* pBullet, u16 TargetID, s16 element, Fvector HitLocation);
    void OnBullet_Remove(SBullet* pBullet);
    //-----------------------------------------------

    u32 m_dwLastRequestSenderID;

    BChA m_Requests;

    void OnBullet_Check_Request(SHit* pHDS);
    void OnBullet_Check_Result(bool Result);
    //-----------------------------------------------
    void Send_Check_Respond();
    void On_Check_Respond(NET_Packet* P);

    void OnPlayerKilled(game_PlayerState* ps);

    void SVUpdateAliveTimes();

    virtual void OnPlayerSpawned(game_PlayerState* ps);
    void OnPlayerAddMoney(game_PlayerState* ps, s32 MoneyAmount);
    virtual void OnPlayerBringArtefact(game_PlayerState* ps);
    void OnPlayerKillPlayer(game_PlayerState* ps, KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType);
    virtual void OnExplosionKill(game_PlayerState* ps, const SHit& hit);
    void OnBleedKill(game_PlayerState* killer_ps, game_PlayerState* victim_ps, u16 weapon_id);
    //-----------------------------------------------
    void Update();
    void OnUpdateRequest(NET_Packet* P);
    void OnUpdateRespond(NET_Packet* P, shared_str const& sender_digest, u32 sender_pid);
    //-----------------------------------------------
    string_path mFileName;
    void SaveData();
    void Write(FILE* pFile);

    void SaveDataLtx(CInifile& ini);
    void WriteLtx(CInifile& ini);

private:
    Lock m_mutex;
};

struct Bullet_Check_Respond_True
{
    u32 BulletID;
    s16 BoneID;
};
