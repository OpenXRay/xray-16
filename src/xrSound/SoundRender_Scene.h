#pragma once

class CSoundRender_Emitter;

class CSoundRender_Scene final : public ISoundScene
{
public:
    CSoundRender_Scene();
    ~CSoundRender_Scene() override;

    void stop_emitters() const override;
    int pause_emitters(bool pauseState) override;

    void set_handler(sound_event* E) override;
    void set_geometry_som(IReader* I) override;
    void set_geometry_occ(CDB::MODEL* M, const Fbox& aabb) override;

    void play(ref_sound& S, IGameObject* O, u32 flags = 0, float delay = 0.f) override;
    void play_at_pos(ref_sound& S, IGameObject* O, const Fvector& pos, u32 flags = 0, float delay = 0.f) override;
    void play_no_feedback(ref_sound& S, IGameObject* O, u32 flags = 0, float delay = 0.f, Fvector* pos = nullptr,
        float* vol = nullptr, float* freq = nullptr, Fvector2* range = nullptr) override;

    float get_occlusion_to(const Fvector& hear_pt, const Fvector& snd_pt, float dispersion = 0.2f) override;
    float get_occlusion(const Fvector& P, float R, Fvector* occ) override;

    void object_relcase(IGameObject* obj) override;

public:
    CSoundRender_Emitter* i_play(ref_sound& S, u32 flags, float delay);

    void update();

    auto  get_events_handler() const { return sound_event_handler; }
    auto& get_events() { return s_events; }
    auto& get_prev_events_count() { return s_events_prev_count; }

    auto& get_emitters() { return s_emitters; }

public:
#ifdef USE_PHONON
    IPLScene ipl_scene{};
    IPLStaticMesh ipl_scene_mesh{};
    IPLProbeBatch ipl_scene_probes{};
    IPLSimulator ipl_simulator{};
#endif

private:
    xr_vector<CSoundRender_Emitter*> s_emitters;

    using event = std::pair<ref_sound, float>;
    xr_vector<event> s_events;
    size_t s_events_prev_count{};

    sound_event* sound_event_handler{};

    // Collider
    CDB::COLLIDER geom_DB;
    CDB::MODEL* geom_SOM{};
    CDB::MODEL* geom_MODEL{};

    int m_iPauseCounter{ 1 };
};
