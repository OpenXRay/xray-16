#pragma once
#include "ai/monsters/basemonster/base_monster.h"
#include "ai/monsters/ai_monster_bones.h"
#include "ai/monsters/controlled_entity.h"
#include "ai/monsters/controlled_actor.h"
#include "ai/monsters/anim_triple.h"

#include "bloodsucker_alien.h"

class CAI_Bloodsucker : public CBaseMonster, public CControlledActor
{
    typedef CBaseMonster inherited;

public:
    CAI_Bloodsucker();
    virtual ~CAI_Bloodsucker();

    virtual void reinit();
    virtual void reload(LPCSTR section);

    virtual void UpdateCL();
    virtual void shedule_Update(u32 dt);
    virtual void Die(IGameObject* who);
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void Load(LPCSTR section);
    virtual void Hit(SHit* pHDS);

    virtual void CheckSpecParams(u32 spec_params);
    virtual bool ability_invisibility() { return true; }
    virtual bool ability_pitch_correction() { return false; }
    virtual bool ability_can_drag() { return true; }
    virtual void post_fsm_update();

    virtual bool use_center_to_aim() const { return true; }
    virtual bool check_start_conditions(ControlCom::EControlType);
    virtual void HitEntity(const CEntity* pEntity, float fDamage, float impulse, Fvector& dir, ALife::EHitType hit_type,
        bool draw_hit_marks);

    //--------------------------------------------------------------------
    // Utils
    //--------------------------------------------------------------------
    void move_actor_cam(float angle);
    bool is_collision_off();
    void set_collision_off(bool b_collision);

    //--------------------------------------------------------------------
    // Bones
    //--------------------------------------------------------------------
private:
    static void BoneCallback(CBoneInstance* B);
    void vfAssignBones();
    void LookDirection(Fvector to_dir, float bone_turn_speed);

    bonesManipulation Bones;

    CBoneInstance* bone_spine;
    CBoneInstance* bone_head;
    bool collision_hit_off;
    bool collision_off;

    //--------------------------------------------------------------------
    // Invisibility
    //--------------------------------------------------------------------
private:
    SMotionVel invisible_vel;
    LPCSTR invisible_particle_name;

public:
    void start_invisible_predator();
    void stop_invisible_predator();

    virtual bool in_solid_state();

    //--------------------------------------------------------------------
    // Vampire
    //--------------------------------------------------------------------
public:
    u32 m_vampire_min_delay;
    static u32 m_time_last_vampire;
    SAnimationTripleData anim_triple_vampire;

    SPPInfo pp_vampire_effector;

    void ActivateVampireEffector();
    bool WantVampire();
    void SatisfyVampire();

    u32 get_last_critical_hit_tick() { return m_last_critical_hit_tick; }
    void clear_last_critical_hit_tick() { m_last_critical_hit_tick = 0; }
private:
    TTime m_last_critical_hit_tick;
    float m_critical_hit_chance; // 0..1
    float m_critical_hit_camera_effector_angle;

    float m_vampire_want_value;
    float m_vampire_want_speed; // load from ltx
    float m_vampire_wound;
    float m_vampire_gain_health;
    float m_vampire_distance;
    float m_vis_state;
    bool m_drag_anim_jump;
    bool m_animated;
    static void animation_end_jump(CBlend* B);

    void LoadVampirePPEffector(LPCSTR section);

    //--------------------------------------------------------------------
    // Alien
    //--------------------------------------------------------------------
public:
    CBloodsuckerAlien m_alien_control;
    u32 m_time_lunge;

    void set_alien_control(bool val);

    //--------------------------------------------------------------------
    // Predator
    //--------------------------------------------------------------------
public:
    shared_str m_visual_default;
    LPCSTR m_visual_predator;
    bool m_predator;
    CEntityAlive* m_cob;
    LPCSTR m_str_cel;
    Fvector j_position;
    float j_factor;

    void predator_start();
    void predator_stop();
    void predator_freeze();
    void predator_unfreeze();
    void set_vis();
    void set_invis();
    void jump(const Fvector& position, float factor);
    void start_drag();
    void set_drag_jump(CEntityAlive* e, LPCSTR s, const Fvector& position, float factor);
    bool is_drag_anim_jump();
    bool is_animated();
    //--------------------------------------------------------------------
    // Sounds
    //--------------------------------------------------------------------
public:
    enum EBloodsuckerSounds
    {
        eAdditionalSounds = MonsterSound::eMonsterSoundCustom,

        eVampireGrasp = eAdditionalSounds | 0,
        eVampireSucking = eAdditionalSounds | 1,
        eVampireHit = eAdditionalSounds | 2,
        eVampireStartHunt = eAdditionalSounds | 3,

        eGrowl = eAdditionalSounds | 5,

        eChangeVisibility = eAdditionalSounds | 6,

        eAlien = eAdditionalSounds | 7,
    };

    //--------------------------------------------------------------------

public:
    void set_manual_control(bool value) {}
    void manual_activate();
    void manual_deactivate();

    float get_vampire_distance() const { return m_vampire_distance; }
    pcstr get_monster_class_name() override { return "bloodsucker"; }
    virtual void renderable_Render();

#ifdef DEBUG
    virtual CBaseMonster::SDebugInfo show_debug_info();

    // Lain: added
    virtual void add_debug_info(debug::text_tree& root_s);

#ifdef _DEBUG
    void debug_on_key(int key);
#endif

#endif

    //-------------------------------------------------------------------
    // Bloodsucker's    Visibility States
    //-------------------------------------------------------------------
public:
    enum visibility_t
    {
        unset = -1,
        no_visibility = 0,
        partial_visibility = 1,
        full_visibility = 2
    };

private:
    u32 m_visibility_state_change_min_delay;

    float m_full_visibility_radius;
    float m_partial_visibility_radius;

    visibility_t m_visibility_state;
    visibility_t m_force_visibility_state;
    TTime m_visibility_state_last_changed_time;

    TTime m_runaway_invisible_time;

public:
    float get_full_visibility_radius();
    float get_partial_visibility_radius();
    float get_no_visibility_radius();
    TTime get_visibility_state_change_min_delay();
    void start_runaway_invisible() { m_runaway_invisible_time = Device.dwTimeGlobal; }
    void clear_runaway_invisible() { m_runaway_invisible_time = 0; }
    virtual bool can_be_seen() const { return get_visibility_state() == full_visibility; }
    visibility_t get_visibility_state() const;
    void set_visibility_state(visibility_t new_state);
    void force_visibility_state(int state);
    void update_invisibility();

    void force_stand_sleep_animation(u32 index);
    void release_stand_sleep_animation();

    virtual bool should_wait_to_use_corspe_visual() { return false; }
public:
    u32 m_hits_before_vampire;
    u32 m_sufficient_hits_before_vampire;
    int m_sufficient_hits_before_vampire_random;
    virtual void on_attack_on_run_hit();
    bool done_enough_hits_before_vampire();
};
