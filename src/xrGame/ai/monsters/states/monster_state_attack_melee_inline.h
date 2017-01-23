#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterAttackMeleeAbstract CStateMonsterAttackMelee<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterAttackMeleeAbstract::CStateMonsterAttackMelee(_Object *obj) : inherited(obj)
{
}

TEMPLATE_SPECIALIZATION
CStateMonsterAttackMeleeAbstract::~CStateMonsterAttackMelee()
{
}


TEMPLATE_SPECIALIZATION
void CStateMonsterAttackMeleeAbstract::execute()
{
	object->set_action			(ACT_ATTACK);
	if (object->control().direction().is_face_target(object->EnemyMan.get_enemy(), PI_DIV_3))
		object->dir().face_target	(object->EnemyMan.get_enemy(), 800);
	else 
		object->dir().face_target	(object->EnemyMan.get_enemy(), 0, deg(15));

	object->set_state_sound		(MonsterSound::eMonsterSoundAggressive);
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackMeleeAbstract::check_start_conditions()
{
	return (
		object->MeleeChecker.can_start_melee(object->EnemyMan.get_enemy()) &&
		object->EnemyMan.see_enemy_now()
	);
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackMeleeAbstract::check_completion()
{
	return (object->MeleeChecker.should_stop_melee(object->EnemyMan.get_enemy()));
}