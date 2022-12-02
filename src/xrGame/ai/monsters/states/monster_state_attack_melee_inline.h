#pragma once

template <typename _Object>
CStateMonsterAttackMelee<_Object>::CStateMonsterAttackMelee(_Object* obj) : inherited(obj) {}

template <typename _Object>
CStateMonsterAttackMelee<_Object>::~CStateMonsterAttackMelee() {}

template <typename _Object>
void CStateMonsterAttackMelee<_Object>::execute()
{
    this->object->set_action(ACT_ATTACK);
    if (this->object->control().direction().is_face_target(this->object->EnemyMan.get_enemy(), PI_DIV_3))
        this->object->dir().face_target(this->object->EnemyMan.get_enemy(), 800);
    else
        this->object->dir().face_target(this->object->EnemyMan.get_enemy(), 0, deg(15));

    this->object->set_state_sound(MonsterSound::eMonsterSoundAggressive);
}

template <typename _Object>
bool CStateMonsterAttackMelee<_Object>::check_start_conditions()
{
    return (this->object->MeleeChecker.can_start_melee(this->object->EnemyMan.get_enemy()) && this->object->EnemyMan.see_enemy_now());
}

template <typename _Object>
bool CStateMonsterAttackMelee<_Object>::check_completion()
{
    return (this->object->MeleeChecker.should_stop_melee(this->object->EnemyMan.get_enemy()));
}
