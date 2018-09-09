////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_rat.h
//	Created 	: 23.04.2002
//  Modified 	: 27.07.2004
//	Author		: Dmitriy Iassenev
//	Description : AI Behaviour for monster "Rat"
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "CustomMonster.h"
#include "eatable_item.h"
#include "seniority_hierarchy_holder.h"
#include "team_hierarchy_holder.h"
#include "squad_hierarchy_holder.h"
#include "group_hierarchy_holder.h"
#include "xrAICore/Navigation/game_graph_space.h"

class CBlend;
class CPatrolPath;
enum ESoundTypes : u32;
class rat_state_manager;

namespace steering_behaviour
{
class manager;
} // namespace steering_behaviour
class CAI_Rat : public CCustomMonster, public CEatableItem
{
private:
    typedef CCustomMonster inherited;
    const CPatrolPath* m_path;

private:
    Fvector m_home_position;
    GameGraph::_GRAPH_ID m_current_graph_point;
    GameGraph::_GRAPH_ID m_next_graph_point;
    u32 m_time_to_change_graph_point;

public:
    rat_state_manager* m_state_manager;

public:
    void init_state_manager();
    void select_speed();
    void move(bool bCanAdjustSpeed = true, bool bStraightForward = false);
    void make_turn();
    void update_morale_broadcast(float const& value, float const& radius);
    void update_morale();
    void load_animations();
    void fire(bool const& value);
    void movement_type(float const& velocity);
    void update_home_position();
    void select_next_home_position();
    // for state manager
    SRotation sub_rotation();

    bool switch_to_attack_melee();
    bool switch_if_enemy();
    bool switch_to_free_recoil();
    bool switch_to_eat();
    bool switch_if_position();
    bool switch_if_diff();
    bool switch_if_porsuit();
    bool switch_if_no_enemy();
    bool switch_if_alife();
    bool switch_if_dist_no_angle();
    bool switch_if_dist_angle();
    bool switch_if_lost_time();
    bool switch_if_lost_rtime();
    bool switch_if_time();
    bool switch_if_home();

    bool get_morale();
    bool get_if_dw_time();
    bool get_if_tp_entity();
    bool get_alife();

    void set_previous_query_time();
    void set_dir();
    void set_dir_m();
    void set_sp_dir();
    void set_way_point();
    void set_home_pos();
    void set_rew_position();
    void set_rew_cur_position();
    void set_goal_time(float f_val = 0.f);
    void set_movement_type(bool bCanAdjustSpeed = true, bool bStraightForward = false);
    void set_firing(bool b_val = false);

    void activate_state_free_active();
    void activate_state_free_passive();
    void activate_state_move();
    void activate_move();
    void activate_turn();
    void activate_state_attack_range();
    void activate_state_free_recoil();
    void activate_state_home();
    void activate_state_eat();

    void init_state_under_fire();
    void init_free_recoil();
    void init_free_active();

    bool calc_node(const Fvector& next_position);
    Fvector calc_position();
    void set_position(Fvector m_position);
    void set_pitch(float pitch, float yaw);
    const CEntityAlive* get_enemy();
    bool check_completion_no_way();
    enum ERatStates
    {
        aiRatDeath = 0,
        aiRatFreeActive,
        aiRatFreePassive,
        aiRatAttackRange,
        aiRatAttackMelee,
        aiRatUnderFire,
        aiRatRetreat,
        aiRatPursuit,
        aiRatFreeRecoil,
        aiRatReturnHome,
        aiRatEatCorpse,
        aiRatNoWay,
    };
    ERatStates get_state();
    float m_newPitch;
    float m_newYaw;
    float m_newCorrect;
    u32 time_to_next_attack;
    u32 time_old_attack;
    u32 time_attack_rebuild;
    u32 m_squad_count;
    bool m_attack_rebuild;
    bool m_walk_on_way;
    u32 m_current_way_point;
    bool m_bWayCanAdjustSpeed;
    bool m_bWayStraightForward;

    // end for state manager

protected:
//////////////////////////
// STRUCTURES
//////////////////////////
#define TIME_TO_GO 2000
#define TIME_TO_RETURN 500

    typedef struct tagSNormalGlobalAnimations
    {
        MotionID tpaDeath[2];
        MotionID tpaAttack[3];
        MotionID tpaIdle[2];
        SAnimState tWalk;
        SAnimState tRun;
        MotionID tRunAttack;
        MotionID tpTurnLeft;
        MotionID tpTurnRight;
    } SNormalGlobalAnimations;

    // normal animations
    typedef struct tagSNormalAnimations
    {
        SNormalGlobalAnimations tGlobal;
    } SNormalAnimations;

    typedef struct tagSRatAnimations
    {
        SNormalAnimations tNormal;
    } SRatAnimations;
    //////////////////////////
    // END OF STRUCTURES
    //////////////////////////

    //////////////////////////
    // CLASS MEMBERS
    //////////////////////////

    // Graph
    enum ERatAction
    {
        eRatActionNone = u32(0),
        eRatActionAttackBegin,
        eRatActionAttackEnd,
    };

    ERatAction m_tAction;
    bool m_turning;

    // FSM
    xr_stack<ERatStates> m_tStateStack;
    ERatStates m_eCurrentState;
    ERatStates m_ePreviousState;
    bool m_bStopThinking;
    bool m_bStateChanged;

    // ANIMATIONS
    SRatAnimations m_tRatAnimations;
    MotionID m_tpCurrentGlobalAnimation;
    CBlend* m_tpCurrentGlobalBlend;

    // ATTACK
    bool m_bActionStarted;
    bool m_bFiring;
    u32 m_dwStartAttackTime;
    float m_fAttackSpeed;
    // HIT
    u32 m_hit_time;
    Fvector m_hit_direction;
    Fvector m_tHitPosition;
    float m_fHitPower;
    u32 m_dwHitInterval;
    // HIT PHYS
    float m_saved_impulse;
    Fvector m_saved_hit_position;
    Fvector m_saved_hit_dir;
    Fvector m_tNewPosition;
    Fvector m_tOldPosition;
    ALife::EHitType m_saved_hit_type;
    // PHYS
    float m_phMass;

    typedef struct tagSSimpleSound
    {
        ESoundTypes eSoundType;
        u32 dwTime;
        float fPower;
        Fvector tSavedPosition;
        CEntityAlive* tpEntity;
    } SSimpleSound;

    float m_fMaxSpeed;
    float m_fMinSpeed;
    // SOUND BEING FELT
    SSimpleSound m_tLastSound;

    // BEHAVIOUR
    Fvector m_tGoalDir;
    Fvector m_tNewDir;
    Fvector m_tCurrentDir;
    Fvector m_tHPB;
    float m_fDHeading;
    Fvector m_tRecoilPosition;

    // constants
    float m_fGoalChangeDelta;
    float m_fSafeSpeed;
    float m_fASpeed;
    Fvector m_tVarGoal;
    float m_fIdleSoundDelta;
    Fvector m_tSpawnPosition;
    float m_fAngleSpeed;
    float m_fSafeGoalChangeDelta;
    Fvector m_tGoalVariation;
    float m_fNullASpeed;
    float m_fMinASpeed;
    float m_fMaxASpeed;
    float m_fAttackASpeed;

    // variables
    float m_fGoalChangeTime;
    bool m_bNoWay;

    // Morale
    float m_fMoraleSuccessAttackQuant;
    float m_fMoraleDeathQuant;
    float m_fMoraleFearQuant;
    float m_fMoraleRestoreQuant;
    u32 m_dwMoraleRestoreTimeInterval;
    u32 m_dwMoraleLastUpdateTime;
    float m_fMoraleMinValue;
    float m_fMoraleMaxValue;
    float m_fMoraleNormalValue;
    float m_fMoraleDeathDistance;

    // active
    float m_fChangeActiveStateProbability;
    u32 m_dwActiveCountPercent;
    u32 m_dwActiveScheduleMin;
    u32 m_dwActiveScheduleMax;
    u32 m_dwPassiveScheduleMin;
    u32 m_dwPassiveScheduleMax;
    u32 m_dwStandingCountPercent;
    bool m_bStanding;
    bool m_bActive;

    // attack parameters
    float m_fAttackDistance;
    float m_fAttackAngle;
    float m_fMaxPursuitRadius;
    float m_fMaxHomeRadius;

    // DDD
    u32 m_dwActionRefreshRate;
    float m_fAttackSuccessProbability;

    // former constants
    u32 m_dwLostMemoryTime;
    u32 m_dwLostRecoilTime;
    float m_fUnderFireDistance;
    u32 m_dwRetreatTime;
    float m_fRetreatDistance;
    float m_fAttackStraightDistance;
    float m_fStableDistance;
    float m_fWallMinTurnValue;
    float m_fWallMaxTurnValue;
    float m_fSoundThreshold;

    // eat troops
    BOOL m_bEatMemberCorpses;
    BOOL m_bCannibalism;
    u32 m_dwEatCorpseInterval;
    u32 m_previous_query_time;

public:
    float m_fSpeed;
    bool m_bMoving;
    bool m_bCanAdjustSpeed;
    bool m_bStraightForward;

public:
    IC void vfChangeGoal();
    IC bool bfCheckIfGoalChanged(bool bForceChangeGoal = true);
    IC void vfChooseNewSpeed();
    IC void vfUpdateTime(float fTimeDelta);
    IC void add_active_member(bool bForceActive = false);
    IC void vfRemoveActiveMember();
    IC void vfAddStandingMember();
    IC void vfRemoveStandingMember();
    IC bool bfCheckIfSoundFrightful();
    IC bool bfCheckIfOutsideAIMap(Fvector& tTemp1);

    //////////////////////////
    // FSM STATES
    //////////////////////////
    void Death();
    void FreeHuntingActive();
    void FreeHuntingPassive();
    void AttackFire();
    void AttackRun();
    void UnderFire();
    void Retreat();
    void Pursuit();
    void FreeRecoil();
    void ReturnHome();
    void EatCorpse();
    void test_movement();
    void init();

public:
    CAI_Rat();
    virtual ~CAI_Rat();
    virtual IFactoryObject* _construct();

public:
    virtual CGameObject* cast_game_object() { return this; };
    virtual CInventoryItem* cast_inventory_item() { return this; }
    virtual CAttachableItem* cast_attachable_item() { return this; }
    virtual CEatableItem* cast_eatable_item() { return this; }
    virtual CEntityAlive* cast_entity_alive() { return this; }
    virtual CEntity* cast_entity() { return this; }
    virtual CPhysicsShellHolder* cast_physics_shell_holder() { return this; }
    virtual CParticlesPlayer* cast_particles_player() { return this; }
    virtual CCustomMonster* cast_custom_monster() { return this; }
    virtual CScriptEntity* cast_script_entity() { return this; }
    virtual CWeapon* cast_weapon() { return NULL; }
    virtual CAI_Rat* dcast_Rat() { return this; };
public:
    virtual BOOL renderable_ShadowReceive();
    virtual BOOL renderable_ShadowGenerate();
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void net_Export(NET_Packet& P);
    virtual void net_Import(NET_Packet& P);
    virtual void HitSignal(float amount, Fvector& vLocalDir, IGameObject* who, s16 element);
    virtual void Die(IGameObject* who);
    virtual void Load(LPCSTR section);
    virtual void Think();
    virtual void SelectAnimation(const Fvector& _view, const Fvector& _move, float speed);
    virtual void Exec_Action(float dt);
    virtual void feel_sound_new(
        IGameObject* who, int type, CSound_UserDataPtr user_data, const Fvector& Position, float power);
    virtual void feel_touch_new(IGameObject* O);
    virtual bool feel_touch_on_contact(IGameObject* O);
    virtual bool feel_vision_isRelevant(IGameObject*);
    virtual void shedule_Update(u32 dt);
    virtual void UpdateCL();
    virtual void Hit(SHit* pHDS);
    void CreateSkeleton();

    virtual void UpdatePositionAnimation();

    /////////////////////////////////////
    // rat as eatable item
    virtual void OnHUDDraw(CCustomHUD* hud) { inherited::OnHUDDraw(hud); }
    virtual void OnH_B_Chield();
    virtual void OnH_B_Independent();
    virtual void OnH_A_Independent();
    virtual void OnEvent(NET_Packet& P, u16 type) { inherited::OnEvent(P, type); }
    virtual bool Useful() const;
    virtual BOOL UsedAI_Locations();
    ///////////////////////////////////////////////////////////////////////
    virtual u16 PHGetSyncItemsNumber() { return inherited::PHGetSyncItemsNumber(); }
    virtual CPHSynchronize* PHGetSyncItem(u16 item) { return inherited::PHGetSyncItem(item); }
    virtual void PHUnFreeze() { return inherited::PHUnFreeze(); }
    virtual void PHFreeze() { return inherited::PHFreeze(); }
///////////////////////////////////////////////////////////////////////
#ifdef DEBUG
    virtual void OnRender();
#endif
    virtual bool useful(const CItemManager* manager, const CGameObject* object) const;
    virtual float evaluate(const CItemManager* manager, const CGameObject* object) const;
    virtual void reinit();
    virtual void reload(LPCSTR section);
    virtual const SRotation Orientation() const { return (inherited::Orientation()); };
public:
    virtual void make_Interpolation();
    virtual void PH_B_CrPr(); // actions & operations before physic correction-prediction steps
    virtual void PH_I_CrPr(); // actions & operations after correction before prediction steps
#ifdef DEBUG
    virtual void PH_Ch_CrPr(); //
#endif
    virtual void PH_A_CrPr(); // actions & operations after phisic correction-prediction steps
    virtual void OnH_A_Chield();
    virtual void create_physic_shell();
    virtual void setup_physic_shell();
    virtual void activate_physic_shell();
    virtual void on_activate_physic_shell();
    virtual Feel::Sound* dcast_FeelSound() { return this; }
    virtual bool use_model_pitch() const;
    virtual float get_custom_pitch_speed(float def_speed);

    // serialization
    virtual void save(NET_Packet& output_packet) { inherited::save(output_packet); }
    virtual void load(IReader& input_packet) { inherited::load(input_packet); }
    virtual BOOL net_SaveRelevant() { return inherited::net_SaveRelevant(); }
    bool can_stand_here();
    bool can_stand_in_position();
    Fvector get_next_target_point();
#ifdef _DEBUG
    void draw_way();
#endif
private:
    steering_behaviour::manager* m_behaviour_manager;

protected:
    virtual bool use_parent_ai_locations() const { return CAttachableItem::use_parent_ai_locations(); }
};

#include "ai_rat_inline.h"
