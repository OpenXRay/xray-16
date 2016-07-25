#pragma once

#include "SoundRender.h"
#include "SoundRender_Environment.h"
#include "xrCore/_std_extensions.h"

class CSoundRender_Emitter : public CSound_emitter
{
    float starting_delay;

public:
    enum State
    {
        stStopped = 0,

        stStartingDelayed,
        stStartingLoopedDelayed,

        stStarting,
        stStartingLooped,

        stPlaying,
        stPlayingLooped,

        stSimulating,
        stSimulatingLooped,

        stFORCEDWORD = u32(-1)
    };

public:
#ifdef DEBUG
    u32 dbg_ID;
#endif

    CSoundRender_Target* target;
    CSoundRender_Source* source() { return (CSoundRender_Source*)owner_data->handle; };
    ref_sound_data_ptr owner_data;

    u32 get_bytes_total() const;
    float get_length_sec() const;

    float priority_scale;
    float smooth_volume;
    float occluder_volume; // USER
    float fade_volume;
    Fvector occluder[3];

    State m_current_state;
    u32 m_stream_cursor;
    u32 m_cur_handle_cursor;
    CSound_params p_source;
    CSoundRender_Environment e_current;
    CSoundRender_Environment e_target;

    int iPaused;
    bool bMoved;
    bool b2D;
    bool bStopping;
    bool bRewind;
    float fTimeStarted; // time of "Start"
    float fTimeToStop; // time to "Stop"
    float fTimeToPropagade;

    u32 marker;
    void i_stop();

    void set_cursor(u32 p);
    u32 get_cursor(bool b_absolute) const;
    void move_cursor(int offset);

public:
    void Event_Propagade();
    void Event_ReleaseOwner();
    bool isPlaying() { return m_current_state != stStopped; }
    bool is_2D() override { return b2D; }
    void switch_to_2D() override;
    void switch_to_3D() override;
    void set_position(const Fvector& pos) override;

    void set_frequency(float scale) override
    {
        VERIFY(_valid(scale));
        p_source.freq = scale;
    }

    void set_range(float min, float max) override
    {
        VERIFY(_valid(min) && _valid(max));
        p_source.min_distance = min;
        p_source.max_distance = max;
    }

    void set_volume(float vol) override
    {
        if (!_valid(vol))
            vol = 0.0f;
        p_source.volume = vol;
    }

    void set_priority(float p) override { priority_scale = p; }
    const CSound_params* get_params() override { return &p_source; }
    void fill_block(void* ptr, u32 size);
    void fill_data(u8* ptr, u32 offset, u32 size);

    float priority();
    void start(ref_sound* _owner, bool _loop, float delay);
    void cancel(); // manager forces out of rendering
    void update(float dt);
    bool update_culling(float dt);
    void update_environment(float dt);
    void rewind();
    void stop(bool isDeffered) override;
    void pause(bool bVal, int id);

    u32 play_time() override;

    CSoundRender_Emitter();
    ~CSoundRender_Emitter();
};
