#include "StdAfx.h"
#include "Common/LevelGameDef.h"
#include "ai_space.h"
#include "ParticlesObject.h"
#include "xrScriptEngine/script_process.hpp"
#include "xrScriptEngine/script_engine.hpp"
#include "Level.h"
#include "game_cl_base.h"
#include "xrEngine/x_ray.h"
#include "xrEngine/GameMtlLib.h"
#include "xrPhysics/PhysicsCommon.h"
#include "level_sounds.h"
#include "GamePersistent.h"

bool CLevel::Load_GameSpecific_Before()
{
    // AI space
    g_pGamePersistent->SetLoadStageTitle("st_loading_ai_objects");
    g_pGamePersistent->LoadTitle();
    string_path fn_game;

    if (GamePersistent().GameType() == eGameIDSingle && !ai().get_alife() && FS.exist(fn_game, "$level$", "level.ai") &&
        !net_Hosts.empty())
        ai().load(net_SessionName());

    if (!GEnv.isDedicatedServer && !ai().get_alife() && ai().get_game_graph() && FS.exist(fn_game, "$level$", "level.game"))
    {
        IReader* stream = FS.r_open(fn_game);
        ai().patrol_path_storage_raw(*stream);
        FS.r_close(stream);
    }

    return (TRUE);
}

bool CLevel::Load_GameSpecific_After()
{
    R_ASSERT(m_StaticParticles.empty());
    // loading static particles
    string_path fn_game;
    if (FS.exist(fn_game, "$level$", "level.ps_static"))
    {
        IReader* F = FS.r_open(fn_game);
        CParticlesObject* pStaticParticles;
        u32 chunk = 0;
        string256 ref_name;
        Fmatrix transform;
        Fvector zero_vel = {0.f, 0.f, 0.f};
        u32 ver = 0;
        for (IReader* OBJ = F->open_chunk_iterator(chunk); OBJ; OBJ = F->open_chunk_iterator(chunk, OBJ))
        {
            if (chunk == 0)
            {
                if (OBJ->length() == sizeof(u32))
                {
                    ver = OBJ->r_u32();
#ifndef MASTER_GOLD
                    Msg("PS new version, %d", ver);
#endif // #ifndef MASTER_GOLD
                    continue;
                }
            }
            u16 gametype_usage = 0;
            if (ver > 0)
            {
                gametype_usage = OBJ->r_u16();
            }
            OBJ->r_stringZ(ref_name, sizeof(ref_name));
            OBJ->r(&transform, sizeof(Fmatrix));
            transform.c.y += 0.01f;

            if ((g_pGamePersistent->m_game_params.m_e_game_type & EGameIDs(gametype_usage)) || (ver == 0))
            {
                pStaticParticles = CParticlesObject::Create(ref_name, FALSE, false);
                pStaticParticles->UpdateParent(transform, zero_vel);
                pStaticParticles->Play(false);
                m_StaticParticles.push_back(pStaticParticles);
            }
        }
        FS.r_close(F);
    }

    if (!GEnv.isDedicatedServer)
    {
        // loading static sounds
        VERIFY(m_level_sound_manager);
        m_level_sound_manager->Load();

        // loading sound environment
        if (FS.exist(fn_game, "$level$", "level.snd_env"))
        {
            IReader* F = FS.r_open(fn_game);
            GEnv.Sound->set_geometry_env(F);
            FS.r_close(F);
        }
        // loading SOM
        if (FS.exist(fn_game, "$level$", "level.som"))
        {
            IReader* F = FS.r_open(fn_game);
            GEnv.Sound->set_geometry_som(F);
            FS.r_close(F);
        }

        // loading random (around player) sounds
        if (pSettings->section_exist("sounds_random"))
        {
            CInifile::Sect& S = pSettings->r_section("sounds_random");
            Sounds_Random.reserve(S.Data.size());
            for (auto I = S.Data.cbegin(); S.Data.cend() != I; ++I)
            {
                Sounds_Random.push_back(ref_sound());
                GEnv.Sound->create(Sounds_Random.back(), *I->first, st_Effect, sg_SourceType);
            }
            Sounds_Random_dwNextTime = Device.TimerAsync() + 50000;
            Sounds_Random_Enabled = FALSE;
        }

        if (FS.exist(fn_game, "$level$", "level.fog_vol"))
        {
            IReader* F = FS.r_open(fn_game);
            u16 version = F->r_u16();
            if (version == 2)
            {
                u32 cnt = F->r_u32();

                Fmatrix volume_matrix;
                for (u32 i = 0; i < cnt; ++i)
                {
                    F->r(&volume_matrix, sizeof(volume_matrix));
                    u32 sub_cnt = F->r_u32();
                    for (u32 is = 0; is < sub_cnt; ++is)
                    {
                        F->r(&volume_matrix, sizeof(volume_matrix));
                    }
                }
            }
            FS.r_close(F);
        }
    }

    if (!GEnv.isDedicatedServer)
    {
        // loading scripts
        auto& scriptEngine = *GEnv.ScriptEngine;
        scriptEngine.remove_script_process(ScriptProcessor::Level);
        shared_str scripts;
        if (pLevel->section_exist("level_scripts") && pLevel->line_exist("level_scripts", "script"))
            scripts = pLevel->r_string("level_scripts", "script");
        else
            scripts = "";
        scriptEngine.add_script_process(ScriptProcessor::Level, scriptEngine.CreateScriptProcess("level", scripts));
    }

    BlockCheatLoad();

    g_pGamePersistent->Environment().SetGameTime(GetEnvironmentGameDayTimeSec(), game->GetEnvironmentGameTimeFactor());

    return TRUE;
}

