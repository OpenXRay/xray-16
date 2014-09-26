#pragma once

#include "../states/state_custom_action.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateCustomGroupAbstract CStateCustomGroup<_Object>


TEMPLATE_SPECIALIZATION
CStateCustomGroupAbstract::CStateCustomGroup(_Object *obj) : inherited(obj)
{
	add_state(eStateCustom,				xr_new<CStateMonsterCustomAction<_Object> >		(obj));
}

TEMPLATE_SPECIALIZATION
CStateCustomGroupAbstract::~CStateCustomGroup	()
{
}

TEMPLATE_SPECIALIZATION
void CStateCustomGroupAbstract::execute()
{
	// check alife control
	select_state	(eStateCustom);
	object->start_animation();

	get_state_current()->execute();
	prev_substate = current_substate;
}
TEMPLATE_SPECIALIZATION
void CStateCustomGroupAbstract::setup_substates()
{
	state_ptr state = get_state_current();
	if (current_substate == eStateCustom) {
		SStateDataAction data;

		data.action		= ACT_STAND_IDLE;
		data.time_out	= 0;			// do not use time out
		switch(object->get_number_animation())
		{
		case u32(5):
			data.sound_type	= MonsterSound::eMonsterSoundSteal;
			break;
		case u32(6):
			data.sound_type	= MonsterSound::eMonsterSoundThreaten;
			break;
		default:
			data.sound_type	= MonsterSound::eMonsterSoundIdle;
			break;
		}
		data.sound_delay = object->db().m_dwEatSndDelay;
		state->fill_data_with(&data, sizeof(SStateDataAction));

		return;
	}
}
#undef TEMPLATE_SPECIALIZATION
#undef CStateCustomGroupAbstract
