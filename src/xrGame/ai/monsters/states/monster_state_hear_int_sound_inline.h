#pragma once

#include "state_custom_action_look.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterHearInterestingSoundAbstract CStateMonsterHearInterestingSound<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterHearInterestingSoundAbstract::CStateMonsterHearInterestingSound(_Object *obj) : inherited(obj)
{
	add_state	(eStateHearInterestingSound_MoveToDest,	xr_new<CStateMonsterMoveToPoint<_Object> >(obj));
	add_state	(eStateHearInterestingSound_LookAround,	xr_new<CStateMonsterCustomActionLook<_Object> >(obj));
}

TEMPLATE_SPECIALIZATION
void CStateMonsterHearInterestingSoundAbstract::reselect_state()
{
	if (prev_substate == u32(-1)){
		if (get_state(eStateHearInterestingSound_MoveToDest)->check_start_conditions()) 
			select_state(eStateHearInterestingSound_MoveToDest);
		else 
			select_state(eStateHearInterestingSound_LookAround);
		return;
	}	

	select_state(eStateHearInterestingSound_LookAround);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterHearInterestingSoundAbstract::setup_substates()
{
	state_ptr state = get_state_current();
	
	if (current_substate == eStateHearInterestingSound_MoveToDest) {
		SStateDataMoveToPoint data;
		data.point			= get_target_position();
		data.vertex			= u32(-1);
		data.action.action	= ACT_WALK_FWD;
		data.accelerated	= true;
		data.braking		= false;
		data.accel_type 	= eAT_Calm;
		data.completion_dist= 2.f;
		data.action.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.action.sound_delay = object->db().m_dwIdleSndDelay;
		

		state->fill_data_with(&data, sizeof(SStateDataMoveToPoint));

		return;
	}

	if (current_substate == eStateHearInterestingSound_LookAround) {
		SStateDataActionLook	data;
		data.action				= ACT_LOOK_AROUND;
		data.sound_type			= MonsterSound::eMonsterSoundIdle;
		data.sound_delay		= object->db().m_dwIdleSndDelay;
		
		Fvector dir;
		object->CoverMan->less_cover_direction(dir);
		data.point.mad			(object->Position(),dir,10.f);

		state->fill_data_with(&data, sizeof(SStateDataActionLook));

		return;
	}
}

TEMPLATE_SPECIALIZATION
Fvector	CStateMonsterHearInterestingSoundAbstract::get_target_position()
{
	Fvector snd_pos = object->SoundMemory.GetSound().position;
	if (!object->Home->has_home() )
		return snd_pos;

	if (object->Home->at_home(snd_pos))
		return snd_pos;

	return ai().level_graph().vertex_position(object->Home->get_place());
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterHearInterestingSoundAbstract 
