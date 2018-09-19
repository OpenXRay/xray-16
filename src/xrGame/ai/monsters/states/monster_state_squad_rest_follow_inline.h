#pragma once

#include "state_custom_action.h"
#include "state_move_to_point.h"
#include "../ai_monster_squad.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateMonsterSquadRestFollowAbstract CStateMonsterSquadRestFollow<_Object>

#define STOP_DISTANCE 2.f
#define STAY_DISTANCE 5 * STOP_DISTANCE
#define MIN_TIME_OUT 2000
#define MAX_TIME_OUT 3000

TEMPLATE_SPECIALIZATION
CStateMonsterSquadRestFollowAbstract::CStateMonsterSquadRestFollow(_Object* obj) : inherited(obj)
{
    this->add_state(eStateSquad_RestFollow_Idle, new CStateMonsterCustomAction<_Object>(obj));
    this->add_state(eStateSquad_RestFollow_WalkToPoint, new CStateMonsterMoveToPointEx<_Object>(obj));
}

TEMPLATE_SPECIALIZATION
CStateMonsterSquadRestFollowAbstract::~CStateMonsterSquadRestFollow() {}
TEMPLATE_SPECIALIZATION
void CStateMonsterSquadRestFollowAbstract::initialize()
{
    inherited::initialize();

    SSquadCommand& command = monster_squad().get_squad(this->object)->GetCommand(this->object);
    last_point = command.position;
}

TEMPLATE_SPECIALIZATION
void CStateMonsterSquadRestFollowAbstract::reselect_state()
{
    SSquadCommand& command = monster_squad().get_squad(this->object)->GetCommand(this->object);
    if (command.position.distance_to(this->object->Position()) < Random.randF(STOP_DISTANCE, STAY_DISTANCE))
    {
        this->select_state(eStateSquad_RestFollow_Idle);
    }
    else
    {
        this->select_state(eStateSquad_RestFollow_WalkToPoint);
    }
}

TEMPLATE_SPECIALIZATION
void CStateMonsterSquadRestFollowAbstract::check_force_state() {}
TEMPLATE_SPECIALIZATION
void CStateMonsterSquadRestFollowAbstract::setup_substates()
{
    state_ptr state = this->get_state_current();

    if (this->current_substate == eStateSquad_RestFollow_Idle)
    {
        SStateDataAction data;
        data.action = ACT_REST;
        data.sound_type = MonsterSound::eMonsterSoundIdle;
        data.sound_delay = this->object->db().m_dwIdleSndDelay;
        data.time_out = Random.randI(MIN_TIME_OUT, MAX_TIME_OUT);

        state->fill_data_with(&data, sizeof(SStateDataAction));

        return;
    }

    if (this->current_substate == eStateSquad_RestFollow_WalkToPoint)
    {
        SStateDataMoveToPointEx data;

        Fvector dest_pos = monster_squad().get_squad(this->object)->GetCommand(this->object).position;
        if (!this->object->control().path_builder().restrictions().accessible(dest_pos))
        {
            data.vertex = this->object->control().path_builder().restrictions().accessible_nearest(dest_pos, data.point);
        }
        else
        {
            data.point = dest_pos;
            data.vertex = u32(-1);
        }

        data.action.action = ACT_WALK_FWD;
        data.accelerated = true;
        data.braking = false;
        data.accel_type = eAT_Calm;
        data.completion_dist = STOP_DISTANCE;
        data.action.sound_type = MonsterSound::eMonsterSoundIdle;
        data.action.sound_delay = this->object->db().m_dwIdleSndDelay;
        data.time_to_rebuild = u32(-1);

        state->fill_data_with(&data, sizeof(SStateDataMoveToPointEx));

        return;
    }
}

#undef STOP_DISTANCE
#undef STAY_DISTANCE
#undef MIN_TIME_OUT
#undef MAX_TIME_OUT
#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterSquadRestFollowAbstract
