#include "StdAfx.h"
#include "game_cl_base.h"
#include "Level.h"
#include "game_cl_base_weapon_usage_statistic.h"

#define WUS_IDENT (('S' << 24) + ('U' << 16) + ('W' << 8) + ' ')
#define WUS_VERSION 2
// 1 - initial save
// 2 - added Bone Names

void WeaponUsageStatistic::SaveDataLtx(CInifile& ini)
{
    if (OnClient())
        return;
    if (!CollectData())
        return;
    if (aPlayersStatistic.empty())
        return;

    WriteLtx(ini);
}

void WeaponUsageStatistic::WriteLtx(CInifile& ini)
{
    LPCSTR sect = "wpn_usage";

    ini.w_u32(sect, "dwTotalPlayersAliveTime_0_sec", m_dwTotalPlayersAliveTime[0] / 1000);
    ini.w_u32(sect, "dwTotalPlayersAliveTime_1_sec", m_dwTotalPlayersAliveTime[1] / 1000);
    ini.w_u32(sect, "dwTotalPlayersAliveTime_2_sec", m_dwTotalPlayersAliveTime[2] / 1000);

    ini.w_u32(sect, "dwTotalPlayersMoneyRound_0", m_dwTotalPlayersMoneyRound[0]);
    ini.w_u32(sect, "dwTotalPlayersMoneyRound_1", m_dwTotalPlayersMoneyRound[1]);
    ini.w_u32(sect, "dwTotalPlayersMoneyRound_2", m_dwTotalPlayersMoneyRound[2]);

    ini.w_u32(sect, "dwTotalNumRespawns_0", m_dwTotalNumRespawns[0]);
    ini.w_u32(sect, "dwTotalNumRespawns_1", m_dwTotalNumRespawns[1]);
    ini.w_u32(sect, "dwTotalNumRespawns_2", m_dwTotalNumRespawns[2]);

    u32 NumPlayers = aPlayersStatistic.size();
    u32 validPlayersCount = 0;
    for (u32 i = 0; i < NumPlayers; ++i)
    {
        if (aPlayersStatistic[i].PDigest.size())
        {
            ++validPlayersCount;
        }
    }
    ini.w_u32(sect, "NumPlayers", validPlayersCount);
    u32 playerIndex = 0;
    for (u32 i = 0; i < NumPlayers; i++)
    {
        Player_Statistic& PS = aPlayersStatistic[i];
        string512 save_sect;
        if (PS.PDigest.size())
        {
            xr_sprintf(save_sect, "%s_player_%d", sect, playerIndex);
            PS.WriteLtx(ini, save_sect);
            ++playerIndex;
        }
    }
}

void WeaponUsageStatistic::SaveData()
{
    if (OnClient())
        return;
    if (!CollectData())
        return;
    if (aPlayersStatistic.empty())
        return;

    string64 GameType;
    switch (GameID())
    {
    case eGameIDDeathmatch: xr_sprintf(GameType, "dm"); break;
    case eGameIDTeamDeathmatch: xr_sprintf(GameType, "tdm"); break;
    case eGameIDArtefactHunt: xr_sprintf(GameType, "ah"); break;
    case eGameIDCaptureTheArtefact: xr_sprintf(GameType, "cta"); break;
    default: return; break;
    };
#ifdef WINDOWS
    SYSTEMTIME Time;
    GetLocalTime(&Time);
    xr_sprintf(mFileName, "(%s)_(%s)_%02d.%02d.%02d_%02d.%02d.%02d.wus", *(Level().name()), GameType, Time.wMonth,
        Time.wDay, Time.wYear, Time.wHour, Time.wMinute, Time.wSecond);
#else
    time_t Time;
    time(&Time);
    xr_sprintf(mFileName, "(%s)_(%s)_%s.wus", *(Level().name()), GameType, ctime(&Time));
#endif
    //---------------------------------------------------------
    FS.update_path(mFileName, "$logs$", mFileName);
    FILE* SFile = fopen(mFileName, "wb");
    if (!SFile)
        return;
    //---------------------------------------------------------
    u32 IDENT = WUS_IDENT;
    fwrite(&IDENT, 4, 1, SFile);
    u32 Ver = WUS_VERSION;
    fwrite(&Ver, 4, 1, SFile);
    //---------------------------------------------------------
    Write(SFile);
    //---------------------------------------------------------
    fclose(SFile);
};

void WeaponUsageStatistic::Write(FILE* pFile)
{
    if (!pFile)
        return;
    //---------------------------------------------
    fwrite(m_dwTotalPlayersAliveTime, 4, 3, pFile);
    fwrite(m_dwTotalPlayersMoneyRound, 4, 3, pFile);
    fwrite(m_dwTotalNumRespawns, 4, 3, pFile);
    //----------------------------------------------
    u32 NumPlayers = aPlayersStatistic.size();
    fwrite(&NumPlayers, 4, 1, pFile);
    //----------------------------------------------
    for (u32 i = 0; i < NumPlayers; i++)
    {
        Player_Statistic& PS = aPlayersStatistic[i];
        PS.Write(pFile);
    }
}

void Player_Statistic::WriteLtx(CInifile& ini, LPCSTR sect)
{
    ini.w_string(sect, "name", PName.c_str());
    ini.w_string(sect, "player_unique_digest", PDigest.c_str());
    ini.w_u32(sect, "player_profile_id", PID);

    ini.w_u32(sect, "TotalShots", m_dwTotalShots);

    ini.w_u32(sect, "dwTotalAliveTime_0_sec", m_dwTotalAliveTime[0] / 1000);
    ini.w_u32(sect, "dwTotalAliveTime_1_sec", m_dwTotalAliveTime[1] / 1000);
    ini.w_u32(sect, "dwTotalAliveTime_2_sec", m_dwTotalAliveTime[2] / 1000);

    ini.w_u32(sect, "dwTotalMoneyRound_0", m_dwTotalMoneyRound[0]);
    ini.w_u32(sect, "dwTotalMoneyRound_1", m_dwTotalMoneyRound[1]);
    ini.w_u32(sect, "dwTotalMoneyRound_2", m_dwTotalMoneyRound[2]);

    ini.w_u32(sect, "dwNumRespawned_0", m_dwNumRespawned[0]);
    ini.w_u32(sect, "dwNumRespawned_1", m_dwNumRespawned[1]);
    ini.w_u32(sect, "dwNumRespawned_2", m_dwNumRespawned[2]);

    ini.w_u8(sect, "m_dwArtefacts_0", m_dwArtefacts[0]);
    ini.w_u8(sect, "m_dwArtefacts_1", m_dwArtefacts[1]);
    ini.w_u8(sect, "m_dwArtefacts_2", m_dwArtefacts[2]);

    ini.w_u8(sect, "dwCurrentTeam", m_dwCurrentTeam);

    u32 NumWeapons = aWeaponStats.size();

    ini.w_u32(sect, "NumWeapons", NumWeapons);

    for (u32 i = 0; i < aWeaponStats.size(); i++)
    {
        string512 save_sect;
        xr_sprintf(save_sect, "%s_wpn_%d", sect, i);
        Weapon_Statistic& WS = aWeaponStats[i];
        WS.WriteLtx(ini, save_sect);
    }
}

void Player_Statistic::Write(FILE* pFile)
{
    if (!pFile)
        return;
    //----------------------------------------------
    fwrite(*PName, xr_strlen(PName) + 1, 1, pFile);
    fwrite(&m_dwTotalShots, 4, 1, pFile);
    fwrite(m_dwTotalAliveTime, 4, 3, pFile);
    fwrite(m_dwTotalMoneyRound, 4, 3, pFile);
    fwrite(m_dwNumRespawned, 4, 3, pFile);
    //----------------------------------------------
    u32 NumWeapons = aWeaponStats.size();
    fwrite(&NumWeapons, 4, 1, pFile);
    //----------------------------------------------
    for (u32 i = 0; i < aWeaponStats.size(); i++)
    {
        Weapon_Statistic& WS = aWeaponStats[i];
        WS.Write(pFile);
    }
};

void Weapon_Statistic::WriteLtx(CInifile& ini, LPCSTR sect)
{
    ini.w_string(sect, "wpn_name", WName.c_str());

    ini.w_string(sect, "wpn_inv_name", InvName.c_str());

    ini.w_u32(sect, "wpn_dwNumBought", NumBought);

    ini.w_u32(sect, "wpn_dwRoundsFired", m_dwRoundsFired);

    ini.w_u32(sect, "wpn_dwBulletsFired", m_dwBulletsFired);

    ini.w_u32(sect, "wpn_dwHitsScored", m_dwHitsScored);

    ini.w_u32(sect, "wpn_dwKillsScored", m_dwKillsScored);

    ini.w_u16(sect, "wpn_dwExplosionKills", m_explosion_kills);

    ini.w_u16(sect, "wpn_dwBleedKills", m_bleed_kills);

    //----------------------------------------------
    u32 NumHits = 0;
    u32 i = 0;
    for (i = 0; i < m_Hits.size(); i++)
    {
        HitData& Hit = m_Hits[i];
        if (Hit.Completed && Hit.count)
            NumHits++;
    };

    ini.w_u32(sect, "NumHits", NumHits);

    u32 hits_size = m_Hits.size();
    i = 0;
    u32 hit_number = 0;
    u8 hit_index = 0;
    while (i < hits_size)
    {
        HitData& Hit = m_Hits[i];
        if (!Hit.Completed)
        {
            ++i;
            hit_index = 0;
            continue;
        }

        string512 save_prefix;
        xr_sprintf(save_prefix, "hit_%d_", hit_number);

        Hit.WriteLtx(ini, sect, save_prefix);

        ++hit_index;
        if (hit_index >= Hit.count)
        {
            hit_index = 0;
            ++i;
        }
        ++hit_number;
    };
};

