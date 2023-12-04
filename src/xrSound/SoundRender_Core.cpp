#include "stdafx.h"

#include "Include/xrAPI/xrAPI.h"
#include "Common/LevelStructure.hpp"
#include "SoundRender_Core.h"
#include "SoundRender_Source.h"
#include "SoundRender_Emitter.h"

// XXX: old SDK functionality
//#if defined(XR_PLATFORM_WINDOWS)
//#define OPENAL
//#include <eax/eax.h>
//#endif

XRSOUND_API Flags32 psSoundFlags =
{
    ss_Hardware | ss_EFX
};

XRSOUND_API int psSoundTargets = 32;
XRSOUND_API float psSoundOcclusionScale = 0.5f;
XRSOUND_API float psSoundVelocityAlpha = 0.05f;
XRSOUND_API float psSoundTimeFactor = 1.0f;
XRSOUND_API float psSoundLinearFadeFactor = 0.4f; //--#SM+#--
XRSOUND_API float psSoundCull = 0.01f;
XRSOUND_API float psSoundRolloff = 0.75f;
XRSOUND_API u32 psSoundModel = 0;
XRSOUND_API float psSoundVEffects = 1.0f;
XRSOUND_API float psSoundVFactor = 1.0f;

XRSOUND_API float psSoundVMusic = 1.f;
XRSOUND_API int psSoundCacheSizeMB = 32;
XRSOUND_API u32 psSoundPrecacheAll = 1;

CSoundRender_Core* SoundRender = nullptr;

CSoundRender_Core::CSoundRender_Core(CSoundManager& p)
    : Parent(p)
{
    bPresent = false;
    bUserEnvironment = false;
    geom_MODEL = nullptr;
    geom_ENV = nullptr;
    geom_SOM = nullptr;
    Handler = nullptr;
    s_targets_pu = 0;
    s_emitters_u = 0;
    e_current.set_identity();
    e_target.set_identity();
    bListenerMoved = false;
    bReady = false;
    isLocked = false;
    fTimer_Value = Timer.GetElapsed_sec();
    fTimer_Delta = 0.0f;
    fTimerPersistent_Value = TimerPersistent.GetElapsed_sec();
    fTimerPersistent_Delta = 0.0f;
    m_iPauseCounter = 1;
}

CSoundRender_Core::~CSoundRender_Core()
{
    xr_delete(geom_ENV);
    xr_delete(geom_SOM);
}

void CSoundRender_Core::_initialize()
{
    Timer.Start();
    TimerPersistent.Start();

    bPresent = true;

    // Cache
    cache_bytes_per_line = (sdef_target_block / 8) * 276400 / 1000;
    cache.initialize(psSoundCacheSizeMB * 1024, cache_bytes_per_line);

    bReady = true;
}

extern xr_vector<u8> g_target_temp_data;

void CSoundRender_Core::_clear()
{
    bReady = false;
    cache.destroy();

    // remove sources
    for (auto& kv : s_sources)
    {
        xr_delete(kv.second);
    }
    s_sources.clear();

    // remove emitters
    for (auto& emit : s_emitters)
        xr_delete(emit);
    s_emitters.clear();

    g_target_temp_data.clear();
}

void CSoundRender_Core::stop_emitters()
{
    for (auto& emit : s_emitters)
        emit->stop(false);
}

int CSoundRender_Core::pause_emitters(bool val)
{
    m_iPauseCounter += val ? +1 : -1;
    VERIFY(m_iPauseCounter >= 0);

    for (auto& emit : s_emitters)
        static_cast<CSoundRender_Emitter*>(emit)->pause(val, val ? m_iPauseCounter : m_iPauseCounter + 1);

    return m_iPauseCounter;
}

void CSoundRender_Core::_restart()
{
    cache.destroy();
    cache.initialize(psSoundCacheSizeMB * 1024, cache_bytes_per_line);
    env_apply();
}

void CSoundRender_Core::set_handler(sound_event* E) { Handler = E; }
void CSoundRender_Core::set_geometry_occ(CDB::MODEL* M) { geom_MODEL = M; }

void CSoundRender_Core::set_geometry_som(IReader* I)
{
    xr_delete(geom_SOM);
    if (nullptr == I)
        return;

    // check version
    R_ASSERT(I->find_chunk(0));
    [[maybe_unused]] u32 version = I->r_u32();
    VERIFY2(version == 0, "Invalid SOM version");

    struct SOM_poly
    {
        Fvector3 v1;
        Fvector3 v2;
        Fvector3 v3;
        u32 b2sided;
        float occ;
    };

    CDB::Collector CL;
    {
        // load geometry
        IReader* geom = I->open_chunk(1);
        VERIFY2(geom, "Corrupted SOM file");
        if (!geom)
            return;

        // Load tris and merge them
        const auto begin = static_cast<SOM_poly*>(geom->pointer());
        const auto end = static_cast<SOM_poly*>(geom->end());
        for (SOM_poly* poly = begin; poly != end; ++poly)
        {
            CL.add_face_packed_D(poly->v1, poly->v2, poly->v3, *(u32*)&poly->occ, 0.01f);
            if (poly->b2sided)
                CL.add_face_packed_D(poly->v3, poly->v2, poly->v1, *(u32*)&poly->occ, 0.01f);
        }
        geom->close();
    }

    // Create AABB-tree
    geom_SOM = xr_new<CDB::MODEL>();
    geom_SOM->build(CL.getV(), int(CL.getVS()), CL.getT(), int(CL.getTS()));
}

void CSoundRender_Core::set_geometry_env(IReader* I)
{
    xr_delete(geom_ENV);
    if (nullptr == I)
        return;
    const auto envLib = Parent.get_env_library();
    if (!envLib)
        return;

    // Associate names
    xr_vector<u16> ids;
    IReader* names = I->open_chunk(0);
    while (!names->eof())
    {
        string256 n;
        names->r_stringZ(n, sizeof(n));
        int id = envLib->GetID(n);
        R_ASSERT(id >= 0);
        ids.push_back((u16)id);
    }
    names->close();

    // Load geometry
    IReader* geom_ch = I->open_chunk(1);

    u8* _data = (u8*)xr_malloc(geom_ch->length());

    memcpy(_data, geom_ch->pointer(), geom_ch->length());

    IReader* geom = xr_new<IReader>(_data, geom_ch->length(), 0);

    hdrCFORM H;
    geom->r(&H, sizeof(hdrCFORM));
    Fvector* verts = (Fvector*)geom->pointer();
    CDB::TRI* tris = (CDB::TRI*)(verts + H.vertcount);
    for (u32 it = 0; it < H.facecount; it++)
    {
        CDB::TRI* T = tris + it;
        u16 id_front = (u16)((T->dummy & 0x0000ffff) >> 0); //	front face
        u16 id_back = (u16)((T->dummy & 0xffff0000) >> 16); //	back face
        R_ASSERT(id_front < (u16)ids.size());
        R_ASSERT(id_back < (u16)ids.size());
        T->dummy = u32(ids[id_back] << 16) | u32(ids[id_front]);
    }
    geom_ENV = xr_new<CDB::MODEL>();
    geom_ENV->build(verts, H.vertcount, tris, H.facecount);
#ifdef _EDITOR // XXX: may be we are interested in applying env in the game build too?
    env_apply();
#endif
    geom_ch->close();
    geom->close();
    xr_free(_data);
}

CSound* CSoundRender_Core::create(pcstr fName, esound_type sound_type, int game_type, bool replaceWithNoSound /*= true*/)
{
    if (!bPresent)
        return nullptr;

    CSound_source* handle{};

    string_path fn;
    xr_strcpy(fn, fName);
    if (strext(fn))
        *strext(fn) = 0;
    const bool found = i_create_source(handle, fn, replaceWithNoSound);
    const bool handleAvailable = found || replaceWithNoSound;

    if (!handleAvailable)
        return nullptr;

    auto* snd = xr_new<CSound>();

    snd->handle = handle;

    snd->g_type = game_type;
    if (game_type == sg_SourceType && handleAvailable)
        snd->g_type = snd->handle->game_type();

    snd->s_type = sound_type;

    snd->dwBytesTotal = handleAvailable ? snd->handle->bytes_total() : 0;
    snd->fTimeTotal = handleAvailable ? snd->handle->length_sec() : 0.f;

    return snd;
}

void CSoundRender_Core::attach_tail(CSound& snd, pcstr fName)
{
    if (!bPresent)
        return;
    string_path fn;
    xr_strcpy(fn, fName);
    if (strext(fn))
        *strext(fn) = 0;
    if (!snd.fn_attached[0].empty() && !snd.fn_attached[1].empty())
    {
#ifndef MASTER_GOLD
        Msg("! 2 file already in queue [%s][%s]", snd.fn_attached[0].c_str(), snd.fn_attached[1].c_str());
#endif
        return;
    }

    const u32 idx = snd.fn_attached[0].empty() ? 0 : 1;

    snd.fn_attached[idx] = fn;

    CSoundRender_Source* s = i_create_source(fn);
    snd.dwBytesTotal += s->bytes_total();
    snd.fTimeTotal += s->length_sec();
    if (snd.feedback)
        ((CSoundRender_Emitter*)snd.feedback)->fTimeToStop += s->length_sec();

    i_destroy_source(s);
}

void CSoundRender_Core::play(ref_sound& S, IGameObject* O, u32 flags, float delay)
{
    if (!bPresent || !S._handle())
        return;
    S->g_object = O;
    if (S._feedback())
        ((CSoundRender_Emitter*)S._feedback())->rewind();
    else
        i_play(S, flags, delay);

    if (flags & sm_2D || S._handle()->channels_num() == 2)
        S._feedback()->switch_to_2D();

    S._feedback()->set_ignore_time_factor(flags & sm_IgnoreTimeFactor);
}

void CSoundRender_Core::play_no_feedback(
    ref_sound& S, IGameObject* O, u32 flags, float delay, Fvector* pos, float* vol, float* freq, Fvector2* range)
{
    if (!bPresent || !S._handle())
        return;
    const ref_sound orig = S;
    S._set(xr_new<CSound>());
    S->handle = orig->handle;
    S->g_type = orig->g_type;
    S->g_object = O;
    S->dwBytesTotal = orig->dwBytesTotal;
    S->fTimeTotal = orig->fTimeTotal;
    S->fn_attached[0] = orig->fn_attached[0];
    S->fn_attached[1] = orig->fn_attached[1];

    i_play(S, flags, delay);

    if (flags & sm_2D || S._handle()->channels_num() == 2)
        S._feedback()->switch_to_2D();

    if (pos)
        S._feedback()->set_position(*pos);
    if (freq)
        S._feedback()->set_frequency(*freq);
    if (range)
        S._feedback()->set_range((*range)[0], (*range)[1]);
    if (vol)
        S._feedback()->set_volume(*vol);
    S = orig;
}

void CSoundRender_Core::play_at_pos(ref_sound& S, IGameObject* O, const Fvector& pos, u32 flags, float delay)
{
    if (!bPresent || !S._handle())
        return;
    S->g_object = O;
    if (S._feedback())
        ((CSoundRender_Emitter*)S._feedback())->rewind();
    else
        i_play(S, flags, delay);

    S._feedback()->set_position(pos);

    if (flags & sm_2D || S._handle()->channels_num() == 2)
        S._feedback()->switch_to_2D();

    S._feedback()->set_ignore_time_factor(flags & sm_IgnoreTimeFactor);
}

void CSoundRender_Core::destroy(CSound& S)
{
    if (auto* emitter = (CSoundRender_Emitter*)S.feedback)
    {
        emitter->stop(false);
        VERIFY(S.feedback == nullptr);
    }
    i_destroy_source((CSoundRender_Source*)S.handle);
    S.handle = nullptr;
}

CSoundRender_Environment* CSoundRender_Core::get_environment(const Fvector& P)
{
    static CSoundRender_Environment identity;

    if (bUserEnvironment)
    {
        return &s_user_environment;
    }
    if (geom_ENV)
    {
        Fvector dir = {0, -1, 0};
        geom_DB.ray_query(CDB::OPT_ONLYNEAREST, geom_ENV, P, dir, 1000.f);
        if (geom_DB.r_count())
        {
            const auto envLib = Parent.get_env_library();

            CDB::RESULT* r = geom_DB.r_begin();
            CDB::TRI* T = geom_ENV->get_tris() + r->id;
            Fvector* V = geom_ENV->get_verts();
            Fvector tri_norm;
            tri_norm.mknormal(V[T->verts[0]], V[T->verts[1]], V[T->verts[2]]);
            float dot = dir.dotproduct(tri_norm);
            if (dot < 0)
            {
                u16 id_front = (u16)((T->dummy & 0x0000ffff) >> 0); //	front face
                return envLib->Get(id_front);
            }
            u16 id_back = (u16)((T->dummy & 0xffff0000) >> 16); //	back face
            return envLib->Get(id_back);
        }
        identity.set_identity();
        return &identity;
    }
    identity.set_identity();
    return &identity;
}

void CSoundRender_Core::env_apply()
{
    /*
    // Force all sounds to change their environment
    // (set their positions to signal changes in environment)
    for (u32 it = 0; it < s_emitters.size(); it++)
    {
        CSoundRender_Emitter* pEmitter = s_emitters[it];
        const CSound_params* pParams = pEmitter->get_params();
        pEmitter->set_position(pParams->position);
    }
    */
    bListenerMoved = true;
}

void CSoundRender_Core::update_listener(const Fvector& P, const Fvector& D, const Fvector& N, float dt) {}

void CSoundRender_Core::object_relcase(IGameObject* obj)
{
    if (obj)
    {
        for (auto& emit : s_emitters)
        {
            if (emit)
                if (emit->owner_data)
                    if (obj == emit->owner_data->g_object)
                        emit->owner_data->g_object = 0;
        }
    }
}

void CSoundRender_Core::set_user_env(CSound_environment* E)
{
    if (0 == E && !bUserEnvironment)
        return;

    if (E)
    {
        s_user_environment = *((CSoundRender_Environment*)E);
        bUserEnvironment = true;
    }
    else
    {
        bUserEnvironment = false;
    }
    env_apply();
}

void CSoundRender_Core::refresh_env_library()
{
    Parent.env_unload();
    Parent.env_load();
    env_apply();
}

void CSoundRender_Core::refresh_sources()
{
    for (auto& emit : s_emitters)
        emit->stop(false);
    for (const auto& kv : s_sources)
    {
        CSoundRender_Source* s = kv.second;
        s->unload();
        s->load(*s->fname);
    }
}

void CSoundRender_Core::set_environment_size(CSound_environment* src_env, CSound_environment** dst_env)
{
    // XXX: old SDK functionality
    /*if (bEAX)
    {
        CSoundRender_Environment* SE = static_cast<CSoundRender_Environment*>(src_env);
        CSoundRender_Environment* DE = static_cast<CSoundRender_Environment*>(*dst_env);
#if defined(XR_PLATFORM_WINDOWS)
        // set environment
        i_eax_set(&DSPROPSETID_EAX_ListenerProperties,
            DSPROPERTY_EAXLISTENER_IMMEDIATE | DSPROPERTY_EAXLISTENER_ENVIRONMENTSIZE, &SE->EnvironmentSize,
            sizeof(SE->EnvironmentSize));
        i_eax_listener_set(SE);
        i_eax_commit_setting();
        i_eax_set(&DSPROPSETID_EAX_ListenerProperties,
            DSPROPERTY_EAXLISTENER_IMMEDIATE | DSPROPERTY_EAXLISTENER_ENVIRONMENTSIZE, &DE->EnvironmentSize,
            sizeof(DE->EnvironmentSize));
        i_eax_listener_get(DE);
#endif
    }*/
}

void CSoundRender_Core::set_environment(u32 id, CSound_environment** dst_env)
{
    // XXX: old SDK functionality
    /*if (bEAX)
    {
        CSoundRender_Environment* DE = static_cast<CSoundRender_Environment*>(*dst_env);
#if defined(XR_PLATFORM_WINDOWS)
        // set environment
        i_eax_set(&DSPROPSETID_EAX_ListenerProperties,
            DSPROPERTY_EAXLISTENER_IMMEDIATE | DSPROPERTY_EAXLISTENER_ENVIRONMENTSIZE, &id, sizeof(id));
        i_eax_listener_get(DE);
#endif
    }*/
}
