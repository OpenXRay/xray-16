#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object>

#define CStateGroupPanicRunAbstract CStateGroupPanicRun<_Object>

TEMPLATE_SPECIALIZATION
void CStateGroupPanicRunAbstract::initialize()
{
    inherited::initialize();

    this->object->path().prepare_builder();
}

TEMPLATE_SPECIALIZATION
void CStateGroupPanicRunAbstract::execute()
{
    this->object->set_action(ACT_RUN);
    this->object->set_state_sound(MonsterSound::eMonsterSoundPanic);
    this->object->anim().accel_activate(eAT_Aggressive);
    this->object->anim().accel_set_braking(false);

    Fvector enemy2home = this->object->Home->get_home_point();
    enemy2home.sub(this->object->EnemyMan.get_enemy_position());
    enemy2home.normalize_safe();

    this->object->path().set_target_point(this->object->Home->get_place_in_max_home_to_direction(enemy2home));

    this->object->path().set_generic_parameters();
}
TEMPLATE_SPECIALIZATION
bool CStateGroupPanicRunAbstract::check_completion()
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
#undef CStateGroupPanicRunAbstract
