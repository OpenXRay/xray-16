#include "StdAfx.h"
#include "boar.h"
#include "boar_state_manager.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "ai/monsters/monster_velocity_space.h"
#include "game_object_space.h"
#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_movement_base.h"

CAI_Boar::CAI_Boar()
{
    StateMan = new CStateManagerBoar(this);

    CControlled::init_external(this);
}

CAI_Boar::~CAI_Boar() { xr_delete(StateMan); }
void CAI_Boar::Load(LPCSTR section)
{
    inherited::Load(section);

    if (!pSettings->line_exist(section, "is_friendly"))
        com_man().add_ability(ControlCom::eControlRunAttack);
    com_man().add_ability(ControlCom::eControlRotationJump);

    anim().AddReplacedAnim(&m_bDamaged, eAnimRun, eAnimRunDamaged);
    anim().AddReplacedAnim(&m_bDamaged, eAnimWalkFwd, eAnimWalkDamaged);
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
    SVelocityParam& velocity_drag = move().get_velocity(MonsterMovement::eVelocityParameterDrag);

    anim().AddAnim(eAnimStandIdle, "stand_idle_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimStandTurnLeft, "stand_turn_ls_", -1, &velocity_turn, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimStandTurnRight, "stand_turn_rs_", -1, &velocity_turn, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");

    anim().AddAnim(
        eAnimLieIdle, "lie_sleep_", -1, &velocity_none, PS_LIE, "fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(
        eAnimSleep, "lie_sleep_", -1, &velocity_none, PS_LIE, "fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");

    anim().AddAnim(eAnimWalkFwd, "stand_walk_fwd_", -1, &velocity_walk, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimWalkDamaged, "stand_walk_fwd_dmg_", -1, &velocity_walk_dmg, PS_STAND, "fx_stand_f",
        "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimRun, "stand_run_fwd_", -1, &velocity_run, PS_STAND, "fx_stand_f", "fx_stand_b", "fx_stand_l",
        "fx_stand_r");
    anim().AddAnim(eAnimRunDamaged, "stand_run_dmg_", -1, &velocity_run_dmg, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimCheckCorpse, "stand_check_corpse_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(
        eAnimEat, "stand_eat_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");

    anim().AddAnim(eAnimAttack, "stand_attack_", -1, &velocity_turn, PS_STAND, "fx_stand_f", "fx_stand_b", "fx_stand_l",
        "fx_stand_r");

    anim().AddAnim(eAnimStandLieDown, "stand_lie_down_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimLieStandUp, "lie_stand_up_", -1, &velocity_none, PS_LIE, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimLieToSleep, "lie_to_sleep_", -1, &velocity_none, PS_LIE, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimDragCorpse, "stand_drag_", -1, &velocity_drag, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimLookAround, "stand_idle_", 2, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimSteal, "stand_steal_", -1, &velocity_steal, PS_STAND, "fx_stand_f", "fx_stand_b", "fx_stand_l",
        "fx_stand_r");
    anim().AddAnim(
        eAnimDie, "stand_idle_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimJumpLeft, "stand_jump_left_", -1, &velocity_turn, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimJumpRight, "stand_jump_right_", -1, &velocity_turn, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");

    anim().AddAnim(eAnimRunTurnLeft, "stand_run_look_left_", -1, &velocity_run, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimRunTurnRight, "stand_run_look_right_", -1, &velocity_run, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");

    // define transitions
    anim().AddTransition(eAnimStandLieDown, eAnimSleep, eAnimLieToSleep, false);
    anim().AddTransition(PS_STAND, eAnimSleep, eAnimStandLieDown, true);
    anim().AddTransition(PS_STAND, PS_LIE, eAnimStandLieDown, false);
    anim().AddTransition(PS_LIE, PS_STAND, eAnimLieStandUp, false, SKIP_IF_AGGRESSIVE);

    // define links from Action to animations
    anim().LinkAction(ACT_STAND_IDLE, eAnimStandIdle);
    anim().LinkAction(ACT_SIT_IDLE, eAnimLieIdle);
    anim().LinkAction(ACT_LIE_IDLE, eAnimLieIdle);
    anim().LinkAction(ACT_WALK_FWD, eAnimWalkFwd);
    anim().LinkAction(ACT_WALK_BKWD, eAnimDragCorpse);
    anim().LinkAction(ACT_RUN, eAnimRun);
    anim().LinkAction(ACT_EAT, eAnimEat);
    anim().LinkAction(ACT_SLEEP, eAnimSleep);
    anim().LinkAction(ACT_REST, eAnimLieIdle);
    anim().LinkAction(ACT_DRAG, eAnimDragCorpse);
    anim().LinkAction(ACT_ATTACK, eAnimAttack);
    anim().LinkAction(ACT_STEAL, eAnimSteal);
    anim().LinkAction(ACT_LOOK_AROUND, eAnimLookAround);

#ifdef DEBUG
    anim().accel_chain_test();
#endif

    PostLoad(section);
}

void CAI_Boar::reinit()
{
    inherited::reinit();
    if (CCustomMonster::use_simplified_visual())
        return;
    com_man().add_rotation_jump_data("stand_jump_left_0", 0, "stand_jump_right_0", 0, PI - PI_DIV_6,
        SControlRotationJumpData::eStopAtOnce | SControlRotationJumpData::eRotateOnce);
}

void CAI_Boar::BoneCallback(CBoneInstance* B)
{
    CAI_Boar* P = static_cast<CAI_Boar*>(B->callback_param());

    if (!P->look_at_enemy)
        return;

    Fmatrix M;
    M.setHPB(0.0f, -P->_cur_delta, 0.0f);
    B->mTransform.mulB_43(M);
}

BOOL CAI_Boar::net_Spawn(CSE_Abstract* DC)
{
    if (!inherited::net_Spawn(DC))
        return (FALSE);

    if (!PPhysicsShell()) //нельзя ставить колбеки, если создан физ шел - у него стоят свои колбеки!!!
    {
        CBoneInstance& BI = smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(
            smart_cast<IKinematics*>(Visual())->LL_BoneID("bip01_head"));
        BI.set_callback(bctCustom, BoneCallback, this);
    }

    _cur_delta = _target_delta = 0.f;
    _velocity = PI;
    look_at_enemy = false;
    return TRUE;
}

void CAI_Boar::CheckSpecParams(u32 spec_params)
{
    // if ((spec_params & ASP_ROTATION_JUMP) == ASP_ROTATION_JUMP) {
    //	float yaw, pitch;
    //	Fvector().sub(EnemyMan.get_enemy()->Position(), Position()).getHP(yaw,pitch);
    //	yaw *= -1;
    //	yaw = angle_normalize(yaw);

    //	EMotionAnim anim = eAnimJumpLeft;
    //	if (from_right(yaw,movement().m_body.current.yaw)) {
    //		anim = eAnimJumpRight;
    //		yaw = angle_normalize(yaw + PI / 20);
    //	} else yaw = angle_normalize(yaw - PI / 20);

    //	anim().Seq_Add(anim);
    //	anim().Seq_Switch();

    //	movement().m_body.target.yaw = yaw;

    //	// calculate angular speed
    //	float new_angular_velocity;
    //	float delta_yaw = angle_difference(yaw,movement().m_body.current.yaw);
    //	float time = anim().GetCurAnimTime();
    //	new_angular_velocity = 2.5f * delta_yaw / time;

    //	anim().ForceAngularSpeed(new_angular_velocity);

    //	return;
    //}

    // 	if ( (spec_params & ASP_ATTACK_RUN) == ASP_ATTACK_RUN )
    // 	{
    // 		anim().SetCurAnim(eAnimAttackRun);
    // 	}
}

void CAI_Boar::UpdateCL()
{
    inherited::UpdateCL();
    angle_lerp(_cur_delta, _target_delta, _velocity, client_update_fdelta());
}
