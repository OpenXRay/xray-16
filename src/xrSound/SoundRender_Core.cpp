#include "stdafx.h"

#include "Include/xrAPI/xrAPI.h"
#include "Common/LevelStructure.hpp"
#include "SoundRender_Core.h"
#include "SoundRender_Source.h"
#include "SoundRender_Emitter.h"

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
    s_targets_pu = 0;
    s_emitters_u = 0;
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

#ifdef USE_PHONON
    IPLSIMDLevel simdLevel = IPL_SIMDLEVEL_SSE2;
    if (CPU::HasAVX512F)
        simdLevel = IPL_SIMDLEVEL_AVX512;
    else if (CPU::HasAVX2)
        simdLevel = IPL_SIMDLEVEL_AVX2;
    else if (CPU::HasAVX)
        simdLevel = IPL_SIMDLEVEL_AVX;
    else if (CPU::HasSSE41)
        simdLevel = IPL_SIMDLEVEL_SSE4;

    IPLContextSettings contextSettings
    {
        STEAMAUDIO_VERSION,

        [](IPLLogLevel level, const char* message)
        {
            char mark = '\0';
            switch (level)
            {
            case IPL_LOGLEVEL_INFO:    mark = '*'; break;
            case IPL_LOGLEVEL_WARNING: mark = '~'; break;
            case IPL_LOGLEVEL_ERROR:   mark = '!'; break;
            case IPL_LOGLEVEL_DEBUG:   mark = '#'; break;
            }
            Msg("%c SOUND: SteamAudio: %s", message);
        },

        [](IPLsize size, IPLsize alignment)
        {
            return Memory.mem_alloc(size, alignment);
        },

        [](void* memoryBlock)
        {
            Memory.mem_free(memoryBlock);
        },

        simdLevel
    };

    iplContextCreate(&contextSettings, &ipl_context);

    IPLHRTFSettings hrtfSettings
    {
        IPL_HRTFTYPE_DEFAULT,
        nullptr, nullptr, 0,
        1.0f, IPL_HRTFNORMTYPE_NONE
    };

    IPLAudioSettings audioSettings{ 44100, 1024 };

    iplHRTFCreate(ipl_context, &audioSettings, &hrtfSettings, &ipl_hrtf);
#endif
    bReady = true;
}

void CSoundRender_Core::_clear()
{
    bReady = false;

#ifdef USE_PHONON
    iplHRTFRelease(&ipl_hrtf);
    iplContextRelease(&ipl_context);
#endif

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
    if (!psSoundFlags.test(ss_EFX) || !bListenerMoved)
        return;

    bListenerMoved = false;
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
