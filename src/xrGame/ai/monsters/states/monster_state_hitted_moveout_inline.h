#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object>

#define CStateMonsterHittedMoveOutAbstract CStateMonsterHittedMoveOut<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonsterHittedMoveOutAbstract::initialize()
{
    inherited::initialize();
    select_target();
    this->object->path().prepare_builder();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterHittedMoveOutAbstract::execute()
{
    // проверить на завершение пути
    if (this->object->control().path_builder().detail().time_path_built() > this->time_state_started)
    {
        if (this->object->control().path_builder().is_path_end(DIST_TO_PATH_END))
            select_target();
    }

    if (target.node != u32(-1))
        this->object->path().set_target_point(target.position, target.node);
    else
        this->object->path().set_target_point(this->object->HitMemory.get_last_hit_position());

    const float dist = this->object->HitMemory.get_last_hit_position().distance_to(this->object->Position());

    if (dist > 10.f)
        this->object->set_action(ACT_WALK_FWD);
    else
        this->object->set_action(ACT_STEAL);

    this->object->anim().accel_deactivate();
    this->object->anim().accel_set_braking(false);
    this->object->set_state_sound(MonsterSound::eMonsterSoundIdle);
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterHittedMoveOutAbstract::check_completion()
{
    if (this->object->HitMemory.get_last_hit_time() > this->time_state_started)
        return true;

    const float dist = this->object->HitMemory.get_last_hit_position().distance_to(this->object->Position());
    if (dist < DIST_TO_HIT_POINT)
        return true;

    return false;
}

TEMPLATE_SPECIALIZATION
void CStateMonsterHittedMoveOutAbstract::select_target()
{
    if (!this->object->GetCoverCloseToPoint(
        this->object->HitMemory.get_last_hit_position(), 10.f, 20.f, 0.f, 15.f, target.position, target.node))
    {
        target.node = u32(-1);
    }
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterHittedMoveOutAbstract
