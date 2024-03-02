#include "stdafx.h"

#include "SoundRender_Emitter.h"
#include "SoundRender_Core.h"
#include "SoundRender_Scene.h"
#include "SoundRender_Source.h"
#include "SoundRender_TargetA.h"

extern u32 psSoundModel;
extern float psSoundVEffects;

void CSoundRender_Emitter::set_position(const Fvector& pos)
{
    if (source()->channels_num() == 1)
        p_source.update_position(pos);
    else
        p_source.update_position({});

    bMoved = true;
}

void CSoundRender_Emitter::set_frequency(float scale)
{
    VERIFY(_valid(scale));
    if (_valid(scale))
        p_source.freq = scale;
}

// Перемотка звука на заданную секунду [rewind snd to target time] --#SM+#--
void CSoundRender_Emitter::set_time(float t)
{
    VERIFY2(get_length_sec() >= t, "set_time: time is bigger than length of sound");
    clamp(t, 0.0f, get_length_sec());
    fTimeToRewind = t;
}

CSoundRender_Emitter::CSoundRender_Emitter(CSoundRender_Scene* s)
    : scene(s),
      priority_scale(1.f),
      smooth_volume(1.f),
      occluder_volume(1.f),
      fade_volume(1.f),
      m_current_state(stStopped),
      bMoved(true),
      marker(0xabababab) {}

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

    auto& events = scene->get_events();

    for (u32 it = 0; it < events.size(); it++)
    {
        if (owner_data == events[it].first)
        {
            events.erase(events.begin() + it);
            it--;
        }
    }
}

void CSoundRender_Emitter::Event_Propagade()
{
    fTimeToPropagade += ::Random.randF(s_f_def_event_pulse - 0.030f, s_f_def_event_pulse + 0.030f);
    if (!owner_data)
        return;
    if (!owner_data->g_type)
        return;
    if (!owner_data->g_object)
        return;
    if (!scene->get_events_handler())
        return;

    VERIFY(_valid(p_source.volume));
    // Calculate range
    const float clip = p_source.max_ai_distance * p_source.volume;
    const float range = std::min(p_source.max_ai_distance, clip);
    if (range < 0.1f)
        return;

    // Inform objects
    scene->get_events().emplace_back(owner_data, range);
}

void CSoundRender_Emitter::switch_to_2D()
{
    b2D = true;
    set_priority(100.f);
}

void CSoundRender_Emitter::switch_to_3D()
{
    b2D = false;
}

u32 CSoundRender_Emitter::play_time()
{
    if (m_current_state == stPlaying || m_current_state == stPlayingLooped || m_current_state == stSimulating ||
        m_current_state == stSimulatingLooped)
        return iFloor((SoundRender->fTimer_Value - fTimeStarted) * 1000.0f);
    return 0;
}

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

void CSoundRender_Emitter::move_cursor(int offset)
{
    set_cursor(get_cursor(true) + offset);
}

void CSoundRender_Emitter::fill_data(u8* _dest, u32 offset, u32 size) const
{
    source()->decompress(_dest, offset, size, target->get_data());
}

void CSoundRender_Emitter::fill_block(void* ptr, u32 size)
{
    // Msg			("stream: %10s - [%X]:%d, p=%d, t=%d",*source->fname,ptr,size,position,source->dwBytesTotal);
    u8* dest = (u8*)(ptr);
    const u32 dwBytesTotal = get_bytes_total();

    if ((get_cursor(true) + size) > dwBytesTotal)
    {
        // We are reaching the end of data, what to do?
        switch (m_current_state)
        {
        case stPlaying:
        { // Fill as much data as we can, zeroing remainder
            if (get_cursor(true) >= dwBytesTotal)
            {
                // ??? We requested the block after remainder - just zero
                memset(dest, 0, size);
            }
            else
            {
                // Calculate remainder
                const u32 sz_data = dwBytesTotal - get_cursor(true);
                const u32 sz_zero = (get_cursor(true) + size) - dwBytesTotal;
                VERIFY(size == (sz_data + sz_zero));
                fill_data(dest, get_cursor(false), sz_data);
                memset(dest + sz_data, 0, sz_zero);
            }
            move_cursor(size);
        }
        break;
        case stPlayingLooped:
        {
            u32 hw_position = 0;
            do
            {
                u32 sz_data = dwBytesTotal - get_cursor(true);
                const u32 sz_write = std::min(size - hw_position, sz_data);
                fill_data(dest + hw_position, get_cursor(true), sz_write);
                hw_position += sz_write;
                move_cursor(sz_write);
                set_cursor(get_cursor(true) % dwBytesTotal);
            } while (0 != (size - hw_position));
        }
        break;
        default: FATAL("SOUND: Invalid emitter state"); break;
        }
    }
    else
    {
        const u32 bt_handle = ((CSoundRender_Source*)owner_data->handle)->dwBytesTotal;
        if (get_cursor(true) + size > m_cur_handle_cursor + bt_handle)
        {
            R_ASSERT(owner_data->fn_attached[0].size());

            u32 rem = 0;
            if ((m_cur_handle_cursor + bt_handle) > get_cursor(true))
            {
                rem = (m_cur_handle_cursor + bt_handle) - get_cursor(true);

#ifdef DEBUG
                Msg("reminder from prev source %d", rem);
#endif // #ifdef DEBUG
                fill_data(dest, get_cursor(false), rem);
                move_cursor(rem);
            }
#ifdef DEBUG
            Msg("recurce from next source %d", size - rem);
#endif // #ifdef DEBUG
            fill_block(dest + rem, size - rem);
        }
        else
        {
            // Everything OK, just stream
            fill_data(dest, get_cursor(false), size);
            move_cursor(size);
        }
    }
}

u32 CSoundRender_Emitter::get_bytes_total() const
{
    return owner_data->dwBytesTotal;
}

float CSoundRender_Emitter::get_length_sec() const
{
    return owner_data->fTimeTotal;
}
