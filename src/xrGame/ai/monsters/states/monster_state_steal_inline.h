#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object>

#define CStateMonsterStealAbstract CStateMonsterSteal<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterStealAbstract::CStateMonsterSteal(_Object* obj) : inherited(obj) {}
TEMPLATE_SPECIALIZATION
void CStateMonsterStealAbstract::initialize()
{
    inherited::initialize();
    this->object->path().prepare_builder();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterStealAbstract::execute()
{
    this->object->set_action(ACT_STEAL);
    this->object->anim().accel_activate(eAT_Calm);
    this->object->anim().accel_set_braking(false);
    this->object->path().set_target_point(this->object->EnemyMan.get_enemy_position(), this->object->EnemyMan.get_enemy_vertex());
    this->object->path().set_generic_parameters();
    this->object->set_state_sound(MonsterSound::eMonsterSoundSteal);
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterStealAbstract::check_completion() { return (!check_conditions()); }
TEMPLATE_SPECIALIZATION
bool CStateMonsterStealAbstract::check_start_conditions() { return (check_conditions()); }
TEMPLATE_SPECIALIZATION
bool CStateMonsterStealAbstract::check_conditions()
{
    // if i see enemy
    if (!this->object->EnemyMan.see_enemy_now())
        return false;

    // This is the only enemy
    if (this->object->EnemyMan.get_enemies_count() > 1)
        return false;

    // There is extended info about enemy?
    if (!this->object->EnemyMan.get_flags().is(FLAG_ENEMY_STATS_NOT_READY))
    {
        // Enemy is not moving fast
        if (this->object->EnemyMan.get_flags().is(FLAG_ENEMY_GO_FARTHER_FAST))
            return false;

        // Enemy doesn't know about me
        if (!this->object->EnemyMan.get_flags().is(FLAG_ENEMY_DOESNT_KNOW_ABOUT_ME))
            return false;
    }

    // Don't hear dangerous sounds
    if (this->object->hear_dangerous_sound)
        return false;

    // Don't get hitted
    if (this->object->HitMemory.is_hit())
        return false;

    // Path with minimal deviation
    // if (object->control().path_builder().detail().time_path_built() >= time_state_started) {
    //	if (object->path().get_path_angle() > STEAL_MAX_PATH_ANGLE)	return false;
    //}

    // check distance to enemy
    const float dist = this->object->MeleeChecker.distance_to_enemy(this->object->EnemyMan.get_enemy());
    if (dist < STEAL_MIN_DISTANCE)
        return false;
    else if (dist > STEAL_MAX_DISTANCE)
        return false;

    return true;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterStealAbstract
