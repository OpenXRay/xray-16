#pragma once

#include "state_custom_action.h"
#include "state_move_to_point.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateMonsterControlledFollowAbstract CStateMonsterControlledFollow<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterControlledFollowAbstract::CStateMonsterControlledFollow(_Object* obj) : inherited(obj)
{
    this->add_state(eStateControlled_Follow_Wait, xr_new<CStateMonsterCustomAction<_Object>>(obj));
    this->add_state(eStateControlled_Follow_WalkToObject, xr_new<CStateMonsterMoveToPointEx<_Object>>(obj));
}

TEMPLATE_SPECIALIZATION
void CStateMonsterControlledFollowAbstract::reselect_state()
{
    CControlledEntityBase* entity = smart_cast<CControlledEntityBase*>(this->object);
    VERIFY(entity);
    const CEntity* target_object = entity->get_data().m_object;

    const float dist = this->object->Position().distance_to(target_object->Position());
    this->select_state(dist < Random.randF(STOP_DISTANCE, STAY_DISTANCE) ? eStateControlled_Follow_Wait :
                                                                     eStateControlled_Follow_WalkToObject);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterControlledFollowAbstract::setup_substates()
{
    state_ptr state = this->get_state_current();

    if (this->current_substate == eStateControlled_Follow_Wait)
    {
        SStateDataAction data;
        data.action = ACT_REST;
        data.sound_type = MonsterSound::eMonsterSoundIdle;
        data.sound_delay = this->object->db().m_dwIdleSndDelay;
        data.time_out = Random.randI(MIN_TIME_OUT, MAX_TIME_OUT);

        state->fill_data_with(&data, sizeof(SStateDataAction));

        return;
    }

    if (this->current_substate == eStateControlled_Follow_WalkToObject)
    {
        SStateDataMoveToPointEx data;

        CControlledEntityBase* entity = smart_cast<CControlledEntityBase*>(this->object);
        VERIFY(entity);
        const CEntity* target_object = entity->get_data().m_object;

        Fvector dest_pos = random_position(target_object->Position(), 10.f);
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

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterControlledFollowAbstract
