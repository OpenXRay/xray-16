#include "stdafx.h"
#pragma hdrstop

#include "SoundRender_Core.h"
#include "SoundRender_Emitter.h"
#include "SoundRender_Source.h"
#include "SoundRender_Target.h"

void CSoundRender_Emitter::start(const ref_sound& _owner, u32 flags, float delay)
{
    const bool _loop = flags & sm_Looped;
    bIgnoringTimeFactor = flags & sm_IgnoreTimeFactor;
    starting_delay = delay;

    VERIFY(_owner);
    owner_data = _owner;
    VERIFY(owner_data);
    p_source.position.set(0, 0, 0);

    const auto info = source()->info();
    p_source.min_distance = info.minDist;
    p_source.max_distance = info.maxDist;
    p_source.base_volume = info.baseVolume;
    p_source.volume = 1.f;
    p_source.freq = 1.f;
    p_source.max_ai_distance = info.maxAIDist;

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
}

void CSoundRender_Emitter::i_stop()
{
    bRewind = FALSE;
    if (target)
        stop_target();
    if (owner_data)
    {
        Event_ReleaseOwner();
        VERIFY(this == owner_data->feedback);
        owner_data->feedback = NULL;
        owner_data = NULL;
    }
    m_current_state = stStopped;
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
        stop_target();
        m_current_state = stSimulating;
        break;
    case stPlayingLooped:
        stop_target();
        m_current_state = stSimulatingLooped;
        break;
    default: VERIFY2(false, "Non playing ref_sound forced out of render queue"); break;
    }
}

void CSoundRender_Emitter::stop_target()
{
    R_ASSERT1_CURE(target, true, { return; });
    target->stop();
    target = nullptr;
}
