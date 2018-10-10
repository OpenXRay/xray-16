#include "StdAfx.h"
#include "cat.h"
#include "cat_state_manager.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "ai/monsters/monster_velocity_space.h"
#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_movement_base.h"

CCat::CCat() { StateMan = new CStateManagerCat(this); }
CCat::~CCat() { xr_delete(StateMan); }
void CCat::Load(LPCSTR section)
{
    inherited::Load(section);

    anim().accel_load(section);
    anim().accel_chain_add(eAnimWalkFwd, eAnimRun);
    anim().accel_chain_add(eAnimWalkDamaged, eAnimRunDamaged);

    anim().AddReplacedAnim(&m_bDamaged, eAnimStandIdle, eAnimStandDamaged);
    anim().AddReplacedAnim(&m_bDamaged, eAnimRun, eAnimRunDamaged);
    anim().AddReplacedAnim(&m_bDamaged, eAnimWalkFwd, eAnimWalkDamaged);

    SVelocityParam& velocity_none = move().get_velocity(MonsterMovement::eVelocityParameterIdle);
    SVelocityParam& velocity_turn = move().get_velocity(MonsterMovement::eVelocityParameterStand);
    SVelocityParam& velocity_walk = move().get_velocity(MonsterMovement::eVelocityParameterWalkNormal);
    SVelocityParam& velocity_run = move().get_velocity(MonsterMovement::eVelocityParameterRunNormal);
    SVelocityParam& velocity_walk_dmg = move().get_velocity(MonsterMovement::eVelocityParameterWalkDamaged);
    SVelocityParam& velocity_run_dmg = move().get_velocity(MonsterMovement::eVelocityParameterRunDamaged);
    SVelocityParam& velocity_steal = move().get_velocity(MonsterMovement::eVelocityParameterSteal);
    SVelocityParam& velocity_drag = move().get_velocity(MonsterMovement::eVelocityParameterDrag);

    anim().AddAnim(eAnimStandIdle, "stand_idle_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimStandDamaged, "stand_idle_dmg_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimStandTurnLeft, "stand_turn_ls_", -1, &velocity_turn, PS_STAND);
    anim().AddAnim(eAnimStandTurnRight, "stand_turn_rs_", -1, &velocity_turn, PS_STAND);
    anim().AddAnim(eAnimWalkFwd, "stand_walk_fwd_", -1, &velocity_walk, PS_STAND);
    anim().AddAnim(eAnimWalkDamaged, "stand_walk_dmg_", -1, &velocity_walk_dmg, PS_STAND);
    anim().AddAnim(eAnimRun, "stand_run_fwd_", -1, &velocity_run, PS_STAND);
    anim().AddAnim(eAnimRunDamaged, "stand_run_dmg_", -1, &velocity_run_dmg, PS_STAND);
    anim().AddAnim(eAnimCheckCorpse, "stand_check_corpse_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimEat, "stand_eat_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimAttack, "stand_attack_", -1, &velocity_turn, PS_STAND);
    anim().AddAnim(eAnimLookAround, "stand_look_around_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimLieIdle, "lie_idle_", -1, &velocity_none, PS_LIE);
    anim().AddAnim(eAnimLieStandUp, "lie_stand_up_", -1, &velocity_none, PS_LIE);
    anim().AddAnim(eAnimDragCorpse, "stand_drag_", -1, &velocity_drag, PS_STAND);
    anim().AddAnim(eAnimSteal, "stand_steal_", -1, &velocity_steal, PS_STAND);
    anim().AddAnim(eAnimStandLieDown, "stand_lie_down_", -1, &velocity_none, PS_STAND);

    anim().AddAnim(eAnimJumpLeft, "stand_jump_ls_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimJumpRight, "stand_jump_rs_", -1, &velocity_none, PS_STAND);

    anim().AddTransition(PS_LIE, PS_STAND, eAnimLieStandUp, false);
    anim().AddTransition(PS_STAND, PS_LIE, eAnimStandLieDown, false);

    // link action
    anim().LinkAction(ACT_STAND_IDLE, eAnimStandIdle);
    anim().LinkAction(ACT_SIT_IDLE, eAnimStandIdle);
    anim().LinkAction(ACT_LIE_IDLE, eAnimLieIdle);
    anim().LinkAction(ACT_WALK_FWD, eAnimWalkFwd);
    anim().LinkAction(ACT_WALK_BKWD, eAnimWalkFwd);
    anim().LinkAction(ACT_RUN, eAnimRun);
    anim().LinkAction(ACT_EAT, eAnimEat);
    anim().LinkAction(ACT_SLEEP, eAnimLieIdle);
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

void CCat::reinit()
{
    inherited::reinit();

    MotionID def1, def2, def3;
    IKinematicsAnimated* pSkel = smart_cast<IKinematicsAnimated*>(Visual());

    def1 = pSkel->ID_Cycle_Safe("jump_attack_0");
    VERIFY(def1);
    def2 = pSkel->ID_Cycle_Safe("jump_attack_1");
    VERIFY(def2);
    def3 = pSkel->ID_Cycle_Safe("jump_attack_2");
    VERIFY(def3);

    // CJumpingAbility::reinit(def1, def2, def3);
}

void CCat::try_to_jump()
{
    IGameObject* target = const_cast<CEntityAlive*>(EnemyMan.get_enemy());
    if (!target || !EnemyMan.see_enemy_now())
        return;
}

void CCat::CheckSpecParams(u32 spec_params)
{
    if ((spec_params & ASP_CHECK_CORPSE) == ASP_CHECK_CORPSE)
    {
        com_man().seq_run(anim().get_motion_id(eAnimCheckCorpse));
    }

    if ((spec_params & ASP_ROTATION_JUMP) == ASP_ROTATION_JUMP)
    {
        // float yaw, pitch;
        // Fvector().sub(EnemyMan.get_enemy()->Position(), Position()).getHP(yaw,pitch);
        // yaw *= -1;
        // yaw = angle_normalize(yaw);

        // EMotionAnim anim = eAnimJumpLeft;
        // if (from_right(yaw,movement().m_body.current.yaw)) {
        //	anim = eAnimJumpRight;
        //	yaw = angle_normalize(yaw + PI / 20);
        //} else yaw = angle_normalize(yaw - PI / 20);

        // anim().Seq_Add(anim);
        // anim().Seq_Switch();

        // movement().stop_linear		();
        // movement().m_body.target.yaw = yaw;

        //// calculate angular speed
        // float new_angular_velocity;
        // float delta_yaw = angle_difference(yaw,movement().m_body.current.yaw);
        // float time = anim().GetCurAnimTime();
        // new_angular_velocity = delta_yaw / time;

        // anim().ForceAngularSpeed(new_angular_velocity);

        // return;
    }
}

void CCat::UpdateCL() { inherited::UpdateCL(); }
void CCat::HitEntityInJump(const CEntity* pEntity)
{
    SAAParam& params = anim().AA_GetParams("jump_attack_2");
    HitEntity(pEntity, params.hit_power, params.impulse, params.impulse_dir);
}
