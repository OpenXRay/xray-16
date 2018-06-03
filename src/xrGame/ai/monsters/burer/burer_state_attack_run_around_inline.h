#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateBurerAttackRunAroundAbstract CStateBurerAttackRunAround<_Object>

#define DIST_QUANT 10.f

TEMPLATE_SPECIALIZATION
CStateBurerAttackRunAroundAbstract::CStateBurerAttackRunAround(_Object* obj) : inherited(obj), time_started(0) {}

TEMPLATE_SPECIALIZATION
void CStateBurerAttackRunAroundAbstract::initialize()
{
    inherited::initialize();

    time_started = Device.dwTimeGlobal;
    dest_direction.set(0.f, 0.f, 0.f);

    // select point
    Fvector dir_to_enemy, dir_from_enemy;
    dir_to_enemy.sub(this->object->EnemyMan.get_enemy()->Position(), this->object->Position());
    dir_to_enemy.normalize();

    dir_from_enemy.sub(this->object->Position(), this->object->EnemyMan.get_enemy()->Position());
    dir_from_enemy.normalize();

    const float dist = this->object->Position().distance_to(this->object->EnemyMan.get_enemy()->Position());

    if (dist > 30.f)
    { // бежать к врагу
        selected_point.mad(this->object->Position(), dir_to_enemy, DIST_QUANT);
    }
    else if ((dist < 20.f) && (dist > 4.f))
    { // убегать от врага
        selected_point.mad(this->object->Position(), dir_from_enemy, DIST_QUANT);
        dest_direction.sub(this->object->EnemyMan.get_enemy()->Position(), selected_point);
        dest_direction.normalize();
    }
    else
    { // выбрать случайную позицию
        selected_point = random_position(this->object->Position(), DIST_QUANT);
        dest_direction.sub(this->object->EnemyMan.get_enemy()->Position(), selected_point);
        dest_direction.normalize();
    }

    this->object->path().prepare_builder();
}

TEMPLATE_SPECIALIZATION
void CStateBurerAttackRunAroundAbstract::execute()
{
    if (!fis_zero(dest_direction.square_magnitude()))
    {
        this->object->path().set_use_dest_orient(true);
        this->object->path().set_dest_direction(dest_direction);
    }
    else
        this->object->path().set_use_dest_orient(false);

    this->object->set_action(ACT_RUN);
    this->object->path().set_target_point(selected_point);
    this->object->path().set_generic_parameters();
    this->object->path().set_use_covers(false);

    this->object->set_state_sound(MonsterSound::eMonsterSoundAggressive);
}

TEMPLATE_SPECIALIZATION
bool CStateBurerAttackRunAroundAbstract::check_start_conditions() { return true; }
TEMPLATE_SPECIALIZATION
bool CStateBurerAttackRunAroundAbstract::check_completion()
{
    if ((time_started + this->object->m_max_runaway_time < Device.dwTimeGlobal) ||
        (this->object->control().path_builder().is_moving_on_path() && this->object->control().path_builder().is_path_end(2.f)))
    {
        this->object->dir().face_target(this->object->EnemyMan.get_enemy());
        return true;
    }

    return false;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateBurerAttackRunAroundAbstract
