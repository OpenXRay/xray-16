////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker.h
//	Created 	: 25.02.2003
//  Modified 	: 25.02.2003
//	Author		: Dmitriy Iassenev
//	Description : AI Behaviour for monster "Stalker"
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "CustomMonster.h"
#include "object_handler.h"
#include "AI_PhraseDialogManager.h"
#include "step_manager.h"

#ifdef DEBUG
template <typename _object_type>
class CActionBase;

template <typename _object_type>
class CPropertyEvaluator;

template <typename _object_type, bool _reverse_search, typename _world_operator, typename _condition_evaluator,
    typename _world_operator_ptr, typename _condition_evaluator_ptr>
class CActionPlanner;

typedef CActionPlanner<CScriptGameObject, false, CActionBase<CScriptGameObject>, CPropertyEvaluator<CScriptGameObject>,
    CActionBase<CScriptGameObject>*, CPropertyEvaluator<CScriptGameObject>*>
    script_planner;
#endif

namespace MonsterSpace
{
enum EMovementDirection : u32;
}

namespace StalkerSpace
{
enum EBodyAction : u32;
}

namespace smart_cover
{
class cover;
class loophole;

namespace transitions
{
class action;
}
}

enum ECriticalWoundType : u32;

class CALifeSimulator;
class CCharacterPhysicsSupport;
class CWeapon;
class CCoverPoint;
class CCoverEvaluatorCloseToEnemy;
class CCoverEvaluatorFarFromEnemy;
class CCoverEvaluatorBest;
class CCoverEvaluatorAngle;
class CCoverEvaluatorSafe;
class CCoverEvaluatorAmbush;
class CAgentManager;
class CMotionDef;
class CStalkerAnimationManager;
class CStalkerPlanner;
class CSightManager;
class stalker_movement_manager_smart_cover;
class CStalkerSoundDataVisitor;
class CWeaponShotEffector;
struct SBoneProtections;
class CDangerLocation;
class CRestrictedObject;

class CAI_Stalker : public CCustomMonster, public CObjectHandler, public CAI_PhraseDialogManager, public CStepManager
{
protected:
    using inherited = CCustomMonster;

public:
    using inherited::useful;
    using inherited::evaluate;

private:
    CStalkerAnimationManager* m_animation_manager;
    CStalkerPlanner* m_brain;
    CSightManager* m_sight_manager;
    stalker_movement_manager_smart_cover* m_movement_manager;

#ifdef DEBUG
    const script_planner* m_debug_planner;
#endif

    // ALife
private:
    SBoneProtections* m_boneHitProtection;

    // weapon dispersion
private:
    float m_disp_walk_stand;
    float m_disp_walk_crouch;
    float m_disp_run_stand;
    float m_disp_run_crouch;
    float m_disp_stand_stand;
    float m_disp_stand_crouch;
    float m_disp_stand_stand_zoom;
    float m_disp_stand_crouch_zoom;

private:
    float m_power_fx_factor;

private:
    float m_fRankDisperison;
    float m_fRankVisibility;
    float m_fRankImmunity;

    // best item/ammo selection members
public:
    bool m_item_actuality;
    CInventoryItem* m_best_item_to_kill;
    float m_best_item_value;
    CInventoryItem* m_best_ammo;
    const CInventoryItem* m_best_found_item_to_kill;
    const CInventoryItem* m_best_found_ammo;

    // covers being used
public:
    CCoverEvaluatorCloseToEnemy* m_ce_close;
    CCoverEvaluatorFarFromEnemy* m_ce_far;
    CCoverEvaluatorBest* m_ce_best;
    CCoverEvaluatorAngle* m_ce_angle;
    CCoverEvaluatorSafe* m_ce_safe;
    CCoverEvaluatorAmbush* m_ce_ambush;

    // physics support
public:
    CCharacterPhysicsSupport* m_pPhysics_support;

public:
    bool m_wounded;

public:
    CAI_Stalker();
    virtual ~CAI_Stalker();

public:
    virtual CCharacterPhysicsSupport* character_physics_support() { return m_pPhysics_support; }
    virtual const CCharacterPhysicsSupport* character_physics_support() const { return m_pPhysics_support; }
    virtual CPHDestroyable* ph_destroyable();
    virtual CAttachmentOwner* cast_attachment_owner() { return this; }
    virtual CInventoryOwner* cast_inventory_owner() { return this; }
    virtual CEntityAlive* cast_entity_alive() { return this; }
    virtual CEntity* cast_entity() { return this; }
    virtual CGameObject* cast_game_object() { return this; }
    virtual CPhysicsShellHolder* cast_physics_shell_holder() { return this; }
    virtual CParticlesPlayer* cast_particles_player() { return this; }
    virtual Feel::Sound* dcast_FeelSound() { return this; }
    virtual CAI_Stalker* cast_stalker() { return this; }
    virtual CCustomMonster* cast_custom_monster() { return this; }
    virtual CScriptEntity* cast_script_entity() { return this; }
public:
    void init();
    virtual void Load(LPCSTR section);
    virtual void reinit();
    virtual void reload(LPCSTR section);
    virtual void LoadSounds(LPCSTR section);

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Export(NET_Packet& P);
    virtual void net_Import(NET_Packet& P);
    virtual void net_Destroy();
    virtual void net_Save(NET_Packet& P);
    virtual BOOL net_SaveRelevant();
    virtual void net_Relcase(IGameObject* O);

    // save/load server serialization
    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);

    virtual void UpdateCL();
    virtual void shedule_Update(u32 dt);
    virtual void Think();
    virtual void SelectAnimation(const Fvector& _view, const Fvector& _move, float speed);
    virtual BOOL UsedAI_Locations();

    virtual void g_WeaponBones(int& L, int& R1, int& R2);
    virtual void g_fireParams(const CHudItem* pHudItem, Fvector& P, Fvector& D);
    virtual void HitSignal(float P, Fvector& vLocalDir, IGameObject* who, s16 element);
    virtual void Die(IGameObject* who);

    virtual void OnEvent(NET_Packet& P, u16 type);
    virtual void feel_touch_new(IGameObject* O);
    virtual void feel_touch_delete(IGameObject* O);
    void on_ownership_reject(IGameObject* O, bool just_before_destroy);
    virtual void renderable_Render();
    virtual void Exec_Look(float dt);
    virtual void Hit(SHit* pHDS);
    virtual void PHHit(SHit& H);
    virtual bool feel_vision_isRelevant(IGameObject* who);
    virtual float Radius() const;
#ifdef DEBUG
    virtual void OnHUDDraw(CCustomHUD* hud);
    virtual void OnRender();
    void debug_text();
    bool m_dbg_hud_draw;
#endif

    virtual bool useful(const CItemManager* manager, const CGameObject* object) const;
    virtual float evaluate(const CItemManager* manager, const CGameObject* object) const;
    virtual bool useful(const CEnemyManager* manager, const CEntityAlive* object) const;

    // PDA && Dialogs
    //	virtual void						ReceivePdaMessage					(u16 who, EPdaMsg msg, shared_str
    // info_id);
    virtual void UpdateAvailableDialogs(CPhraseDialogManager* partner);

    // scripts
    virtual CWeapon* GetCurrentWeapon() const;
    virtual u32 GetWeaponAmmo() const;
    //	virtual CInventoryItem				*GetCurrentEquipment	() const; <- moved to
    // InventoryOwner::GetCurrentOutfit
    virtual CInventoryItem* GetMedikit() const;
    virtual CInventoryItem* GetFood() const;
    virtual bool bfAssignMovement(CScriptEntityAction* tpEntityAction);
    virtual bool bfAssignWatch(CScriptEntityAction* tpEntityAction);
    virtual void ResetScriptData(void* P = 0);
    virtual bool bfAssignObject(CScriptEntityAction* tpEntityAction);
    virtual bool bfAssignAnimation(CScriptEntityAction* tpEntityAction);

    // physics
    virtual u16 PHGetSyncItemsNumber() { return inherited::PHGetSyncItemsNumber(); }
    virtual CPHSynchronize* PHGetSyncItem(u16 item) { return inherited::PHGetSyncItem(item); }
    virtual void PHUnFreeze() { return inherited::PHUnFreeze(); }
    virtual void PHFreeze() { return inherited::PHFreeze(); }
    // miscellanious functions
    void DropItemSendMessage(IGameObject* O);
    bool bfCheckForNodeVisibility(u32 dwNodeID, bool bIfRyPick = false);
    virtual ALife::ERelationType tfGetRelationType(const CEntityAlive* tpEntityAlive) const;
    virtual const SRotation Orientation() const;
    virtual const MonsterSpace::SBoneRotation& head_orientation() const;

    // InventoryOwner stuff
    virtual bool CanPutInSlot(PIItem item, u32 slot) { return (slot != OUTFIT_SLOT) && (slot != PDA_SLOT); };
    //////////////////////////////////////////////////////////////////////////
    // action/evaluators support functions
    //////////////////////////////////////////////////////////////////////////
