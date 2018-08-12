#include "StdAfx.h"
#include "chimera.h"
#include "chimera_state_manager.h"
#include "detail_path_manager.h"
#include "ai/monsters/monster_velocity_space.h"
#include "Level.h"
#include "sound_player.h"
#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_movement_base.h"
#include "ai/monsters/control_path_builder_base.h"

CChimera::CChimera()
{
    StateMan = new CStateManagerChimera(this);
    com_man().add_ability(ControlCom::eControlJump);
}

CChimera::~CChimera() { xr_delete(StateMan); }
void CChimera::Load(LPCSTR section)
{
    inherited::Load(section);

    anim().accel_load(section);
    anim().accel_chain_add(eAnimWalkFwd, eAnimRun);
    anim().accel_chain_add(eAnimWalkFwd, eAnimRunTurnLeft);
    anim().accel_chain_add(eAnimWalkFwd, eAnimRunTurnRight);
    anim().accel_chain_add(eAnimWalkDamaged, eAnimRunDamaged);

    anim().AddReplacedAnim(&m_bDamaged, eAnimRun, eAnimRunDamaged);
    anim().AddReplacedAnim(&m_bDamaged, eAnimWalkFwd, eAnimWalkDamaged);
    anim().AddReplacedAnim(&m_bRunTurnLeft, eAnimRun, eAnimRunTurnLeft);
    anim().AddReplacedAnim(&m_bRunTurnRight, eAnimRun, eAnimRunTurnRight);

    SVelocityParam& velocity_none = move().get_velocity(MonsterMovement::eVelocityParameterIdle);
    SVelocityParam& velocity_turn = move().get_velocity(MonsterMovement::eVelocityParameterStand);
    SVelocityParam& velocity_walk = move().get_velocity(MonsterMovement::eVelocityParameterWalkNormal);
    SVelocityParam& velocity_run = move().get_velocity(MonsterMovement::eVelocityParameterRunNormal);
    SVelocityParam& velocity_walk_dmg = move().get_velocity(MonsterMovement::eVelocityParameterWalkDamaged);
    SVelocityParam& velocity_run_dmg = move().get_velocity(MonsterMovement::eVelocityParameterRunDamaged);
    SVelocityParam& velocity_steal = move().get_velocity(MonsterMovement::eVelocityParameterSteal);

    m_velocity_rotate.Load(section, "Velocity_Rotate");
    m_velocity_jump_start.Load(section, "Velocity_JumpStart");

    anim().AddAnim(eAnimStandIdle, "stand_idle_", -1, &velocity_none, PS_STAND);

    //@

    IKinematicsAnimated* KA = smart_cast<IKinematicsAnimated*>(Visual());
    MotionID idle_motion_id1 = KA->LL_MotionID("stand_idle_0");
    MotionID idle_motion_id2 = KA->LL_MotionID("stand_idle_1");

    anim().AddAnim(eAnimLieIdle, "stand_idle_", -1, &velocity_none, PS_LIE);
    anim().AddAnim(eAnimSleep, "stand_idle_", -1, &velocity_none, PS_LIE);

    anim().AddAnim(eAnimWalkFwd, "stand_walk_", -1, &velocity_walk, PS_STAND);
    anim().AddAnim(eAnimStandTurnLeft, "stand_turn_ls_", -1, &velocity_turn, PS_STAND);
    anim().AddAnim(eAnimStandTurnRight, "stand_turn_rs_", -1, &velocity_turn, PS_STAND);

    anim().AddAnim(eAnimFastStandTurnLeft, "stand_run_turn_90_ls_", -1, &m_velocity_rotate, PS_STAND);
    anim().AddAnim(eAnimFastStandTurnRight, "stand_run_turn_90_rs_", -1, &m_velocity_rotate, PS_STAND);

    anim().AddAnim(eAnimWalkDamaged, "stand_walk_dmg_", -1, &velocity_walk_dmg, PS_STAND);
    anim().AddAnim(eAnimRun, "stand_run_fwd_", -1, &velocity_run, PS_STAND);
    anim().AddAnim(eAnimRunDamaged, "stand_run_dmg_", -1, &velocity_run_dmg, PS_STAND);
    anim().AddAnim(eAnimCheckCorpse, "stand_check_corpse_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimEat, "stand_eat_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimAttack, "stand_idle_", -1, &velocity_turn, PS_STAND);

    anim().AddAnim(eAnimLookAround, "stand_idle_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimSteal, "stand_walk_", -1, &velocity_steal, PS_STAND);
    anim().AddAnim(eAnimPrepareAttack, "stand_agressive_idle_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimSteal, "stand_walk_", -1, &velocity_steal, PS_STAND);
    anim().AddAnim(eAnimDie, "stand_idle_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimThreaten, "stand_idle_", -1, &velocity_none, PS_STAND);

    anim().AddAnim(eAnimRunTurnLeft, "stand_run_turn_ls_", -1, &velocity_run, PS_STAND);
    anim().AddAnim(eAnimRunTurnRight, "stand_run_turn_rs_", -1, &velocity_run, PS_STAND);

    anim().AddAnim(eAnimUpperAttack, "jump_attack_", -1, &m_velocity_jump_start, PS_STAND);

    // link action
    anim().LinkAction(ACT_STAND_IDLE, eAnimStandIdle);
    anim().LinkAction(ACT_SIT_IDLE, eAnimLieIdle);
    anim().LinkAction(ACT_LIE_IDLE, eAnimLieIdle);
    anim().LinkAction(ACT_WALK_FWD, eAnimWalkFwd);
    // anim().LinkAction						(ACT_WALK_BKWD,		eAnimDragCorpse);
    anim().LinkAction(ACT_RUN, eAnimRun);
    anim().LinkAction(ACT_EAT, eAnimEat);
    anim().LinkAction(ACT_SLEEP, eAnimSleep);
    anim().LinkAction(ACT_REST, eAnimLieIdle);
    // anim().LinkAction						(ACT_DRAG,			eAnimDragCorpse);
    anim().LinkAction(ACT_ATTACK, eAnimAttack);
    anim().LinkAction(ACT_STEAL, eAnimSteal);
    anim().LinkAction(ACT_LOOK_AROUND, eAnimLookAround);

    m_attack_params.attack_radius = READ_IF_EXISTS(pSettings, r_float, section, "attack_radius", 10.f);
    m_attack_params.prepare_jump_timeout = READ_IF_EXISTS(pSettings, r_u32, section, "prepare_jump_timeout", 2000);
    m_attack_params.attack_jump_timeout = READ_IF_EXISTS(pSettings, r_u32, section, "attack_jump_timeout", 1000);
    m_attack_params.stealth_timeout = READ_IF_EXISTS(pSettings, r_u32, section, "stealth_timeout", 2000);
    m_attack_params.force_attack_distance = READ_IF_EXISTS(pSettings, r_float, section, "force_attack_distance", 8);
    m_attack_params.num_attack_jumps = READ_IF_EXISTS(pSettings, r_u32, section, "num_attack_jumps", 4);
    m_attack_params.num_prepare_jumps = READ_IF_EXISTS(pSettings, r_u32, section, "num_prepare_jumps", 2);
#ifdef DEBUG
    anim().accel_chain_test();
#endif

    PostLoad(section);
}

EAction CChimera::CustomVelocityIndex2Action(u32 velocity_index)
{
    switch (velocity_index)
    {
    case MonsterMovement::eChimeraVelocityParameterJumpGround: return ACT_RUN;
    case MonsterMovement::eChimeraVelocityParameterPrepare: return ACT_RUN;
    }

    return ACT_STAND_IDLE;
}

void CChimera::reinit()
{
    inherited::reinit();

    move().load_velocity(*cNameSect(), "Velocity_JumpGround", MonsterMovement::eChimeraVelocityParameterJumpGround);

    com_man().load_jump_data(0, //"jump_attack_0",
        0, //"jump_attack_0",
        "jump_attack_1", "jump_attack_2",
        u32(-1), // MonsterMovement::eVelocityParameterRunNormal,
        MonsterMovement::eChimeraVelocityParameterJumpGround, 0);
}

void CChimera::CheckSpecParams(u32 spec_params)
{
    // 	if ( (spec_params & ASP_THREATEN) == ASP_THREATEN )
    // 	{
    // 		anim().SetCurAnim(eAnimThreaten);
    // 	}
    // 	if ( (spec_params & ASP_ATTACK_RUN) == ASP_ATTACK_RUN )
    // 	{
    // 		anim().SetCurAnim(eAnimAttackRun);
    // 	}
}

void CChimera::HitEntityInJump(const CEntity* pEntity)
{
    SAAParam& params = anim().AA_GetParams("jump_attack_1");

    HitEntity(pEntity, params.hit_power, params.impulse, params.impulse_dir);
}

void CChimera::jump(Fvector const& position, float const factor)
{
    com_man().script_jump(position, factor);
    sound().play(MonsterSound::eMonsterSoundAggressive);
}

void CChimera::UpdateCL() { inherited::UpdateCL(); }
