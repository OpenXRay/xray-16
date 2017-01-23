#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterFindEnemyAngryAbstract CStateMonsterFindEnemyAngry<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterFindEnemyAngryAbstract::CStateMonsterFindEnemyAngry(_Object *obj) : inherited(obj)
{
}

TEMPLATE_SPECIALIZATION
CStateMonsterFindEnemyAngryAbstract::~CStateMonsterFindEnemyAngry()
{
}

TEMPLATE_SPECIALIZATION
void CStateMonsterFindEnemyAngryAbstract::execute()
{
	object->set_action					(ACT_STAND_IDLE);
	object->anim().SetSpecParams		(ASP_THREATEN);
	object->set_state_sound				(MonsterSound::eMonsterSoundAggressive);
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterFindEnemyAngryAbstract::check_completion()
{	
	if (time_state_started + 4000 > Device.dwTimeGlobal) return false;
	return true;
}