void Weapon_Statistic::Write(FILE* pFile)
{
    if (!pFile)
        return;
    //----------------------------------------------
    fwrite(*WName, xr_strlen(WName) + 1, 1, pFile);
    fwrite(*InvName, xr_strlen(InvName) + 1, 1, pFile);
    //----------------------------------------------
    fwrite(&NumBought, 4, 1, pFile);
    fwrite(&m_dwRoundsFired, 4, 1, pFile);
    fwrite(&m_dwBulletsFired, 4, 1, pFile);
    fwrite(&m_dwHitsScored, 4, 1, pFile);
    fwrite(&m_dwKillsScored, 4, 1, pFile);
    fwrite(m_Basket, 4, 3 * MAX_BASKET, pFile);
    //----------------------------------------------
    u32 NumHits = 0;
    for (u32 i = 0; i < m_Hits.size(); i++)
    {
        HitData& Hit = m_Hits[i];
        if (Hit.Completed)
            NumHits++;
    };
    fwrite(&NumHits, 4, 1, pFile);
    for (u32 i = 0; i < m_Hits.size(); i++)
    {
        HitData& Hit = m_Hits[i];
        if (!Hit.Completed)
            continue;
        Hit.Write(pFile);
    };
};

#define ARCHIVE_HIT_RADIUS 0.5f
// this method searches hit in last 30 hits (magazine size) - optimization
void Weapon_Statistic::add_hit(HitData const& hit)
{
    u32 magazine_size = 30;
    for (HITS_VEC::reverse_iterator i = m_Hits.rbegin(), ie = m_Hits.rend(); i != ie; ++i)
    {
        HitData& tmp_hit = *i;
        if ((tmp_hit.BoneID == hit.BoneID) && (tmp_hit.TargetID == hit.TargetID) &&
            (tmp_hit.Pos0.distance_to(hit.Pos0) < ARCHIVE_HIT_RADIUS) &&
            (tmp_hit.Pos1.distance_to(hit.Pos1) < ARCHIVE_HIT_RADIUS) && (tmp_hit.count < 254))
        {
            ++tmp_hit.count;
            return;
        }
        if (--magazine_size == 0)
            break;
    }
    m_Hits.push_back(hit);
}

void HitData::WriteLtx(CInifile& ini, LPCSTR sect, LPCSTR prefix)
{
    string512 buff;

    ini.w_fvector3(sect, strconcat(sizeof(buff), buff, prefix, "pos_0"), Pos0);
    ini.w_fvector3(sect, strconcat(sizeof(buff), buff, prefix, "pos_1"), Pos1);

    ini.w_u16(sect, strconcat(sizeof(buff), buff, prefix, "BoneID"), BoneID);

    ini.w_bool(sect, strconcat(sizeof(buff), buff, prefix, "Deadly"), Deadly);

    ini.w_string(sect, strconcat(sizeof(buff), buff, prefix, "TargetName"), TargetName.c_str());

    ini.w_string(sect, strconcat(sizeof(buff), buff, prefix, "BoneName"), BoneName.c_str());
};

void HitData::Write(FILE* pFile)
{
    if (!pFile)
        return;
    //----------------------------------------------
    fwrite(&Pos0.x, 4, 1, pFile);
    fwrite(&Pos0.y, 4, 1, pFile);
    fwrite(&Pos0.z, 4, 1, pFile);

    fwrite(&Pos1.x, 4, 1, pFile);
    fwrite(&Pos1.y, 4, 1, pFile);
    fwrite(&Pos1.z, 4, 1, pFile);

    fwrite(&BoneID, 2, 1, pFile);
    fwrite(&Deadly, 1, 1, pFile);

    fwrite(*TargetName, xr_strlen(TargetName) + 1, 1, pFile);
    fwrite(*BoneName, xr_strlen(BoneName) + 1, 1, pFile);
};
