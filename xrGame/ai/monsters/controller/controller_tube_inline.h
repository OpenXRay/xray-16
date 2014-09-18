#pragma once

#include "controller_psy_hit.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateControllerTubeAbstract CStateControllerTube<_Object>

TEMPLATE_SPECIALIZATION
void CStateControllerTubeAbstract::execute()
{
	object->control().activate	(ControlCom::eComCustom1);
	object->set_action			(ACT_STAND_IDLE);
}

#define SEE_ENEMY_DURATION 1000

TEMPLATE_SPECIALIZATION
bool CStateControllerTubeAbstract::check_start_conditions()
{
	if (object->EnemyMan.see_enemy_duration() < SEE_ENEMY_DURATION) return false;
	if (!object->m_psy_hit->check_start_conditions()) return false;

	return true;
}

TEMPLATE_SPECIALIZATION
bool CStateControllerTubeAbstract::check_completion()
{
	return (!object->m_psy_hit->is_active());
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateControllerTubeAbstract
