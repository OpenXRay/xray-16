#include "stdafx.h"
#include "bloodsucker.h"
#include "bloodsucker_state_manager.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "Level.h"
#include "material_manager.h"
#include "bloodsucker_vampire_effector.h"
#include "detail_path_manager.h"
#include "level_debug.h"
#include "ai/monsters/monster_velocity_space.h"
#include "GamePersistent.h"
#include "game_object_space.h"
#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_movement_base.h"
#include "ai/Monsters/control_rotation_jump.h"
#include "sound_player.h"
#include "xrEngine/camerabase.h"
#include "xr_level_controller.h"
#include "ActorCondition.h"
#include "PHDestroyable.h"
#include "CharacterPhysicsSupport.h"

namespace detail
{
namespace bloodsucker
{
// default hit settings
float const default_critical_hit_chance = 0.25f;
float const default_hit_camera_effector_angle = 0;
float const default_critical_hit_camera_effector_angle = 3.1415f / 6;

float const default_camera_effector_move_angular_speed = 1.5f;
u32 const default_visibility_state_change_min_delay = 1000;

float const default_full_visibility_radius = 5;
float const default_partial_visibility_radius = 10;
float const default_runaway_invisible_time = 3000;

char const* const full_visibility_radius_string = "full_visibility_radius";
char const* const partial_visibility_radius_string = "partial_visibility_radius";
char const* const visibility_state_change_min_delay_string = "visibility_state_change_min_delay";

} // namespace bloodsucker
} // namespace detail

u32 CAI_Bloodsucker::m_time_last_vampire = 0;

CAI_Bloodsucker::CAI_Bloodsucker()
{
    StateMan = new CStateManagerBloodsucker(this);
    m_alien_control.init_external(this);
    m_drag_anim_jump = false;
    m_animated = false;
    collision_off = false;
    m_force_visibility_state = unset;
    m_runaway_invisible_time = 0;

    using namespace detail::bloodsucker;
}

CAI_Bloodsucker::~CAI_Bloodsucker() { xr_delete(StateMan); }
void CAI_Bloodsucker::Load(LPCSTR section)
{
    inherited::Load(section);

    if (pSettings->line_exist(section, "collision_hit_off"))
    {
        collision_hit_off = true;
    }
    else
        collision_hit_off = false;
    if (!pSettings->line_exist(section, "is_friendly"))
        com_man().add_ability(ControlCom::eControlRunAttack);
    com_man().add_ability(ControlCom::eControlRotationJump);
    com_man().add_ability(ControlCom::eControlJump);

    invisible_vel.set(0.1f, 0.1f);

    EnemyMemory.init_external(this, 40000);

    anim().AddReplacedAnim(&m_bDamaged, eAnimRun, eAnimRunDamaged);
    anim().AddReplacedAnim(&m_bDamaged, eAnimWalkFwd, eAnimWalkDamaged);
    anim().AddReplacedAnim(&m_bDamaged, eAnimStandIdle, eAnimStandDamaged);
    anim().AddReplacedAnim(&m_bRunTurnLeft, eAnimRun, eAnimRunTurnLeft);
    anim().AddReplacedAnim(&m_bRunTurnRight, eAnimRun, eAnimRunTurnRight);

    anim().accel_load(section);
    anim().accel_chain_add(eAnimWalkFwd, eAnimRun);
    anim().accel_chain_add(eAnimWalkFwd, eAnimRunTurnLeft);
    anim().accel_chain_add(eAnimWalkFwd, eAnimRunTurnRight);
    anim().accel_chain_add(eAnimWalkDamaged, eAnimRunDamaged);

    SVelocityParam& velocity_none = move().get_velocity(MonsterMovement::eVelocityParameterIdle);
    SVelocityParam& velocity_turn = move().get_velocity(MonsterMovement::eVelocityParameterStand);
    SVelocityParam& velocity_walk = move().get_velocity(MonsterMovement::eVelocityParameterWalkNormal);
    SVelocityParam& velocity_run = move().get_velocity(MonsterMovement::eVelocityParameterRunNormal);
    SVelocityParam& velocity_walk_dmg = move().get_velocity(MonsterMovement::eVelocityParameterWalkDamaged);
    SVelocityParam& velocity_run_dmg = move().get_velocity(MonsterMovement::eVelocityParameterRunDamaged);
    SVelocityParam& velocity_steal = move().get_velocity(MonsterMovement::eVelocityParameterSteal);

    if (pSettings->line_exist(section, "is_no_fx"))
    {
        anim().AddAnim(eAnimStandIdle, "stand_idle_", -1, &velocity_none, PS_STAND);
        anim().AddAnim(eAnimStandDamaged, "stand_damaged_", -1, &velocity_none, PS_STAND);
        anim().AddAnim(eAnimStandTurnLeft, "stand_turn_ls_", -1, &velocity_turn, PS_STAND);
        anim().AddAnim(eAnimStandTurnRight, "stand_turn_rs_", -1, &velocity_turn, PS_STAND);
        anim().AddAnim(eAnimSleep, "lie_sleep_", -1, &velocity_none, PS_LIE);
        anim().AddAnim(eAnimSleepStanding, "stand_sleep_", -1, &velocity_none, PS_STAND);
        anim().AddAnim(eAnimWalkFwd, "stand_walk_fwd_", -1, &velocity_walk, PS_STAND);
        anim().AddAnim(eAnimWalkDamaged, "stand_walk_fwd_dmg_", -1, &velocity_walk_dmg, PS_STAND);
        anim().AddAnim(eAnimRun, "stand_run_", -1, &velocity_run, PS_STAND);
        anim().AddAnim(eAnimRunDamaged, "stand_run_dmg_", -1, &velocity_run_dmg, PS_STAND);

        anim().AddAnim(eAnimRunTurnLeft, "stand_run_turn_left_", -1, &velocity_run, PS_STAND);
        anim().AddAnim(eAnimRunTurnRight, "stand_run_turn_right_", -1, &velocity_run, PS_STAND);
        anim().AddAnim(eAnimScared, "stand_scared_", -1, &velocity_none, PS_STAND);

        anim().AddAnim(eAnimCheckCorpse, "stand_check_corpse_", -1, &velocity_none, PS_STAND);
        anim().AddAnim(eAnimEat, "sit_eat_", -1, &velocity_none, PS_SIT);

        anim().AddAnim(eAnimDie, "stand_idle_", -1, &velocity_none, PS_STAND);

        anim().AddAnim(eAnimAttack, "stand_attack_", -1, &velocity_turn, PS_STAND);
        anim().AddAnim(eAnimAttackRun, "stand_attack_run_", -1, &velocity_run, PS_STAND);

        anim().AddAnim(eAnimLookAround, "stand_look_around_", -1, &velocity_none, PS_STAND);
        anim().AddAnim(eAnimSitIdle, "sit_idle_", -1, &velocity_none, PS_SIT);
        anim().AddAnim(eAnimSitStandUp, "sit_stand_up_", -1, &velocity_none, PS_SIT);
        anim().AddAnim(eAnimSitToSleep, "sit_sleep_down_", -1, &velocity_none, PS_SIT);
        anim().AddAnim(eAnimStandSitDown, "stand_sit_down_", -1, &velocity_none, PS_STAND);

        anim().AddAnim(eAnimSteal, "stand_steal_", -1, &velocity_steal, PS_STAND);

        anim().AddAnim(eAnimThreaten, "stand_threaten_", -1, &velocity_none, PS_STAND);
        anim().AddAnim(eAnimMiscAction_00, "stand_to_aggressive_", -1, &velocity_none, PS_STAND);
    }
    else
    {
        anim().AddAnim(eAnimStandIdle, "stand_idle_", -1, &velocity_none, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");
        anim().AddAnim(eAnimStandDamaged, "stand_damaged_", -1, &velocity_none, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");
        anim().AddAnim(eAnimStandTurnLeft, "stand_turn_ls_", -1, &velocity_turn, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");
        anim().AddAnim(eAnimStandTurnRight, "stand_turn_rs_", -1, &velocity_turn, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");
        anim().AddAnim(
            eAnimSleep, "lie_sleep_", -1, &velocity_none, PS_LIE, "fx_run_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
        anim().AddAnim(eAnimSleepStanding, "stand_sleep_", -1, &velocity_none, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");
        anim().AddAnim(eAnimWalkFwd, "stand_walk_fwd_", -1, &velocity_walk, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");
        anim().AddAnim(eAnimWalkDamaged, "stand_walk_fwd_dmg_", -1, &velocity_walk_dmg, PS_STAND, "fx_run_f",
            "fx_stand_b", "fx_stand_l", "fx_stand_r");
        anim().AddAnim(
            eAnimRun, "stand_run_", -1, &velocity_run, PS_STAND, "fx_run_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
        anim().AddAnim(eAnimRunDamaged, "stand_run_dmg_", -1, &velocity_run_dmg, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");

        anim().AddAnim(eAnimRunTurnLeft, "stand_run_turn_left_", -1, &velocity_run, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");
        anim().AddAnim(eAnimRunTurnRight, "stand_run_turn_right_", -1, &velocity_run, PS_STAND, "fx_run_f",
            "fx_stand_b", "fx_stand_l", "fx_stand_r");
        anim().AddAnim(eAnimScared, "stand_scared_", -1, &velocity_none, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");

        anim().AddAnim(eAnimCheckCorpse, "stand_check_corpse_", -1, &velocity_none, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");
        anim().AddAnim(
            eAnimEat, "sit_eat_", -1, &velocity_none, PS_SIT, "fx_run_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");

        anim().AddAnim(eAnimDie, "stand_idle_", -1, &velocity_none, PS_STAND, "fx_run_f", "fx_stand_b", "fx_stand_l",
            "fx_stand_r");

        anim().AddAnim(eAnimAttack, "stand_attack_", -1, &velocity_turn, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");
        anim().AddAnim(eAnimAttackRun, "stand_attack_run_", -1, &velocity_run, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");

        anim().AddAnim(eAnimLookAround, "stand_look_around_", -1, &velocity_none, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");
        anim().AddAnim(eAnimSitIdle, "sit_idle_", -1, &velocity_none, PS_SIT, "fx_run_f", "fx_stand_b", "fx_stand_l",
            "fx_stand_r");
        anim().AddAnim(eAnimSitStandUp, "sit_stand_up_", -1, &velocity_none, PS_SIT, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");
        anim().AddAnim(eAnimSitToSleep, "sit_sleep_down_", -1, &velocity_none, PS_SIT, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");
        anim().AddAnim(eAnimStandSitDown, "stand_sit_down_", -1, &velocity_none, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");

        anim().AddAnim(eAnimSteal, "stand_steal_", -1, &velocity_steal, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");

        anim().AddAnim(eAnimThreaten, "stand_threaten_", -1, &velocity_none, PS_STAND, "fx_run_f", "fx_stand_b",
            "fx_stand_l", "fx_stand_r");
        anim().AddAnim(eAnimMiscAction_00, "stand_to_aggressive_", -1, &velocity_none, PS_STAND, "fx_run_f",
            "fx_stand_b", "fx_stand_l", "fx_stand_r");
    }

    // define transitions
    //	anim().AddTransition(PS_STAND,			eAnimThreaten,	eAnimMiscAction_00,	false);
    anim().AddTransition(eAnimStandSitDown, eAnimSleep, eAnimSitToSleep, false);
    anim().AddTransition(PS_STAND, eAnimSleep, eAnimStandSitDown, true);
    anim().AddTransition(PS_STAND, PS_SIT, eAnimStandSitDown, false);
    anim().AddTransition(PS_STAND, PS_LIE, eAnimStandSitDown, false);
    anim().AddTransition(PS_SIT, PS_STAND, eAnimSitStandUp, false);
    anim().AddTransition(PS_LIE, PS_STAND, eAnimSitStandUp, false);

    // define links from Action to animations
    anim().LinkAction(ACT_STAND_IDLE, eAnimStandIdle);
    // anim().LinkAction(ACT_CAPTURE_PREPARE,	eAnimCapturePrepare);
    anim().LinkAction(ACT_SIT_IDLE, eAnimSitIdle);
    anim().LinkAction(ACT_LIE_IDLE, eAnimSitIdle);
    anim().LinkAction(ACT_WALK_FWD, eAnimWalkFwd);
    anim().LinkAction(ACT_WALK_BKWD, eAnimWalkBkwd);
    anim().LinkAction(ACT_RUN, eAnimRun);
    anim().LinkAction(ACT_EAT, eAnimEat);
    anim().LinkAction(ACT_SLEEP, eAnimSleep);
    anim().LinkAction(ACT_REST, eAnimSitIdle);
    // anim().LinkAction(ACT_DRAG,			eAnimWalkBkwd);
    anim().LinkAction(ACT_ATTACK, eAnimAttack);
    anim().LinkAction(ACT_STEAL, eAnimSteal);
    anim().LinkAction(ACT_LOOK_AROUND, eAnimLookAround);

    m_hits_before_vampire = 0;

#ifdef DEBUG
    anim().accel_chain_test();
#endif

    // load other misc stuff
    invisible_vel.set(pSettings->r_float(section, "Velocity_Invisible_Linear"),
        pSettings->r_float(section, "Velocity_Invisible_Angular"));
    movement().detail().add_velocity(MonsterMovement::eVelocityParameterInvisible,
        CDetailPathManager::STravelParams(invisible_vel.linear, invisible_vel.angular));

    LoadVampirePPEffector(pSettings->r_string(section, "vampire_effector"));
    m_vampire_min_delay = pSettings->r_u32(section, "Vampire_Delay");

    m_visual_predator = pSettings->r_string(section, "Predator_Visual");

    m_vampire_want_speed = pSettings->r_float(section, "Vampire_Want_Speed");
    m_vampire_wound = pSettings->r_float(section, "Vampire_Wound");
    m_vampire_gain_health = pSettings->r_float(section, "Vampire_GainHealth");
    m_vampire_distance = pSettings->r_float(section, "Vampire_Distance");
    m_sufficient_hits_before_vampire = pSettings->r_u32(section, "Vampire_Sufficient_Hits");
    m_sufficient_hits_before_vampire_random = -1 + (rand() % 3);

    invisible_particle_name = pSettings->r_string(section, "Particle_Invisible");

    using namespace detail::bloodsucker;

    READ_IF_EXISTS(pSettings, r_float, section, "separate_factor", 0.f);

    m_visibility_state_change_min_delay = READ_IF_EXISTS(
        pSettings, r_u32, section, "visibility_state_change_min_delay", default_visibility_state_change_min_delay);

    m_full_visibility_radius =
        READ_IF_EXISTS(pSettings, r_float, section, full_visibility_radius_string, default_full_visibility_radius);
    m_partial_visibility_radius = READ_IF_EXISTS(
        pSettings, r_float, section, partial_visibility_radius_string, default_partial_visibility_radius);
    m_visibility_state = unset;
    m_visibility_state_last_changed_time = 0;

    PostLoad(section);
}

void CAI_Bloodsucker::reinit()
{
    m_force_visibility_state = unset;

    inherited::reinit();
    CControlledActor::reinit();
    m_visual_default = cNameVisual();

    if (CCustomMonster::use_simplified_visual())
        return;

    Bones.Reset();

    com_man().ta_fill_data(anim_triple_vampire, "vampire_0", "vampire_1", "vampire_2", TA_EXECUTE_LOOPED,
        TA_DONT_SKIP_PREPARE, 0); // ControlCom::eCapturePath | ControlCom::eCaptureMovement);

    m_alien_control.reinit();

    state_invisible = false;

    // com_man().add_rotation_jump_data("run_turn_r_0","run_turn_r_1","run_turn_r_0","run_turn_r_1", PI - 0.01f,
    // SControlRotationJumpData::eStopAtOnce | SControlRotationJumpData::eRotateOnce);
    com_man().add_rotation_jump_data("run_turn_l_0", "run_turn_l_1", "run_turn_r_0", "run_turn_r_1", PI_DIV_2);

    com_man().load_jump_data("boloto_jump_prepare", 0, "boloto_jump_fly", "boloto_jump_end", u32(-1),
        MonsterMovement::eBloodsuckerVelocityParameterJumpGround, 0);

    // save visual
    m_visual_default = cNameVisual();

    m_vampire_want_value = 0.f;
    m_predator = false;
    m_vis_state = 0;

    start_invisible_predator();
}

void CAI_Bloodsucker::reload(LPCSTR section)
{
    inherited::reload(section);

    sound().add(pSettings->r_string(section, "Sound_Vampire_Grasp"), DEFAULT_SAMPLE_COUNT, SOUND_TYPE_MONSTER_ATTACKING,
        MonsterSound::eHighPriority + 4, MonsterSound::eBaseChannel, eVampireGrasp, "bip01_head");
    sound().add(pSettings->r_string(section, "Sound_Vampire_Sucking"), DEFAULT_SAMPLE_COUNT,
        SOUND_TYPE_MONSTER_ATTACKING, MonsterSound::eHighPriority + 3, MonsterSound::eBaseChannel, eVampireSucking,
        "bip01_head");
    sound().add(pSettings->r_string(section, "Sound_Vampire_Hit"), DEFAULT_SAMPLE_COUNT, SOUND_TYPE_MONSTER_ATTACKING,
        MonsterSound::eHighPriority + 2, MonsterSound::eBaseChannel, eVampireHit, "bip01_head");
    sound().add(pSettings->r_string(section, "Sound_Vampire_StartHunt"), DEFAULT_SAMPLE_COUNT,
        SOUND_TYPE_MONSTER_ATTACKING, MonsterSound::eHighPriority + 5, MonsterSound::eBaseChannel, eVampireStartHunt,
        "bip01_head");
    sound().add(pSettings->r_string(section, "Sound_Invisibility_Change_State"), DEFAULT_SAMPLE_COUNT,
        SOUND_TYPE_MONSTER_ATTACKING, MonsterSound::eNormalPriority, MonsterSound::eChannelIndependent << 1,
        eChangeVisibility, "bip01_head");
    sound().add(pSettings->r_string(section, "Sound_Growl"), DEFAULT_SAMPLE_COUNT, SOUND_TYPE_MONSTER_ATTACKING,
        MonsterSound::eHighPriority + 6, MonsterSound::eBaseChannel, eGrowl, "bip01_head");
    sound().add(pSettings->r_string(section, "Sound_Alien"), DEFAULT_SAMPLE_COUNT, SOUND_TYPE_MONSTER_ATTACKING,
        MonsterSound::eCriticalPriority, u32(MonsterSound::eCaptureAllChannels), eAlien, "bip01_head");
}

void CAI_Bloodsucker::LoadVampirePPEffector(LPCSTR section)
{
    pp_vampire_effector.duality.h = pSettings->r_float(section, "duality_h");
    pp_vampire_effector.duality.v = pSettings->r_float(section, "duality_v");
    pp_vampire_effector.gray = pSettings->r_float(section, "gray");
    pp_vampire_effector.blur = pSettings->r_float(section, "blur");
    pp_vampire_effector.noise.intensity = pSettings->r_float(section, "noise_intensity");
    pp_vampire_effector.noise.grain = pSettings->r_float(section, "noise_grain");
    pp_vampire_effector.noise.fps = pSettings->r_float(section, "noise_fps");
    VERIFY(!fis_zero(pp_vampire_effector.noise.fps));

    sscanf(pSettings->r_string(section, "color_base"), "%f,%f,%f", &pp_vampire_effector.color_base.r,
        &pp_vampire_effector.color_base.g, &pp_vampire_effector.color_base.b);
    sscanf(pSettings->r_string(section, "color_gray"), "%f,%f,%f", &pp_vampire_effector.color_gray.r,
        &pp_vampire_effector.color_gray.g, &pp_vampire_effector.color_gray.b);
    sscanf(pSettings->r_string(section, "color_add"), "%f,%f,%f", &pp_vampire_effector.color_add.r,
        &pp_vampire_effector.color_add.g, &pp_vampire_effector.color_add.b);
}

void CAI_Bloodsucker::BoneCallback(CBoneInstance* B)
{
    CAI_Bloodsucker* this_class = static_cast<CAI_Bloodsucker*>(B->callback_param());

    this_class->Bones.Update(B, Device.dwTimeGlobal);
}

void CAI_Bloodsucker::vfAssignBones()
{
    // Установка callback на кости

    bone_spine = &smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(
        smart_cast<IKinematics*>(Visual())->LL_BoneID("bip01_spine"));
    bone_head = &smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(
        smart_cast<IKinematics*>(Visual())->LL_BoneID("bip01_head"));
    if (!PPhysicsShell()) //нельзя ставить колбеки, если создан физ шел - у него стоят свои колбеки!!!
    {
        bone_spine->set_callback(bctCustom, BoneCallback, this);
        bone_head->set_callback(bctCustom, BoneCallback, this);
    }

    // Bones settings
    Bones.Reset();
    Bones.AddBone(bone_spine, AXIS_X);
    Bones.AddBone(bone_spine, AXIS_Y);
    Bones.AddBone(bone_head, AXIS_X);
    Bones.AddBone(bone_head, AXIS_Y);
}

//#define MAX_BONE_ANGLE PI_DIV_4

void CAI_Bloodsucker::LookDirection(Fvector to_dir, float bone_turn_speed)
{
    //// получаем вектор направления к источнику звука и его мировые углы
    // float		yaw,pitch;
    // to_dir.getHP(yaw,pitch);

    //// установить параметры вращения по yaw
    // float cur_yaw = -movement().m_body.current.yaw;						// текущий мировой угол монстра
    // float bone_angle;											// угол для боны

    // float dy = _abs(angle_normalize_signed(yaw - cur_yaw));		// дельта, на которую нужно поворачиваться

    // if (angle_difference(cur_yaw,yaw) <= MAX_BONE_ANGLE) {		// bone turn only
    //	bone_angle = dy;
    //} else {													// torso & bone turn
    //	if (movement().IsMoveAlongPathFinished() || !movement().enabled()) movement().m_body.target.yaw =
    // angle_normalize(-yaw);
    //	if (dy / 2 < MAX_BONE_ANGLE) bone_angle = dy / 2;
    //	else bone_angle = MAX_BONE_ANGLE;
    //}

    // bone_angle /= 2;
    // if (from_right(yaw,cur_yaw)) bone_angle *= -1.f;

    // Bones.SetMotion(bone_spine, AXIS_X, bone_angle, bone_turn_speed, 100);
    // Bones.SetMotion(bone_head,	AXIS_X, bone_angle, bone_turn_speed, 100);

    //// установить параметры вращения по pitch
    // clamp(pitch, -MAX_BONE_ANGLE, MAX_BONE_ANGLE);
    // pitch /= 2;

    // Bones.SetMotion(bone_spine, AXIS_Y, pitch, bone_turn_speed, 100);
    // Bones.SetMotion(bone_head,	AXIS_Y, pitch, bone_turn_speed, 100);
}

void CAI_Bloodsucker::ActivateVampireEffector()
{
    Actor()->Cameras().AddCamEffector(
        new CVampireCameraEffector(6.0f, get_head_position(this), get_head_position(Actor())));
    Actor()->Cameras().AddPPEffector(new CVampirePPEffector(pp_vampire_effector, 6.0f));
}

bool CAI_Bloodsucker::WantVampire() { return !!fsimilar(m_vampire_want_value, 1.f); }
void CAI_Bloodsucker::SatisfyVampire()
{
    m_vampire_want_value = 0.f;

    float health = conditions().GetHealth();
    health += m_vampire_gain_health;

    health = std::min(health, conditions().GetMaxHealth());
    conditions().SetHealth(health);
}

void CAI_Bloodsucker::CheckSpecParams(u32 spec_params)
{
    if ((spec_params & ASP_CHECK_CORPSE) == ASP_CHECK_CORPSE)
    {
        com_man().seq_run(anim().get_motion_id(eAnimCheckCorpse));
    }

    if ((spec_params & ASP_THREATEN) == ASP_THREATEN)
    {
        anim().SetCurAnim(eAnimThreaten);
        return;
    }

    if ((spec_params & ASP_STAND_SCARED) == ASP_STAND_SCARED)
    {
        anim().SetCurAnim(eAnimLookAround);
        return;
    }
}

BOOL CAI_Bloodsucker::net_Spawn(CSE_Abstract* DC)
{
    if (!inherited::net_Spawn(DC))
        return (FALSE);

    vfAssignBones();

    return (TRUE);
}

float CAI_Bloodsucker::get_full_visibility_radius()
{
    using namespace detail::bloodsucker;
    return override_if_debug(full_visibility_radius_string, m_full_visibility_radius);
}

float CAI_Bloodsucker::get_partial_visibility_radius()
{
    using namespace detail::bloodsucker;
    return override_if_debug(partial_visibility_radius_string, m_partial_visibility_radius);
}

TTime CAI_Bloodsucker::get_visibility_state_change_min_delay()
{
    using namespace detail::bloodsucker;
    return override_if_debug(visibility_state_change_min_delay_string, m_visibility_state_change_min_delay);
}

CAI_Bloodsucker::visibility_t CAI_Bloodsucker::get_visibility_state() const
{
    return m_force_visibility_state != unset ? m_force_visibility_state : m_visibility_state;
}

void CAI_Bloodsucker::set_visibility_state(visibility_t new_state)
{
    if (m_force_visibility_state != unset)
    {
        new_state = m_force_visibility_state;
    }

    if (new_state == unset)
    {
        return;
    }

    if (m_visibility_state == new_state)
    {
        return;
    }

    if (Device.dwTimeGlobal < m_visibility_state_last_changed_time + get_visibility_state_change_min_delay())
    {
        return;
    }

    m_visibility_state_last_changed_time = Device.dwTimeGlobal;

    m_visibility_state = new_state;

    if (m_visibility_state == full_visibility)
    {
        stop_invisible_predator();
    }
    else if (m_visibility_state == partial_visibility)
    {
        start_invisible_predator();
    }
    else
    {
        sound().play(CAI_Bloodsucker::eChangeVisibility);
    }
}

void CAI_Bloodsucker::force_visibility_state(int state)
{
    m_force_visibility_state = (visibility_t)state;
    set_visibility_state((visibility_t)state);
}

void CAI_Bloodsucker::update_invisibility()
{
    if (CCustomMonster::use_simplified_visual())
        return;

    using namespace detail::bloodsucker;

    if (!g_Alive())
    {
        set_visibility_state(full_visibility);
    }
    else if (Device.dwTimeGlobal < m_runaway_invisible_time + default_runaway_invisible_time / 6)
    {
        set_visibility_state(partial_visibility);
    }
    else if (Device.dwTimeGlobal < m_runaway_invisible_time + default_runaway_invisible_time)
    {
        set_visibility_state(no_visibility);
    }
    else if (CEntityAlive const* const enemy = EnemyMan.get_enemy())
    {
        float const dist2enemy = enemy->Position().distance_to(Position());

        if (dist2enemy <= get_full_visibility_radius())
        {
            set_visibility_state(full_visibility);
        }
        else if (dist2enemy <= get_partial_visibility_radius())
        {
            set_visibility_state(partial_visibility);
        }
        else
        {
            set_visibility_state(no_visibility);
        }
    }
    else
    {
        set_visibility_state(full_visibility);
    }
}

void CAI_Bloodsucker::UpdateCL()
{
    update_invisibility();
    inherited::UpdateCL();
    CControlledActor::frame_update();
    character_physics_support()->movement()->CollisionEnable(!is_collision_off());

    if (g_Alive())
    {
        // update vampire need
        m_vampire_want_value += m_vampire_want_speed * client_update_fdelta();
        clamp(m_vampire_want_value, 0.f, 1.f);
    }
}

void CAI_Bloodsucker::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);

    if (!g_Alive())
    {
        setVisible(TRUE);
        if (state_invisible)
        {
            stop_invisible_predator();
        }
    }

    if (m_alien_control.active())
        sound().play(eAlien);
}

void CAI_Bloodsucker::Die(IGameObject* who)
{
    inherited::Die(who);
    stop_invisible_predator();
}

void CAI_Bloodsucker::post_fsm_update()
{
    inherited::post_fsm_update();

    // EMonsterState state = StateMan->get_state_type();
    //
    // установить агрессивность
    // bool aggressive =	(is_state(state, eStateAttack)) ||
    //					(is_state(state, eStatePanic))	||
    //					(is_state(state, eStateHitted));
}

bool CAI_Bloodsucker::check_start_conditions(ControlCom::EControlType type)
{
    if (type == ControlCom::eControlJump)
    {
        return false;
    }

    if (!inherited::check_start_conditions(type))
    {
        return false;
    }

    if (type == ControlCom::eControlRunAttack)
    {
        return !state_invisible;
    }

    return true;
}

void CAI_Bloodsucker::set_alien_control(bool val) { val ? m_alien_control.activate() : m_alien_control.deactivate(); }
void CAI_Bloodsucker::set_vis()
{
    m_vis_state = 1;
    predator_stop();
}

void CAI_Bloodsucker::set_invis()
{
    m_vis_state = -1;
    predator_start();
}

void CAI_Bloodsucker::set_collision_off(bool b_collision) { collision_off = b_collision; }
bool CAI_Bloodsucker::is_collision_off() { return collision_off; }
void CAI_Bloodsucker::jump(const Fvector& position, float factor)
{
    com_man().script_jump(position, factor);
    sound().play(MonsterSound::eMonsterSoundAggressive);
}

void CAI_Bloodsucker::set_drag_jump(CEntityAlive* e, LPCSTR s, const Fvector& position, float factor)
{
    j_position = position;
    j_factor = factor;
    m_cob = e;
    m_str_cel = s;
    m_drag_anim_jump = true;
    m_animated = true;
}
bool CAI_Bloodsucker::is_drag_anim_jump() { return m_drag_anim_jump; }
bool CAI_Bloodsucker::is_animated() { return m_animated; }
void CAI_Bloodsucker::start_drag()
{
    if (m_animated)
    {
        com_man().script_capture(ControlCom::eControlAnimation);
        smart_cast<IKinematicsAnimated*>(Visual())->PlayCycle(
            "boloto_attack_link_bone", TRUE, animation_end_jump, this);
        m_animated = false;
    }
}

void CAI_Bloodsucker::animation_end_jump(CBlend* B)
{
    ((CAI_Bloodsucker*)B->CallbackParam)->set_invis();
    ((CAI_Bloodsucker*)B->CallbackParam)
        ->jump(((CAI_Bloodsucker*)B->CallbackParam)->j_position, ((CAI_Bloodsucker*)B->CallbackParam)->j_factor);
}

void CAI_Bloodsucker::predator_start()
{
    // if m_predator==false  is_invisible
    if (m_vis_state != 0)
    {
        if (m_vis_state == 1)
        {
            return;
        }
        m_predator = false;
    }
    if (m_predator)
        return;

    cNameVisual_set(m_visual_predator);
    CDamageManager::reload(*cNameSect(), "damage", pSettings);

    control().animation().restart();

    CParticlesPlayer::StartParticles(invisible_particle_name, Fvector().set(0.0f, 0.1f, 0.0f), ID());
    sound().play(CAI_Bloodsucker::eChangeVisibility);

    m_predator = true;
    // state_invisible				= false;
}

void CAI_Bloodsucker::predator_stop()
{
    if (m_vis_state != 0)
    {
        if (m_vis_state == -1)
        {
            return;
        }

        m_predator = true;
    }
    // if m_predator==true  is_visible
    if (!m_predator)
    {
        return;
    }

    cNameVisual_set(*m_visual_default);
    character_physics_support()->in_ChangeVisual();

    CDamageManager::reload(*cNameSect(), "damage", pSettings);

    control().animation().restart();

    CParticlesPlayer::StartParticles(invisible_particle_name, Fvector().set(0.0f, 0.1f, 0.0f), ID());
    sound().play(CAI_Bloodsucker::eChangeVisibility);
    m_predator = false;
}

void CAI_Bloodsucker::predator_freeze() { control().animation().freeze(); }
void CAI_Bloodsucker::predator_unfreeze() { control().animation().unfreeze(); }
void CAI_Bloodsucker::move_actor_cam(float angle)
{
    if (Actor()->cam_Active())
    {
        Actor()->cam_Active()->Move(Random.randI(2) ? kRIGHT : kLEFT, angle);
        Actor()->cam_Active()->Move(Random.randI(2) ? kUP : kDOWN, angle);
    }
}

void CAI_Bloodsucker::HitEntity(
    const CEntity* pEntity, float fDamage, float impulse, Fvector& dir, ALife::EHitType hit_type, bool draw_hit_marks)
{
    bool is_critical = rand() / (float)RAND_MAX <= m_critical_hit_chance;

    if (is_critical)
    {
        impulse *= 10.f;
    }

    inherited::HitEntity(pEntity, fDamage, impulse, dir, hit_type, draw_hit_marks);
}

bool CAI_Bloodsucker::in_solid_state() { return true; }
void CAI_Bloodsucker::Hit(SHit* pHDS)
{
    if (!collision_hit_off)
    {
        inherited::Hit(pHDS);
    }
}

void CAI_Bloodsucker::start_invisible_predator()
{
    state_invisible = true;
    predator_start();
}
void CAI_Bloodsucker::stop_invisible_predator()
{
    state_invisible = false;
    predator_stop();
}

void CAI_Bloodsucker::manual_activate()
{
    state_invisible = true;
    setVisible(FALSE);
}

void CAI_Bloodsucker::manual_deactivate()
{
    state_invisible = false;
    setVisible(TRUE);
}

void CAI_Bloodsucker::renderable_Render()
{
    if (m_visibility_state != no_visibility)
    {
        inherited::renderable_Render();
    }
}

bool CAI_Bloodsucker::done_enough_hits_before_vampire()
{
    return (int)m_hits_before_vampire >=
        (int)m_sufficient_hits_before_vampire + m_sufficient_hits_before_vampire_random;
}

void CAI_Bloodsucker::on_attack_on_run_hit() { ++m_hits_before_vampire; }
void CAI_Bloodsucker::force_stand_sleep_animation(u32 index)
{
    anim().set_override_animation(eAnimSleepStanding, index);
}

void CAI_Bloodsucker::release_stand_sleep_animation() { anim().clear_override_animation(); }
#ifdef DEBUG
CBaseMonster::SDebugInfo CAI_Bloodsucker::show_debug_info()
{
    CBaseMonster::SDebugInfo info = inherited::show_debug_info();
    if (!info.active)
        return CBaseMonster::SDebugInfo();

    string128 text;
    xr_sprintf(text, "Vampire Want Value = [%f] Speed = [%f]", m_vampire_want_value, m_vampire_want_speed);
    DBG().text(this).add_item(text, info.x, info.y += info.delta_y, info.color);
    DBG().text(this).add_item(
        "---------------------------------------", info.x, info.y += info.delta_y, info.delimiter_color);

    return CBaseMonster::SDebugInfo();
}

// Lain: added
void CAI_Bloodsucker::add_debug_info(debug::text_tree& root_s)
{
    typedef debug::text_tree TextTree;
    TextTree& general_s = root_s.find_or_add("General");

    TextTree& current_visual_s = general_s.add_line("Predator_Visual");
    current_visual_s.add_line(m_visual_predator);
    CBaseMonster::add_debug_info(root_s);
}

#ifdef _DEBUG
void CAI_Bloodsucker::debug_on_key(int key)
{
    switch (key)
    {
    case SDL_SCANCODE_MINUS:
        Actor()->cam_Active()->Move(Random.randI(2) ? kRIGHT : kLEFT, PI_DIV_2);
        // set_alien_control(true);
        break;
    case SDL_SCANCODE_EQUALS:
        Actor()->cam_Active()->Move(Random.randI(2) ? kUP : kDOWN, PI_DIV_2);
        // set_alien_control(false);
        break;
    }
}
#endif //_DEBUG

#endif // DEBUG
