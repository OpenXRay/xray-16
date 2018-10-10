#pragma once

#include "ai/monsters/states/state_move_to_point.h"
#include "ai/monsters/states/state_look_point.h"
#include "cover_point.h"
#include "ai/monsters/monster_cover_manager.h"
#include "ai/monsters/states/state_custom_action.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateGroupRestIdleAbstract CStateGroupRestIdle<_Object>

TEMPLATE_SPECIALIZATION
CStateGroupRestIdleAbstract::CStateGroupRestIdle(_Object* obj) : inherited(obj)
{
    this->add_state(eStateRest_WalkToCover, new CStateMonsterMoveToPointEx<_Object>(obj));
    this->add_state(eStateRest_LookOpenPlace, new CStateMonsterLookToPoint<_Object>(obj));
    this->add_state(eStateRest_WalkGraphPoint, new CStateMonsterMoveToPointEx<_Object>(obj));
    this->add_state(eStateCustom, new CStateMonsterCustomAction<_Object>(obj));
}

TEMPLATE_SPECIALIZATION
void CStateGroupRestIdleAbstract::initialize()
{
    inherited::initialize();

    m_target_node = u32(-1);

    if (this->object->b_end_state_eat)
    {
        m_target_node = this->object->Home->get_place_in_min_home();

        if (m_target_node == u32(-1))
        {
            const CCoverPoint* point =
                this->object->CoverMan->find_cover(this->object->Home->get_home_point(), 1, this->object->Home->get_min_radius());
            if (!point)
                return;
            m_target_node = point->level_vertex_id();
        }
    }
    else
    {
        m_target_node = this->object->Home->get_place_in_mid_home();

        if (m_target_node == u32(-1))
        {
            const CCoverPoint* point =
                this->object->CoverMan->find_cover(this->object->Home->get_home_point(), 1, this->object->Home->get_mid_radius());
            if (!point)
                return;
            m_target_node = point->level_vertex_id();
        }
    }

    m_move_type = 0;

    CMonsterSquad* squad = monster_squad().get_squad(this->object);
    squad->lock_cover(m_target_node);
}

TEMPLATE_SPECIALIZATION
void CStateGroupRestIdleAbstract::finalize()
{
    inherited::finalize();
    CMonsterSquad* squad = monster_squad().get_squad(this->object);
    squad->unlock_cover(m_target_node);
}

TEMPLATE_SPECIALIZATION
void CStateGroupRestIdleAbstract::critical_finalize()
{
    inherited::critical_finalize();

    CMonsterSquad* squad = monster_squad().get_squad(this->object);
    squad->unlock_cover(m_target_node);
}

TEMPLATE_SPECIALIZATION
void CStateGroupRestIdleAbstract::reselect_state()
{
    if (this->object->saved_state == eStateRest_LookOpenPlace)
    {
        this->object->saved_state = u32(-1);
        this->select_state(eStateRest_WalkGraphPoint);
        return;
    }

    if ((this->prev_substate == u32(-1) && m_target_node != u32(-1)) || (this->prev_substate == eStateRest_WalkGraphPoint))
    {
        this->select_state(eStateRest_WalkToCover);
        return;
    }

    if ((this->prev_substate == eStateRest_WalkToCover) || (this->prev_substate == u32(-1)))
    {
        this->object->saved_state = eStateRest_LookOpenPlace;
        if (this->object->b_end_state_eat)
        {
            this->object->set_current_animation(8);
            this->object->b_end_state_eat = false;
        }
        else
        {
            this->object->set_current_animation(this->object->random_anim());
        }
        this->select_state(eStateCustom);
        return;
    }

    this->select_state(eStateRest_WalkGraphPoint);
}

