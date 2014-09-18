////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_evaluators.cpp
//	Created 	: 05.11.2007
//	Author		: Alexander Dudin
//	Description : Smart cover evaluators classes
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_evaluators.h"
#include "stalker_property_evaluators.h"
#include "script_game_object.h"
#include "stalker_decision_space.h"
#include "ai/stalker/ai_stalker.h"
#include "ai_space.h"
#include "stalker_movement_manager_smart_cover.h"
#include "memory_manager.h"
#include "memory_space.h"
#include "enemy_manager.h"
#include "visual_memory_manager.h"
#include "cover_point.h"
#include "smart_cover.h"
#include "smart_cover_animation_planner.h"
#include "smart_cover_planner_target_selector.h"
#include "smart_cover_loophole.h"
#include "smart_cover_transition.hpp"
#include "smart_cover_transition_animation.hpp"
#include "smart_cover_description.h"
#include "stalker_animation_manager.h"


namespace smart_cover {

shared_str	transform_vertex(shared_str const &vertex_id, bool const &in);

};

using namespace StalkerDecisionSpace;
using smart_cover::evaluators::in_cover_evaluator;
using smart_cover::evaluators::cover_actual_evaluator;
using smart_cover::evaluators::cover_entered_evaluator;
using smart_cover::evaluators::loophole_actual_evaluator;
using smart_cover::evaluators::loophole_hit_long_ago_evaluator;
using smart_cover::evaluators::is_action_available_evaluator;
using smart_cover::evaluators::loophole_planner_const_evaluator;
using smart_cover::evaluators::loophole_exitable_evaluator;
using smart_cover::evaluators::can_exit_loophole_with_animation;
using smart_cover::evaluators::default_behaviour_evaluator;
using smart_cover::evaluators::can_fire_at_enemy_evaluator;
using smart_cover::evaluators::idle_time_interval_passed_evaluator;
using smart_cover::evaluators::lookout_time_interval_passed_evaluator;
using smart_cover::animation_planner;

typedef CStalkerPropertyEvaluator::_value_type _value_type;

//////////////////////////////////////////////////////////////////////////
// in_cover_evaluator
//////////////////////////////////////////////////////////////////////////

