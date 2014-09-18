#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>
#define CStateControlFireAbstract CStateControlFire<_Object>

#define MIN_ENEMY_DISTANCE	10.f
#define STATE_MAX_TIME		3000
#define STATE_EXECUTE_DELAY	5000

TEMPLATE_SPECIALIZATION
void CStateControlFireAbstract::reinit()
{
	inherited::reinit();
	
	m_time_state_last_execute = 0;

}

TEMPLATE_SPECIALIZATION
void CStateControlFireAbstract::initialize()
{
	inherited::initialize			();
	object->set_psy_fire_delay_zero	();
	m_time_started					= time();
}

TEMPLATE_SPECIALIZATION
void CStateControlFireAbstract::execute()
{
	object->dir().face_target				(object->EnemyMan.get_enemy());
	object->custom_dir().head_look_point	(get_head_position(const_cast<CEntityAlive *>(object->EnemyMan.get_enemy())));
	
	object->custom_anim().set_body_state	(CControllerAnimation::eTorsoIdle,CControllerAnimation::eLegsTypeSteal);
}

TEMPLATE_SPECIALIZATION
void CStateControlFireAbstract::finalize()
{
	inherited::finalize();
	object->set_psy_fire_delay_default	();
	m_time_state_last_execute			= time();
}
TEMPLATE_SPECIALIZATION
void CStateControlFireAbstract::critical_finalize()
{
	inherited::critical_finalize();
	object->set_psy_fire_delay_default	();
	m_time_state_last_execute			= time();
}


TEMPLATE_SPECIALIZATION
bool CStateControlFireAbstract::check_start_conditions()
{
	if (!object->EnemyMan.see_enemy_now()) return false;
	if (object->EnemyMan.get_enemy()->Position().distance_to(object->Position()) < MIN_ENEMY_DISTANCE) return false;
	if (m_time_state_last_execute + STATE_EXECUTE_DELAY > time()) return false;

	return true;
}

TEMPLATE_SPECIALIZATION
bool CStateControlFireAbstract::check_completion()
{
	if (!object->EnemyMan.see_enemy_now()) return true;
	if (object->HitMemory.is_hit()) return true;
	if (object->EnemyMan.get_enemy()->Position().distance_to(object->Position()) < MIN_ENEMY_DISTANCE) return true;
	if (m_time_started + STATE_MAX_TIME < time()) return true;

	return false;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateControlFireAbstract
