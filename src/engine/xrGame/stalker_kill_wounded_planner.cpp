////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_kill_wounded_planner.cpp
//	Created 	: 25.05.2006
//  Modified 	: 25.05.2006
//	Author		: Dmitriy Iassenev
//	Description : Stalker kill wounded planner
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_kill_wounded_planner.h"
#include "stalker_kill_wounded_actions.h"
#include "ai/stalker/ai_stalker_space.h"
#include "stalker_decision_space.h"
#include "stalker_property_evaluators.h"
#include "script_game_object.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_movement_manager_smart_cover.h"
#include "memory_manager.h"
#include "enemy_manager.h"

using namespace StalkerSpace;
using namespace StalkerDecisionSpace;

CStalkerKillWoundedPlanner::CStalkerKillWoundedPlanner	(CAI_Stalker *object, LPCSTR action_name) :
	inherited			(object,action_name)
{
}

CStalkerKillWoundedPlanner::~CStalkerKillWoundedPlanner	()
{
}

void CStalkerKillWoundedPlanner::setup					(CAI_Stalker *object, CPropertyStorage *storage)
{
	inherited::setup		(object,storage);

	CScriptActionPlanner::m_storage.set_property	(eWorldPropertyWoundedEnemyPrepared,false);

	clear					();
	add_evaluators			();
	add_actions				();
}

void CStalkerKillWoundedPlanner::update					()
{
	inherited::update		();

//	if (current_action_id() == eWorldOperatorKillWoundedEnemy)
//		inherited_action::m_storage->set_property	(eWorldPropertyKilledWounded,true);
//	else
//		inherited_action::m_storage->set_property	(eWorldPropertyKilledWounded,false);
}

void CStalkerKillWoundedPlanner::initialize				()
{
	inherited::initialize	();

	CScriptActionPlanner::m_storage.set_property	(eWorldPropertyWoundedEnemyPrepared,false);
	CScriptActionPlanner::m_storage.set_property	(eWorldPropertyWoundedEnemyAimed,false);
	CScriptActionPlanner::m_storage.set_property	(eWorldPropertyPausedAfterKill,false);

	inherited_action::m_storage->set_property		(eWorldPropertyKilledWounded,true);
}

void CStalkerKillWoundedPlanner::execute				()
{
	inherited::execute		();
}

void CStalkerKillWoundedPlanner::finalize				()
{
	inherited::finalize		();

	if (object().memory().enemy().selected()) {
		inherited_action::m_storage->set_property	(eWorldPropertyKilledWounded,false);
		object().movement().set_mental_state		(MonsterSpace::eMentalStateDanger);
	}
}

void CStalkerKillWoundedPlanner::add_evaluators			()
{
	add_evaluator			(eWorldPropertyEnemy				,xr_new<CStalkerPropertyEvaluatorEnemies>		(m_object,"is_there_enemies_delayed"));
	add_evaluator			(eWorldPropertyWoundedEnemyReached	,xr_new<CStalkerPropertyEvaluatorEnemyReached>	(m_object,"is enemy reached"));
	
	add_evaluator			(eWorldPropertyWoundedEnemyPrepared	,xr_new<CStalkerPropertyEvaluatorMember>		((CPropertyStorage*)0,eWorldPropertyWoundedEnemyPrepared,true,true,"is enemy prepared"));
	add_evaluator			(eWorldPropertyWoundedEnemyAimed	,xr_new<CStalkerPropertyEvaluatorMember>		((CPropertyStorage*)0,eWorldPropertyWoundedEnemyAimed,true,true,"is enemy aimed"));
	add_evaluator			(eWorldPropertyPausedAfterKill		,xr_new<CStalkerPropertyEvaluatorMember>		((CPropertyStorage*)0,eWorldPropertyPausedAfterKill,true,true,"is paused after enemy kill"));
}

void CStalkerKillWoundedPlanner::add_actions			()
{
	CStalkerActionBase		*action;

	action					= xr_new<CStalkerActionReachWounded>	(m_object,"reach wounded enemy");
	add_condition			(action,eWorldPropertyPausedAfterKill,		false);
	add_condition			(action,eWorldPropertyEnemy,				true);
	add_condition			(action,eWorldPropertyWoundedEnemyReached,	false);
	add_effect				(action,eWorldPropertyWoundedEnemyReached,	true);
	add_operator			(eWorldOperatorReachWoundedEnemy,		action);

	action					= xr_new<CStalkerActionAimWounded>		(m_object,"aim at wounded enemy");
	add_condition			(action,eWorldPropertyPausedAfterKill,		false);
	add_condition			(action,eWorldPropertyWoundedEnemyReached,	true);
	add_condition			(action,eWorldPropertyWoundedEnemyAimed,	false);
	add_effect				(action,eWorldPropertyWoundedEnemyAimed,	true);
	add_operator			(eWorldOperatorAimWoundedEnemy,			action);
	action->set_inertia_time(1000);

	action					= xr_new<CStalkerActionPrepareWounded>	(m_object,"prepare wounded enemy");
	add_condition			(action,eWorldPropertyPausedAfterKill,		false);
	add_condition			(action,eWorldPropertyWoundedEnemyReached,	true);
	add_condition			(action,eWorldPropertyWoundedEnemyAimed,	true);
	add_condition			(action,eWorldPropertyWoundedEnemyPrepared,	false);
	add_effect				(action,eWorldPropertyWoundedEnemyPrepared,	true);
	add_operator			(eWorldOperatorPrepareWoundedEnemy,		action);

	action					= xr_new<CStalkerActionKillWounded>		(m_object,"kill wounded enemy");
	add_condition			(action,eWorldPropertyWoundedEnemyReached,	true);
	add_condition			(action,eWorldPropertyWoundedEnemyPrepared,	true);
	add_condition			(action,eWorldPropertyWoundedEnemyAimed,	true);
	add_effect				(action,eWorldPropertyEnemy,				false);
	add_operator			(eWorldOperatorKillWoundedEnemy,		action);

	action					= xr_new<CStalkerActionPauseAfterKill>	(m_object,"pause after kill");
	add_condition			(action,eWorldPropertyPausedAfterKill,		true);
	add_effect				(action,eWorldPropertyPausedAfterKill,		false);
	add_operator			(eWorldOperatorPauseAfterKill,			action);
	action->set_inertia_time(1000);
}
