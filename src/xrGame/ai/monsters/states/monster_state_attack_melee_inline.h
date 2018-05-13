#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateMonsterAttackMeleeAbstract CStateMonsterAttackMelee<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterAttackMeleeAbstract::CStateMonsterAttackMelee(_Object* obj) : inherited(obj) {}
TEMPLATE_SPECIALIZATION
CStateMonsterAttackMeleeAbstract::~CStateMonsterAttackMelee() {}
TEMPLATE_SPECIALIZATION
void CStateMonsterAttackMeleeAbstract::execute()
{
    this->object->set_action(ACT_ATTACK);
    if (this->object->control().direction().is_face_target(this->object->EnemyMan.get_enemy(), PI_DIV_3))
        this->object->dir().face_target(this->object->EnemyMan.get_enemy(), 800);
    else
        this->object->dir().face_target(this->object->EnemyMan.get_enemy(), 0, deg(15));

    this->object->set_state_sound(MonsterSound::eMonsterSoundAggressive);
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackMeleeAbstract::check_start_conditions()
{
    return (this->object->MeleeChecker.can_start_melee(this->object->EnemyMan.get_enemy()) && this->object->EnemyMan.see_enemy_now());
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackMeleeAbstract::check_completion()
{
    return (this->object->MeleeChecker.should_stop_melee(this->object->EnemyMan.get_enemy()));
}
