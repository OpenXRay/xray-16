#pragma once
#include "ai/monsters/basemonster/base_monster.h"
#include "ai/monsters/telekinesis.h"
#include "ai/monsters/energy_holder.h"

class CPhysicsShellHolder;
class CStateManagerPoltergeist;
class CPoltergeisMovementManager;
class CPolterSpecialAbility;
class CPolterTele;

//////////////////////////////////////////////////////////////////////////

class CPoltergeist : public CBaseMonster, public CTelekinesis, public CEnergyHolder
{
    typedef CBaseMonster inherited;
    typedef CEnergyHolder Energy;

    friend class CPoltergeisMovementManager;
    friend class CPolterTele;

    float m_height;
    bool m_disable_hide;

    SMotionVel invisible_vel;

    CPolterSpecialAbility* m_flame;
    CPolterSpecialAbility* m_tele;

    bool m_actor_ignore;

    TTime m_last_detection_time;
    Fvector m_last_actor_pos;
    char const* m_detection_pp_effector_name;
    u32 m_detection_pp_type_index;
    float m_detection_near_range_factor;
    float m_detection_far_range_factor;
    float m_detection_far_range;
    float m_detection_speed_factor;
    float m_detection_loose_speed;
    float m_current_detection_level;
    float m_detection_success_level;
    float m_detection_max_level;

public:
    bool m_detect_without_sight;

public:
    CPoltergeist();
    virtual ~CPoltergeist();

    virtual void Load(LPCSTR section);
    virtual void reload(LPCSTR section);
    virtual void reinit();

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void net_Relcase(IGameObject* O);

    virtual void UpdateCL();
    virtual void shedule_Update(u32 dt);

    void set_actor_ignore(bool const actor_ignore) { m_actor_ignore = actor_ignore; }
    bool get_actor_ignore() const { return m_actor_ignore; }
    virtual void Die(IGameObject* who);

    virtual CMovementManager* create_movement_manager();

    virtual void ForceFinalAnimation();

    virtual void on_activate();
    virtual void on_deactivate();
    virtual void Hit(SHit* pHDS);
    pcstr get_monster_class_name() override { return "poltergeist"; }
    bool detected_enemy();
    float get_fly_around_distance() const { return m_fly_around_distance; }
    float get_fly_around_change_direction_time() const { return m_fly_around_change_direction_time; }
    virtual void renderable_Render();

    IC CPolterSpecialAbility* ability() { return (m_flame ? m_flame : m_tele); }
    IC bool is_hidden() { return state_invisible; }
    // Poltergeist ability
    void PhysicalImpulse(const Fvector& position);
    void StrangeSounds(const Fvector& position);

    ref_sound m_strange_sound;

    // Movement
    Fvector m_current_position; // Позиция на ноде

    // Dynamic Height
    u32 time_height_updated;
    float target_height;

    void UpdateHeight();

    // Invisibility

    void EnableHide() { m_disable_hide = false; }
    void DisableHide() { m_disable_hide = true; }
public:
    virtual bool run_home_point_when_enemy_inaccessible() const { return false; }
private:
    void Hide();
    void Show();

    float m_height_change_velocity;
    u32 m_height_change_min_time;
    u32 m_height_change_max_time;
    float m_height_min;
    float m_height_max;

    float m_fly_around_level;
    float m_fly_around_distance;
    float m_fly_around_change_direction_time;

    float get_current_detection_level() const { return m_current_detection_level; }
    bool check_work_condition() const;
    void remove_pp_effector();
    void update_detection();

    float get_detection_near_range_factor();
    float get_detection_far_range_factor();
    float get_detection_loose_speed();
    float get_detection_far_range();
    float get_detection_speed_factor();
    float get_detection_success_level();
    float xr_stdcall get_post_process_factor() const;

public:
#ifdef DEBUG
    virtual CBaseMonster::SDebugInfo show_debug_info();
#endif

    friend class CPolterFlame;
};

//////////////////////////////////////////////////////////////////////////
// Interface
//////////////////////////////////////////////////////////////////////////

class CPolterSpecialAbility
{
    CParticlesObject* m_particles_object;
    CParticlesObject* m_particles_object_electro;

    LPCSTR m_particles_hidden;
    LPCSTR m_particles_damage;
    LPCSTR m_particles_death;
    LPCSTR m_particles_idle;

