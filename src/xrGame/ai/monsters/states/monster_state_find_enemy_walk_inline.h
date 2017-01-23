#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterFindEnemyWalkAbstract CStateMonsterFindEnemyWalkAround<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonsterFindEnemyWalkAbstract::execute()
{
	object->set_action			(ACT_STAND_IDLE);
	object->set_state_sound		(MonsterSound::eMonsterSoundAggressive);
}

