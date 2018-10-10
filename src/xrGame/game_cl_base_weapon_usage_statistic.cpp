#include "StdAfx.h"
#include "game_cl_base.h"
#include "Level.h"
#include "Weapon.h"
#include "alife_space.h"
#include "Hit.h"
#include "Actor.h"
#include "ExplosiveItem.h"
#include "xrServer.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "Include/xrRender/Kinematics.h"
#include "game_cl_mp.h"
#include "Common/Noncopyable.hpp"

class statistic_sync_quard : private Noncopyable
{
    Lock& m_mutex;

public:
    statistic_sync_quard(Lock& mutex) : m_mutex(mutex) { m_mutex.Enter(); }
    ~statistic_sync_quard() { m_mutex.Leave(); }
};

BulletData::BulletData(shared_str FName, shared_str WName, SBullet* pBullet)
{
    FirerName = FName;
    WeaponName = WName;
    Bullet = *pBullet;
    HitRefCount = 0;
    HitResponds = 0;
    Removed = false;
};

u32 const HitData::net_packet_size = (3 * 2 * sizeof(float) + 1 + 2 + 1 + 1);
void HitData::net_save(NET_Packet* P, victims_table const& vt, bone_table const& bt)
{
    P->w_vec3(Pos0);
    P->w_vec3(Pos1);
    P->w_u8(vt.get_id_by_name(TargetName)); // P->w_stringZ(TargetName);
    // P->w_stringZ(BoneName);
    P->w_s16(BoneID);
    P->w_u8(Deadly ? 1 : 0);
    P->w_u8(count);
};
void HitData::net_load(NET_Packet* P, victims_table const& vt, bone_table const& bt)
{
    P->r_vec3(Pos0);
    P->r_vec3(Pos1);
    TargetName = vt.get_name_by_id(P->r_u8()); // P->r_stringZ(TargetName);
    P->r_s16(BoneID);
    BoneName = bt.get_name_by_id(BoneID); // P->r_stringZ(BoneName);
    Deadly = !!P->r_u8();
    count = P->r_u8();
    Completed = true;
};

Weapon_Statistic::Weapon_Statistic(LPCSTR Name)
{
    WName = Name;
    InvName = NULL;
    NumBought = 0;

    m_dwRoundsFired = m_dwRoundsFired_d = 0;
    m_dwBulletsFired = m_dwBulletsFired_d = 0;
    m_dwHitsScored = m_dwHitsScored_d = 0;
    m_dwKillsScored = m_dwKillsScored_d = 0;
    m_explosion_kills = 0;
    m_bleed_kills = 0;

    ZeroMemory(m_Basket, sizeof(m_Basket));
};

Weapon_Statistic::~Weapon_Statistic(){};

u32 const Weapon_Statistic::net_packet_size = 5 * sizeof(u32);
void Weapon_Statistic::net_save(NET_Packet* P, victims_table const& vt, bone_table const& bt)
{
    class CompleteFilter
    {
        u32& complete_hits_count;
        NET_Packet* packet_to_write;
        victims_table const& vtable;
        bone_table const& btable;

        CompleteFilter& operator=(CompleteFilter& copy) { return *this; }; // C4512
    public:
        CompleteFilter(u32& hits_count_result, NET_Packet* P, victims_table const& vt, bone_table const& bt)
            : complete_hits_count(hits_count_result), packet_to_write(P), vtable(vt), btable(bt){};
        bool operator()(HitData& hit)
        {
            if (hit.Completed)
            {
                if (NET_PacketSizeLimit - packet_to_write->w_tell() < HitData::net_packet_size)
                    return false;
                hit.net_save(packet_to_write, vtable, btable);
                ++complete_hits_count;
                return true;
            }
            return false;
        }
    };
    m_dwRoundsFired_d = m_dwRoundsFired - m_dwRoundsFired_d;
    P->w_u32(m_dwRoundsFired_d);
    m_dwRoundsFired_d = m_dwRoundsFired;
    P->w_u32(m_dwBulletsFired_d);
    m_dwBulletsFired_d = 0;
    P->w_u32(m_dwHitsScored_d);
    m_dwHitsScored_d = 0;
    P->w_u32(m_dwKillsScored_d);
    m_dwKillsScored_d = 0;

    u32 hit_count_position = P->w_tell();
    u32 complete_hits = 0;
    P->w_u32(0); // <- complete_hits_count

    // P->w_u16(m_explosion_kills);
    // P->w_u16(m_bleed_kills);

    CompleteFilter tmp_filter(complete_hits, P, vt, bt);
    m_Hits.erase(std::remove_if(m_Hits.begin(), m_Hits.end(), tmp_filter), m_Hits.end());
    P->w_seek(hit_count_position, &complete_hits, sizeof(complete_hits));
};

void Weapon_Statistic::net_load(NET_Packet* P, victims_table const& vt, bone_table const& bt)
{
    m_dwRoundsFired += P->r_u32();
    m_dwBulletsFired += P->r_u32();
    m_dwHitsScored += P->r_u32();
    m_dwKillsScored += P->r_u32();
    u32 HitsSize = P->r_u32();
    // P->r_u16(m_explosion_kills); //server sets this parameter ..
    // P->r_u16(m_bleed_kills);
    for (u32 i = 0; i < HitsSize; i++)
    {
        HitData NewHit;
        NewHit.net_load(P, vt, bt);
        m_Hits.push_back(NewHit);
    }
};

u32 const victims_table::header_count_size = sizeof(u8);
u8 victims_table::get_id_by_name(shared_str const& player_name) const
{
    u8 index = 0;
    for (victims_table_t::const_iterator i = m_data.begin(), ie = m_data.end(); i != ie; ++i)
    {
        if (*i == player_name)
            return index;
        ++index;
    }
    return 0;
}

shared_str victims_table::get_name_by_id(u8 id) const
{
    size_t name_index = static_cast<size_t>(id);
    if (name_index > m_data.size())
        return shared_str();
    return m_data[name_index];
}

bool victims_table::add_name(shared_str const& player_name)
{
    if (m_data.size() > 254)
    {
        Msg("! WARNING: victims table in statistics exceeds limit count");
        return false;
    }
    victims_table_t::const_iterator tmp_iter = std::find(m_data.begin(), m_data.end(), player_name);
    if (tmp_iter != m_data.end())
        return false;
    m_data.push_back(player_name);
    return true;
}

void victims_table::net_save(NET_Packet* P)
{
    P->w_u8(static_cast<u8>(m_data.size()));
    for (victims_table_t::const_iterator i = m_data.begin(), ie = m_data.end(); i != ie; ++i)
    {
        P->w_stringZ(*i);
    }
}

void victims_table::net_load(NET_Packet* P)
{
    m_data.clear();
    size_t table_size = static_cast<size_t>(P->r_u8());
    for (size_t i = 0; i < table_size; ++i)
    {
        shared_str tmp_string;
        P->r_stringZ(tmp_string);
        m_data.push_back(tmp_string);
    }
}

u32 const bone_table::header_count_size = sizeof(u16);

class bone_id_searcher
{
    s16 const& id;
    bone_id_searcher& operator=(bone_id_searcher& copy) { return *this; };
public:
    bone_id_searcher(s16 const& id_to_search) : id(id_to_search) {}
    bool operator()(bone_table::bone_table_t::value_type const& item)
    {
        if (item.second == id)
            return true;
        return false;
    }
};

shared_str bone_table::get_name_by_id(s16 id) const
{
    bone_table_t::const_iterator tmp_iter = std::find_if(m_data.begin(), m_data.end(), bone_id_searcher(id));
    if (tmp_iter == m_data.end())
        return shared_str();
    return tmp_iter->first;
}

bool bone_table::add_bone(shared_str const& bone_name, s16 bone_id)
{
    class bone_name_searcher
    {
        shared_str const& name;
        bone_name_searcher& operator=(bone_name_searcher& copy) { return *this; };
    public:
        bone_name_searcher(shared_str const& name_to_search) : name(name_to_search) {}
        bool operator()(bone_table::bone_table_t::value_type const& item)
        {
            if (item.first == name)
                return true;
            return false;
        }
    };
    bone_table_t::const_iterator tmp_iter = std::find_if(m_data.begin(), m_data.end(), bone_name_searcher(bone_name));
    if (tmp_iter != m_data.end())
        return false;
    tmp_iter = std::find_if(m_data.begin(), m_data.end(), bone_id_searcher(bone_id));
    if (tmp_iter != m_data.end())
        return false;
    m_data.push_back(std::make_pair(bone_name, bone_id));
    return true;
}

void bone_table::net_save(NET_Packet* P)
{
    P->w_u16(static_cast<u16>(m_data.size()));
    for (bone_table_t::const_iterator i = m_data.begin(), ie = m_data.end(); i != ie; ++i)
    {
        P->w_stringZ(i->first);
        P->w_s16(i->second);
    }
}

void bone_table::net_load(NET_Packet* P)
{
    m_data.clear();
    u16 bones_count = P->r_u16();
    for (u16 i = 0; i < bones_count; ++i)
    {
        shared_str tmp_string;
        s16 tmp_bone_id;
        P->r_stringZ(tmp_string);
        P->r_s16(tmp_bone_id);
        m_data.push_back(std::make_pair(tmp_string, tmp_bone_id));
    }
}

Player_Statistic::Player_Statistic(LPCSTR Name)
    : last_alive_update_time(0), m_dwCurMoneyRoundDelta(0)
{
    PName = Name;
    PID = 0;
    m_dwTotalShots = 0;
    m_dwTotalShots_d = 0;
    m_dwCurrentTeam = 0;

    ZeroMemory(m_dwTotalAliveTime, sizeof(m_dwTotalAliveTime));
    ZeroMemory(m_dwTotalMoneyRound, sizeof(m_dwTotalMoneyRound));
    ZeroMemory(m_dwNumRespawned, sizeof(m_dwNumRespawned));
    ZeroMemory(m_dwArtefacts, sizeof(m_dwArtefacts));
    ZeroMemory(m_dwSpecialKills, sizeof(m_dwSpecialKills));
}

Player_Statistic::~Player_Statistic() { aWeaponStats.clear(); };
u32 Player_Statistic::create_victims_table(victims_table& victims_table)
{
    u32 result_size = victims_table::header_count_size;
    victims_table.m_data.clear();
    for (u32 wi = 0, weapons_size = aWeaponStats.size(); wi < weapons_size; ++wi)
    {
        Weapon_Statistic& WS = aWeaponStats[wi];
        for (u32 hi = 0, hits_count = WS.m_Hits.size(); hi < hits_count; ++hi)
        {
            HitData& Hit = WS.m_Hits[hi];
            if (victims_table.add_name(Hit.TargetName))
                result_size += Hit.TargetName.size() + 1;
        }
    }
    return result_size;
}

u32 Player_Statistic::create_bone_table(bone_table& bone_table)
{
    u32 result_size = bone_table::header_count_size;
    bone_table.m_data.clear();
    for (u32 wi = 0, weapons_size = aWeaponStats.size(); wi < weapons_size; ++wi)
    {
        Weapon_Statistic& WS = aWeaponStats[wi];
        for (u32 hi = 0, hits_count = WS.m_Hits.size(); hi < hits_count; ++hi)
        {
            HitData& Hit = WS.m_Hits[hi];
            if (bone_table.add_bone(Hit.BoneName, Hit.BoneID))
                result_size += Hit.BoneName.size() + 1 + sizeof(s16);
        }
    }
    return result_size;
}

void Player_Statistic::net_save(NET_Packet* P)
{
    P->w_u32(m_dwTotalShots_d);
    m_dwTotalShots_d = 0;
    P->w_u32(aWeaponStats.size());

    victims_table::victims_table_t vt_storage(_alloca(sizeof(victims_table::victims_table_t::value_type) * 255), 255);
    bone_table::bone_table_t bt_storage(_alloca(sizeof(bone_table::bone_table_t::value_type) * 65), 65);

    victims_table vict_table(vt_storage);
    bone_table bone_table(bt_storage);

    u32 tables_size = create_victims_table(vict_table);
    tables_size += create_bone_table(bone_table);
    if (NET_PacketSizeLimit - P->w_tell() < tables_size + Weapon_Statistic::net_packet_size)
        return;

    vict_table.net_save(P);
    bone_table.net_save(P);

    for (u32 i = 0; i < aWeaponStats.size(); i++)
    {
        Weapon_Statistic& WS = aWeaponStats[i];
        P->w_stringZ(WS.WName);
        WS.net_save(P, vict_table, bone_table);
    }
};

void Player_Statistic::net_load(NET_Packet* P)
{
    m_dwTotalShots += P->r_u32();
    u32 NumWeapons = P->r_u32();

    victims_table::victims_table_t vt_storage(_alloca(sizeof(victims_table::victims_table_t::value_type) * 255), 255);
    bone_table::bone_table_t bt_storage(_alloca(sizeof(bone_table::bone_table_t::value_type) * 65), 65);

    victims_table vict_table(vt_storage);
    bone_table bone_table(bt_storage);

    vict_table.net_load(P);
    bone_table.net_load(P);

    for (u32 i = 0; i < NumWeapons; i++)
    {
        shared_str WName;
        P->r_stringZ(WName);
        auto tmp_wst_it = FindPlayersWeapon(WName.c_str());
        R_ASSERT(tmp_wst_it != aWeaponStats.end());
        tmp_wst_it->net_load(P, vict_table, bone_table);
    }
};

WeaponUsageStatistic::WeaponUsageStatistic()
#ifdef CONFIG_PROFILE_LOCKS
    : m_mutex(MUTEX_PROFILE_ID(WeaponUsageStatistic))
{
}
#endif // CONFIG_PROFILE_LOCKS
{
    Clear();
    m_dwUpdateTimeDelta = 30000; // 30 seconds
    m_bCollectStatistic = false;
};

void WeaponUsageStatistic::Clear()
{
    statistic_sync_quard syncg(m_mutex);
    ActiveBullets.clear();
    aPlayersStatistic.clear();
    m_Requests.clear();
    m_dwLastRequestSenderID = 0;

    ZeroMemory(m_dwTotalPlayersAliveTime, sizeof(m_dwTotalPlayersAliveTime));
    ZeroMemory(m_dwTotalPlayersMoneyRound, sizeof(m_dwTotalPlayersMoneyRound));
    ZeroMemory(m_dwTotalNumRespawns, sizeof(m_dwTotalNumRespawns));

    m_dwLastUpdateTime = Level().timeServer();
    mFileName[0] = 0;
};

WeaponUsageStatistic::~WeaponUsageStatistic() { m_dwLastRequestSenderID = 0; };
bool WeaponUsageStatistic::GetPlayer(LPCSTR PlayerName, PLAYERS_STATS_it& pPlayerI)
{
    statistic_sync_quard syncg(m_mutex);
    pPlayerI = std::find(aPlayersStatistic.begin(), aPlayersStatistic.end(), PlayerName);
    if (pPlayerI == aPlayersStatistic.end() || !((*pPlayerI) == PlayerName))
        return false;
    return true;
}

PLAYERS_STATS_it WeaponUsageStatistic::FindPlayer(LPCSTR PlayerName)
{
    statistic_sync_quard syncg(m_mutex);
    PLAYERS_STATS_it pPlayerI;
    if (!GetPlayer(PlayerName, pPlayerI))
    {
#ifdef DEBUG
        Msg("--- Adding new player [%s] to statistics", PlayerName);
#endif // #ifdef DEBUG
        aPlayersStatistic.push_back(Player_Statistic(PlayerName));
        pPlayerI = aPlayersStatistic.end() - 1;
    };
    return pPlayerI;
};

void WeaponUsageStatistic::ChangePlayerName(LPCSTR from, LPCSTR to)
{
    statistic_sync_quard syncg(m_mutex);
    if (!CollectData())
        return;
    auto pPlayerI = FindPlayer(from);
    pPlayerI->PName = to;
}

WEAPON_STATS_it Player_Statistic::FindPlayersWeapon(LPCSTR WeaponName)
{
    R_ASSERT(WeaponName);
    auto pWeaponI = std::find(aWeaponStats.begin(), aWeaponStats.end(), WeaponName);
    if (pWeaponI == aWeaponStats.end() || !((*pWeaponI) == WeaponName))
    {
        aWeaponStats.push_back(Weapon_Statistic(WeaponName));
        pWeaponI = aWeaponStats.end() - 1;
        pWeaponI->InvName = pSettings->r_string_wb(WeaponName, "inv_name");
#ifdef DEBUG
        Msg("--- Just added weapon %s to statistics", WeaponName);
#endif // #ifdef DEBUG
    }
    return pWeaponI;
};

bool WeaponUsageStatistic::FindBullet(u32 BulletID, ABULLETS_it& Bullet_It)
{
    Bullet_It = std::find(ActiveBullets.begin(), ActiveBullets.end(), BulletID);
    if (Bullet_It == ActiveBullets.end() || (*Bullet_It) != BulletID)
        return false;
    return true;
}

bool Weapon_Statistic::FindHit(u32 BulletID, HITS_VEC_it& Hit_it)
{
    Hit_it = std::find(m_Hits.begin(), m_Hits.end(), BulletID);
    if (Hit_it == m_Hits.end() || (*Hit_it) != BulletID)
        return false;
    return true;
};

void WeaponUsageStatistic::RemoveBullet(ABULLETS_it& Bullet_it)
{
    statistic_sync_quard syncg(m_mutex);
    if (!Bullet_it->Removed || Bullet_it->HitRefCount != Bullet_it->HitResponds)
        return;
    //-------------------------------------------------------------
    auto PlayerIt = FindPlayer(*(Bullet_it->FirerName));
    auto WeaponIt = PlayerIt->FindPlayersWeapon(*(Bullet_it->WeaponName));
    HITS_VEC_it HitIt;
    if (WeaponIt->FindHit(Bullet_it->Bullet.m_dwID, HitIt))
    {
        HitIt->Completed = true;
    };
    //-------------------------------------------------------------
    *Bullet_it = ActiveBullets.back();
    ActiveBullets.pop_back();
}

void WeaponUsageStatistic::OnWeaponBought(game_PlayerState* ps, LPCSTR WeaponName)
{
    statistic_sync_quard syncg(m_mutex);
    if (!CollectData())
        return;
    if (!ps)
        return;
    auto PlayerIt = FindPlayer(ps->getName());
    auto WeaponIt = PlayerIt->FindPlayersWeapon(WeaponName);
    WeaponIt->NumBought++;
    //-----------------------------------------------
    int BasketPos = 0;
    if (ps->money_for_round > 500)
    {
        BasketPos = (ps->money_for_round - 1) / 1000 + 1;
    };
    u8 team_index = ConvertToTeamIndex(ps->team);
    u8 bascet_pos = static_cast<u8>(BasketPos);
    if ((team_index >= STAT_TEAM_COUNT) || (bascet_pos >= MAX_BASKET))
    {
        return;
    }
    WeaponIt->m_Basket[team_index][bascet_pos]++;
};

void WeaponUsageStatistic::OnBullet_Fire(SBullet* pBullet, const CCartridge& cartridge)
{
    statistic_sync_quard syncg(m_mutex);
    if (!CollectData())
        return;

    if (!pBullet || !pBullet->flags.allow_sendhit)
        return;
    IGameObject* object_weapon = Level().Objects.net_Find(pBullet->weapon_id);
    if (!object_weapon)
        return;
    IGameObject* object_parent = Level().Objects.net_Find(pBullet->parent_id);
    if (!object_parent)
        return;
    CActor* pActor = smart_cast<CActor*>(object_parent);
    if (!pActor)
        return;
    //-----------------------------------------------------------------------------------
    auto PlayerIt = FindPlayer(*object_parent->cName());
    pBullet->m_dwID = PlayerIt->m_dwTotalShots++;
    PlayerIt->m_dwTotalShots_d++;
    auto WeaponIt = PlayerIt->FindPlayersWeapon(*object_weapon->cNameSect());
    WeaponIt->m_dwRoundsFired = (++WeaponIt->m_dwBulletsFired) / cartridge.param_s.buckShot;
    WeaponIt->m_dwBulletsFired_d++;
    //-----------------------------------------------------------------------------------
    ActiveBullets.push_back(BulletData(object_parent->cName(), object_weapon->cNameSect(), pBullet));

    //	Msg("! OnBullet Fire ID[%d]", pBullet->m_dwID);
}

void WeaponUsageStatistic::OnBullet_Hit(SBullet* pBullet, u16 TargetID, s16 element, Fvector HitLocation)
{
    statistic_sync_quard syncg(m_mutex);
    if (!pBullet || !pBullet->flags.allow_sendhit)
        return;
    //	Msg("! OnBullet Hit ID[%d]", pBullet->m_dwID);
    ABULLETS_it BulletIt;
    if (!FindBullet(pBullet->m_dwID, BulletIt))
        return;
    //-----------------------------------------------------
    auto PlayerIt = FindPlayer(*(BulletIt->FirerName));
    auto WeaponIt = PlayerIt->FindPlayersWeapon(*(BulletIt->WeaponName));
    if (!BulletIt->HitRefCount++)
    {
        WeaponIt->m_dwHitsScored++;
        WeaponIt->m_dwHitsScored_d++;
        //---------------------------
        IGameObject* pTarget = Level().Objects.net_Find(TargetID);
        if (!pTarget)
            return;
        CActor* pActor = smart_cast<CActor*>(pTarget);
        if (!pActor)
            return;
        //---------------------------
        BulletData& BD = *BulletIt;
        HitData NewHit;
        //---------------------------
        NewHit.Completed = false;
        NewHit.Deadly = false;
        NewHit.BoneID = element;
        NewHit.TargetID = TargetID;
        NewHit.BulletID = BD.Bullet.m_dwID;
        NewHit.Pos0 = BD.Bullet.bullet_pos;
        NewHit.Pos1 = HitLocation;
        NewHit.TargetName = pTarget->cName();
        NewHit.BoneName = smart_cast<IKinematics*>(pTarget->Visual())->LL_BoneName_dbg(element);
        NewHit.count = 1;
        //---------------------------
        WeaponIt->add_hit(NewHit);
    };
}

void WeaponUsageStatistic::OnBullet_Remove(SBullet* pBullet)
{
    if (!pBullet || !pBullet->flags.allow_sendhit)
        return;
    ABULLETS_it BulletIt;
    if (!FindBullet(pBullet->m_dwID, BulletIt))
        return;
    //	Msg("! Bullet Removed ID[%d]", BulletIt->Bullet.m_dwID);
    BulletIt->Removed = true;
    RemoveBullet(BulletIt);
}

void WeaponUsageStatistic::OnBullet_Check_Request(SHit* pHDS)
{
    if (!pHDS || OnClient())
        return;
    s16 BoneID = pHDS->bone();
    u32 BulletID = pHDS->BulletID;
    u32 SenderID = pHDS->SenderID;

    auto pSenderI = std::find(m_Requests.begin(), m_Requests.end(), SenderID);
    if (pSenderI == m_Requests.end() || (*pSenderI) != SenderID)
    {
        m_Requests.push_back(Bullet_Check_Array(SenderID));
        pSenderI = m_Requests.end() - 1;
    };

    (*pSenderI).Requests.push_back(Bullet_Check_Request(BulletID, BoneID));
    m_dwLastRequestSenderID = SenderID;

    //	HitChecksReceived++;
};

void WeaponUsageStatistic::OnBullet_Check_Result(bool Result)
{
    if (OnClient())
        return;
    if (m_dwLastRequestSenderID)
    {
        auto pSenderI = std::find(m_Requests.begin(), m_Requests.end(), m_dwLastRequestSenderID);
        if (pSenderI != m_Requests.end() && (*pSenderI) == m_dwLastRequestSenderID)
        {
            (*pSenderI).Requests.back().Result = Result;
            (*pSenderI).Requests.back().Processed = true;
            if (Result)
                (*pSenderI).NumTrue++;
            else
                (*pSenderI).NumFalse++;
        }
        else
        {
            Msg("! Warning can't Find Check!");
            R_ASSERT(0);
        }
        m_dwLastRequestSenderID = 0;
    }
};

void WeaponUsageStatistic::Send_Check_Respond()
{
    if (!OnServer())
        return;
    NET_Packet P;
    string1024 STrue, SFalse;
    for (u32 i = 0; i < m_Requests.size(); i++)
    {
        Bullet_Check_Array& BChA_Request = m_Requests[i];
        if (BChA_Request.Requests.empty())
            continue;
        Bullet_Check_Respond_True* pSTrue = (Bullet_Check_Respond_True*)STrue;
        u32* pSFalse = (u32*)SFalse;
        //-----------------------------------------------------
        u32 NumFalse = 0;
        u32 NumTrue = 0;
        u32 j = 0;
        while (j < BChA_Request.Requests.size())
        {
            Bullet_Check_Request& curBChR = BChA_Request.Requests[j];
            if (!curBChR.Processed)
            {
                j++;
                continue;
            }
            else
            {
                if (curBChR.Result)
                {
                    pSTrue->BulletID = curBChR.BulletID;
                    pSTrue->BoneID = curBChR.BoneID;
                    pSTrue++;
                    //					HitChecksRespondedTrue++;
                    NumTrue++;
                }
                else
                {
                    *(pSFalse++) = curBChR.BulletID;
                    //					HitChecksRespondedFalse++;
                    NumFalse++;
                };
                //				HitChecksResponded++;
                //-----------------------------------------------------
                *(BChA_Request.Requests.begin() + j) = BChA_Request.Requests.back();
                BChA_Request.Requests.pop_back();
            }
        }
        //-----------------------------------------------------
        P.w_begin(M_BULLET_CHECK_RESPOND);
        //		Msg("%d-%d || %d-%d", NumFalse, BChA_Request.NumFalse, NumTrue, BChA_Request.NumTrue);
        P.w_u8(BChA_Request.NumFalse);
        BChA_Request.NumFalse = 0;
        P.w_u8(BChA_Request.NumTrue);
        BChA_Request.NumTrue = 0;

        if ((char*)pSFalse != (char*)SFalse)
            P.w(SFalse, u32((char*)pSFalse - (char*)SFalse));
        if ((char*)pSTrue != (char*)STrue)
            P.w(STrue, u32((char*)pSTrue - (char*)STrue));
        //-----------------------------------------------------
        ClientID ClID;
        ClID.set(BChA_Request.SenderID);
        if (Level().Server)
            Level().Server->SendTo(ClID, P);
    };
}

void WeaponUsageStatistic::On_Check_Respond(NET_Packet* P)
{
    statistic_sync_quard syncg(m_mutex);
    if (!P)
        return;
    u8 NumFalse = P->r_u8();
    u8 NumTrue = P->r_u8();

    u8 i;
    ABULLETS_it BulletIt;
    for (i = 0; i < NumFalse; i++)
    {
        u32 BulletID = P->r_u32();
        if (!FindBullet(BulletID, BulletIt))
        {
            Msg("! Warning: No bullet found! ID[%d]", BulletID);
            continue;
        };
        BulletIt->HitResponds++;
        RemoveBullet(BulletIt);
    }

    for (i = 0; i < NumTrue; i++)
    {
        u32 BulletID = P->r_u32();
        s16 BoneID = P->r_s16();
        if (!FindBullet(BulletID, BulletIt))
        {
            Msg("! Warning: No bullet found! ID[%d]", BulletID);
            continue;
        };
        BulletIt->HitResponds++;

        //---------------------------------------------------------------
        auto PlayerIt = FindPlayer(*(BulletIt->FirerName));
        auto WeaponIt = PlayerIt->FindPlayersWeapon(*(BulletIt->WeaponName));
        (*WeaponIt).m_dwKillsScored++;
        (*WeaponIt).m_dwKillsScored_d++;

        HITS_VEC_it HitIt;
        if (WeaponIt->FindHit(BulletID, HitIt))
        {
            HitData& HData = *HitIt;
            HData.Deadly = true;
            HData.BoneID = BoneID;
            IGameObject* pObj = Level().Objects.net_Find(HData.TargetID);

            if (pObj)
                HData.BoneName = smart_cast<IKinematics*>(pObj->Visual())->LL_BoneName_dbg(BoneID);
        }
        //---------------------------------------------------------------
        RemoveBullet(BulletIt);
    }
};

void WeaponUsageStatistic::OnPlayerBringArtefact(game_PlayerState* ps)
{
    if (!CollectData())
        return;
    if (!ps)
        return;
    Player_Statistic& PlayerStat = *(FindPlayer(ps->getName()));

    PlayerStat.m_dwArtefacts[ConvertToTeamIndex(ps->team)]++;
}

void WeaponUsageStatistic::OnPlayerSpawned(game_PlayerState* ps)
{
    if (!CollectData())
        return;
    if (!ps)
        return;
    Player_Statistic& PlayerStat = *(FindPlayer(ps->getName()));
    PlayerStat.m_dwNumRespawned[ConvertToTeamIndex(ps->team)]++;
    PlayerStat.m_dwCurMoneyRoundDelta = 0;
    m_dwTotalNumRespawns[ConvertToTeamIndex(ps->team)]++;
    PlayerStat.m_dwCurrentTeam = ConvertToTeamIndex(ps->team);
    PlayerStat.last_alive_update_time = Device.dwTimeGlobal;
}

void WeaponUsageStatistic::OnPlayerAddMoney(game_PlayerState* ps, s32 MoneyAmount)
{
    if (!CollectData())
        return;
    if (!ps || MoneyAmount <= 0)
        return;
    Player_Statistic& PlayerStat = *(FindPlayer(ps->getName()));
    PlayerStat.m_dwCurMoneyRoundDelta += MoneyAmount;
};

void WeaponUsageStatistic::OnPlayerKillPlayer(
    game_PlayerState* ps, KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType)
{
    if (!CollectData())
        return;
    if (!ps)
        return;

    Player_Statistic& PlayerStat = *(FindPlayer(ps->getName()));

    //.	m_dwSpecialKills[0];//headshot, backstab, knifekill
    switch (SpecialKillType)
    {
    case SKT_HEADSHOT: PlayerStat.m_dwSpecialKills[0]++; break;
    case SKT_BACKSTAB: PlayerStat.m_dwSpecialKills[1]++; break;
    case SKT_KNIFEKILL: PlayerStat.m_dwSpecialKills[2]++; break;
    case SKT_EYESHOT: PlayerStat.m_dwSpecialKills[3]++; break;
    };
}

void WeaponUsageStatistic::OnExplosionKill(game_PlayerState* ps, const SHit& hit)
{
    if (!CollectData())
        return;
    if (!ps)
        return;
    if (!OnServer())
        return;

    statistic_sync_quard syncg(m_mutex);

    IGameObject* killer = hit.who;
    if (!killer)
        return;

    u16 killer_id = hit.whoID;
    game_PlayerState* killerPS = Game().GetPlayerByGameID(killer_id);
    if (!killerPS)
        return;

    Player_Statistic& PlayerStatKiller = *(FindPlayer(killerPS->getName()));

    IGameObject* weapon_object = Level().Objects.net_Find(hit.weaponID);
    auto WeaponIt = PlayerStatKiller.FindPlayersWeapon(weapon_object->cNameSect().c_str());
    ++WeaponIt->m_dwHitsScored;
    ++WeaponIt->m_dwKillsScored;
    ++WeaponIt->m_explosion_kills;

    HitData NewHit;
    //---------------------------
    NewHit.Completed = true;
    NewHit.Deadly = true;
    NewHit.BoneID = hit.boneID;
    NewHit.TargetID = ps->GameID;
    NewHit.BulletID = 0;
    NewHit.Pos0 = killer->Position();
    NewHit.Pos1 = weapon_object->Position();
    NewHit.TargetName = ps->getName();
    NewHit.BoneName = 0;
    NewHit.count = 1;
    //---------------------------
    WeaponIt->add_hit(NewHit);
}

void WeaponUsageStatistic::OnBleedKill(game_PlayerState* killer_ps, game_PlayerState* victim_ps, u16 weapon_id)
{
    statistic_sync_quard syncg(m_mutex);
    if (!killer_ps || !victim_ps)
        return;
    Player_Statistic& PlayerStatKiller = *(FindPlayer(killer_ps->getName()));
    IGameObject* weapon_object = Level().Objects.net_Find(weapon_id);
    if (!weapon_object)
        return;

    auto WeaponIt = PlayerStatKiller.FindPlayersWeapon(weapon_object->cNameSect().c_str());

    ++WeaponIt->m_dwHitsScored;
    ++WeaponIt->m_dwKillsScored;
    ++WeaponIt->m_bleed_kills;

    HitData NewHit;
    //---------------------------
    NewHit.Completed = true;
    NewHit.Deadly = true;
    NewHit.BoneID = 0;
    NewHit.TargetID = victim_ps->GameID;
    NewHit.BulletID = 0;
    NewHit.Pos0 = Fvector3();
    NewHit.Pos1 = Fvector3();
    NewHit.TargetName = victim_ps->getName();
    NewHit.BoneName = 0;
    NewHit.count = 1;
    //---------------------------
    WeaponIt->add_hit(NewHit);
}

u8 WeaponUsageStatistic::ConvertToTeamIndex(s16 team)
{
    game_cl_mp* cl_game = static_cast<game_cl_mp*>(&Game());
    s16 team_index = cl_game->ModifyTeam(team);
    if (Game().Type() == eGameIDTeamDeathmatch)
    {
        if (team_index == -1)
        {
            //			Msg("! ERROR: can't process spectators in deathmatch statistics.");
            return 1;
        }
    }
    else
    {
        if ((team_index == etSpectatorsTeam) || (team_index == -1))
        {
            team_index = 0;
        }
        else
        {
            ++team_index;
        }
    }
    return static_cast<u8>(team_index);
}

void WeaponUsageStatistic::OnPlayerKilled(game_PlayerState* ps)
{
    if (!CollectData())
        return;
    if (!ps)
        return;
    u8 team = ConvertToTeamIndex(ps->team);
    Player_Statistic& PlayerStat = *(FindPlayer(ps->getName()));
    PlayerStat.m_dwTotalMoneyRound[team] += PlayerStat.m_dwCurMoneyRoundDelta;
    m_dwTotalPlayersMoneyRound[team] += PlayerStat.m_dwCurMoneyRoundDelta;
};

void WeaponUsageStatistic::SVUpdateAliveTimes()
{
    if (!OnServer())
        return;

    class alive_time_updator
    {
        WeaponUsageStatistic& owner;
        alive_time_updator& operator=(alive_time_updator& copy) { return *this; };
    public:
        alive_time_updator(WeaponUsageStatistic& stats_owner) : owner(stats_owner) {}
        void operator()(IClient* client)
        {
            xrClientData* tmp_client = static_cast<xrClientData*>(client);
            game_PlayerState* ps = tmp_client->ps;
            if (ps && !ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
            {
                if (xr_strlen(ps->getName()))
                {
                    Player_Statistic& pstat = *(owner.FindPlayer(ps->getName()));
                    u8 team = owner.ConvertToTeamIndex(ps->team);
                    u32 time_dif = Device.dwTimeGlobal - pstat.last_alive_update_time;
                    pstat.m_dwTotalAliveTime[team] += time_dif;
                    pstat.last_alive_update_time = Device.dwTimeGlobal;
                }
            }
        }
    };
    alive_time_updator tmp_functor(*this);
    Level().Server->ForEachClientDo(tmp_functor);

    m_dwTotalPlayersAliveTime[0] = 0;
    m_dwTotalPlayersAliveTime[1] = 0;
    m_dwTotalPlayersAliveTime[2] = 0;
    for (u32 player_index = 0, max_players = aPlayersStatistic.size(); player_index < max_players; ++player_index)
    {
        Player_Statistic& pstat = aPlayersStatistic[player_index];
        m_dwTotalPlayersAliveTime[0] += pstat.m_dwTotalAliveTime[0];
        m_dwTotalPlayersAliveTime[1] += pstat.m_dwTotalAliveTime[1];
        m_dwTotalPlayersAliveTime[2] += pstat.m_dwTotalAliveTime[2];
    }
}

void WeaponUsageStatistic::Update()
{
    if (!CollectData())
        return;
    SVUpdateAliveTimes(); // update client alive time and servers total alive times
    if (!OnServer())
        return;
    if (Level().timeServer() > (m_dwLastUpdateTime + m_dwUpdateTimeDelta))
    {
        //---------------------------------------------
        m_dwLastUpdateTime = Level().timeServer();
        //---------------------------------------------
        NET_Packet P;
        P.w_begin(M_STATISTIC_UPDATE);
        P.w_u32(Level().timeServer());
        Level().Send(P);
    }
};

void WeaponUsageStatistic::OnUpdateRequest(NET_Packet*)
{
    if (aPlayersStatistic.empty() || !Game().local_player)
        return;

    statistic_sync_quard syncg(m_mutex);

    game_PlayerState* local_player = Game().local_player;
    if (!xr_strlen(local_player->getName()))
        return;

    Player_Statistic& PS = *(FindPlayer(local_player->getName()));
    //-------------------------------------------------
    NET_Packet P;
    P.w_begin(M_STATISTIC_UPDATE_RESPOND);
    //-------------------------------------------------
    P.w_stringZ(PS.PName);
    PS.net_save(&P);
    //-------------------------------------------------
    Level().Send(P);
};

void WeaponUsageStatistic::OnUpdateRespond(NET_Packet* P, shared_str const& sender_digest, u32 sender_pid)
{
    if (!P)
        return;

    statistic_sync_quard syncg(m_mutex);

    shared_str PName;
    P->r_stringZ(PName);
    Player_Statistic& PS = *(FindPlayer(*PName));
    PS.PDigest = sender_digest;
    PS.PID = sender_pid;
    Msg("--- CL: On Update Respond from [%s]", PName.c_str());
    PS.net_load(P);
};

void WeaponUsageStatistic::SetCollectData(bool Collect)
{
    if (Collect && !m_bCollectStatistic)
        Clear();
    m_bCollectStatistic = Collect;
}
