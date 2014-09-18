////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_manager_planner.cpp
//	Created 	: 24.05.2004
//  Modified 	: 02.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Agent manager planner
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "agent_manager.h"
#include "agent_manager_space.h"
#include "agent_manager_actions.h"
#include "agent_manager_properties.h"
#include "agent_manager_planner.h"

using namespace AgentManager;

void CAgentManagerPlanner::setup	(CAgentManager *object)
{
	inherited::setup		(object);
	
	clear					();
	add_evaluators			();
	add_actions				();
	
	CWorldState				goal;
	goal.clear				();
	goal.add_condition		(CWorldProperty(ePropertyOrders,true));
	set_target_state		(goal);
}

void CAgentManagerPlanner::add_evaluators	()
{
	add_evaluator			(ePropertyOrders,xr_new<CAgentManagerPropertyEvaluatorConst>(false,"property_order"));
	add_evaluator			(ePropertyItem	,xr_new<CAgentManagerPropertyEvaluatorItem>(&object(),"property_item"));
	add_evaluator			(ePropertyEnemy	,xr_new<CAgentManagerPropertyEvaluatorEnemy>(&object(),"property_enemy"));
	add_evaluator			(ePropertyDanger,xr_new<CAgentManagerPropertyEvaluatorDanger>(&object(),"property_danger"));
}

void CAgentManagerPlanner::add_actions		()
{
	CAgentManagerActionBase	*action;

	action					= xr_new<CAgentManagerActionNoOrders>		(&object(),"no_orders");
	add_condition			(action,ePropertyOrders,			false);
	add_condition			(action,ePropertyItem,				false);
	add_condition			(action,ePropertyDanger,			false);
	add_condition			(action,ePropertyEnemy,				false);
	add_effect				(action,ePropertyOrders,			true);
	add_operator			(eOperatorNoOrders,					action);

	action					= xr_new<CAgentManagerActionGatherItems>	(&object(),"gather_items");
	add_condition			(action,ePropertyItem,				true);
	add_condition			(action,ePropertyEnemy,				false);
	add_condition			(action,ePropertyDanger,			false);
	add_effect				(action,ePropertyItem,				false);
	add_operator			(eOperatorGatherItem,				action);

	action					= xr_new<CAgentManagerActionKillEnemy>		(&object(),"kill_enemy");
	add_condition			(action,ePropertyEnemy,				true);
	add_effect				(action,ePropertyEnemy,				false);
	add_operator			(eOperatorKillEnemy,				action);

	action					= xr_new<CAgentManagerActionReactOnDanger>	(&object(),"react_on_danger");
	add_condition			(action,ePropertyEnemy,				false);
	add_condition			(action,ePropertyDanger,			true);
	add_effect				(action,ePropertyDanger,			false);
	add_operator			(eOperatorReactOnDanger,			action);
}

void CAgentManagerPlanner::remove_links		(CObject *object)
{
}
