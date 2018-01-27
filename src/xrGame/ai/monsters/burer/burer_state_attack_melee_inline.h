#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateBurerAttackMeleeAbstract CStateBurerAttackMelee<_Object>

#define MIN_DIST_MELEE_ATTACK 5.f
#define MAX_DIST_MELEE_ATTACK 9.f

TEMPLATE_SPECIALIZATION
CStateBurerAttackMeleeAbstract::CStateBurerAttackMelee(_Object* obj) : inherited(obj) {}
TEMPLATE_SPECIALIZATION
bool CStateBurerAttackMeleeAbstract::check_start_conditions()
{
    const float dist = this->object->Position().distance_to(this->object->EnemyMan.get_enemy()->Position());
    if (dist > MIN_DIST_MELEE_ATTACK)
        return false;

    return true;
}

TEMPLATE_SPECIALIZATION
bool CStateBurerAttackMeleeAbstract::check_completion()
{
    const float dist = this->object->Position().distance_to(this->object->EnemyMan.get_enemy()->Position());
    if (dist < MAX_DIST_MELEE_ATTACK)
        return false;

    return true;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateBurerAttackMeleeAbstract
