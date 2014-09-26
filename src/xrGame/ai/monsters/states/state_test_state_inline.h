#pragma once

#include "../../../level.h"
#include "state_move_to_point.h"
#include "state_custom_action.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterTestStateAbstract CStateMonsterTestState<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterTestStateAbstract::CStateMonsterTestState(_Object *obj) : inherited(obj) 
{
	add_state(eStateCustom,xr_new<CStateMonsterMoveToPointEx<_Object> >(obj));
}

TEMPLATE_SPECIALIZATION
void CStateMonsterTestStateAbstract::reselect_state()
{
	select_state(eStateCustom);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterTestStateAbstract::setup_substates()
{
	state_ptr state = get_state_current();

	if (current_substate == eStateCustom) {
		SStateDataMoveToPointEx data;

		Fvector dest_pos = Level().CurrentEntity()->Position();
		dest_pos = random_position(dest_pos, 20.f);

		if (!object->control().path_builder().restrictions().accessible(dest_pos)) {
			data.vertex		= object->control().path_builder().restrictions().accessible_nearest(dest_pos, data.point);
		} else {
			data.point		= dest_pos;
			data.vertex		= u32(-1);
		}

		data.action.action		= ACT_RUN;
		data.action.time_out	= 20000;
		data.accelerated		= true;
		data.braking			= false;
		data.accel_type 		= eAT_Calm;
		data.completion_dist	= 3.f;
		data.action.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.action.sound_delay = object->db().m_dwIdleSndDelay;
		data.time_to_rebuild	= 0;

		state->fill_data_with(&data, sizeof(SStateDataMoveToPointEx));

		return;
	}
}


#define CStateMonsterTestCoverAbstract CStateMonsterTestCover<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterTestCoverAbstract::CStateMonsterTestCover(_Object *obj) : inherited(obj) 
{
	add_state(eStateAttack_HideInCover,xr_new<CStateMonsterMoveToPointEx<_Object> >(obj));
	add_state(eStateAttack_CampInCover,xr_new<CStateMonsterCustomAction<_Object> >(obj));
}
TEMPLATE_SPECIALIZATION
void CStateMonsterTestCoverAbstract::initialize()
{
	inherited::initialize();

	m_last_node	= object->m_target_node;
}


TEMPLATE_SPECIALIZATION
void CStateMonsterTestCoverAbstract::check_force_state()
{
	if (m_last_node	!= object->m_target_node) {
		m_last_node			= object->m_target_node;
		current_substate	= u32(-1);
		return;
	}
	
	if (current_substate == eStateAttack_CampInCover)
		if (object->ai_location().level_vertex_id() != m_last_node)
			current_substate = u32(-1);
}
TEMPLATE_SPECIALIZATION
void CStateMonsterTestCoverAbstract::reselect_state()
{
	if (object->ai_location().level_vertex_id() != m_last_node)
		select_state(eStateAttack_HideInCover);
	else 
		select_state(eStateAttack_CampInCover);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterTestCoverAbstract::setup_substates()
{
	state_ptr state = get_state_current();

	if (current_substate == eStateAttack_HideInCover) {
		SStateDataMoveToPointEx data;
		data.vertex				= m_last_node;
		data.point				= ai().level_graph().vertex_position(data.vertex);
		data.action.action		= ACT_RUN;
		data.action.time_out	= 200000;
		data.accelerated		= true;
		data.braking			= false;
		data.accel_type 		= eAT_Aggressive;
		data.completion_dist	= 0.f;
		data.action.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.action.sound_delay = object->db().m_dwIdleSndDelay;
		data.time_to_rebuild	= 0;

		state->fill_data_with(&data, sizeof(SStateDataMoveToPointEx));
		return;
	}

	if (current_substate == eStateAttack_CampInCover) {
		SStateDataAction	data;
		data.action			= ACT_STAND_IDLE;
		data.sound_type		= MonsterSound::eMonsterSoundIdle;
		data.sound_delay	= object->db().m_dwIdleSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataAction));
		return;
	}

}


#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterTestStateAbstract
#undef CStateMonsterTestCoverAbstract
