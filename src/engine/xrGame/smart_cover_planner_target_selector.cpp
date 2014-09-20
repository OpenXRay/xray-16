////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_planner_target_selector.cpp
//	Created 	: 18.09.2007
//	Author		: Alexander Dudin
//	Description : Target selector for smart covers animation planner
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_planner_target_selector.h"
#include "smart_cover_animation_planner.h"
#include "script_game_object.h"
#include "stalker_property_evaluators.h"
#include "smart_cover_planner_target_provider.h"
#include "stalker_decision_space.h"
#include "smart_cover_loophole.h"
#include "stalker_movement_manager_smart_cover.h"
#include "smart_cover_evaluators.h"
#include "ai/stalker/ai_stalker.h"
#include "smart_cover_default_behaviour_planner.hpp"

using namespace StalkerDecisionSpace;
using smart_cover::target_selector;
using smart_cover::animation_planner;
using smart_cover::target_provider;
using smart_cover::default_behaviour_planner;

void target_selector::setup				(animation_planner *object, CPropertyStorage *storage)
{
	inherited::setup		(object, storage);
//	inherited_planner::m_use_log = true;
//	inherited_action::m_use_log = true;
	CActionPlanner::m_storage.set_property(eWorldPropertyLookedOut, m_random.randF() <= .7f ? true : false);
	CActionPlanner::m_storage.set_property(eWorldPropertyLoopholeTooMuchTimeFiring, false);
	add_evaluators			();
	add_actions				();

	CWorldState				target;
	target.add_condition	(CWorldProperty(eWorldPropertyPlannerHasTarget, true));
	set_target_state		(target);
}

void target_selector::callback			(callback_type const &callback)
{
	m_script_callback		= callback;
}

void target_selector::update			()
{
	//. think about this line
	//. probably, we should avoid this update
	//. when script callback is setup
	inherited::update		();

	m_script_callback		(object().object().lua_game_object());
}

void target_selector::add_evaluators	()
{
	add_evaluator			(
		eWorldPropertyLookedOut,
		xr_new<CPropertyEvaluatorMember<animation_planner> >(
			(CPropertyStorage*)0,
			eWorldPropertyLookedOut,
			true,
			true,
			"looked out"
		)
	);	
	add_evaluator			(
		eWorldPropertyLoopholeTooMuchTimeFiring,
		xr_new<CPropertyEvaluatorMember<animation_planner> >(
			(CPropertyStorage*)0,
			eWorldPropertyLoopholeTooMuchTimeFiring,
			true,
			true,
			"too much time firing"
		)
	);	
	add_evaluator			(
		eWorldPropertyLoopholeLastHitWasLongAgo,
		xr_new<evaluators::loophole_hit_long_ago_evaluator>(
			&object(),
			"last hit was long ago",
			16000
		)
	);
	add_evaluator			(
		eWorldPropertyLoopholeCanLookout,
		xr_new<evaluators::is_action_available_evaluator>(
			&object(),
			"can lookout",
			"lookout"
		)
	);
	add_evaluator			(
		eWorldPropertyLoopholeCanFire,
		xr_new<evaluators::is_action_available_evaluator>(
			&object(),
			"can fire",
			"fire"
		)
	);
	add_evaluator			(
		eWorldPropertyLoopholeCanFireNoLookout,
		xr_new<evaluators::is_action_available_evaluator>(
			&object(),
			"can fire_no_lookout",
			"fire_no_lookout"
		)
	);
	add_evaluator			(
		eWorldPropertyLoopholeUseDefaultBehaviour,
		xr_new<evaluators::default_behaviour_evaluator>(
			&object(),
			"use default behaviour"
		)
	);
	add_evaluator			(
		eWorldPropertyLoopholeCanFireAtEnemy,
		xr_new<evaluators::can_fire_at_enemy_evaluator>(
			&object(),
			"can fire at enemy"
		)
	);
	add_evaluator			(
		eWorldPropertyPlannerHasTarget,
		xr_new<evaluators::loophole_planner_const_evaluator>(
			&object(),
			"loophole planner has target",
			false
		)
	);
}

void target_selector::add_actions		()
{
	CActionBase<animation_planner> *action;

	action					= xr_new<target_idle>(&object(), "idle", eWorldPropertyLoopholeIdle, 0);
//	add_condition			(action, eWorldPropertyLoopholeCanFireAtEnemy,		true);
//	add_condition			(action, eWorldPropertyLoopholeCanFire,				true);
//	add_condition			(action, eWorldPropertyLookedOut,					true);
//	add_condition			(action, eWorldPropertyLoopholeLastHitWasLongAgo,	true);
	add_condition			(action, eWorldPropertyLoopholeTooMuchTimeFiring,	true);
	add_effect				(action, eWorldPropertyLoopholeTooMuchTimeFiring,	false);
	add_operator			(eWorldOperatorLoopholeTargetIdle,					action);
	action->set_inertia_time(1000);

	action					= xr_new<target_provider>(&object(), "lookout", eWorldPropertyLookedOut, 0);
	add_condition			(action, eWorldPropertyLoopholeCanLookout,			true);
	add_condition			(action, eWorldPropertyLoopholeUseDefaultBehaviour,	false);
	add_condition			(action, eWorldPropertyLookedOut,					false);
	add_condition			(action, eWorldPropertyLoopholeLastHitWasLongAgo,	true);
	add_condition			(action, eWorldPropertyPlannerHasTarget,			false);
	add_effect				(action, eWorldPropertyPlannerHasTarget,			true);
	add_operator			(eWorldOperatorLoopholeTargetLookout,				action);

	action					= xr_new<target_fire>(&object(), "fire", eWorldPropertyLoopholeFire, 0);
	add_condition			(action, eWorldPropertyLoopholeCanFireAtEnemy,		true);
	add_condition			(action, eWorldPropertyLoopholeCanFire,				true);
	add_condition			(action, eWorldPropertyLookedOut,					true);
	add_condition			(action, eWorldPropertyLoopholeLastHitWasLongAgo,	true);
	add_condition			(action, eWorldPropertyLoopholeTooMuchTimeFiring,	false);
	add_condition			(action, eWorldPropertyPlannerHasTarget,			false);
	add_effect				(action, eWorldPropertyPlannerHasTarget,			true);
	add_operator			(eWorldOperatorLoopholeTargetFire,					action);

	action					= xr_new<target_fire_no_lookout>(&object(), "fire_no_lookout", eWorldPropertyLoopholeFireNoLookout, 0);
	add_condition			(action, eWorldPropertyLoopholeCanFireAtEnemy,		true);
	add_condition			(action, eWorldPropertyLoopholeCanFireNoLookout,	true);
	add_condition			(action, eWorldPropertyLoopholeLastHitWasLongAgo,	false);
	add_condition			(action, eWorldPropertyPlannerHasTarget,			false);
	add_effect				(action, eWorldPropertyPlannerHasTarget,			true);
	add_operator			(eWorldOperatorLoopholeTargetFireNoLookout,			action);

	action					= xr_new<default_behaviour_planner>(&object(), "default_behaviour");
	add_condition			(action, eWorldPropertyLoopholeUseDefaultBehaviour,	true);
	add_condition			(action, eWorldPropertyLoopholeTooMuchTimeFiring,	false);
	add_condition			(action, eWorldPropertyPlannerHasTarget,			false);
	add_effect				(action, eWorldPropertyPlannerHasTarget,			true);
	add_operator			(eWorldOperatorLoopholeTargetDefaultBehaviour,		action);
}

LPCSTR	target_selector::object_name	() const
{
	return					("target_selector");
}