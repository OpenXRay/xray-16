#pragma once
#include "ai/monsters/monster_velocity_space.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateMonsterAttackRunAttackAbstract CStateMonsterAttackRunAttack<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackRunAttackAbstract::initialize()
{
    inherited::initialize();

    this->object->m_time_last_attack_success = 0;
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackRunAttackAbstract::execute()
{
    this->object->set_action(ACT_RUN);
    this->object->set_state_sound(MonsterSound::eMonsterSoundAggressive);
    this->object->anim().SetSpecParams(ASP_ATTACK_RUN);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackRunAttackAbstract::finalize() { inherited::finalize(); }
TEMPLATE_SPECIALIZATION
void CStateMonsterAttackRunAttackAbstract::critical_finalize() { inherited::critical_finalize(); }
TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackRunAttackAbstract::check_start_conditions()
{
    const float dist = this->object->MeleeChecker.distance_to_enemy(this->object->EnemyMan.get_enemy());

    if (dist > this->object->db().m_run_attack_start_dist)
        return false;
    if (dist < this->object->MeleeChecker.get_min_distance())
        return false;

    // check angle
    if (!this->object->control().direction().is_face_target(this->object->EnemyMan.get_enemy(), deg(30)))
        return false;

    // try to build path
    Fvector target_position;
    target_position.mad(this->object->Position(), this->object->Direction(), this->object->db().m_run_attack_path_dist);

    // if (!object->control().path_builder().build_special(target_position, u32(-1),
    // MonsterMovement::eVelocityParamsRunAttack)) return false;
    // else object->path().enable_path();

    return true;
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackRunAttackAbstract::check_completion()
{
    if (!this->object->control().path_builder().is_moving_on_path() || (this->object->m_time_last_attack_success != 0))
        return true;
    return false;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterAttackRunAttackAbstract
