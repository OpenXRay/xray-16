#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterPanicRunAbstract CStateMonsterPanicRun<_Object>

#define MIN_UNSEEN_TIME			15000
#define MIN_DIST_TO_ENEMY		15.f

TEMPLATE_SPECIALIZATION
void CStateMonsterPanicRunAbstract::initialize()
{
	inherited::initialize();
	
	object->path().prepare_builder		();	
}

TEMPLATE_SPECIALIZATION
void CStateMonsterPanicRunAbstract::execute()
{
	object->set_action							(ACT_RUN);
	object->set_state_sound						(MonsterSound::eMonsterSoundPanic);
	object->anim().accel_activate			(eAT_Aggressive);
	object->anim().accel_set_braking			(false);
	object->path().set_retreat_from_point	(object->EnemyMan.get_enemy_position());
	object->path().set_generic_parameters	();
}
TEMPLATE_SPECIALIZATION
bool CStateMonsterPanicRunAbstract::check_completion()
{
	float dist_to_enemy = object->Position().distance_to(object->EnemyMan.get_enemy_position());
	u32 time_delta	= Device.dwTimeGlobal - object->EnemyMan.get_enemy_time_last_seen();

	if (dist_to_enemy < MIN_DIST_TO_ENEMY)  return false;
	if (time_delta	  < MIN_UNSEEN_TIME)	return false;

	return true;
}

#undef DIST_TO_PATH_END
#undef MIN_DIST_TO_ENEMY
#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterPanicRunAbstract
