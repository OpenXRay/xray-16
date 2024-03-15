#pragma once

#include "xrCommon/xr_unordered_map.h"

#include "SoundRender.h"
#include "SoundRender_Environment.h"
#include "SoundRender_Effects.h"
#include "SoundRender_Scene.h"

class CSoundRender_Core : public ISoundManager
{
protected:
    struct SoundStatistics
    {
        CStatTimer Update; // total time taken by sound subsystem (accurate only in single-threaded mode)

        SoundStatistics() { FrameStart(); }
        void FrameStart() { Update.FrameStart(); }
        void FrameEnd() { Update.FrameEnd(); }
    };

private:
    volatile bool isLocked;

public:
    struct SListener
    {
        Fvector position;
        Fvector orientation[3];

        [[nodiscard]]
        SListener ToRHS() const
        {
            return
            {
                { position.x, position.y, -position.z },
                {
                    { orientation[0].x, orientation[0].y, -orientation[0].z },
                    { orientation[1].x, orientation[1].y, -orientation[1].z },
                    { orientation[2].x, orientation[2].y, -orientation[2].z },
                },
            };
        }
    };

protected:
    SListener Listener;

    bool bListenerMoved{};

    CSoundRender_Environment e_current;
    CSoundRender_Environment e_target;
    SoundStatistics Stats;

public:
    CSoundManager& Parent;

    bool bPresent;
    bool bReady;

    CTimer Timer;
    CTimer TimerPersistent; // time-factor is always 1.0f
    float fTimer_Value;
    float fTimer_Delta;
    float fTimerPersistent_Value;
    float fTimerPersistent_Delta;

protected:
    // Containers
    xr_vector<CSoundRender_Scene*> m_scenes;

    Lock s_sources_lock;
    xr_unordered_map<xr_string, CSoundRender_Source*> s_sources;

    u32 s_emitters_u; // emitter update marker
    xr_vector<CSoundRender_Target*> s_targets;
    xr_vector<CSoundRender_Target*> s_targets_defer;
    u32 s_targets_pu; // parameters update

    CSoundRender_Effects* m_effects{};

public:
    bool supports_float_pcm{};

public:
    CSoundRender_Core(CSoundManager& p);

    // General
    virtual void _initialize_devices_list() = 0;
    virtual void _initialize() = 0;
    virtual void _clear() = 0;

    ISoundScene* create_scene() override;
    void destroy_scene(ISoundScene*&) override;

    void _restart() override;

    // Sound interface
    CSound* create(pcstr fName, esound_type sound_type, int game_type, bool replaceWithNoSound = true) override;
    void attach_tail(CSound& S, pcstr fName) override;

    void destroy(CSound& S) override;

    void prefetch() override
    {
        if (!bPresent)
            return;
        i_create_all_sources();
    }

    void stop_emitters() override;
    int pause_emitters(bool pauseState) override;

    void set_master_volume(float f) override = 0;

    void update(const Fvector& P, const Fvector& D, const Fvector& N, const Fvector& R) override;
    void statistic(CSound_stats* dest, CSound_stats_ext* ext) override;
    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;

    // listener
    const auto& listener_params() const { return Listener; }
    const Fvector& listener_position() override { return Listener.position; }
    virtual void update_listener(const Fvector& P, const Fvector& D, const Fvector& N, const Fvector& R, float dt);

    void refresh_sources() override;

public:
    bool i_create_source(CSound_source*& result, pcstr name, bool replaceWithNoSound = true);
    bool i_create_source(CSoundRender_Source*& result, pcstr name, bool replaceWithNoSound = true);
    CSoundRender_Source* i_create_source(pcstr name, bool replaceWithNoSound = true);
    void i_create_all_sources();

    void i_destroy_source(CSoundRender_Source* S);
    void i_start(CSoundRender_Emitter* E);
    void i_stop(CSoundRender_Emitter* E);
    void i_rewind(CSoundRender_Emitter* E);
    bool i_allow_play(CSoundRender_Emitter* E);
    bool i_locked() override { return isLocked; }

    void env_apply();
};

extern CSoundRender_Core* SoundRender;