public:
    virtual void OnItemTake(CInventoryItem* inventory_item);
    virtual void OnItemDrop(CInventoryItem* inventory_item, bool just_before_destroy);
    bool item_to_kill();
    bool item_can_kill();
    bool remember_item_to_kill();
    bool remember_ammo();
    bool ready_to_kill();
    bool ready_to_detour();
    void update_best_item_info();
    void update_best_item_info_impl();
    virtual float GetWeaponAccuracy() const;
    virtual bool unlimited_ammo();
    virtual void spawn_supplies();
    IC CAgentManager& agent_manager() const;

    virtual bool human_being() const { return (true); }
    bool undetected_anomaly();
    bool inside_anomaly();

private:
    bool m_can_kill_member;
    bool m_can_kill_enemy;
    float m_pick_distance;
    u32 m_pick_frame_id;
    collide::rq_results rq_storage;

private:
    void can_kill_entity(
        const Fvector& position, const Fvector& direction, float distance, collide::rq_results& rq_storage);
    void can_kill_entity_from(const Fvector& position, Fvector direction, float distance);
    void update_can_kill_info();

public:
    bool can_kill_member();
    bool can_kill_enemy();
    float pick_distance();
    IC float start_pick_distance() const;
    bool fire_make_sense();

    virtual LPCSTR Name() const;
    virtual bool feel_touch_contact(IGameObject* O);
    virtual bool feel_touch_on_contact(IGameObject* O);

    //флаги, какие действия совершал актер по отношению к сталкеру
    //(помог, атаковал и т.д.)
    Flags32 m_actor_relation_flags;

    // ALife
private:
    struct CTradeItem
    {
        CInventoryItem* m_item;
        ALife::_OBJECT_ID m_owner_id;
        ALife::_OBJECT_ID m_new_owner_id;

        IC CTradeItem(CInventoryItem* item, ALife::_OBJECT_ID owner_id, ALife::_OBJECT_ID new_owner_id)
        {
            m_item = item;
            m_owner_id = owner_id;
            m_new_owner_id = new_owner_id;
        }

        IC bool operator<(const CTradeItem& trade_item) const;
        IC bool operator==(u16 id) const;
    };

private:
    CGameObject* m_trader_game_object;
    CInventoryOwner* m_current_trader;
    xr_vector<CTradeItem> m_temp_items;
    u32 m_total_money;
    bool m_sell_info_actuality;
    bool m_can_select_weapon;

public:
    bool can_select_weapon() { return m_can_select_weapon; }
    void can_select_weapon(bool can) { m_can_select_weapon = can; }
    bool can_take(CInventoryItem const* item);

protected:
    u32 fill_items(CInventory& inventory, CGameObject* old_owner, ALife::_OBJECT_ID new_owner_id);

    IC void buy_item_virtual(CTradeItem& item);
    void attach_available_ammo(CWeapon* weapon);
    void choose_food();
    void choose_weapon(ALife::EWeaponPriorityType weapon_priority_type);
    void choose_medikit();
    void choose_detector();
    void choose_equipment();

    void select_items();
    void transfer_item(CInventoryItem* item, CGameObject* old_owner, CGameObject* new_owner);

    void update_sell_info();
    bool tradable_item(CInventoryItem* inventory_item, const u16& current_owner_id);
    bool can_sell(CInventoryItem* item);

    bool non_conflicted(const CInventoryItem* item, const CWeapon* new_weapon) const;
    bool enough_ammo(const CWeapon* new_weapon) const;
    bool conflicted(
        const CInventoryItem* item, const CWeapon* new_weapon, bool new_wepon_enough_ammo, int new_weapon_rank) const;
    void update_conflicted(CInventoryItem* item, const CWeapon* new_weapon);
    void remove_personal_only_ammo(const CInventoryItem* item);
    void on_after_take(const CGameObject* object);
    virtual bool AllowItemToTrade(CInventoryItem const* item, const SInvItemPlace& place) const;

public:
    IC CStalkerAnimationManager& animation() const;
    IC CStalkerPlanner& brain() const;
    IC CSightManager& sight() const;

private:
    CStalkerSoundDataVisitor* m_sound_user_data_visitor;

protected:
    virtual CSound_UserDataVisitor* create_sound_visitor();
    virtual CMemoryManager* create_memory_manager();
    virtual CMovementManager* create_movement_manager();

public:
    IC stalker_movement_manager_smart_cover& movement() const;
    virtual IFactoryObject* _construct();

private:
    IC bool frame_check(u32& frame);
    virtual bool natural_weapon() const { return false; }
    virtual bool natural_detector() const { return false; }
    virtual bool use_center_to_aim() const;
    void process_enemies();

private:
    bool m_group_behaviour;

public:
    IC bool group_behaviour() const;
    virtual void update_range_fov(float& new_range, float& new_fov, float start_range, float start_fov);
    void __stdcall update_object_handler();
    bool zoom_state() const;
    void react_on_grenades();
    void react_on_member_death();

private:
    CWeaponShotEffector* m_weapon_shot_effector;
    s32 m_weapon_shot_random_seed;

public:
    virtual void on_weapon_shot_start(CWeapon* weapon);
    virtual void on_weapon_shot_update();
    virtual void on_weapon_shot_stop();
    virtual void on_weapon_shot_remove(CWeapon* weapon);
    virtual void on_weapon_hide(CWeapon* weapon);
    IC CWeaponShotEffector& weapon_shot_effector() const;
    IC Fvector weapon_shot_effector_direction(const Fvector& current) const;
    virtual void UpdateCamera();
    virtual bool can_attach(const CInventoryItem* inventory_item) const;
    // because we don't want to use this feature for stalkers
    virtual bool use_simplified_visual() const { return false; } //(already_dead());};
#ifdef DEBUG
    void debug_planner(const script_planner* planner);
#endif

private:
    u32 m_pstl_min_queue_size_far;
    u32 m_pstl_max_queue_size_far;
    u32 m_pstl_min_queue_interval_far;
    u32 m_pstl_max_queue_interval_far;

    u32 m_pstl_min_queue_size_medium;
    u32 m_pstl_max_queue_size_medium;
    u32 m_pstl_min_queue_interval_medium;
    u32 m_pstl_max_queue_interval_medium;

    u32 m_pstl_min_queue_size_close;
    u32 m_pstl_max_queue_size_close;
    u32 m_pstl_min_queue_interval_close;
    u32 m_pstl_max_queue_interval_close;

    u32 m_shtg_min_queue_size_far;
    u32 m_shtg_max_queue_size_far;
    u32 m_shtg_min_queue_interval_far;
    u32 m_shtg_max_queue_interval_far;

    u32 m_shtg_min_queue_size_medium;
    u32 m_shtg_max_queue_size_medium;
    u32 m_shtg_min_queue_interval_medium;
    u32 m_shtg_max_queue_interval_medium;

    u32 m_shtg_min_queue_size_close;
    u32 m_shtg_max_queue_size_close;
    u32 m_shtg_min_queue_interval_close;
    u32 m_shtg_max_queue_interval_close;

    u32 m_snp_min_queue_size_far;
    u32 m_snp_max_queue_size_far;
    u32 m_snp_min_queue_interval_far;
    u32 m_snp_max_queue_interval_far;

    u32 m_snp_min_queue_size_medium;
    u32 m_snp_max_queue_size_medium;
    u32 m_snp_min_queue_interval_medium;
    u32 m_snp_max_queue_interval_medium;

    u32 m_snp_min_queue_size_close;
    u32 m_snp_max_queue_size_close;
    u32 m_snp_min_queue_interval_close;
    u32 m_snp_max_queue_interval_close;

    u32 m_mchg_min_queue_size_far;
    u32 m_mchg_max_queue_size_far;
    u32 m_mchg_min_queue_interval_far;
    u32 m_mchg_max_queue_interval_far;

    u32 m_mchg_min_queue_size_medium;
    u32 m_mchg_max_queue_size_medium;
    u32 m_mchg_min_queue_interval_medium;
    u32 m_mchg_max_queue_interval_medium;

    u32 m_mchg_min_queue_size_close;
    u32 m_mchg_max_queue_size_close;
    u32 m_mchg_min_queue_interval_close;
    u32 m_mchg_max_queue_interval_close;

    u32 m_auto_min_queue_size_far;
    u32 m_auto_max_queue_size_far;
    u32 m_auto_min_queue_interval_far;
    u32 m_auto_max_queue_interval_far;

    u32 m_auto_min_queue_size_medium;
    u32 m_auto_max_queue_size_medium;
    u32 m_auto_min_queue_interval_medium;
    u32 m_auto_max_queue_interval_medium;

    u32 m_auto_min_queue_size_close;
    u32 m_auto_max_queue_size_close;
    u32 m_auto_min_queue_interval_close;
    u32 m_auto_max_queue_interval_close;

    //	float			m_pstl_queue_fire_dist_close;
    float m_pstl_queue_fire_dist_med;
    float m_pstl_queue_fire_dist_far;

    //	float			m_shtg_queue_fire_dist_close;
    float m_shtg_queue_fire_dist_med;
    float m_shtg_queue_fire_dist_far;

    //	float			m_snp_queue_fire_dist_close;
    float m_snp_queue_fire_dist_med;
    float m_snp_queue_fire_dist_far;

    //	float			m_mchg_queue_fire_dist_close;
    float m_mchg_queue_fire_dist_med;
    float m_mchg_queue_fire_dist_far;

    //	float			m_auto_queue_fire_dist_close;
    float m_auto_queue_fire_dist_med;
    float m_auto_queue_fire_dist_far;

public:
    IC u32 pstl_min_queue_size_far() const;
    IC u32 pstl_max_queue_size_far() const;
    IC u32 pstl_min_queue_interval_far() const;
    IC u32 pstl_max_queue_interval_far() const;

    IC u32 pstl_min_queue_size_medium() const;
    IC u32 pstl_max_queue_size_medium() const;
    IC u32 pstl_min_queue_interval_medium() const;
    IC u32 pstl_max_queue_interval_medium() const;

    IC u32 pstl_min_queue_size_close() const;
    IC u32 pstl_max_queue_size_close() const;
    IC u32 pstl_min_queue_interval_close() const;
    IC u32 pstl_max_queue_interval_close() const;

    IC u32 shtg_min_queue_size_far() const;
    IC u32 shtg_max_queue_size_far() const;
    IC u32 shtg_min_queue_interval_far() const;
    IC u32 shtg_max_queue_interval_far() const;

    IC u32 shtg_min_queue_size_medium() const;
    IC u32 shtg_max_queue_size_medium() const;
    IC u32 shtg_min_queue_interval_medium() const;
    IC u32 shtg_max_queue_interval_medium() const;

    IC u32 shtg_min_queue_size_close() const;
    IC u32 shtg_max_queue_size_close() const;
    IC u32 shtg_min_queue_interval_close() const;
    IC u32 shtg_max_queue_interval_close() const;

    IC u32 snp_min_queue_size_far() const;
    IC u32 snp_max_queue_size_far() const;
    IC u32 snp_min_queue_interval_far() const;
    IC u32 snp_max_queue_interval_far() const;

    IC u32 snp_min_queue_size_medium() const;
    IC u32 snp_max_queue_size_medium() const;
    IC u32 snp_min_queue_interval_medium() const;
    IC u32 snp_max_queue_interval_medium() const;

    IC u32 snp_min_queue_size_close() const;
    IC u32 snp_max_queue_size_close() const;
    IC u32 snp_min_queue_interval_close() const;
    IC u32 snp_max_queue_interval_close() const;

    IC u32 mchg_min_queue_size_far() const;
    IC u32 mchg_max_queue_size_far() const;
    IC u32 mchg_min_queue_interval_far() const;
    IC u32 mchg_max_queue_interval_far() const;

    IC u32 mchg_min_queue_size_medium() const;
    IC u32 mchg_max_queue_size_medium() const;
    IC u32 mchg_min_queue_interval_medium() const;
    IC u32 mchg_max_queue_interval_medium() const;

    IC u32 mchg_min_queue_size_close() const;
    IC u32 mchg_max_queue_size_close() const;
    IC u32 mchg_min_queue_interval_close() const;
    IC u32 mchg_max_queue_interval_close() const;

    IC u32 auto_min_queue_size_far() const;
    IC u32 auto_max_queue_size_far() const;
    IC u32 auto_min_queue_interval_far() const;
    IC u32 auto_max_queue_interval_far() const;

    IC u32 auto_min_queue_size_medium() const;
    IC u32 auto_max_queue_size_medium() const;
    IC u32 auto_min_queue_interval_medium() const;
    IC u32 auto_max_queue_interval_medium() const;

    IC u32 auto_min_queue_size_close() const;
    IC u32 auto_max_queue_size_close() const;
    IC u32 auto_min_queue_interval_close() const;
    IC u32 auto_max_queue_interval_close() const;

    //	IC		float						pstl_queue_fire_dist_close		() const;
    IC float pstl_queue_fire_dist_med() const;
    IC float pstl_queue_fire_dist_far() const;

    //	IC		float						shtg_queue_fire_dist_close		() const;
    IC float shtg_queue_fire_dist_med() const;
    IC float shtg_queue_fire_dist_far() const;

    //	IC		float						snp_queue_fire_dist_close		() const;
    IC float snp_queue_fire_dist_med() const;
    IC float snp_queue_fire_dist_far() const;

    //	IC		float						mchg_queue_fire_dist_close		() const;
    IC float mchg_queue_fire_dist_med() const;
    IC float mchg_queue_fire_dist_far() const;

    //	IC		float						auto_queue_fire_dist_close		() const;
    IC float auto_queue_fire_dist_med() const;
    IC float auto_queue_fire_dist_far() const;

public:
    typedef fastdelegate::FastDelegate<void(const CCoverPoint*, const CCoverPoint*)> on_best_cover_changed_delegate;
    typedef fastdelegate::FastDelegate<bool(SHit const*)> HitCallback;

private:
    typedef xr_vector<on_best_cover_changed_delegate> cover_delegates;

private:
    cover_delegates m_cover_delegates;
    const CCoverPoint* m_best_cover;
    float m_best_cover_value;
    bool m_best_cover_actual;
    bool m_best_cover_can_try_advance;
    const CCoverPoint* m_best_cover_advance_cover;

public:
    const CCoverPoint* find_best_cover(const Fvector& position_to_cover_from);

private:
    float best_cover_value(const Fvector& position_to_cover_from);
    void compute_enemy_distances(float& minimum_enemy_distance, float& maximum_enemy_distance);
    void update_best_cover_actuality(const Fvector& position_to_cover_from);
    void on_best_cover_changed(const CCoverPoint* new_cover, const CCoverPoint* old_cover);

public:
    void best_cover_invalidate();
    void best_cover_can_try_advance();
    const CCoverPoint* best_cover(const Fvector& position_to_cover_from);
    IC CCoverPoint const* const& current_best_cover() const;

public:
    void subscribe_on_best_cover_changed(const on_best_cover_changed_delegate& delegate);
    void unsubscribe_on_best_cover_changed(const on_best_cover_changed_delegate& delegate);

public:
    virtual void on_enemy_change(const CEntityAlive* enemy);
    virtual void on_restrictions_change();
    void on_cover_blocked(const CCoverPoint* cover);
    void on_danger_location_add(const CDangerLocation& location);
    void on_danger_location_remove(const CDangerLocation& location);

public:
    void wounded(bool value);
    bool wounded(const CRestrictedObject* object) const;
    IC bool wounded() const;

    // throwing grenades
private:
    // actuality parameters
    bool m_throw_actual;
    Fvector m_computed_object_position;
    Fvector m_computed_object_direction;
    // target parameters
    Fvector m_throw_target_position;
    IGameObject* m_throw_ignore_object;
    // computed
    Fvector m_throw_position;
    Fvector m_throw_velocity;
    // collision prediction
    Fvector m_throw_collide_position;
    bool m_throw_enabled;

    u32 m_last_throw_time;
    u32 m_throw_time_interval;

#ifdef DEBUG
    xr_vector<trajectory_pick> m_throw_picks;
    xr_vector<Fvector> m_throw_collide_tris;
#endif // DEBUG

public:
    IC const bool& throw_enabled();

private:
    bool m_can_throw_grenades;

public:
    IC const bool& can_throw_grenades() const;
    IC void can_throw_grenades(const bool& value);

private:
    bool throw_check_error(
        float low, float high, const Fvector& start, const Fvector& velocity, const Fvector& gravity);
    void check_throw_trajectory(const float& throw_time);
    void throw_target_impl(const Fvector& position, IGameObject* throw_ignore_object);
    void compute_throw_miss(u32 const vertex_id);

public:
    virtual bool use_default_throw_force();
    virtual float missile_throw_force();
    virtual bool use_throw_randomness();
    void throw_target(const Fvector& position, IGameObject* throw_ignore_object);
    void throw_target(const Fvector& position, u32 const vertex_id, IGameObject* throw_ignore_object);
    IC const Fvector& throw_target() const;
    void update_throw_params();
    void on_throw_completed();
    IC const u32& last_throw_time() const;

#ifdef DEBUG
public:
    void dbg_draw_vision();
    void dbg_draw_visibility_rays();
#endif

    //////////////////////////////////////////////////////////////////////////
    // Critical Wounds
    //////////////////////////////////////////////////////////////////////////
private:
    virtual void load_critical_wound_bones();
    virtual bool critical_wound_external_conditions_suitable();
    virtual void critical_wounded_state_start();

    void fill_bones_body_parts(LPCSTR bone_id, const ECriticalWoundType& wound_type);

public:
    typedef xr_vector<float> CRITICAL_WOUND_WEIGHTS;

private:
    CRITICAL_WOUND_WEIGHTS m_critical_wound_weights;

public:
    bool critically_wounded();
    IC const CRITICAL_WOUND_WEIGHTS& critical_wound_weights() const;

private:
    bool can_cry_enemy_is_wounded() const;
    void on_critical_wound_initiator(const CAI_Stalker* critically_wounded);
    void on_enemy_wounded_or_killed(const CAI_Stalker* wounded_or_killed);
    void notify_on_wounded_or_killed(IGameObject* object);
    void notify_on_wounded_or_killed();
    void xr_stdcall remove_critical_hit();
    //////////////////////////////////////////////////////////////////////////
private:
    bool m_registered_in_combat_on_migration;

public:
    virtual void on_before_change_team();
    virtual void on_after_change_team();

public:
    virtual void create_anim_mov_ctrl(CBlend* b, Fmatrix* start_pose, bool local_animation);
    virtual void destroy_anim_mov_ctrl();

private:
    bool m_can_select_items;

public:
    IC const u32& throw_time_interval() const;
    IC void throw_time_interval(const u32& value);

public:
    bool too_far_to_kill_enemy(const Fvector& position);

public:
    virtual float shedule_Scale();

private:
    bool m_sniper_update_rate;
    bool m_sniper_fire_mode;

public:
    IC void sniper_update_rate(bool value);
    IC bool sniper_update_rate() const;
    IC void sniper_fire_mode(bool value);
    IC bool sniper_fire_mode() const;

private:
    shared_str m_aim_bone_id;

public:
    void aim_bone_id(shared_str const& bone_id);
    shared_str const& aim_bone_id() const;
    void aim_target(Fvector& result, const CGameObject* object);

private:
    HitCallback m_hit_callback;

public:
    IC void hit_callback(HitCallback const& callback);
    void use_smart_covers_only(bool value);
    bool use_smart_covers_only() const;

public:
    typedef fastdelegate::FastDelegate<void(Fmatrix&)> EyeMatrixCallback;

private:
    virtual BOOL AlwaysTheCrow();

public:
    IC void take_items_enabled(bool value);
    IC bool take_items_enabled() const;

    IC void death_sound_enabled(bool value);
    IC bool death_sound_enabled() const;

private:
    bool m_take_items_enabled;
    bool m_death_sound_enabled;

public:
    smart_cover::cover const* get_current_smart_cover();
    smart_cover::loophole const* get_current_loophole();
    bool can_fire_right_now();

public:
    typedef xr_vector<IGameObject*> ignored_touched_objects_type;
    inline ignored_touched_objects_type& ignored_touched_objects() { return m_ignored_touched_objects; }
    void generate_take_event(IGameObject const* const object) const;

private:
    ignored_touched_objects_type m_ignored_touched_objects;
};

#include "ai_stalker_inline.h"
