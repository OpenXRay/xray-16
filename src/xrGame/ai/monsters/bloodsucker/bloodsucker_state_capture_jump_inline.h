#pragma once

#include "ai/monsters/states/state_custom_action.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateCaptureJumpBloodsuckerAbstract CStateCaptureJumpBloodsucker<_Object>

TEMPLATE_SPECIALIZATION
CStateCaptureJumpBloodsuckerAbstract::CStateCaptureJumpBloodsucker(_Object* obj) : inherited(obj)
{
    add_state(eStateCustom, new CStateMonsterCustomAction<_Object>(obj));
}

TEMPLATE_SPECIALIZATION
CStateCaptureJumpBloodsuckerAbstract::~CStateCaptureJumpBloodsucker() {}
TEMPLATE_SPECIALIZATION
void CStateCaptureJumpBloodsuckerAbstract::execute()
{
    // check alife control
    select_state(eStateCustom);

    get_state_current()->execute();
    prev_substate = current_substate;
}
TEMPLATE_SPECIALIZATION
void CStateCaptureJumpBloodsuckerAbstract::setup_substates()
{
    state_ptr state = get_state_current();
    if (current_substate == eStateCustom)
    {
        SStateDataAction data;

        data.action = ACT_STAND_IDLE;
        data.time_out = 0; // do not use time out
        /*data.sound_type	= MonsterSound::eMonsterSoundIdle;
        data.sound_delay = object->db().m_dwIdleSndDelay;
        */
        state->fill_data_with(&data, sizeof(SStateDataAction));

        return;
    }
}
#undef TEMPLATE_SPECIALIZATION
#undef CStateCaptureJumpBloodsuckerAbstract
