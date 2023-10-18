#pragma once

#include "SoundRender.h"
#include "SoundRender_Cache.h"
#include "SoundRender_Environment.h"
#include "SoundRender_Effects.h"
#include "xrCommon/xr_unordered_map.h"

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

protected:
    bool _create_data(ref_sound_data& S, pcstr fName, esound_type sound_type, int game_type, bool replaceWithNoSound = true) override;
    void _destroy_data(ref_sound_data& S) override;

    bool bListenerMoved;

    CSoundRender_Environment e_current;
    CSoundRender_Environment e_target;
    SoundStatistics Stats;

public:
    using event = std::pair<ref_sound_data_ptr, float>;
    xr_vector<event> s_events;

    bool bPresent;
    bool bUserEnvironment;
    bool bReady;

    CTimer Timer;
    CTimer TimerPersistent; // time-factor is always 1.0f
    float fTimer_Value;
    float fTimer_Delta;
    float fTimerPersistent_Value;
    float fTimerPersistent_Delta;

    sound_event* Handler;

protected:
    // Collider
#ifndef _EDITOR
    CDB::COLLIDER geom_DB;
#endif
    CDB::MODEL* geom_SOM;
    CDB::MODEL* geom_MODEL;
    CDB::MODEL* geom_ENV;

    // Containers
    Lock s_sources_lock;
    xr_unordered_map<xr_string, CSoundRender_Source*> s_sources;
    xr_vector<CSoundRender_Emitter*> s_emitters;
    u32 s_emitters_u; // emitter update marker
    xr_vector<CSoundRender_Target*> s_targets;
    xr_vector<CSoundRender_Target*> s_targets_defer;
    u32 s_targets_pu; // parameters update
    SoundEnvironment_LIB* s_environment;
    CSoundRender_Environment s_user_environment;
    CSoundRender_Effects* m_effects{};

    int m_iPauseCounter;

public:
    // Cache
    CSoundRender_Cache cache;
    u32 cache_bytes_per_line;

public:
    CSoundRender_Core();
    virtual ~CSoundRender_Core();

    // General
    void _initialize() override = 0;
    void _clear() override = 0;
    void _restart() override;

    // Sound interface
    void verify_refsound(ref_sound& S);
    bool create(ref_sound& S, pcstr fName, esound_type sound_type, int game_type, bool replaceWithNoSound = true) override;
    void attach_tail(ref_sound& S, pcstr fName) override;

    void clone(ref_sound& S, const ref_sound& from, esound_type sound_type, int game_type) override;
    void destroy(ref_sound& S) override;

    void prefetch() override
    {
        if (!bPresent)
            return;
        i_create_all_sources();
    }

    void stop_emitters() override;
    int pause_emitters(bool val) override;

    void play(ref_sound& S, IGameObject* O, u32 flags = 0, float delay = 0.f) override;
    void play_at_pos(ref_sound& S, IGameObject* O, const Fvector& pos, u32 flags = 0, float delay = 0.f) override;
    void play_no_feedback(ref_sound& S, IGameObject* O, u32 flags = 0, float delay = 0.f, Fvector* pos = nullptr,
                          float* vol = nullptr, float* freq = nullptr, Fvector2* range = nullptr) override;
    void set_master_volume(float f) override = 0;
    void set_geometry_env(IReader* I) override;
    void set_geometry_som(IReader* I) override;
    void set_geometry_occ(CDB::MODEL* M) override;
    void set_handler(sound_event* E) override;

    void update(const Fvector& P, const Fvector& D, const Fvector& N) override;
    virtual void update_events();
    void statistic(CSound_stats* dest, CSound_stats_ext* ext) override;
    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;

    // listener
    //	virtual const Fvector&				listener_position		( )=0;
    virtual void update_listener(const Fvector& P, const Fvector& D, const Fvector& N, float dt) = 0;

    virtual SoundEnvironment_LIB* get_env_library() { return s_environment; }
    virtual void refresh_env_library();
    virtual void set_user_env(CSound_environment* E);
    virtual void refresh_sources();
    virtual void set_environment(u32 id, CSound_environment** dst_env);
    virtual void set_environment_size(CSound_environment* src_env, CSound_environment** dst_env);

public:
    bool i_create_source(CSound_source*& result, pcstr name, bool replaceWithNoSound = true);
    bool i_create_source(CSoundRender_Source*& result, pcstr name, bool replaceWithNoSound = true);
    CSoundRender_Source* i_create_source(pcstr name, bool replaceWithNoSound = true);
    void i_create_all_sources();

    void i_destroy_source(CSoundRender_Source* S);
    CSoundRender_Emitter* i_play(ref_sound* S, u32 flags, float delay);
    void i_start(CSoundRender_Emitter* E);
    void i_stop(CSoundRender_Emitter* E);
    void i_rewind(CSoundRender_Emitter* E);
    bool i_allow_play(CSoundRender_Emitter* E);
    bool i_locked() override { return isLocked; }
    void object_relcase(IGameObject* obj) override;

    float get_occlusion_to(const Fvector& hear_pt, const Fvector& snd_pt, float dispersion = 0.2f) override;
    float get_occlusion(Fvector& P, float R, Fvector* occ) override;
    CSoundRender_Environment* get_environment(const Fvector& P);

    void env_load();
    void env_unload();
    void env_apply();
};

extern CSoundRender_Core* SoundRender;
