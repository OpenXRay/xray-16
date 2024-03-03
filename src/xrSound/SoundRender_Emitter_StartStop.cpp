#include "stdafx.h"
#pragma hdrstop

#include "SoundRender_Core.h"
#include "SoundRender_Emitter.h"
#include "SoundRender_Source.h"

void CSoundRender_Emitter::start(const ref_sound& _owner, u32 flags, float delay)
{
    const bool _loop = flags & sm_Looped;
    bIgnoringTimeFactor = flags & sm_IgnoreTimeFactor;
    starting_delay = delay;

    VERIFY(_owner);
    owner_data = _owner;
    VERIFY(owner_data);
    //	source					= (CSoundRender_Source*)owner_data->handle;
    p_source.position.set(0, 0, 0);
    p_source.min_distance = source()->m_fMinDist; // DS3D_DEFAULTMINDISTANCE;
    p_source.max_distance = source()->m_fMaxDist; // 300.f;
    p_source.base_volume = source()->m_fBaseVolume; // 1.f
    p_source.volume = 1.f; // 1.f
    p_source.freq = 1.f;
    p_source.max_ai_distance = source()->m_fMaxAIDist; // 300.f;

    if (fis_zero(delay, EPS_L))
    {
        m_current_state = _loop ? stStartingLooped : stStarting;
    }
    else
    {
        m_current_state = _loop ? stStartingLoopedDelayed : stStartingDelayed;
        fTimeToPropagade = bIgnoringTimeFactor ? SoundRender->TimerPersistent.GetElapsed_sec() : SoundRender->Timer.GetElapsed_sec();
    }
    bStopping = FALSE;
    bRewind = FALSE;

    const auto wvf = source()->m_wformat;
    target_buffer_size = sdef_target_block * wvf.nAvgBytesPerSec / 1000;

#ifdef USE_PHONON
    ipl_settings = IPLAudioSettings{ s32(wvf.nSamplesPerSec), target_buffer_size / wvf.nBlockAlign };

    IPLDirectEffectSettings direct{ wvf.nChannels };
    iplDirectEffectCreate(SoundRender->ipl_context, &ipl_settings, &direct, &ipl_effects.direct);

    IPLReflectionEffectSettings refl{ IPL_REFLECTIONEFFECTTYPE_CONVOLUTION, ipl_settings.frameSize * 2, 4 };
    iplReflectionEffectCreate(SoundRender->ipl_context, &ipl_settings, &refl, &ipl_effects.reflection);

    IPLPathEffectSettings path{ 1, IPL_FALSE, {}, SoundRender->ipl_hrtf };
    iplPathEffectCreate(SoundRender->ipl_context, &ipl_settings, &path, &ipl_effects.path);
#endif
}

void CSoundRender_Emitter::i_stop()
{
    bRewind = FALSE;
    if (target)
        SoundRender->i_stop(this);
    if (owner_data)
    {
        Event_ReleaseOwner();
        VERIFY(this == owner_data->feedback);
        owner_data->feedback = NULL;
        owner_data = NULL;
    }
    m_current_state = stStopped;

#ifdef USE_PHONON
    ipl_settings = {};
    iplDirectEffectRelease(&ipl_effects.direct);
    iplReflectionEffectRelease(&ipl_effects.reflection);
    iplPathEffectRelease(&ipl_effects.path);
#endif
}

void CSoundRender_Emitter::stop(bool isDeffered)
{
    if (isDeffered)
        bStopping = TRUE;
    else
        i_stop();
}

void CSoundRender_Emitter::rewind()
{
    bStopping = FALSE;

    const float fTime = bIgnoringTimeFactor ? SoundRender->TimerPersistent.GetElapsed_sec() : SoundRender->Timer.GetElapsed_sec();
    float fDiff = fTime - fTimeStarted;
    fTimeStarted += fDiff;
    fTimeToStop += fDiff;
    fTimeToPropagade = fTime;

    set_cursor(0);
    bRewind = TRUE;
}

void CSoundRender_Emitter::pause(bool bVal, int id)
{
    if (bVal)
    {
        if (0 == iPaused)
            iPaused = id;
    }
    else
    {
        if (id == iPaused)
            iPaused = 0;
    }
}

void CSoundRender_Emitter::cancel()
{
    // Msg		("- %10s : %3d[%1.4f] : %s","cancel",dbg_ID,priority(),source->fname);
    switch (m_current_state)
    {
    case stPlaying:
        // switch to: SIMULATE
        m_current_state = stSimulating; // switch state
        SoundRender->i_stop(this);
        break;
    case stPlayingLooped:
        // switch to: SIMULATE
        m_current_state = stSimulatingLooped; // switch state
        SoundRender->i_stop(this);
        break;
    default: FATAL("Non playing ref_sound forced out of render queue"); break;
    }
}
