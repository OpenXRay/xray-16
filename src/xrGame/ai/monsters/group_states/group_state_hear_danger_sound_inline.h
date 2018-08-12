#pragma once
#include "ai/monsters/states/state_move_to_point.h"
#include "ai/monsters/states/monster_state_home_point_danger.h"
#include "ai/monsters/ai_monster_squad.h"
#include "Entity.h"
#include "xrAICore/Navigation/ai_object_location.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateGroupHearDangerousSoundAbstract CStateGroupHearDangerousSound<_Object>

#define LEADER_RADIUS 20.f
#define FIND_POINT_ATTEMPTS 5

TEMPLATE_SPECIALIZATION
CStateGroupHearDangerousSoundAbstract::CStateGroupHearDangerousSound(_Object* obj) : inherited(obj)
{
    this->add_state(eStateHearDangerousSound_Hide, new CStateMonsterMoveToPointEx<_Object>(obj));
    this->add_state(eStateSquad, new CStateMonsterMoveToPoint<_Object>(obj));
    this->add_state(eStateHearDangerousSound_Home, new CStateMonsterDangerMoveToHomePoint<_Object>(obj));
}

TEMPLATE_SPECIALIZATION
void CStateGroupHearDangerousSoundAbstract::initialize() { inherited::initialize(); }
TEMPLATE_SPECIALIZATION
void CStateGroupHearDangerousSoundAbstract::reselect_state()
{
    CMonsterSquad* squad = monster_squad().get_squad(this->object);
    VERIFY(squad);

    if (this->get_state(eStateHearDangerousSound_Home)->check_start_conditions())
    {
        this->select_state(eStateHearDangerousSound_Home);
        return;
    }

    if (squad->SquadActive() && squad->GetCommand(this->object).type == SC_REST)
    {
        if (this->object != squad->GetLeader())
        {
            this->select_state(eStateSquad);
        }
        else
        {
            this->select_state(eStateHearDangerousSound_Hide);
        }
    }
    else
    {
        squad->SetLeader(this->object);
        SMemberGoal goal;

        goal.type = MG_Rest;
        goal.entity = const_cast<CEntityAlive*>(this->object->EnemyMan.get_enemy());

        squad->UpdateGoal(this->object, goal);

        this->select_state(eStateHearDangerousSound_Hide);
        squad->UpdateSquadCommands();
    }
}

TEMPLATE_SPECIALIZATION
void CStateGroupHearDangerousSoundAbstract::setup_substates()
{
    state_ptr state = this->get_state_current();

    if (this->current_substate == eStateSquad)
    {
        SStateDataMoveToPoint data;
        CMonsterSquad* squad = monster_squad().get_squad(this->object);

        if (this->object->control().path_builder().get_node_in_radius(squad->GetLeader()->ai_location().level_vertex_id(),
                8.f, LEADER_RADIUS, FIND_POINT_ATTEMPTS, data.vertex))
        {
            data.point = ai().level_graph().vertex_position(data.vertex);
        }
        else
        {
            Fvector dest_pos = random_position(squad->GetLeader()->Position(), LEADER_RADIUS);
            if (!this->object->control().path_builder().restrictions().accessible(dest_pos))
            {
                data.vertex = this->object->control().path_builder().restrictions().accessible_nearest(dest_pos, data.point);
            }
            else
            {
                data.point = dest_pos;
                data.vertex = u32(-1);
            }
        }

        data.action.action = ACT_RUN;
        data.accelerated = true;
        data.braking = false;
        data.accel_type = eAT_Calm;
        data.completion_dist = 3.f;
        data.action.sound_type = MonsterSound::eMonsterSoundIdle;
        data.action.sound_delay = this->object->db().m_dwIdleSndDelay;

        state->fill_data_with(&data, sizeof(SStateDataMoveToPoint));

        return;
    }

    if (this->current_substate == eStateHearDangerousSound_Hide)
    {
        SStateDataMoveToPointEx data;

        data.vertex = 0;

        Fvector home2sound = this->object->Home->get_home_point();
        home2sound.sub(this->object->SoundMemory.GetSound().position);
        home2sound.normalize_safe();

        m_target_node = this->object->Home->get_place_in_max_home_to_direction(home2sound);

        if (m_target_node == u32(-1))
        {
            data.point = this->object->Position();
        }
        else
        {
            data.point = ai().level_graph().vertex_position(m_target_node);
        }

        data.action.action = ACT_RUN;
        data.action.time_out = 0; // do not use time out
        data.completion_dist = 3.f; // get exactly to the point
        data.time_to_rebuild = 0;
        data.accelerated = true;
        data.braking = true;
        data.accel_type = eAT_Aggressive;
        data.action.sound_type = (u32)MonsterSound::eMonsterSoundDummy;
        data.action.sound_delay = this->object->db().m_dwAttackSndDelay;

        state->fill_data_with(&data, sizeof(SStateDataMoveToPointEx));

        return;
    }
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateGroupHearDangerousSoundAbstract
