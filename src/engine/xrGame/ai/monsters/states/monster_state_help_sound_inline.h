#pragma once

#include "state_move_to_point.h"
#include "state_custom_action_look.h"


#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterHearHelpSoundAbstract CStateMonsterHearHelpSound<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterHearHelpSoundAbstract::CStateMonsterHearHelpSound(_Object *obj) : inherited(obj)
{
	add_state	(eStateHearHelpSound_MoveToDest,	xr_new<CStateMonsterMoveToPointEx<_Object> >(obj));
	add_state	(eStateHearHelpSound_LookAround,	xr_new<CStateMonsterCustomActionLook<_Object> >(obj));
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterHearHelpSoundAbstract::check_start_conditions()
{
	if (!object->SoundMemory.hear_help_sound()) return false;
	if (object->Home->has_home())				return object->Home->at_home(ai().level_graph().vertex_position(object->SoundMemory.hear_help_sound_node()));

	return true;
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterHearHelpSoundAbstract::check_completion()
{
	if (current_substate == u32(-1)) return true;
	return false;
}

TEMPLATE_SPECIALIZATION
void CStateMonsterHearHelpSoundAbstract::reselect_state()
{
	if (prev_substate == u32(-1))
		select_state(eStateHearHelpSound_MoveToDest);
	else if (prev_substate == eStateHearHelpSound_MoveToDest) 
		select_state(eStateHearHelpSound_LookAround);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterHearHelpSoundAbstract::setup_substates()
{
	state_ptr state = get_state_current();

	if (current_substate == eStateHearHelpSound_MoveToDest) {
		SStateDataMoveToPointEx data;

		data.vertex				= object->SoundMemory.hear_help_sound_node();
		data.point				= ai().level_graph().vertex_position(data.vertex);
		data.action.action		= ACT_RUN;
		data.action.time_out	= 0;		// do not use time out
		data.completion_dist	= 0.f;		// get exactly to the point
		data.time_to_rebuild	= 0;		// do not rebuild
		data.accelerated		= true;
		data.braking			= true;
		data.accel_type 		= eAT_Aggressive;
		data.action.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.action.sound_delay = object->db().m_dwIdleSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataMoveToPointEx));

		return;
	}

	if (current_substate == eStateHearHelpSound_LookAround) {
		SStateDataAction	data;
		data.action			= ACT_LOOK_AROUND;
		data.time_out		= 3000;
		data.sound_type		= MonsterSound::eMonsterSoundIdle;
		data.sound_delay	= object->db().m_dwIdleSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataAction));

		return;
	}
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterHearHelpSoundAbstract 
