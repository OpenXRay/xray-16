////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_low_cover_planner.cpp
//	Created 	: 04.09.2007
//  Modified 	: 04.09.2007
//	Author		: Dmitriy Iassenev
//	Description : Stalker low cover planner
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_low_cover_planner.h"
#include "script_game_object.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_decision_space.h"
#include "stalker_property_evaluators.h"
#include "stalker_low_cover_actions.h"
#include "ai/stalker/ai_stalker_space.h"
#include "ai_monster_space.h"
#include "stalker_movement_manager_smart_cover.h"
#include "movement_manager_space.h"
#include "stalker_decision_space.h"
#include "detail_path_manager_space.h"
#include "stalker_planner.h"
#include "memory_space.h"
#include "memory_manager.h"
#include "enemy_manager.h"

using namespace StalkerDecisionSpace;
using namespace MonsterSpace;
using namespace StalkerSpace;

stalker_low_cover_planner::stalker_low_cover_planner	(CAI_Stalker *object, LPCSTR action_name) :
	inherited				(object,action_name)
{
}

void stalker_low_cover_planner::setup					(CAI_Stalker *object, CPropertyStorage *storage)
{
	inherited::setup		(object,storage);
	clear					();
	add_evaluators			();
	add_actions				();
}

void stalker_low_cover_planner::update					()
{
	MemorySpace::CMemoryInfo	mem_object = object().memory().memory(object().memory().enemy().selected());
	if (mem_object.m_object)
		object().best_cover	(mem_object.m_object_params.m_position);

	inherited::update		();
}

void stalker_low_cover_planner::initialize				()
{
	inherited::initialize	();
	
	object().movement().set_movement_type(eMovementTypeStand);
	object().movement().set_nearest_accessible_position	();
	object().movement().set_desired_direction			(0);
	object().movement().set_path_type					(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type			(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_mental_state				(eMentalStateDanger);

	CScriptActionPlanner::m_storage.set_property		(eWorldPropertyInCover,true);
}

void stalker_low_cover_planner::execute					()
{
	inherited::execute		();
}

void stalker_low_cover_planner::finalize				()
{
	inherited::finalize		();
}

void stalker_low_cover_planner::add_evaluators			()
{
	add_evaluator			(eWorldPropertyLowCover,	xr_new<CStalkerPropertyEvaluatorConst>		(true,"using low cover"));
	add_evaluator			(eWorldPropertyReadyToKill,	xr_new<CStalkerPropertyEvaluatorReadyToKill>(m_object,"ready to kill"));
	add_evaluator			(eWorldPropertySeeEnemy,	xr_new<CStalkerPropertyEvaluatorSeeEnemy>	(m_object,"see enemy"));
}

void stalker_low_cover_planner::add_actions				()
{
	CStalkerActionBase		*action;

	action					= xr_new<CStalkerActionGetReadyToKillLowCover>	(m_object,"get_ready_to_kill");
	add_condition			(action,eWorldPropertyReadyToKill,		false);
	add_effect				(action,eWorldPropertyReadyToKill,		true);
	add_operator			(eWorldOperatorGetReadyToKill,			action);

	action					= xr_new<CStalkerActionKillEnemyLowCover>		(m_object,"kill_enemy");
	add_condition			(action,eWorldPropertyReadyToKill,		true);
	add_condition			(action,eWorldPropertySeeEnemy,			true);
	add_effect				(action,eWorldPropertyLowCover,			false);
	add_operator			(eWorldOperatorKillEnemy,				action);

	action					= xr_new<CStalkerActionHoldPositionLowCover>	(m_object,"hold_position");
	add_condition			(action,eWorldPropertyReadyToKill,		true);
	add_condition			(action,eWorldPropertySeeEnemy,			false);
	add_effect				(action,eWorldPropertyLowCover,			false);
	add_operator			(eWorldOperatorHoldPosition,			action);
}
