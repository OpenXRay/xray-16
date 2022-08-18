#pragma once

#include "sound_player.h"
#include "xrCore/_vector3d_ext.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStatePoltergeistAttackHiddenAbstract CStatePoltergeistAttackHidden<_Object>

TEMPLATE_SPECIALIZATION
CStatePoltergeistAttackHiddenAbstract::CStatePoltergeistAttackHidden(_Object* obj) : inherited(obj)
{
    this->add_state(eStateAttack_MoveToHomePoint, xr_new<CStateMonsterAttackMoveToHomePoint<_Object>>(obj));
}

TEMPLATE_SPECIALIZATION
void CStatePoltergeistAttackHiddenAbstract::initialize()
{
    inherited::initialize();
    this->object->path().prepare_builder();
    m_fly_side_select_tick = 0;
    m_fly_radius_factor = 1.f;
}

TEMPLATE_SPECIALIZATION
void CStatePoltergeistAttackHiddenAbstract::select_target_for_move()
{
    CEntityAlive const* const enemy = this->object->EnemyMan.get_enemy();
    Fvector const enemy_pos = enemy->Position();
    Fvector const self_pos = this->object->Position();

    Fvector const self2enemy = enemy_pos - self_pos;

    float const fly_radius = this->object->get_fly_around_distance() * m_fly_radius_factor;

    Fvector const enemy_dir = normalize(enemy->Direction());
    Fvector const front_point = enemy_pos + (enemy_dir * fly_radius);

    if (xr_current_time() > m_fly_side_select_tick)
    {
        Fvector const self2front = front_point - self_pos;
        bool left_side = self2enemy.x * self2front.z - self2enemy.z * self2front.x > 0.f;
        if (!(rand() % 2))
        {
            left_side ^= true;
        }

        m_fly_left = left_side;
        m_fly_side_select_tick = xr_current_time() + (u32)(1000 * this->object->get_fly_around_change_direction_time());
    }

    Fvector const enemy2self = -fly_radius * normalize(self2enemy);

    float const move_scan_points = 12;
    float const move_scan_angle = deg2rad(360.f) / move_scan_points;

    for (u32 index = 0; index < move_scan_points; ++index)
    {
        float const angle = move_scan_angle * (index + 1) * (m_fly_left ? -1.f : +1.f);

        Fvector const scan_point = enemy_pos + rotate_point(enemy2self, angle);

        if (ai().level_graph().valid_vertex_position(scan_point))
        {
            m_target = scan_point;
            m_target_vertex = ai().level_graph().vertex_id(m_target);

            m_fly_radius_factor += 0.1f;
            if (m_fly_radius_factor > 1.f)
            {
                m_fly_radius_factor = 1.f;
            }

            return;
        }
    }

    m_fly_radius_factor -= 0.1f;
    if (m_fly_radius_factor < 0.1f)
    {
        m_fly_radius_factor = 0.1f;
    }

    m_target = self_pos;
    m_target_vertex = this->object->ai_location().level_vertex_id();
}

TEMPLATE_SPECIALIZATION
bool CStatePoltergeistAttackHiddenAbstract::check_home_point()
{
    if (this->prev_substate != eStateAttack_MoveToHomePoint)
    {
        if (this->get_state(eStateAttack_MoveToHomePoint)->check_start_conditions())
            return true;
    }
    else
    {
        if (!this->get_state(eStateAttack_MoveToHomePoint)->check_completion())
            return true;
    }

    return false;
}

TEMPLATE_SPECIALIZATION
void CStatePoltergeistAttackHiddenAbstract::execute()
{
    if (check_home_point())
    {
        this->select_state(eStateAttack_MoveToHomePoint);
        this->get_state_current()->execute();
        this->prev_substate = this->current_substate;
        return;
    }
    else
    {
        this->current_substate = (u32)eStateUnknown;
        this->prev_substate = this->current_substate;
    }

    select_target_for_move();

    this->object->path().set_target_point(m_target, m_target_vertex);
    this->object->path().set_rebuild_time(200);
    this->object->path().set_distance_to_end(3.f);
    this->object->path().set_use_covers(false);

    this->object->anim().m_tAction = ACT_RUN;
    this->object->anim().accel_activate(eAT_Aggressive);
    this->object->anim().accel_set_braking(false);
    this->object->sound().play(MonsterSound::eMonsterSoundAggressive, 0, 0, this->object->db().m_dwAttackSndDelay);
}

#undef TEMPLATE_SPECIALIZATION
#undef CStatePoltergeistAttackHiddenAbstract
