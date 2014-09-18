////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_danger_by_sound_planner.cpp
//	Created 	: 31.05.2005
//  Modified 	: 31.05.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker danger by sound planner class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "stalker_danger_by_sound_planner.h"
#include "ai/stalker/ai_stalker.h"
#include "script_game_object.h"
#include "stalker_danger_by_sound_actions.h"
#include "stalker_decision_space.h"
#include "stalker_danger_property_evaluators.h"

using namespace StalkerDecisionSpace;

CStalkerDangerBySoundPlanner::CStalkerDangerBySoundPlanner	(CAI_Stalker *object, LPCSTR action_name) :
	inherited				(object,action_name)
{
}

void CStalkerDangerBySoundPlanner::setup					(CAI_Stalker *object, CPropertyStorage *storage)
{
	inherited::setup		(object,storage);
	clear					();
	add_evaluators			();
	add_actions				();
}

void CStalkerDangerBySoundPlanner::initialize				()
{
	inherited::initialize	();
}

void CStalkerDangerBySoundPlanner::update					()
{
	inherited::update		();
}

void CStalkerDangerBySoundPlanner::finalize					()
{
	inherited::finalize		();
}
void CStalkerDangerBySoundPlanner::add_evaluators			()
{
	add_evaluator			(eWorldPropertyDanger			,xr_new<CStalkerPropertyEvaluatorDangers>			(m_object,"danger"));
	add_evaluator			(eWorldPropertyDangerUnknown	,xr_new<CStalkerPropertyEvaluatorConst>				(false,"fake"));
}

void CStalkerDangerBySoundPlanner::add_actions				()
{
	CStalkerActionBase		*action;

	action					= xr_new<CStalkerActionDangerBySoundTakeCover>(m_object,"fake");
	add_effect				(action,eWorldPropertyDanger,		false);
	add_operator			(eWorldOperatorDangerUnknownTakeCover,	action);
}
