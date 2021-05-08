#include "stdafx.h"

#include "SoundRender_Emitter.h"
#include "SoundRender_Core.h"
#include "SoundRender_Source.h"
#include "SoundRender_TargetA.h"

extern u32 psSoundModel;
extern float psSoundVEffects;

void CSoundRender_Emitter::set_position(const Fvector& pos)
{
    if (source()->channels_num() == 1)
        p_source.position = pos;
    else
        p_source.position.set(0, 0, 0);

    bMoved = true;
}

CSoundRender_Emitter::CSoundRender_Emitter()
{
#ifdef DEBUG
    static u32 incrementalID = 0;
    dbg_ID = ++incrementalID;
#endif
    target = nullptr;
    //source = nullptr;
    owner_data = nullptr;
    smooth_volume = 1.f;
    occluder_volume = 1.f;
    fade_volume = 1.f;
    occluder[0].set(0, 0, 0);
    occluder[1].set(0, 0, 0);
    occluder[2].set(0, 0, 0);
    m_current_state = stStopped;
    set_cursor(0);
    bMoved = true;
    b2D = false;
    bStopping = false;
    bRewind = false;
    iPaused = 0;
    fTimeStarted = 0.0f;
    fTimeToStop = 0.0f;
    fTimeToPropagade = 0.0f;
    marker = 0xabababab;
    starting_delay = 0.f;
    priority_scale = 1.f;
    m_cur_handle_cursor = 0;
}

CSoundRender_Emitter::~CSoundRender_Emitter()
{
    // try to release dependencies, events, for example
    Event_ReleaseOwner();
}

//////////////////////////////////////////////////////////////////////
void CSoundRender_Emitter::Event_ReleaseOwner()
{
    if (!owner_data)
        return;

    for (u32 it = 0; it < SoundRender->s_events.size(); it++)
    {
        if (owner_data == SoundRender->s_events[it].first)
        {
            SoundRender->s_events.erase(SoundRender->s_events.begin() + it);
            it--;
        }
    }
}

void CSoundRender_Emitter::Event_Propagade()
{
    fTimeToPropagade += ::Random.randF(s_f_def_event_pulse - 0.030f, s_f_def_event_pulse + 0.030f);
    if (!(owner_data))
        return;
    if (0 == owner_data->g_type)
        return;
    if (0 == owner_data->g_object)
        return;
    if (0 == SoundRender->Handler)
        return;

    VERIFY(_valid(p_source.volume));
    // Calculate range
    float clip = p_source.max_ai_distance * p_source.volume;
    float range = std::min(p_source.max_ai_distance, clip);
    if (range < 0.1f)
        return;

    // Inform objects
    SoundRender->s_events.emplace_back(owner_data, range);
}

void CSoundRender_Emitter::switch_to_2D()
{
    b2D = true;
    set_priority(100.f);
}

void CSoundRender_Emitter::switch_to_3D() { b2D = false; }
u32 CSoundRender_Emitter::play_time()
{
    if (m_current_state == stPlaying || m_current_state == stPlayingLooped || m_current_state == stSimulating ||
        m_current_state == stSimulatingLooped)
        return iFloor((SoundRender->fTimer_Value - fTimeStarted) * 1000.0f);
    return 0;
}

#include "SoundRender_Source.h"
void CSoundRender_Emitter::set_cursor(u32 p)
{
    m_stream_cursor = p;

    if (owner_data._get() && owner_data->fn_attached[0].size())
    {
        u32 bt = ((CSoundRender_Source*)owner_data->handle)->dwBytesTotal;
        if (m_stream_cursor >= m_cur_handle_cursor + bt)
        {
            SoundRender->i_destroy_source((CSoundRender_Source*)owner_data->handle);
            owner_data->handle = SoundRender->i_create_source(owner_data->fn_attached[0].c_str());
            owner_data->fn_attached[0] = owner_data->fn_attached[1];
            owner_data->fn_attached[1] = "";
            m_cur_handle_cursor = get_cursor(true);

            if (target)
                ((CSoundRender_TargetA*)target)->source_changed();
        }
    }
}

u32 CSoundRender_Emitter::get_cursor(bool b_absolute) const
{
    if (b_absolute)
        return m_stream_cursor;
    VERIFY(m_stream_cursor - m_cur_handle_cursor >= 0);
    return m_stream_cursor - m_cur_handle_cursor;
}

void CSoundRender_Emitter::move_cursor(int offset) { set_cursor(get_cursor(true) + offset); }
