#pragma once

#include "../states/state_move_to_point.h"
#include "../states/state_look_point.h"
#include "../../../cover_point.h"
#include "../monster_cover_manager.h"
#include "../states/state_custom_action.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateGroupRestIdleAbstract CStateGroupRestIdle<_Object>

TEMPLATE_SPECIALIZATION
CStateGroupRestIdleAbstract::CStateGroupRestIdle(_Object *obj) : inherited(obj)
{
	add_state	(eStateRest_WalkToCover,	xr_new<CStateMonsterMoveToPointEx<_Object> >	(obj));
	add_state	(eStateRest_LookOpenPlace,	xr_new<CStateMonsterLookToPoint<_Object> >		(obj));
	add_state	(eStateRest_WalkGraphPoint,	xr_new<CStateMonsterMoveToPointEx<_Object> >	(obj));
	add_state	(eStateCustom,				xr_new<CStateMonsterCustomAction<_Object> >				(obj));
}

TEMPLATE_SPECIALIZATION
void CStateGroupRestIdleAbstract::initialize()
{
	inherited::initialize();

	m_target_node = u32(-1);

	if (object->b_end_state_eat)
	{
		m_target_node = object->Home->get_place_in_min_home();

		if (m_target_node == u32(-1))
		{
			const CCoverPoint *point = object->CoverMan->find_cover(object->Home->get_home_point(), 1, object->Home->get_min_radius());
			if (!point) return;
			m_target_node		= point->level_vertex_id();
		}
	} else {
		m_target_node = object->Home->get_place_in_mid_home();

		if (m_target_node == u32(-1))
		{
			const CCoverPoint *point = object->CoverMan->find_cover(object->Home->get_home_point(), 1, object->Home->get_mid_radius());
			if (!point) return;
			m_target_node		= point->level_vertex_id();
		}
	}
	
	m_move_type = 0;

	CMonsterSquad *squad = monster_squad().get_squad(object);
	squad->lock_cover(m_target_node);
}

TEMPLATE_SPECIALIZATION
void CStateGroupRestIdleAbstract::finalize()
{
	inherited::finalize();
	CMonsterSquad *squad = monster_squad().get_squad(object);
	squad->unlock_cover(m_target_node);
}

TEMPLATE_SPECIALIZATION
void CStateGroupRestIdleAbstract::critical_finalize()
{
	inherited::critical_finalize();

	CMonsterSquad *squad = monster_squad().get_squad(object);
	squad->unlock_cover(m_target_node);
}


TEMPLATE_SPECIALIZATION
void CStateGroupRestIdleAbstract::reselect_state()
{
	if (object->saved_state == eStateRest_LookOpenPlace) {
		object->saved_state = u32(-1);
		select_state(eStateRest_WalkGraphPoint);
		return;
	}

	if ((prev_substate == u32(-1) && m_target_node != u32(-1)) || (prev_substate == eStateRest_WalkGraphPoint)) {
		select_state(eStateRest_WalkToCover);
		return;
	} 

	if ((prev_substate == eStateRest_WalkToCover) || (prev_substate == u32(-1))) {
		object->saved_state = eStateRest_LookOpenPlace;
		if (object->b_end_state_eat)
		{
			object->set_current_animation(8);
			object->b_end_state_eat = false;
		} else {
			object->set_current_animation(object->random_anim());
		}
		select_state(eStateCustom);
		return;
	}

	select_state(eStateRest_WalkGraphPoint);
}

TEMPLATE_SPECIALIZATION
void CStateGroupRestIdleAbstract::setup_substates()
{
	state_ptr state = get_state_current();

	if (current_substate == eStateRest_WalkGraphPoint) {
		SStateDataMoveToPointEx data;
		data.vertex				= object->Home->get_place_in_mid_home();
		if (data.vertex == u32(-1))
		{
			data.vertex = object->ai_location().level_vertex_id();
		}
		data.point				= ai().level_graph().vertex_position(data.vertex);
		if (object->Position().distance_to(data.point) > 8.f)
		{
			m_move_type = 1;
			object->m_start_smelling = u32(-1);
		} else {
			if (object->m_start_smelling == u32(-1) || object->m_start_smelling > u32(4) + object->m_smelling_count)
			{
				m_move_type = (Random.randI(2));
				object->m_start_smelling = m_move_type ? u32(1) : u32(-1);
				object->m_smelling_count = Random.randI(3);
			} else {
				m_move_type = 0;
				object->m_start_smelling = object->m_start_smelling + u32(1);
			}
		}
		data.action.action		= m_move_type ? ACT_WALK_FWD : ACT_HOME_WALK_SMELLING;
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

	if (current_substate == eStateRest_WalkToCover) {
		SStateDataMoveToPointEx data;
		data.vertex				= m_target_node;
		if (data.vertex == u32(-1))
		{
			data.vertex = object->ai_location().level_vertex_id();
		}
		data.point				= ai().level_graph().vertex_position(data.vertex);
		if (object->Position().distance_to(data.point) > 8.f)
		{
			m_move_type = 1;
			object->m_start_smelling = u32(-1);
		} else {
			if (object->m_start_smelling == u32(-1) || object->m_start_smelling > u32(4) + object->m_smelling_count)
			{
				m_move_type = (Random.randI(2));
				object->m_start_smelling = m_move_type ? u32(1) : u32(-1);
				object->m_smelling_count = Random.randI(3);
			} else {
				m_move_type = 0;
				object->m_start_smelling = object->m_start_smelling + u32(1);
			}
		}
		data.action.action		= m_move_type ? ACT_WALK_FWD : ACT_HOME_WALK_SMELLING;
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
		data.action.time_out	= 1000;		
		data.action.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.action.sound_delay = object->db().m_dwIdleSndDelay;
		data.face_delay			= 0;

		state->fill_data_with(&data, sizeof(SStateDataLookToPoint));
		return;
	}

	if (current_substate == eStateCustom) {
		SStateDataAction data;

		data.action		= ACT_STAND_IDLE;
		data.time_out	= 0;			// do not use time out
		if (object->get_number_animation() == u32(6))
		{
			data.sound_type	= MonsterSound::eMonsterSoundThreaten;
		} else {
			data.sound_type	= MonsterSound::eMonsterSoundIdle;
		}
		data.sound_delay = object->db().m_dwIdleSndDelay;
		state->fill_data_with(&data, sizeof(SStateDataAction));

		return;
	}
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateGroupRestIdleAbstract
