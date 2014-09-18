////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_default_behaviour_planner.cpp
//	Created 	: 15.11.2007
//	Author		: Alexander Dudin
//	Description : Default behaviour planner for target selector
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_default_behaviour_planner.hpp"
#include "script_game_object.h"
#include "smart_cover_animation_planner.h"
#include "smart_cover_planner_target_provider.h"
#include "smart_cover_evaluators.h"
#include "stalker_decision_space.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_property_evaluators.h"
#include "script_game_object.h"

using namespace StalkerDecisionSpace;
using smart_cover::default_behaviour_planner;
using smart_cover::animation_planner;

default_behaviour_planner::default_behaviour_planner	(animation_planner *object, LPCSTR action_name) :
	inherited											(object, action_name)
{

}

void default_behaviour_planner::setup					(animation_planner *object, CPropertyStorage *storage)
{
	inherited::setup			(object, storage);
	add_evaluators				();
	add_actions					();
	CWorldState					target;
	target.add_condition		(CWorldProperty(eWorldPropertyPlannerHasTarget, true));
	set_target_state			(target);
}

void default_behaviour_planner::initialize				()
{
	inherited::initialize		();
}

void default_behaviour_planner::update					()
{
	inherited::update			();
}

void default_behaviour_planner::finalize				()
{
	inherited::finalize			();
}

void default_behaviour_planner::add_evaluators			()
{
	add_evaluator			(
		eWorldPropertyPlannerHasTarget,
		xr_new<evaluators::loophole_planner_const_evaluator>(
			&object(),
			"default behaviour planner has target",
			false
		)
	);
	add_evaluator			(
		eWorldPropertyLoopholeCanStayIdle,
		xr_new<evaluators::is_action_available_evaluator>(
			&object(),
			"can stay idle",
			"idle"
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
		eWorldPropertyReadyToLookout,
		xr_new<evaluators::lookout_time_interval_passed_evaluator>(
			&object(),
			"ready to lookout",
			object().default_lookout_interval()
		)
	);
	add_evaluator			(
		eWorldPropertyReadyToIdle,
		xr_new<evaluators::idle_time_interval_passed_evaluator>(
			&object(),
			"stay idle",
			object().default_idle_interval()
		)
	);
}

void default_behaviour_planner::add_actions				()
{
	CActionBase<animation_planner> *action;
	action					= xr_new<target_provider>(&object(), "idle", eWorldPropertyLoopholeIdle, 0);
	add_condition			(action, eWorldPropertyLoopholeCanStayIdle,			true);
	add_condition			(action, eWorldPropertyReadyToIdle,					true);
	add_condition			(action, eWorldPropertyPlannerHasTarget,			false);
	add_effect				(action, eWorldPropertyPlannerHasTarget,			true);
	add_operator			(eWorldOperatorLoopholeTargetIdle,					action);
	action->setup			(&object(), inherited_action::m_storage);

	action					= xr_new<target_provider>(&object(), "lookout", eWorldPropertyLookedOut, 0);
	add_condition			(action, eWorldPropertyLoopholeCanLookout,			true);
	add_condition			(action, eWorldPropertyReadyToLookout,				true);
	add_condition			(action, eWorldPropertyPlannerHasTarget,			false);
	add_effect				(action, eWorldPropertyPlannerHasTarget,			true);
	add_operator			(eWorldOperatorLoopholeTargetLookout,				action);
	action->setup			(&object(), inherited_action::m_storage);
}

LPCSTR default_behaviour_planner::object_name			() const
{
	return					("default_behaviour_planner");
}