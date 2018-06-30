#include "stdafx.h"
#pragma hdrstop

#include "SoundRender_Emitter.h"
#include "SoundRender_Core.h"
#include "SoundRender_Source.h"

XRSOUND_API extern float psSoundCull;

inline u32 calc_cursor(const float& fTimeStarted, float& fTime, const float& fTimeTotal, const WAVEFORMATEX& wfx)
{
    if (fTime < fTimeStarted)
        fTime = fTimeStarted; // Андрюха посоветовал, ассерт что ниже вылетел из за паузы как то хитро
    R_ASSERT((fTime - fTimeStarted) >= 0.0f);
    while ((fTime - fTimeStarted) > fTimeTotal) // looped
    {
        fTime -= fTimeTotal;
    }
    u32 curr_sample_num = iFloor((fTime - fTimeStarted) * wfx.nSamplesPerSec);
    return curr_sample_num * (wfx.wBitsPerSample / 8) * wfx.nChannels;
}

void CSoundRender_Emitter::update(float dt)
{
    float fTime = SoundRender->fTimer_Value;
    float fDeltaTime = SoundRender->fTimer_Delta;

    VERIFY2(!!(owner_data) || (!(owner_data) && (m_current_state == stStopped)), "owner");
    VERIFY2(owner_data ? *(int*)(&owner_data->feedback) : 1, "owner");

    if (bRewind)
    {
        if (target)
            SoundRender->i_rewind(this);
        bRewind = FALSE;
    }

    switch (m_current_state)
    {
    case stStopped: break;
    case stStartingDelayed:
        if (iPaused)
            break;
        starting_delay -= dt;
        if (starting_delay <= 0)
            m_current_state = stStarting;
        break;
    case stStarting:
        if (iPaused)
            break;
        fTimeStarted = fTime;
        fTimeToStop = fTime + get_length_sec();
        fTimeToPropagade = fTime;
        fade_volume = 1.f;
        occluder_volume = SoundRender->get_occlusion(p_source.position, .2f, occluder);
        smooth_volume = p_source.base_volume * p_source.volume *
            (owner_data->s_type == st_Effect ? psSoundVEffects * psSoundVFactor : psSoundVMusic) *
            (b2D ? 1.f : occluder_volume);
        e_current = e_target = *SoundRender->get_environment(p_source.position);
        if (update_culling(dt))
        {
            m_current_state = stPlaying;
            set_cursor(0);
            SoundRender->i_start(this);
        }
        else
            m_current_state = stSimulating;
        break;
    case stStartingLoopedDelayed:
        if (iPaused)
            break;
        starting_delay -= dt;
        if (starting_delay <= 0)
            m_current_state = stStartingLooped;
        break;
    case stStartingLooped:
        if (iPaused)
            break;
        fTimeStarted = fTime;
        fTimeToStop = 0xffffffff;
        fTimeToPropagade = fTime;
        fade_volume = 1.f;
        occluder_volume = SoundRender->get_occlusion(p_source.position, .2f, occluder);
        smooth_volume = p_source.base_volume * p_source.volume *
            (owner_data->s_type == st_Effect ? psSoundVEffects * psSoundVFactor : psSoundVMusic) *
            (b2D ? 1.f : occluder_volume);
        e_current = e_target = *SoundRender->get_environment(p_source.position);
        if (update_culling(dt))
        {
            m_current_state = stPlayingLooped;
            set_cursor(0);
            SoundRender->i_start(this);
        }
        else
            m_current_state = stSimulatingLooped;
        break;
    case stPlaying:
        if (iPaused)
        {
            if (target)
            {
                SoundRender->i_stop(this);
                m_current_state = stSimulating;
            }
            fTimeStarted += fDeltaTime;
            fTimeToStop += fDeltaTime;
            fTimeToPropagade += fDeltaTime;
            break;
        }
        if (fTime >= fTimeToStop)
        {
            // STOP
            m_current_state = stStopped;
            SoundRender->i_stop(this);
        }
        else
        {
            if (!update_culling(dt))
            {
                // switch to: SIMULATE
                m_current_state = stSimulating; // switch state
                SoundRender->i_stop(this);
            }
            else
            {
                // We are still playing
                update_environment(dt);
            }
        }
        break;
    case stSimulating:
        if (iPaused)
        {
            fTimeStarted += fDeltaTime;
            fTimeToStop += fDeltaTime;
            fTimeToPropagade += fDeltaTime;
            break;
        }
        if (fTime >= fTimeToStop)
        {
            // STOP
            m_current_state = stStopped;
        }
        else
        {
            u32 ptr = calc_cursor(fTimeStarted, fTime, get_length_sec(), source()->m_wformat);
            set_cursor(ptr);

            if (update_culling(dt))
            {
                // switch to: PLAY
                m_current_state = stPlaying;
                /*
                                u32 ptr						= calc_cursor(	fTimeStarted,
                                                                            fTime,
                                                                            get_length_sec(),
                                                                            source()->m_wformat);
                                set_cursor					(ptr);
                */
                SoundRender->i_start(this);
            }
        }
        break;
    case stPlayingLooped:
        if (iPaused)
        {
            if (target)
            {
                SoundRender->i_stop(this);
                m_current_state = stSimulatingLooped;
            }
            fTimeStarted += fDeltaTime;
            fTimeToPropagade += fDeltaTime;
            break;
        }
        if (!update_culling(dt))
        {
            // switch to: SIMULATE
            m_current_state = stSimulatingLooped; // switch state
            SoundRender->i_stop(this);
        }
        else
        {
            // We are still playing
            update_environment(dt);
        }
        break;
    case stSimulatingLooped:
        if (iPaused)
        {
            fTimeStarted += fDeltaTime;
            fTimeToPropagade += fDeltaTime;
            break;
        }
        if (update_culling(dt))
        {
            // switch to: PLAY
            m_current_state = stPlayingLooped; // switch state
            u32 ptr = calc_cursor(fTimeStarted, fTime, get_length_sec(), source()->m_wformat);
            set_cursor(ptr);

            SoundRender->i_start(this);
        }
        break;
    }

    // if deffered stop active and volume==0 -> physically stop sound
    if (bStopping && fis_zero(fade_volume))
        i_stop();

    VERIFY2(!!(owner_data) || (!(owner_data) && (m_current_state == stStopped)), "owner");
    VERIFY2(owner_data ? *(int*)(owner_data->feedback) : 1, "owner");

    // footer
    bMoved = FALSE;
    if (m_current_state != stStopped)
    {
        if (fTime >= fTimeToPropagade)
            Event_Propagade();
    }
    else if (owner_data)
    {
        VERIFY(this == owner_data->feedback);
        owner_data->feedback = 0;
        owner_data = 0;
    }
}