TEMPLATE_SPECIALIZATION
void CStateGroupRestIdleAbstract::setup_substates()
{
    state_ptr state = this->get_state_current();

    if (this->current_substate == eStateRest_WalkGraphPoint)
    {
        SStateDataMoveToPointEx data;
        data.vertex = this->object->Home->get_place_in_mid_home();
        if (data.vertex == u32(-1))
        {
            data.vertex = this->object->ai_location().level_vertex_id();
        }
        data.point = ai().level_graph().vertex_position(data.vertex);
        if (this->object->Position().distance_to(data.point) > 8.f)
        {
            m_move_type = 1;
            this->object->m_start_smelling = u32(-1);
        }
        else
        {
            if (this->object->m_start_smelling == u32(-1) || this->object->m_start_smelling > u32(4) + this->object->m_smelling_count)
            {
                m_move_type = (Random.randI(2));
                this->object->m_start_smelling = m_move_type ? u32(1) : u32(-1);
                this->object->m_smelling_count = Random.randI(3);
            }
            else
            {
                m_move_type = 0;
                this->object->m_start_smelling = this->object->m_start_smelling + u32(1);
            }
        }
        data.action.action = m_move_type ? ACT_WALK_FWD : ACT_HOME_WALK_SMELLING;
        data.action.time_out = 0; // do not use time out
        data.completion_dist = 0.f; // get exactly to the point
        data.time_to_rebuild = 0; // do not rebuild
        data.accelerated = true;
        data.braking = true;
        data.accel_type = eAT_Calm;
        data.action.sound_type = MonsterSound::eMonsterSoundIdle;
        data.action.sound_delay = this->object->db().m_dwIdleSndDelay;
        state->fill_data_with(&data, sizeof(SStateDataMoveToPointEx));
        return;
    }

    if (this->current_substate == eStateRest_WalkToCover)
    {
        SStateDataMoveToPointEx data;
        data.vertex = m_target_node;
        if (data.vertex == u32(-1))
        {
            data.vertex = this->object->ai_location().level_vertex_id();
        }
        data.point = ai().level_graph().vertex_position(data.vertex);
        if (this->object->Position().distance_to(data.point) > 8.f)
        {
            m_move_type = 1;
            this->object->m_start_smelling = u32(-1);
        }
        else
        {
            if (this->object->m_start_smelling == u32(-1) || this->object->m_start_smelling > u32(4) + this->object->m_smelling_count)
            {
                m_move_type = (Random.randI(2));
                this->object->m_start_smelling = m_move_type ? u32(1) : u32(-1);
                this->object->m_smelling_count = Random.randI(3);
            }
            else
            {
                m_move_type = 0;
                this->object->m_start_smelling = this->object->m_start_smelling + u32(1);
            }
        }
        data.action.action = m_move_type ? ACT_WALK_FWD : ACT_HOME_WALK_SMELLING;
        data.action.time_out = 0; // do not use time out
        data.completion_dist = 0.f; // get exactly to the point
        data.time_to_rebuild = 0; // do not rebuild
        data.accelerated = true;
        data.braking = true;
        data.accel_type = eAT_Calm;
        data.action.sound_type = MonsterSound::eMonsterSoundIdle;
        data.action.sound_delay = this->object->db().m_dwIdleSndDelay;
        state->fill_data_with(&data, sizeof(SStateDataMoveToPointEx));
        return;
    }

    if (this->current_substate == eStateRest_LookOpenPlace)
    {
        SStateDataLookToPoint data;

        Fvector dir;
        this->object->CoverMan->less_cover_direction(dir);

        data.point.mad(this->object->Position(), dir, 10.f);
        data.action.action = ACT_STAND_IDLE;
        data.action.time_out = 1000;
        data.action.sound_type = MonsterSound::eMonsterSoundIdle;
        data.action.sound_delay = this->object->db().m_dwIdleSndDelay;
        data.face_delay = 0;

        state->fill_data_with(&data, sizeof(SStateDataLookToPoint));
        return;
    }

    if (this->current_substate == eStateCustom)
    {
        SStateDataAction data;

        data.action = ACT_STAND_IDLE;
        data.time_out = 0; // do not use time out
        if (this->object->get_number_animation() == u32(6))
        {
            data.sound_type = MonsterSound::eMonsterSoundThreaten;
        }
        else
        {
            data.sound_type = MonsterSound::eMonsterSoundIdle;
        }
        data.sound_delay = this->object->db().m_dwIdleSndDelay;
        state->fill_data_with(&data, sizeof(SStateDataAction));

        return;
    }
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateGroupRestIdleAbstract
