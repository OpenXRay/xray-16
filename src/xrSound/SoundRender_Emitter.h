#pragma once

#include "xrCore/_std_extensions.h"

#include "SoundRender.h"
#include "SoundRender_Scene.h"

class CSoundRender_Emitter final : public CSound_emitter
{
public:
    enum State : u32
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
    };

public:
    static constexpr float TIME_TO_STOP_INFINITE = static_cast<float>(0xffffffff);

    CSoundRender_Target* target{};
    CSoundRender_Scene* scene{};
    ref_sound owner_data;

    [[nodiscard]]
    CSoundRender_Source* source() const { return (CSoundRender_Source*)owner_data->handle; }

    [[nodiscard]]
    u32 get_bytes_total() const;
    [[nodiscard]]
    float get_length_sec() const;

    float starting_delay{};
    float priority_scale;
    float smooth_volume;
    float occluder_volume; // USER
    float fade_volume;
    Fvector occluder[3]{};

    State m_current_state;
    u32 m_stream_cursor{};
    u32 m_cur_handle_cursor{};
    CSound_params p_source;

    int iPaused{};
    bool bMoved;
    bool b2D{};
    bool bStopping{};
    bool bRewind{};
    bool bIgnoringTimeFactor{};
    float fTimeStarted{}; // time of "Start"
    float fTimeToStop{}; // time to "Stop"
    float fTimeToPropagade{};
    float fTimeToRewind{}; // --#SM+#--

    u32 marker;
    void i_stop();

    [[nodiscard]]
    u32  get_cursor(bool b_absolute) const;
    void set_cursor(u32 p);
    void move_cursor(int offset);

public:
    void Event_Propagade();
    void Event_ReleaseOwner();
    bool isPlaying() { return m_current_state != stStopped; }
    bool is_2D() override { return b2D; }
    void switch_to_2D() override;
    void switch_to_3D() override;

    void set_position(const Fvector& pos) override;
    void set_frequency(float scale) override;

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
    void set_time(float t) override; //--#SM+#--
    const CSound_params* get_params() override { return &p_source; }
    void fill_block(void* ptr, u32 size);
    void fill_data(void* dest, u32 offset, u32 size) const;

    float priority() const;
    void start(const ref_sound& _owner, u32 flags, float delay);
    void cancel(); // manager forces out of rendering
    void update(float time, float dt);
    bool update_culling(float dt);
    void update_environment(float dt);
    void rewind();
    void stop(bool isDeffered) override;
    void pause(bool bVal, int id);

    u32 play_time() override;

    void set_ignore_time_factor(bool ignore) override { bIgnoringTimeFactor = ignore; };

    CSoundRender_Emitter(CSoundRender_Scene* s);
    ~CSoundRender_Emitter() override;
};
