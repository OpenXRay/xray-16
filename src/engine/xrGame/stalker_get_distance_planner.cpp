////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_get_distance_planner.cpp
//	Created 	: 25.07.2007
//  Modified 	: 25.07.2007
//	Author		: Dmitriy Iassenev
//	Description : Stalker get distance planner
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_get_distance_planner.h"
#include "ai/stalker/ai_stalker_space.h"
#include "stalker_decision_space.h"
#include "stalker_property_evaluators.h"
#include "stalker_get_distance_actions.h"
#include "script_game_object.h"
#include "ai/stalker/ai_stalker.h"

using namespace StalkerSpace;
using namespace StalkerDecisionSpace;

CStalkerGetDistancePlanner::CStalkerGetDistancePlanner	(CAI_Stalker *object, LPCSTR action_name) :
	inherited			(object,action_name)
{
}

CStalkerGetDistancePlanner::~CStalkerGetDistancePlanner	()
{
}

void CStalkerGetDistancePlanner::setup					(CAI_Stalker *object, CPropertyStorage *storage)
{
	inherited::setup		(object,storage);

	CActionPlanner<CScriptGameObject>::m_storage.set_property	(eWorldPropertyInCover,	false);

	clear					();
	add_evaluators			();
	add_actions				();

	object->best_cover_invalidate	();
}

void CStalkerGetDistancePlanner::add_evaluators			()
{
	add_evaluator			(
		eWorldPropertyInCover,
		xr_new<CStalkerPropertyEvaluatorMember>(
			(CPropertyStorage*)0,//&CScriptActionPlanner::m_storage,
			eWorldPropertyInCover,
			true,
			true,
			"in cover"
		)
	);
	
	add_evaluator			(
		eWorldPropertyTooFarToKillEnemy,
		xr_new<CStalkerPropertyEvaluatorTooFarToKillEnemy>(
			m_object,
			"too far to kill"
		)
	);
}

void CStalkerGetDistancePlanner::add_actions			()
{
	CStalkerActionBase		*action;

	action					= xr_new<CStalkerActionRunToCover>		(m_object,"run to cover");
	add_condition			(action,eWorldPropertyInCover,			false);
	add_effect				(action,eWorldPropertyInCover,			true);
	add_operator			(eWorldOperatorRunToCover,				action);

	action					= xr_new<CStalkerActionWaitInCover>		(m_object,"wait in cover");
	add_condition			(action,eWorldPropertyInCover,			true);
	add_condition			(action,eWorldPropertyTooFarToKillEnemy,true);
	add_effect				(action,eWorldPropertyTooFarToKillEnemy,false);
	add_operator			(eWorldOperatorWaitInCover,				action);
}
