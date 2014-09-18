#pragma once

#include "state_move_to_point.h"
#include "state_look_point.h"
#include "state_custom_action.h"
#include "../../../cover_point.h"
#include "../monster_cover_manager.h"


#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterRestIdleAbstract CStateMonsterRestIdle<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterRestIdleAbstract::CStateMonsterRestIdle(_Object *obj) : inherited(obj)
{
	add_state	(eStateRest_WalkToCover,	xr_new<CStateMonsterMoveToPointEx<_Object> >	(obj));
	add_state	(eStateRest_LookOpenPlace,	xr_new<CStateMonsterLookToPoint<_Object> >		(obj));
	add_state	(eStateRest_Idle,			xr_new<CStateMonsterCustomAction<_Object> >		(obj));
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestIdleAbstract::initialize()
{
	inherited::initialize();

	m_target_node = u32(-1);
	
	// try to get cover
	const CCoverPoint *point = object->CoverMan->find_cover(object->Position(), 5.f, 10.f);
	if (!point) {
		point = object->CoverMan->find_cover(object->Position(), 10.f, 30.f);
		if (!point) return;
	}
	
	m_target_node = point->level_vertex_id();

	CMonsterSquad *squad = monster_squad().get_squad(object);
	squad->lock_cover(m_target_node);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestIdleAbstract::finalize()
{
	inherited::finalize();
	CMonsterSquad *squad = monster_squad().get_squad(object);
	squad->unlock_cover(m_target_node);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestIdleAbstract::critical_finalize()
{
	inherited::critical_finalize();

	CMonsterSquad *squad = monster_squad().get_squad(object);
	squad->unlock_cover(m_target_node);
}


TEMPLATE_SPECIALIZATION
void CStateMonsterRestIdleAbstract::reselect_state()
{
	if ((prev_substate == u32(-1)) && (m_target_node != u32(-1))) {
		select_state(eStateRest_WalkToCover);
		return;
	} 

	if ((prev_substate == eStateRest_WalkToCover) || (prev_substate == u32(-1))) {
		select_state(eStateRest_LookOpenPlace);
		return;
	}

	select_state(eStateRest_Idle);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestIdleAbstract::setup_substates()
{
	state_ptr state = get_state_current();

	if (current_substate == eStateRest_WalkToCover) {
		SStateDataMoveToPointEx data;

		data.vertex				= m_target_node;
		data.point				= ai().level_graph().vertex_position(data.vertex);
		data.action.action		= ACT_WALK_FWD;
		data.action.time_out	= 0;		// do not use time out
		data.completion_dist	= 0.f;		// get exactly to the point
		data.time_to_rebuild	= 0;		// do not rebuild
		data.accelerated		= true;
		data.braking			= true;
		data.accel_type 		= eAT_Calm;
		data.action.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.action.sound_delay = object->db().m_dwIdleSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataMoveToPointEx));
		return;
	}

	if (current_substate == eStateRest_LookOpenPlace) {

		SStateDataLookToPoint	data;

		Fvector dir;
		object->CoverMan->less_cover_direction(dir);

		data.point.mad			(object->Position(),dir,10.f);
		data.action.action		= ACT_STAND_IDLE;
		data.action.time_out	= 2000;		
		data.action.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.action.sound_delay = object->db().m_dwIdleSndDelay;
		data.face_delay			= 0;

		state->fill_data_with(&data, sizeof(SStateDataLookToPoint));
		return;
	}

	if (current_substate == eStateRest_Idle) {
		SStateDataAction data;

		data.action		= ACT_REST;
		data.time_out	= 0;			// do not use time out
		data.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.sound_delay = object->db().m_dwIdleSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataAction));

		return;
	}

}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterRestIdleAbstract
