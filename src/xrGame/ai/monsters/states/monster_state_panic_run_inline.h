#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object>

#define CStateMonsterPanicRunAbstract CStateMonsterPanicRun<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonsterPanicRunAbstract::initialize()
{
    inherited::initialize();

    this->object->path().prepare_builder();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterPanicRunAbstract::execute()
{
    this->object->set_action(ACT_RUN);
    this->object->set_state_sound(MonsterSound::eMonsterSoundPanic);
    this->object->anim().accel_activate(eAT_Aggressive);
    this->object->anim().accel_set_braking(false);
    this->object->path().set_retreat_from_point(this->object->EnemyMan.get_enemy_position());
    this->object->path().set_generic_parameters();
}
TEMPLATE_SPECIALIZATION
bool CStateMonsterPanicRunAbstract::check_completion()
{
    const float dist_to_enemy = this->object->Position().distance_to(this->object->EnemyMan.get_enemy_position());
    const u32 time_delta = Device.dwTimeGlobal - this->object->EnemyMan.get_enemy_time_last_seen();

    if (dist_to_enemy < MIN_DIST_TO_ENEMY)
        return false;
    if (time_delta < MIN_UNSEEN_TIME)
        return false;

    return true;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterPanicRunAbstract
