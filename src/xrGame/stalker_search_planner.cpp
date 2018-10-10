////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_search_planner.cpp
//	Created 	: 31.05.2005
//  Modified 	: 31.05.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker search planner class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_search_planner.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_decision_space.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "stalker_property_evaluators.h"
#include "stalker_search_actions.h"

using namespace StalkerDecisionSpace;

CStalkerSearchPlanner::CStalkerSearchPlanner(CAI_Stalker* object, LPCSTR action_name) : inherited(object, action_name)
{
}

void CStalkerSearchPlanner::setup(CAI_Stalker* object, CPropertyStorage* storage)
{
    inherited::setup(object, storage);
    clear();
    add_evaluators();
    add_actions();
}

void CStalkerSearchPlanner::initialize()
{
    inherited::initialize();

    object().agent_manager().member().member(&object()).cover(0);

    CScriptActionPlanner::m_storage.set_property(eWorldPropertyEnemyLocationReached, false);
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyAmbushLocationReached, false);
}

void CStalkerSearchPlanner::update() { inherited::update(); }
void CStalkerSearchPlanner::finalize() { inherited::finalize(); }
void CStalkerSearchPlanner::add_evaluators()
{
    add_evaluator(eWorldPropertyPureEnemy, new CStalkerPropertyEvaluatorConst(true, "is_there_enemies_delayed"));
    add_evaluator(eWorldPropertyEnemyLocationReached,
        new CStalkerPropertyEvaluatorMember(
            (CPropertyStorage*)0, eWorldPropertyEnemyLocationReached, true, true, "enemy location reached"));
    add_evaluator(eWorldPropertyAmbushLocationReached,
        new CStalkerPropertyEvaluatorMember(
            (CPropertyStorage*)0, eWorldPropertyAmbushLocationReached, true, true, "ambush location reached"));
}

void CStalkerSearchPlanner::add_actions()
{
    CStalkerActionBase* action;

    action = new CStalkerActionReachEnemyLocation(
        m_object, CActionBase<CScriptGameObject>::m_storage, "reach enemy location");
    add_condition(action, eWorldPropertyEnemyLocationReached, false);
    add_effect(action, eWorldPropertyEnemyLocationReached, true);
    add_operator(eWorldOperatorReachEnemyLocation, action);

    action = new CStalkerActionReachAmbushLocation(
        m_object, CActionBase<CScriptGameObject>::m_storage, "reach ambush location");
    add_condition(action, eWorldPropertyEnemyLocationReached, true);
    add_condition(action, eWorldPropertyAmbushLocationReached, false);
    add_effect(action, eWorldPropertyAmbushLocationReached, true);
    add_operator(eWorldOperatorReachAmbushLocation, action);

    action = new CStalkerActionHoldAmbushLocation(
        m_object, CActionBase<CScriptGameObject>::m_storage, "hold ambush location");
    add_condition(action, eWorldPropertyAmbushLocationReached, true);
    add_effect(action, eWorldPropertyPureEnemy, false);
    add_operator(eWorldOperatorHoldAmbushLocation, action);
    action->set_inertia_time(15000);
}