IC void volume_lerp(float& c, float t, float s, float dt)
{
    float diff = t - c;
    float diff_a = _abs(diff);
    if (diff_a < EPS_S)
        return;
    float mot = s * dt;
    if (mot > diff_a)
        mot = diff_a;
    c += (diff / diff_a) * mot;
}

#include "xrServerEntities/ai_sounds.h"

bool CSoundRender_Emitter::update_culling(float dt)
{
    if (b2D)
    {
        occluder_volume = 1.f;
        fade_volume += dt * 10.f * (bStopping ? -1.f : 1.f);
    }
    else
    {
        // Check range
        float dist = SoundRender->listener_position().distance_to(p_source.position);
        if (dist > p_source.max_distance)
        {
            smooth_volume = 0;
            return FALSE;
        }

        // Calc attenuated volume
        float att = p_source.min_distance / (psSoundRolloff * dist);
        clamp(att, 0.f, 1.f);
        float fade_scale =
            bStopping || (att * p_source.base_volume * p_source.volume *
                                 (owner_data->s_type == st_Effect ? psSoundVEffects * psSoundVFactor : psSoundVMusic) <
                             psSoundCull) ?
            -1.f :
            1.f;
        fade_volume += dt * 10.f * fade_scale;

        // Update occlusion
        float occ = (owner_data->g_type == SOUND_TYPE_WORLD_AMBIENT) ?
            1.0f :
            SoundRender->get_occlusion(p_source.position, .2f, occluder);
        volume_lerp(occluder_volume, occ, 1.f, dt);
        clamp(occluder_volume, 0.f, 1.f);
    }
    clamp(fade_volume, 0.f, 1.f);
    // Update smoothing
    smooth_volume = .9f * smooth_volume +
        .1f * (p_source.base_volume * p_source.volume *
                  (owner_data->s_type == st_Effect ? psSoundVEffects * psSoundVFactor : psSoundVMusic) *
                  occluder_volume * fade_volume);
    if (smooth_volume < psSoundCull)
        return FALSE; // allow volume to go up
    // Here we has enought "PRIORITY" to be soundable
    // If we are playing already, return OK
    // --- else check availability of resources
    if (target)
        return TRUE;
    else
        return SoundRender->i_allow_play(this);
}

float CSoundRender_Emitter::priority()
{
    float dist = SoundRender->listener_position().distance_to(p_source.position);
    float att = p_source.min_distance / (psSoundRolloff * dist);
    clamp(att, 0.f, 1.f);
    return smooth_volume * att * priority_scale;
}

void CSoundRender_Emitter::update_environment(float dt)
{
    if (bMoved)
        e_target = *SoundRender->get_environment(p_source.position);
    e_current.lerp(e_current, e_target, dt);
}
