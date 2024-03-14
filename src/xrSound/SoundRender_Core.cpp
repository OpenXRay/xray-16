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
}

void CSoundRender_Core::_initialize()
{
    Timer.Start();
    TimerPersistent.Start();

    bPresent = true;

    bReady = true;
}

void CSoundRender_Core::_clear()
{
    bReady = false;

    // remove sources
    for (auto& kv : s_sources)
    {
        xr_delete(kv.second);
    }
    s_sources.clear();
}

ISoundScene* CSoundRender_Core::create_scene()
{
    return m_scenes.emplace_back(xr_new<CSoundRender_Scene>());
}

void CSoundRender_Core::destroy_scene(ISoundScene*& sound_scene)
{
    m_scenes.erase(std::remove(m_scenes.begin(), m_scenes.end(), sound_scene), m_scenes.end());
    xr_delete(sound_scene);
}

void CSoundRender_Core::stop_emitters()
{
    for (const auto& scene : m_scenes)
        scene->stop_emitters();
}

int CSoundRender_Core::pause_emitters(bool pauseState)
{
    int cnt = 0;
    for (const auto& scene : m_scenes)
        cnt += scene->pause_emitters(pauseState);
    return cnt;
}

void CSoundRender_Core::_restart()
{
    env_apply();
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

void CSoundRender_Core::update_listener(const Fvector& P, const Fvector& D, const Fvector& N, float dt)
{
    if (!psSoundFlags.test(ss_EFX) || !m_effects)
        return;

    // Update effects
    if (bListenerMoved)
    {
        bListenerMoved = false;
        e_target = *(CSoundRender_Environment*)DefaultSoundScene->get_environment(P);
    }

    e_current.lerp(e_current, e_target, fTimer_Delta);

    m_effects->set_listener(e_current);
    m_effects->commit();
}

void CSoundRender_Core::refresh_sources()
{
    stop_emitters();

    for (const auto& kv : s_sources)
    {
        CSoundRender_Source* s = kv.second;
        s->unload();
        s->load(*s->fname);
    }
}
