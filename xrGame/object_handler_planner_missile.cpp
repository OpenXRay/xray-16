////////////////////////////////////////////////////////////////////////////
//	Module 		: object_handler_planner_missile.cpp
//	Created 	: 11.03.2004
//  Modified 	: 01.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Object handler action planner missile handling
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "object_handler_planner.h"
#include "object_property_evaluators.h"
#include "object_actions.h"
#include "object_handler_space.h"
#include "missile.h"
#include "object_handler_planner_impl.h"
#include "ai/stalker/ai_stalker.h"

using namespace ObjectHandlerSpace;

void CObjectHandlerPlanner::add_evaluators		(CMissile *missile)
{
	u16					id = missile->ID();
	// dynamic state properties
	add_evaluator		(uid(id,eWorldPropertyHidden)		,xr_new<CObjectPropertyEvaluatorMissileHidden>(missile,m_object));
	add_evaluator		(uid(id,eWorldPropertyThrowStarted)	,xr_new<CObjectPropertyEvaluatorMissileStarted>(missile,m_object));
//	add_evaluator		(uid(id,eWorldPropertyThrowIdle)	,xr_new<CObjectPropertyEvaluatorMissile>(missile,m_object,MS_THROW));
	add_evaluator		(uid(id,eWorldPropertyThrow)		,xr_new<CObjectPropertyEvaluatorMissile>(missile,m_object,CMissile::eThrowEnd));

	// const properties
	add_evaluator		(uid(id,eWorldPropertyDropped)		,xr_new<CObjectPropertyEvaluatorConst>(false));
	add_evaluator		(uid(id,eWorldPropertyFiring1)		,xr_new<CObjectPropertyEvaluatorConst>(false));
	add_evaluator		(uid(id,eWorldPropertyIdle)			,xr_new<CObjectPropertyEvaluatorConst>(false));
//	add_evaluator		(uid(id,eWorldPropertyAimingReady1)	,xr_new<CObjectPropertyEvaluatorConst>(false));
//	add_evaluator		(uid(id,eWorldPropertyStrapped)		,xr_new<CObjectPropertyEvaluatorConst>(false));
}

void CObjectHandlerPlanner::add_operators		(CMissile *missile)
{
	u16					id = missile->ID(), ff = u16(-1);
	CActionBase<CAI_Stalker>	*action;

	// show
	action				= xr_new<CObjectActionShow>(missile,m_object,&m_storage,"show");
	add_condition		(action,id,eWorldPropertyHidden,	true);
	add_condition		(action,ff,eWorldPropertyItemID,	true);
	add_effect			(action,ff,eWorldPropertyItemID,	false);
	add_effect			(action,id,eWorldPropertyHidden,	false);
	add_operator		(uid(id,eWorldOperatorShow),		action);

	// hide
	action				= xr_new<CObjectActionHide>(missile,m_object,&m_storage,"hide");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,ff,eWorldPropertyItemID,	false);
	add_effect			(action,ff,eWorldPropertyItemID,	true);
	add_effect			(action,id,eWorldPropertyHidden,	true);
	add_operator		(uid(id,eWorldOperatorHide),		action);

	// drop
	action				= xr_new<CObjectActionDrop>(missile,m_object,&m_storage,"drop");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_effect			(action,id,eWorldPropertyDropped,	true);
	add_operator		(uid(id,eWorldOperatorDrop),		action);

	// idle
	action				= xr_new<CObjectActionIdleMissile>(missile,m_object,&m_storage,"idle");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_effect			(action,id,eWorldPropertyIdle,		true);
	add_effect			(action,id,eWorldPropertyThrowStarted,false);
//	add_effect			(action,id,eWorldPropertyThrowIdle,	false);
	add_effect			(action,id,eWorldPropertyFiring1,	false);
//	add_effect			(action,id,eWorldPropertyStrapped,	true);
//	add_effect			(action,id,eWorldPropertyAimingReady1,true);
	add_operator		(uid(id,eWorldOperatorIdle),		action);

	// fire start
	action				= xr_new<CObjectActionThrowMissile>(missile,m_object,&m_storage,"throw start");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyThrowStarted,false);
	add_effect			(action,id,eWorldPropertyThrowStarted,true);
	add_operator		(uid(id,eWorldOperatorThrowStart),	action);
	action->set_inertia_time(1500);

	// fire throw
	action				= xr_new<CSObjectActionBase>(missile,m_object,&m_storage,"throwing");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyThrowStarted,	true);
	add_condition		(action,id,eWorldPropertyThrow,		false);
	add_effect			(action,id,eWorldPropertyThrow,		true);
	add_operator		(uid(id,eWorldOperatorThrow),		action);

	action				= xr_new<CSObjectActionBase>(missile,m_object,&m_storage,"threaten");
	add_condition		(action,id,eWorldPropertyThrow,		true);
	add_condition		(action,id,eWorldPropertyFiring1,	false);
	add_effect			(action,id,eWorldPropertyFiring1,	true);
	add_operator		(uid(id,eWorldOperatorThreaten),action);

	this->action(uid(id,eWorldOperatorThrowIdle)).set_inertia_time	(2000);
}