struct translation_pair
{
    u32 m_id;
    u16 m_index;

    IC translation_pair(u32 id, u16 index)
    {
        m_id = id;
        m_index = index;
    }

    IC bool operator==(const u16& id) const { return (m_id == id); }
    IC bool operator<(const translation_pair& pair) const { return (m_id < pair.m_id); }
    IC bool operator<(const u16& id) const { return (m_id < id); }
};

void CLevel::Load_GameSpecific_CFORM(CDB::TRI* tris, u32 count)
{
    typedef xr_vector<translation_pair> ID_INDEX_PAIRS;
    ID_INDEX_PAIRS translator;
    translator.reserve(GMLib.CountMaterial());
    u16 default_id = (u16)GMLib.GetMaterialIdx("default");
    translator.push_back(translation_pair(u32(-1), default_id));

    u16 index = 0, static_mtl_count = 1;
    int max_ID = 0;
    int max_static_ID = 0;
    for (GameMtlIt I = GMLib.FirstMaterial(); GMLib.LastMaterial() != I; ++I, ++index)
    {
        if (!(*I)->Flags.test(SGameMtl::flDynamic))
        {
            ++static_mtl_count;
            translator.push_back(translation_pair((*I)->GetID(), index));
            if ((*I)->GetID() > max_static_ID)
                max_static_ID = (*I)->GetID();
        }
        if ((*I)->GetID() > max_ID)
            max_ID = (*I)->GetID();
    }
    // Msg("* Material remapping ID: [Max:%d, StaticMax:%d]",max_ID,max_static_ID);
    VERIFY(max_static_ID < 0xFFFF);

    if (static_mtl_count < 128)
    {
        CDB::TRI* I = tris;
        CDB::TRI* E = tris + count;
        for (; I != E; ++I)
        {
            ID_INDEX_PAIRS::iterator i = std::find(translator.begin(), translator.end(), (u16)(*I).material);
            if (i != translator.end())
            {
                (*I).material = (*i).m_index;
                SGameMtl* mtl = GMLib.GetMaterialByIdx((*i).m_index);
                (*I).suppress_shadows = mtl->Flags.is(SGameMtl::flSuppressShadows);
                (*I).suppress_wm = mtl->Flags.is(SGameMtl::flSuppressWallmarks);
                continue;
            }

            xrDebug::Fatal(DEBUG_INFO, "Game material '%d' not found", (*I).material);
        }
        return;
    }

    std::sort(translator.begin(), translator.end());
    {
        CDB::TRI* I = tris;
        CDB::TRI* E = tris + count;
        for (; I != E; ++I)
        {
            ID_INDEX_PAIRS::iterator i = std::lower_bound(translator.begin(), translator.end(), (u16)(*I).material);
            if ((i != translator.end()) && ((*i).m_id == (*I).material))
            {
                (*I).material = (*i).m_index;
                SGameMtl* mtl = GMLib.GetMaterialByIdx((*i).m_index);
                (*I).suppress_shadows = mtl->Flags.is(SGameMtl::flSuppressShadows);
                (*I).suppress_wm = mtl->Flags.is(SGameMtl::flSuppressWallmarks);
                continue;
            }

            xrDebug::Fatal(DEBUG_INFO, "Game material '%d' not found", (*I).material);
        }
    }
}

void CLevel::BlockCheatLoad()
{
#ifndef DEBUG
    if (game && (GameID() != eGameIDSingle))
        phTimefactor = 1.f;
#endif
}
