#pragma once

#include "SoundRender.h"
#include "SoundRender_Environment.h"
#include "SoundRender_Cache.h"

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
    void _create_data(ref_sound_data& S, pcstr fName, esound_type sound_type, int game_type) override;
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
    bool bEAX; // Boolean variable to indicate presence of EAX Extension
    bool bDeferredEAX;
    bool bReady;

    CTimer Timer;
    float fTimer_Value;
    float fTimer_Delta;
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
    xr_vector<CSoundRender_Source*> s_sources;
    xr_vector<CSoundRender_Emitter*> s_emitters;
    u32 s_emitters_u; // emitter update marker
    xr_vector<CSoundRender_Target*> s_targets;
    xr_vector<CSoundRender_Target*> s_targets_defer;
    u32 s_targets_pu; // parameters update
    SoundEnvironment_LIB* s_environment;
    CSoundRender_Environment s_user_environment;

    int m_iPauseCounter;

public:
    // Cache
    CSoundRender_Cache cache;
    u32 cache_bytes_per_line;

protected:
#if defined(WINDOWS)
    virtual void i_eax_set(const GUID* guid, u32 prop, void* val, u32 sz) = 0;
    virtual void i_eax_get(const GUID* guid, u32 prop, void* val, u32 sz) = 0;
#endif

public:
    CSoundRender_Core();
    virtual ~CSoundRender_Core();

    // General
    void _initialize() override = 0;
    void _clear() override = 0;
    void _restart() override;

    // Sound interface
    void verify_refsound(ref_sound& S);
    void create(ref_sound& S, pcstr fName, esound_type sound_type, int game_type) override;
    void attach_tail(ref_sound& S, pcstr fName) override;

    void clone(ref_sound& S, const ref_sound& from, esound_type sound_type, int game_type) override;
    void destroy(ref_sound& S) override;
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
#if defined(WINDOWS)
    // eax listener
    void i_eax_commit_setting();
    void i_eax_listener_set(CSound_environment* E);
    void i_eax_listener_get(CSound_environment* E);
#endif
    virtual SoundEnvironment_LIB* get_env_library() { return s_environment; }
    virtual void refresh_env_library();
    virtual void set_user_env(CSound_environment* E);
    virtual void refresh_sources();
    virtual void set_environment(u32 id, CSound_environment** dst_env);
    virtual void set_environment_size(CSound_environment* src_env, CSound_environment** dst_env);

public:
    CSoundRender_Source* i_create_source(pcstr name);
    void i_destroy_source(CSoundRender_Source* S);
    CSoundRender_Emitter* i_play(ref_sound* S, bool _loop, float delay);
    void i_start(CSoundRender_Emitter* E);
    void i_stop(CSoundRender_Emitter* E);
    void i_rewind(CSoundRender_Emitter* E);
    bool i_allow_play(CSoundRender_Emitter* E);
    bool i_locked() override { return isLocked; }
    void object_relcase(IGameObject* obj) override;

    float get_occlusion_to(const Fvector& hear_pt, const Fvector& snd_pt, float dispersion = 0.2f) override;
    float get_occlusion(Fvector& P, float R, Fvector* occ);
    CSoundRender_Environment* get_environment(const Fvector& P);

    void env_load();
    void env_unload();
    void env_apply();
};

extern CSoundRender_Core* SoundRender;
