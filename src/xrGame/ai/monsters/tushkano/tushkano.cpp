#include "StdAfx.h"
#include "tushkano.h"
#include "tushkano_state_manager.h"
#include "ai/monsters/monster_velocity_space.h"
#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_movement_base.h"

CTushkano::CTushkano()
{
    StateMan = new CStateManagerTushkano(this);

    CControlled::init_external(this);
}

CTushkano::~CTushkano() { xr_delete(StateMan); }
void CTushkano::Load(LPCSTR section)
{
    inherited::Load(section);

    //	anim().AddReplacedAnim(&m_bDamaged, eAnimRun,		eAnimRunDamaged);
    //	anim().AddReplacedAnim(&m_bDamaged, eAnimWalkFwd,	eAnimWalkDamaged);

    anim().accel_load(section);
    anim().accel_chain_add(eAnimWalkFwd, eAnimRun);
    // anim().accel_chain_add		(eAnimWalkDamaged,	eAnimRunDamaged);

    SVelocityParam& velocity_none = move().get_velocity(MonsterMovement::eVelocityParameterIdle);
    SVelocityParam& velocity_turn = move().get_velocity(MonsterMovement::eVelocityParameterStand);
    SVelocityParam& velocity_walk = move().get_velocity(MonsterMovement::eVelocityParameterWalkNormal);
    SVelocityParam& velocity_run = move().get_velocity(MonsterMovement::eVelocityParameterRunNormal);
    // SVelocityParam &velocity_walk_dmg	= move().get_velocity(MonsterMovement::eVelocityParameterWalkDamaged);
    // SVelocityParam &velocity_run_dmg	= move().get_velocity(MonsterMovement::eVelocityParameterRunDamaged);
    // SVelocityParam &velocity_steal		= move().get_velocity(MonsterMovement::eVelocityParameterSteal);
    // SVelocityParam &velocity_drag		= move().get_velocity(MonsterMovement::eVelocityParameterDrag);

    anim().AddAnim(eAnimStandIdle, "stand_idle_", -1, &velocity_none, PS_STAND);
    // anim().AddAnim(eAnimStandDamaged,	"stand_idle_dmg_",		-1, &velocity_none,				PS_STAND);
    anim().AddAnim(eAnimStandTurnLeft, "stand_turn_left_", -1, &velocity_turn, PS_STAND);
    anim().AddAnim(eAnimStandTurnRight, "stand_turn_right_", -1, &velocity_turn, PS_STAND);
    anim().AddAnim(eAnimWalkFwd, "stand_walk_fwd_", -1, &velocity_walk, PS_STAND);
    // anim().AddAnim(eAnimWalkDamaged,		"stand_walk_fwd_dmg_",	-1, &velocity_walk_dmg,	PS_STAND);
    anim().AddAnim(eAnimRun, "stand_run_", -1, &velocity_run, PS_STAND);
    // anim().AddAnim(eAnimRunDamaged,		"stand_run_dmg_",		-1,	&velocity_run_dmg,	PS_STAND);
    anim().AddAnim(eAnimAttack, "stand_attack_", -1, &velocity_turn, PS_STAND);
    // anim().AddAnim(eAnimDie,				"stand_die_",			0,  &velocity_none,				PS_STAND);
    // anim().AddAnim(eAnimCheckCorpse,		"stand_check_corpse_",	-1,	&velocity_none,				PS_STAND);
    // anim().AddAnim(eAnimSteal,			"stand_crawl_",			-1, &velocity_steal,			PS_STAND);
    // anim().AddAnim(eAnimSitIdle,			"sit_idle_",			-1, &velocity_none,				PS_SIT);
    // anim().AddAnim(eAnimSitStandUp,		"sit_stand_up_",		-1, &velocity_none,				PS_SIT);
    // anim().AddAnim(eAnimStandSitDown,	"stand_sit_down_",		-1, &velocity_none,				PS_STAND);
    // anim().AddAnim(eAnimLookAround,		"stand_look_around_",	-1, &velocity_none,				PS_STAND);
    // anim().AddAnim(eAnimEat,				"sit_eat_",				-1, &velocity_none,				PS_SIT);

    // anim().AddTransition(PS_STAND,		PS_SIT,		eAnimStandSitDown,	false);
    // anim().AddTransition(PS_SIT,			PS_STAND,	eAnimSitStandUp,	false);

    anim().LinkAction(ACT_STAND_IDLE, eAnimStandIdle);
    anim().LinkAction(ACT_SIT_IDLE, eAnimStandIdle);
    anim().LinkAction(ACT_LIE_IDLE, eAnimStandIdle);
    anim().LinkAction(ACT_WALK_FWD, eAnimWalkFwd);
    anim().LinkAction(ACT_WALK_BKWD, eAnimWalkFwd);
    anim().LinkAction(ACT_RUN, eAnimRun);
    anim().LinkAction(ACT_EAT, eAnimStandIdle);
    anim().LinkAction(ACT_SLEEP, eAnimStandIdle);
    anim().LinkAction(ACT_REST, eAnimStandIdle);
    anim().LinkAction(ACT_DRAG, eAnimStandIdle);
    anim().LinkAction(ACT_ATTACK, eAnimAttack);
    anim().LinkAction(ACT_STEAL, eAnimStandIdle);
    anim().LinkAction(ACT_LOOK_AROUND, eAnimStandIdle);

#ifdef DEBUG
    anim().accel_chain_test();
#endif

    PostLoad(section);
}

void CTushkano::CheckSpecParams(u32 spec_params)
{
    // if ((spec_params & ASP_CHECK_CORPSE) == ASP_CHECK_CORPSE) {
    //	anim().Seq_Add(eAnimCheckCorpse);
    //	anim().Seq_Switch();
    //}

    // if ((spec_params & ASP_STAND_SCARED) == ASP_STAND_SCARED) {
    //	anim().SetCurAnim(eAnimLookAround);
    //	return;
    //}
}