in_cover_evaluator::in_cover_evaluator				(CAI_Stalker *object, LPCSTR evaluator_name) :
	inherited										(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type in_cover_evaluator::evaluate			()
{
	return						(!!object().movement().current_params().cover());
}

//////////////////////////////////////////////////////////////////////////
// cover_actual_evaluator
//////////////////////////////////////////////////////////////////////////

cover_actual_evaluator::cover_actual_evaluator		(CAI_Stalker *object, LPCSTR evaluator_name) :
	inherited					(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type cover_actual_evaluator::evaluate		()
{
	VERIFY						(object().movement().current_params().cover());
	return						(object().movement().current_params().cover() == object().movement().target_params().cover());
}

//////////////////////////////////////////////////////////////////////////
// cover_entered_evaluator
//////////////////////////////////////////////////////////////////////////

cover_entered_evaluator::cover_entered_evaluator	(CAI_Stalker *object, LPCSTR evaluator_name) :
	inherited					(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type cover_entered_evaluator::evaluate		()
{
	return						(!!object().movement().current_params().cover());
}

//////////////////////////////////////////////////////////////////////////
// loophole_actual_evaluator
//////////////////////////////////////////////////////////////////////////

loophole_actual_evaluator::loophole_actual_evaluator(CAI_Stalker *object, LPCSTR evaluator_name, animation_planner *planner, u32 const &loophole_value) :
	inherited					(object ? object->lua_game_object() : 0, evaluator_name),
	m_loophole_value			(loophole_value),
	m_planner					(planner)
{

}

_value_type loophole_actual_evaluator::evaluate		()
{
	if (object().movement().current_params().cover() != object().movement().target_params().cover())
		return					(false);

	if (object().movement().current_params().cover_loophole() != object().movement().target_params().cover_loophole())
		return					(false);

	return						(true);
}

//////////////////////////////////////////////////////////////////////////
// loophole_hit_long_ago_evaluator
//////////////////////////////////////////////////////////////////////////

loophole_hit_long_ago_evaluator::loophole_hit_long_ago_evaluator(animation_planner *object, LPCSTR evaluator_name, u32 const &time_to_wait) :
	inherited					(object, evaluator_name),
	m_time_to_wait				(time_to_wait)
{
	
}

_value_type loophole_hit_long_ago_evaluator::evaluate()
{
	return						((m_object->time_object_hit() + m_time_to_wait) < Device.dwTimeGlobal);
}

//////////////////////////////////////////////////////////////////////////
// is_action_available_evaluator
//////////////////////////////////////////////////////////////////////////

is_action_available_evaluator::is_action_available_evaluator	(animation_planner *object, LPCSTR evaluator_name, LPCSTR action_id) : 
	inherited					(object, evaluator_name),
	m_action_id					(action_id)
{

}

_value_type is_action_available_evaluator::evaluate	()
{
	if (!m_object->m_object->movement().current_params().cover())
		return					(false);

	if (!m_object->m_object->movement().current_params().cover_loophole())
		return					(false);

	return						(m_object->m_object->movement().current_params().cover_loophole()->is_action_available(m_action_id));
}

//////////////////////////////////////////////////////////////////////////
// loophole_planner_const_evaluator
//////////////////////////////////////////////////////////////////////////

loophole_planner_const_evaluator::loophole_planner_const_evaluator	(animation_planner *object, LPCSTR evaluator_name, bool const &value) :
	inherited					(object, evaluator_name),
	m_value						(value)
{

}

_value_type loophole_planner_const_evaluator::evaluate		()
{
	return						(m_value);
}

//////////////////////////////////////////////////////////////////////////
// loophole_exitable_evaluator
//////////////////////////////////////////////////////////////////////////

loophole_exitable_evaluator::loophole_exitable_evaluator(CAI_Stalker *object, LPCSTR evaluator_name) :
	inherited					(object ? object->lua_game_object() : 0, evaluator_name)
{

}

_value_type loophole_exitable_evaluator::evaluate			()
{
	if (!m_object->movement().current_params().cover_loophole())
		return					(false);

	return						(object().movement().current_params().cover_loophole()->exitable());
}

//////////////////////////////////////////////////////////////////////////
// can_exit_loophole_with_animation
//////////////////////////////////////////////////////////////////////////

can_exit_loophole_with_animation::can_exit_loophole_with_animation(CAI_Stalker *object, LPCSTR evaluator_name) :
	inherited					(object ? object->lua_game_object() : 0, evaluator_name)
{

}

_value_type can_exit_loophole_with_animation::evaluate			()
{
	stalker_movement_manager_smart_cover& movement = object().movement();
	stalker_movement_params const& current	= movement.current_params();
	VERIFY						(current.cover());

	stalker_movement_params const& target	= object().movement().target_params();

	smart_cover::cover const*	current_cover = current.cover();
	smart_cover::cover const*	target_cover = target.cover();
	if (current_cover != target_cover) {
#ifdef DEBUG
		Msg						(
			"transition guard(cover): [%s] -> [%s]",
			current_cover ? current_cover->id().c_str() : "<world>",
			target_cover ? target_cover->id().c_str() : "<world>"
		);
#endif // #ifdef DEBUG
		return					(movement.current_transition().animation().has_animation());
	}

	smart_cover::loophole const*current_loophole = current.cover_loophole();
	smart_cover::loophole const*target_loophole = target.cover_loophole();
	if (current_loophole != target_loophole) {
#ifdef DEBUG
		Msg						(
			"transition guard(loophole): [%s] -> [%s]",
			current_loophole ? current_loophole->id().c_str() : "<world>",
			target_loophole ? target_loophole->id().c_str() : "<world>"
		);
#endif // #ifdef DEBUG
		return					(movement.current_transition().animation().has_animation());
	}

	return						(false);
}

//////////////////////////////////////////////////////////////////////////
// default_behaviour_evaluator
//////////////////////////////////////////////////////////////////////////

default_behaviour_evaluator::default_behaviour_evaluator		(animation_planner *object, LPCSTR evaluator_name) :
	inherited					(object, evaluator_name)
{

}

_value_type default_behaviour_evaluator::evaluate			()
{
	return						(
		m_object->m_object->movement().default_behaviour() ||
		m_object->m_object->movement().combat_behaviour()
	);
}

//////////////////////////////////////////////////////////////////////////
// can_fire_at_enemy_evaluator
//////////////////////////////////////////////////////////////////////////

can_fire_at_enemy_evaluator::can_fire_at_enemy_evaluator	(animation_planner *object, LPCSTR evaluator_name) :
	inherited					(object, evaluator_name)
{

}

_value_type can_fire_at_enemy_evaluator::evaluate			()
{
	if (!m_object->m_object->movement().default_behaviour())
		return					(true);

	if (m_object->m_object->movement().current_params().cover_fire_position())
		return					(true);

	if (!m_object->m_object->movement().enemy_in_fov())
		return					(false);

	return						(true);
}

//////////////////////////////////////////////////////////////////////////
// idle_time_interval_passed_evaluator
//////////////////////////////////////////////////////////////////////////

idle_time_interval_passed_evaluator::idle_time_interval_passed_evaluator(animation_planner *object, LPCSTR evaluator_name, u32 const &time_interval) :
	inherited					(object, evaluator_name),
	m_time_interval				(time_interval)
{

}

_value_type idle_time_interval_passed_evaluator::evaluate		()
{
	if (!m_object->stay_idle())
		return					(false);

	u32 const					&current_time = Device.dwTimeGlobal;
	if (current_time <= m_object->last_idle_time() + m_time_interval) {
		m_object->stay_idle		(true);
		
		return					(true);
	}
	else {
		m_object->last_lookout_time(current_time);
		m_time_interval			= m_object->default_idle_interval();
		m_object->stay_idle		(false);
		
		return					(false);
	}
}

//////////////////////////////////////////////////////////////////////////
// lookout_time_interval_passed_evaluator
//////////////////////////////////////////////////////////////////////////

lookout_time_interval_passed_evaluator::lookout_time_interval_passed_evaluator(animation_planner *object, LPCSTR evaluator_name, u32 const &time_interval) :
	inherited					(object, evaluator_name),
	m_time_interval				(time_interval)
{

}

_value_type lookout_time_interval_passed_evaluator::evaluate		()
{
	if (m_object->stay_idle())
		return					(false);

	u32 const					&current_time = Device.dwTimeGlobal;
	if (current_time <= m_object->last_lookout_time() + m_time_interval) {
		m_object->stay_idle		(false);

		return					(true);
	}
	else {
		m_object->last_idle_time(current_time);
		m_time_interval			= m_object->default_lookout_interval();
		m_object->stay_idle		(true);

		return					(false);
	}
}