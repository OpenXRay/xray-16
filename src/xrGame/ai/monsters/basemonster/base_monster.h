#pragma once

#include "CustomMonster.h"
#include "ai/Monsters/monster_enemy_memory.h"
#include "ai/Monsters/monster_corpse_memory.h"
#include "ai/Monsters/monster_sound_memory.h"
#include "ai/Monsters/monster_hit_memory.h"
#include "ai/Monsters/monster_enemy_manager.h"
#include "ai/Monsters/monster_corpse_manager.h"
#include "step_manager.h"
#include "ai/Monsters/monster_event_manager.h"
#include "ai/Monsters/melee_checker.h"
#include "ai/Monsters/monster_morale.h"
#include "ai/Monsters/control_manager.h"
#include "ai/Monsters/control_sequencer.h"
#include "ai/Monsters/ai_monster_utils.h"
#include "ai/Monsters/control_manager_custom.h"
#include "ai/Monsters/ai_monster_shared_data.h"
#include "ai/Monsters/monster_sound_defs.h"
#include "ai/Monsters/monster_aura.h"

class CCharacterPhysicsSupport;
class CMonsterCorpseCoverEvaluator;
class CCoverEvaluatorFarFromEnemy;
class CCoverEvaluatorCloseToEnemy;
class CMonsterEventManager;
class CJumping;
class CControlledEntityBase;
class CMovementManager;
class IStateManagerBase;
class CAnomalyDetector;

class CControlAnimationBase;
class CControlMovementBase;
class CControlPathBuilderBase;
class CControlDirectionBase;
class CMonsterCoverManager;

class CMonsterHome;

// Lain: added
class CMonsterSquad;
class squad_grouping_behaviour;

#ifdef DEBUG
namespace debug
{
class text_tree;
}
#endif

class anti_aim_ability;

class CBaseMonster : public CCustomMonster, public CStepManager
{
protected:
    using inherited = CCustomMonster;

public:
    CBaseMonster();
    virtual ~CBaseMonster();

public:
    virtual Feel::Sound* dcast_FeelSound() { return this; }
    virtual CCharacterPhysicsSupport* character_physics_support() { return m_pPhysics_support; }
    virtual const CCharacterPhysicsSupport* character_physics_support() const { return m_pPhysics_support; }
    virtual CPHDestroyable* ph_destroyable();
    virtual CEntityAlive* cast_entity_alive() { return this; }
    virtual CEntity* cast_entity() { return this; }
    virtual CPhysicsShellHolder* cast_physics_shell_holder() { return this; }
    virtual CParticlesPlayer* cast_particles_player() { return this; }
    virtual CCustomMonster* cast_custom_monster() { return this; }
    virtual CScriptEntity* cast_script_entity() { return this; }
    virtual CBaseMonster* cast_base_monster() { return this; }
    virtual CGameObject* cast_game_object() { return this; }

public:
    virtual BOOL renderable_ShadowReceive() { return TRUE; }
    virtual void Die(IGameObject* who);
    virtual void HitSignal(float amount, Fvector& vLocalDir, IGameObject* who, s16 element);
    virtual void Hit(SHit* pHDS);
    virtual void PHHit(SHit& H);
    virtual void SelectAnimation(const Fvector& _view, const Fvector& _move, float speed);

    virtual void Load(LPCSTR section);

    // must be called at the end of most derived's Load
    virtual void PostLoad(LPCSTR section);

    virtual IFactoryObject* _construct();

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void net_Save(NET_Packet& P);
    virtual BOOL net_SaveRelevant();
    virtual void net_Export(NET_Packet& P);
    virtual void net_Import(NET_Packet& P);
    virtual void net_Relcase(IGameObject* O);

    // save/load server serialization
    virtual void save(NET_Packet& output_packet) { inherited::save(output_packet); }
    virtual void load(IReader& input_packet) { inherited::load(input_packet); }
    virtual void UpdateCL();
    virtual void shedule_Update(u32 dt);

    virtual void InitThink() {}
    virtual void Think();
    virtual void reinit();
    virtual void reload(LPCSTR section);

    virtual void init() {}
    virtual void feel_sound_new(
        IGameObject* who, int eType, CSound_UserDataPtr user_data, const Fvector& Position, float power);
    virtual bool feel_vision_isRelevant(IGameObject* O);
    virtual bool feel_touch_on_contact(IGameObject* O);
    virtual bool feel_touch_contact(IGameObject*);

    virtual bool useful(const CItemManager* manager, const CGameObject* object) const;
    virtual float evaluate(const CItemManager* manager, const CGameObject* object) const;

    virtual void OnEvent(NET_Packet& P, u16 type);
    virtual void OnHUDDraw(CCustomHUD* hud) { return inherited::OnHUDDraw(hud); }
    virtual u16 PHGetSyncItemsNumber() { return inherited::PHGetSyncItemsNumber(); }
    virtual CPHSynchronize* PHGetSyncItem(u16 item) { return inherited::PHGetSyncItem(item); }
    virtual void PHUnFreeze() { return inherited::PHUnFreeze(); }
    virtual void PHFreeze() { return inherited::PHFreeze(); }
    virtual BOOL UsedAI_Locations() { return inherited::UsedAI_Locations(); }
    virtual const SRotation Orientation() const { return inherited::Orientation(); }
    virtual void renderable_Render() { return inherited::renderable_Render(); }
    virtual void on_restrictions_change();

    virtual void SetAttackEffector();

    virtual void update_fsm();

    virtual void post_fsm_update();
    void squad_notify();

    virtual bool IsTalkEnabled() { return false; }
    virtual void HitEntity(const CEntity* pEntity, float fDamage, float impulse, Fvector& dir,
        ALife::EHitType hit_type = ALife::eHitTypeWound, bool draw_hit_marks = true);

    virtual void HitEntityInJump(const CEntity* pEntity) {}
    virtual void on_before_sell(CInventoryItem* item);
    float GetSatiety() { return 0.5f; }
    void ChangeSatiety(float v) {}
    // ---------------------------------------------------------------------------------
    // Process scripts
    // ---------------------------------------------------------------------------------
    virtual bool bfAssignMovement(CScriptEntityAction* tpEntityAction);
    bool AssignGamePathIfNeeded(Fvector target_pos, u32 level_vertex);
    virtual bool bfAssignObject(CScriptEntityAction* tpEntityAction);
    virtual bool bfAssignWatch(CScriptEntityAction* tpEntityAction);
    virtual bool bfAssignAnimation(CScriptEntityAction* tpEntityAction);
    virtual bool bfAssignMonsterAction(CScriptEntityAction* tpEntityAction);
    virtual bool bfAssignSound(CScriptEntityAction* tpEntityAction);

    virtual void vfFinishAction(CScriptEntityAction* tpEntityAction);

    virtual void ProcessScripts();

    virtual CEntity* GetCurrentEnemy();
    virtual CEntity* GetCurrentCorpse();
    virtual int get_enemy_strength();

    virtual void SetScriptControl(const bool bScriptControl, shared_str caSciptName);

    virtual void SetEnemy(const CEntityAlive* sent);
    bool m_force_real_speed;
    bool m_script_processing_active;
    bool m_script_state_must_execute;

    virtual void jump(const Fvector& position, float factor) {}
    bool m_skip_transfer_enemy;
    IC void skip_transfer_enemy(bool value) { m_skip_transfer_enemy = value; }
    IC int Rank() { return m_rank; }
    //----------------------------------------------------------------------------------

    virtual void SetTurnAnimation(bool turn_left);

    // установка специфических анимаций
    virtual void CheckSpecParams(u32 /**spec_params/**/) {}
    virtual void ForceFinalAnimation() {}
    virtual void LookPosition(Fvector to_point,
        float angular_speed =
            PI_DIV_3); // каждый монстр может по-разному реализвать эту функ (e.g. кровосос с поворотом головы и т.п.)

    // Team
    virtual void ChangeTeam(int team, int squad, int group);

    // ---------------------------------------------------------------------------------
    // Abilities
    // ---------------------------------------------------------------------------------
    virtual bool ability_invisibility() { return false; }
    virtual bool ability_can_drag() { return false; }
    virtual bool ability_psi_attack() { return false; }
    virtual bool ability_earthquake() { return false; }
    virtual bool ability_can_jump() { return false; }
    virtual bool ability_distant_feel() { return false; }
    virtual bool ability_run_attack() { return false; }
    virtual bool ability_rotation_jump() { return false; }
    virtual bool ability_jump_over_physics() { return false; }
    virtual bool ability_pitch_correction() { return true; }
    // ---------------------------------------------------------------------------------

    virtual void event_on_step() {}
    virtual void on_threaten_execute() {}
    // ---------------------------------------------------------------------------------
    // Memory
    void UpdateMemory();

    // Cover
    bool GetCorpseCover(Fvector& position, u32& vertex_id);
    bool GetCoverFromEnemy(const Fvector& enemy_pos, Fvector& position, u32& vertex_id);
    bool GetCoverFromPoint(
        const Fvector& pos, Fvector& position, u32& vertex_id, float min_dist, float max_dist, float radius);
    bool GetCoverCloseToPoint(const Fvector& dest_pos, float min_dist, float max_dist, float deviation, float radius,
        Fvector& position, u32& vertex_id);

    // Movement Manager
protected:
    CControlPathBuilder* m_movement_manager;

protected:
    virtual CMovementManager* create_movement_manager();

    // members
public:
    void set_force_anti_aim(bool force_anti_aim) { m_force_anti_aim = force_anti_aim; }
    bool get_force_anti_aim() const { return m_force_anti_aim; }
    // --------------------------------------------------------------------------------------
    // Monster Settings
    ref_smem<SMonsterSettings> m_base_settings;
    ref_smem<SMonsterSettings> m_current_settings;

    void settings_read(CInifile const* ini, LPCSTR section, SMonsterSettings& data);
    void settings_load(LPCSTR section);
    void settings_overrides();

    SMonsterSettings& db() { return *(*m_current_settings); }
    // --------------------------------------------------------------------------------------

    CCharacterPhysicsSupport* m_pPhysics_support;

    CMonsterCorpseCoverEvaluator* m_corpse_cover_evaluator;
    CCoverEvaluatorFarFromEnemy* m_enemy_cover_evaluator;
    CCoverEvaluatorCloseToEnemy* m_cover_evaluator_close_point;

    // ---------------------------------------------------------------------------------
    IStateManagerBase* StateMan;
    // ---------------------------------------------------------------------------------

    CMonsterEnemyMemory EnemyMemory;
    CMonsterSoundMemory SoundMemory;
    CMonsterCorpseMemory CorpseMemory;
    CMonsterHitMemory HitMemory;

    CMonsterEnemyManager EnemyMan;
    CMonsterCorpseManager CorpseMan;

    const CEntityAlive* EatedCorpse;
    // Lain: added
    bool check_eated_corpse_draggable();
    virtual bool is_base_monster_with_enemy() { return EnemyMan.get_enemy() != NULL; }
    bool hear_dangerous_sound;
    bool hear_interesting_sound;

    // -----------------------------------------------------------------------------
    CMonsterEventManager EventMan;
    // -----------------------------------------------------------------------------

    CMeleeChecker MeleeChecker;
    CMonsterMorale Morale;

    // -----------------------------------------------------------------------------

    CMonsterCoverManager* CoverMan;

    // -----------------------------------------------------------------------------

    CControlledEntityBase* m_controlled;

    // -----------------------------------------------------------------------------
    enum EMonsterType
    {
        eMonsterTypeUniversal = u32(0),
        eMonsterTypeIndoor,
        eMonsterTypeOutdoor,
    } m_monster_type;

    // -----------------------------------------------------------------------------
    // Home
    CMonsterHome* Home;

    // -----------------------------------------------------------------------------
    // Anomaly Detector
private:
    CAnomalyDetector* m_anomaly_detector;
    bool m_force_anti_aim;

public:
    CAnomalyDetector& anomaly_detector() { return (*m_anomaly_detector); }
    // -----------------------------------------------------------------------------

    //	//-----------------------------------------------------------------
    //	// Spawn Inventory Item
    //	//-----------------------------------------------------------------
    // private:
    //	LPCSTR					m_item_section;
    //	float					m_spawn_probability;

    //--------------------------------------------------------------------
    // Berserk
    //--------------------------------------------------------------------
public:
    u32 time_berserk_start;
    IC void set_berserk() { time_berserk_start = time(); }
    bool berserk_always;

    //--------------------------------------------------------------------
    // Panic Threshold (extension for scripts)
    //--------------------------------------------------------------------

    float m_default_panic_threshold;
    IC void set_custom_panic_threshold(float value);
    IC void set_default_panic_threshold();
    //--------------------------------------------------------------------

    //////////////////////////////////////////////////////////////////////////
    // -----------------------------------------------------------------------------
    // Special Services (refactoring needed)

    void on_kill_enemy(const CEntity* obj);
    void Hit_Psy(IGameObject* object, float value);
    void Hit_Wound(IGameObject* object, float value, const Fvector& dir, float impulse);
    CParticlesObject* PlayParticles(const shared_str& name, const Fvector& position, const Fvector& dir,
        BOOL auto_remove = TRUE, BOOL xformed = TRUE);
    void load_effector(LPCSTR section, LPCSTR line, SAttackEffector& effector);

    // --------------------------------------------------------------------------------------
    // Kill From Here
    // --------------------------------------------------------------------------------------
    // State flags
    bool m_bDamaged;
    bool m_bAngry;
    bool m_bGrowling;
    bool m_bAggressive;
    bool m_bSleep;
    bool m_bRunTurnLeft;
    bool m_bRunTurnRight;

    void set_aggressive(bool val = true) { m_bAggressive = val; }
    //---------------------------------------------------------------------------------------

    u32 m_prev_sound_type;
    virtual u32 get_attack_rebuild_time();

    IC virtual EAction CustomVelocityIndex2Action(u32 velocity_index) { return ACT_STAND_IDLE; }
    virtual void TranslateActionToPathParams();

    bool state_invisible;

    void set_action(EAction action);
    void set_state_sound(u32 type, bool once = false);
    IC void fall_asleep() { m_bSleep = true; }
    IC void wake_up() { m_bSleep = false; }
    // Temp
    u32 m_time_last_attack_success;
    int m_rank;
    float m_melee_rotation_factor;

private:
    bool ignore_collision_hit;

public:
    IC void set_ignore_collision_hit(bool value) { ignore_collision_hit = value; }
    // -----------------------------------------------------------------------------
    //////////////////////////////////////////////////////////////////////////

public:
    CControl_Manager& control() { return (*m_control_manager); }
    CControlAnimationBase& anim() { return (*m_anim_base); }
    CControlMovementBase& move() { return (*m_move_base); }
    CControlPathBuilderBase& path() { return (*m_path_base); }
    CControlDirectionBase& dir() { return (*m_dir_base); }
    CControlManagerCustom& com_man() { return m_com_manager; }
    virtual bool check_start_conditions(ControlCom::EControlType);
    virtual void on_activate_control(ControlCom::EControlType) {}
protected:
    CControl_Manager* m_control_manager;

    CControlAnimationBase* m_anim_base;
    CControlMovementBase* m_move_base;
    CControlPathBuilderBase* m_path_base;
    CControlDirectionBase* m_dir_base;

    CControlManagerCustom m_com_manager;

    virtual void create_base_controls();

    //////////////////////////////////////////////////////////////////////////
    // Critical Wounded
    //////////////////////////////////////////////////////////////////////////
    enum
    {
        critical_wound_type_head = u32(0),
        critical_wound_type_torso,
        critical_wound_type_legs
    };

    virtual void load_critical_wound_bones();
    virtual bool critical_wound_external_conditions_suitable();
    virtual void critical_wounded_state_start();

    void fill_bones_body_parts(LPCSTR body_part, CriticalWoundType wound_type);

    LPCSTR m_critical_wound_anim_head;
    LPCSTR m_critical_wound_anim_torso;
    LPCSTR m_critical_wound_anim_legs;

    //////////////////////////////////////////////////////////////////////////
public:
    virtual pcstr get_monster_class_name() = 0;

//////////////////////////////////////////////////////////////////////////
// DEBUG stuff
#ifdef DEBUG

    template <class Type>
    bool get_debug_var(pcstr var_name, OUT Type& result);

public:
    struct SDebugInfo
    {
        bool active;
        float x;
        float y;
        float delta_y;
        u32 color;
        u32 delimiter_color;

        SDebugInfo() : active(false) {}
        SDebugInfo(float px, float py, float dy, u32 c, u32 dc)
            : active(true), x(px), y(py), delta_y(dy), color(c), delimiter_color(dc)
        {
        }
    };

    u8 m_show_debug_info; // 0 - none, 1 - first column, 2 - second column
    void set_show_debug_info(u8 show = 1) { m_show_debug_info = show; }
    virtual SDebugInfo show_debug_info();
    virtual void add_debug_info(debug::text_tree& root_s);

    void debug_fsm();
#endif

#ifdef _DEBUG
    virtual void debug_on_key(int key) {}
#endif
    //////////////////////////////////////////////////////////////////////////

public:
    bool is_jumping();
    virtual bool can_be_seen() const { return true; }
#ifdef DEBUG
    bool is_paused() const;
#endif

    //-------------------------------------------------------------------
    // CBaseMonster's      Steering Behaviour
    //-------------------------------------------------------------------
public:
    steering_behaviour::manager* get_steer_manager();

    float get_feel_enemy_who_just_hit_max_distance() { return m_feel_enemy_who_just_hit_max_distance; }
    float get_feel_enemy_who_made_sound_max_distance() { return m_feel_enemy_who_made_sound_max_distance; }
    float get_feel_enemy_max_distance() { return m_feel_enemy_max_distance; }
    virtual bool can_use_agressive_jump(const IGameObject*) { return false; }
private:
    steering_behaviour::manager* m_steer_manager;
    squad_grouping_behaviour* m_grouping_behaviour; // freed by manager

    void update_enemy_accessible_and_at_home_info();
    // updates position by applying little "pushing" force
    // so that monsters rarely intersect
    void update_pos_by_grouping_behaviour();
    TTime m_last_grouping_behaviour_update_tick;

    float m_feel_enemy_who_made_sound_max_distance;
    float m_feel_enemy_who_just_hit_max_distance;
    float m_feel_enemy_max_distance;

    //-------------------------------------------------------------------
    // CBaseMonster's  Atack on Move Parameters
    //-------------------------------------------------------------------
public:
    struct attack_on_move_params_t
    {
        bool enabled;
        float max_go_close_time;
        float far_radius;
        float prepare_radius;
        float prepare_time;
        float attack_radius;
        float update_side_period;
        float prediction_factor;
    };

    bool can_attack_on_move();
    float get_attack_on_move_max_go_close_time();
    float get_attack_on_move_far_radius();
    float get_attack_on_move_attack_radius();
    float get_attack_on_move_update_side_period();
    float get_attack_on_move_prediction_factor();
    float get_attack_on_move_prepare_radius();
    float get_attack_on_move_prepare_time();

    bool enemy_accessible();
    bool at_home();

protected:
    attack_on_move_params_t m_attack_on_move_params;

public:
    template <class Type>
    Type override_if_debug(pcstr var_name, Type value);

    //-------------------------------------------------------------------
    // CBaseMonster's  Auras
    //-------------------------------------------------------------------
public:
    float get_psy_influence();
    float get_radiation_influence();
    float get_fire_influence();
    void play_detector_sound();

private:
    monster_aura m_psy_aura;
    monster_aura m_radiation_aura;
    monster_aura m_fire_aura;
    monster_aura m_base_aura;

protected:
    //-------------------------------------------------------------------
    // CBaseMonster's  Anti-Aim Ability
    //-------------------------------------------------------------------
    anti_aim_ability* m_anti_aim;

    //-------------------------------------------------------------------
    // CBaseMonster's  protections
    //-------------------------------------------------------------------
    float m_fSkinArmor;
    float m_fHitFracMonster;

private:
    pcstr m_head_bone_name;
    pcstr m_left_eye_bone_name;
    pcstr m_right_eye_bone_name;
    shared_str m_section;

public:
    pcstr get_head_bone_name() const { return m_head_bone_name; }
    shared_str get_section() const { return m_section; }
    anti_aim_ability* get_anti_aim() { return m_anti_aim; }
    virtual void on_attack_on_run_hit() {}
private:
    void update_eyes_visibility();
    float get_screen_space_coverage_diagonal();

    void GenerateNewOffsetFromLeader();
    u32 m_offset_from_leader_chosen_tick;
    Fvector m_offset_from_leader;

    // very special copies, used when pos is not on ai-map
    // in that situation m_action_target_node is close node
    Fvector m_action_target_pos;
    u32 m_action_target_node;

    TTime m_first_tick_enemy_inaccessible;
    TTime m_last_tick_enemy_inaccessible;
    TTime m_first_tick_object_not_at_home;

public:
    virtual bool run_home_point_when_enemy_inaccessible() const { return true; }
    virtual bool need_shotmark() const { return true; }
};

//-------------------------------------------------------------------
// CBaseMonster's  debug template functions
//-------------------------------------------------------------------

#include "ai_debug_variables.h"

#ifdef DEBUG
template <class Type>
bool CBaseMonster::get_debug_var(pcstr var_name, OUT Type& result)
{
    char* full_var_name;
    STRCONCAT(full_var_name, get_monster_class_name(), "_", var_name);
    return ai_dbg::get_var(full_var_name, result);
}
#endif // DEBUG

template <class Type>
Type CBaseMonster::override_if_debug(pcstr var_name, Type value)
{
#ifdef DEBUG
    Type debug_value;
    return get_debug_var(var_name, debug_value) ? debug_value : value;
#else // DEBUG
    return value;
#endif // DEBUG
}

#include "base_monster_inline.h"
