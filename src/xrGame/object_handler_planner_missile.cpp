////////////////////////////////////////////////////////////////////////////
//	Module 		: object_handler_planner_missile.cpp
//	Created 	: 11.03.2004
//  Modified 	: 01.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Object handler action planner missile handling
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "object_handler_planner.h"
#include "object_property_evaluators.h"
#include "object_actions.h"
#include "object_handler_space.h"
#include "Missile.h"
#include "object_handler_planner_impl.h"
#include "ai/stalker/ai_stalker.h"

using namespace ObjectHandlerSpace;

void CObjectHandlerPlanner::add_evaluators(CMissile* missile)
{
    u16 id = missile->ID();
    // dynamic state properties
    add_evaluator(uid(id, eWorldPropertyHidden), new CObjectPropertyEvaluatorMissileHidden(missile, m_object));
    add_evaluator(uid(id, eWorldPropertyThrowStarted), new CObjectPropertyEvaluatorMissileStarted(missile, m_object));
    //	add_evaluator		(uid(id,eWorldPropertyThrowIdle)	,new
    // CObjectPropertyEvaluatorMissile(missile,m_object,MS_THROW));
    add_evaluator(
        uid(id, eWorldPropertyThrow), new CObjectPropertyEvaluatorMissile(missile, m_object, CMissile::eThrowEnd));

    // const properties
    add_evaluator(uid(id, eWorldPropertyDropped), new CObjectPropertyEvaluatorConst(false));
    add_evaluator(uid(id, eWorldPropertyFiring1), new CObjectPropertyEvaluatorConst(false));
    add_evaluator(uid(id, eWorldPropertyIdle), new CObjectPropertyEvaluatorConst(false));
    //	add_evaluator		(uid(id,eWorldPropertyAimingReady1)	,new CObjectPropertyEvaluatorConst(false));
    //	add_evaluator		(uid(id,eWorldPropertyStrapped)		,new CObjectPropertyEvaluatorConst(false));
}

void CObjectHandlerPlanner::add_operators(CMissile* missile)
{
    u16 id = missile->ID(), ff = u16(-1);
    CActionBase<CAI_Stalker>* action;

    // show
    action = new CObjectActionShow(missile, m_object, &m_storage, "show");
    add_condition(action, id, eWorldPropertyHidden, true);
    add_condition(action, ff, eWorldPropertyItemID, true);
    add_effect(action, ff, eWorldPropertyItemID, false);
    add_effect(action, id, eWorldPropertyHidden, false);
    add_operator(uid(id, eWorldOperatorShow), action);

    // hide
    action = new CObjectActionHide(missile, m_object, &m_storage, "hide");
    add_condition(action, id, eWorldPropertyHidden, false);
    add_condition(action, ff, eWorldPropertyItemID, false);
    add_effect(action, ff, eWorldPropertyItemID, true);
    add_effect(action, id, eWorldPropertyHidden, true);
    add_operator(uid(id, eWorldOperatorHide), action);

    // drop
    action = new CObjectActionDrop(missile, m_object, &m_storage, "drop");
    add_condition(action, id, eWorldPropertyHidden, false);
    add_effect(action, id, eWorldPropertyDropped, true);
    add_operator(uid(id, eWorldOperatorDrop), action);

    // idle
    action = new CObjectActionIdleMissile(missile, m_object, &m_storage, "idle");
    add_condition(action, id, eWorldPropertyHidden, false);
    add_effect(action, id, eWorldPropertyIdle, true);
    add_effect(action, id, eWorldPropertyThrowStarted, false);
    //	add_effect			(action,id,eWorldPropertyThrowIdle,	false);
    add_effect(action, id, eWorldPropertyFiring1, false);
    //	add_effect			(action,id,eWorldPropertyStrapped,	true);
    //	add_effect			(action,id,eWorldPropertyAimingReady1,true);
    add_operator(uid(id, eWorldOperatorIdle), action);

    // fire start
    action = new CObjectActionThrowMissile(missile, m_object, &m_storage, "throw start");
    add_condition(action, id, eWorldPropertyHidden, false);
    add_condition(action, id, eWorldPropertyThrowStarted, false);
    add_effect(action, id, eWorldPropertyThrowStarted, true);
    add_operator(uid(id, eWorldOperatorThrowStart), action);
    action->set_inertia_time(1500);

    // fire throw
    action = new CSObjectActionBase(missile, m_object, &m_storage, "throwing");
    add_condition(action, id, eWorldPropertyHidden, false);
    add_condition(action, id, eWorldPropertyThrowStarted, true);
    add_condition(action, id, eWorldPropertyThrow, false);
    add_effect(action, id, eWorldPropertyThrow, true);
    add_operator(uid(id, eWorldOperatorThrow), action);

    action = new CSObjectActionBase(missile, m_object, &m_storage, "threaten");
    add_condition(action, id, eWorldPropertyThrow, true);
    add_condition(action, id, eWorldPropertyFiring1, false);
    add_effect(action, id, eWorldPropertyFiring1, true);
    add_operator(uid(id, eWorldOperatorThreaten), action);

    this->action(uid(id, eWorldOperatorThrowIdle)).set_inertia_time(2000);
}