    ref_sound m_sound_base;
    u32 m_last_hit_frame;

protected:
    CPoltergeist* m_object;

public:
    CPolterSpecialAbility(CPoltergeist* polter);
    virtual ~CPolterSpecialAbility();

    virtual void load(LPCSTR section);
    virtual void update_schedule();
    virtual void update_frame();
    virtual void on_hide();
    virtual void on_show();
    virtual void on_destroy() {}
    virtual void on_die();
    virtual void on_hit(SHit* pHDS);
};

//////////////////////////////////////////////////////////////////////////
// Flame
//////////////////////////////////////////////////////////////////////////
class CPolterFlame : public CPolterSpecialAbility
{
    typedef CPolterSpecialAbility inherited;

    ref_sound m_sound;
    LPCSTR m_particles_prepare;
    LPCSTR m_particles_fire;
    LPCSTR m_particles_stop;
    u32 m_time_fire_delay;
    u32 m_time_fire_play;

    float m_length;
    float m_hit_value;
    u32 m_hit_delay;

    u32 m_count;
    u32 m_delay; // between 2 flames

    u32 m_time_flame_started;

    float m_min_flame_dist;
    float m_max_flame_dist;
    float m_min_flame_height;
    float m_max_flame_height;

    float m_pmt_aura_radius;

    // Scanner
    float m_scan_radius;
    u32 m_scan_delay_min;
    u32 m_scan_delay_max;

    SPPInfo m_scan_effector_info;
    float m_scan_effector_time;
    float m_scan_effector_time_attack;
    float m_scan_effector_time_release;
    ref_sound m_scan_sound;

    bool m_state_scanning;
    u32 m_scan_next_time;

    enum EFlameState
    {
        ePrepare,
        eFire,
        eStop
    };

public:
    struct SFlameElement
    {
        const IGameObject* target_object;
        Fvector position;
        Fvector target_dir;
        u32 time_started;
        ref_sound sound;
        CParticlesObject* particles_object;
        EFlameState state;
        u32 time_last_hit;
    };

private:
    using FLAME_ELEMS_VEC = xr_vector<SFlameElement*>;
    FLAME_ELEMS_VEC m_flames;

public:
    CPolterFlame(CPoltergeist* polter);
    virtual ~CPolterFlame();

    virtual void load(LPCSTR section);
    virtual void update_schedule();
    virtual void on_destroy();
    virtual void on_die();

private:
    void select_state(SFlameElement* elem, EFlameState state);
    bool get_valid_flame_position(const IGameObject* target_object, Fvector& res_pos);
    void create_flame(const IGameObject* target_object);
};

//////////////////////////////////////////////////////////////////////////
// TELE
//////////////////////////////////////////////////////////////////////////
class CPolterTele : public CPolterSpecialAbility
{
    typedef CPolterSpecialAbility inherited;

    xr_vector<IGameObject*> m_nearest;

    // external params
    float m_pmt_radius;
    float m_pmt_object_min_mass;
    float m_pmt_object_max_mass;
    u32 m_pmt_object_count;
    u32 m_pmt_time_to_hold;
    u32 m_pmt_time_to_wait;
    u32 m_pmt_time_to_wait_in_objects;
    u32 m_pmt_raise_time_to_wait_in_objects;
    float m_pmt_distance;
    float m_pmt_object_height;
    u32 m_pmt_time_object_keep;
    float m_pmt_raise_speed;
    float m_pmt_fly_velocity;

    float m_pmt_object_collision_damage;

    ref_sound m_sound_tele_hold;
    ref_sound m_sound_tele_throw;

    enum ETeleState
    {
        eStartRaiseObjects,
        eRaisingObjects,
        eFireObjects,
        eWait
    } m_state;

    u32 m_time;
    u32 m_time_next;

public:
    CPolterTele(CPoltergeist* polter);
    virtual ~CPolterTele();

    virtual void load(LPCSTR section);
    virtual void update_schedule();
    virtual void update_frame();

private:
    void tele_find_objects(xr_vector<IGameObject*>& objects, const Fvector& pos);
    bool tele_raise_objects();
    void tele_fire_objects();

    bool trace_object(IGameObject* obj, const Fvector& target);
};
