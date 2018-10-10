////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_danger_grenade_planner.cpp
//	Created 	: 31.05.2005
//  Modified 	: 31.05.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker danger grenade planner class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_danger_grenade_planner.h"
#include "ai/stalker/ai_stalker.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "stalker_danger_grenade_actions.h"
#include "stalker_decision_space.h"
#include "stalker_danger_property_evaluators.h"

using namespace StalkerDecisionSpace;

CStalkerDangerGrenadePlanner::CStalkerDangerGrenadePlanner(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerDangerGrenadePlanner::setup(CAI_Stalker* object, CPropertyStorage* storage)
{
    inherited::setup(object, storage);
    clear();
    add_evaluators();
    add_actions();
}

void CStalkerDangerGrenadePlanner::initialize()
{
    inherited::initialize();

    CScriptActionPlanner::m_storage.set_property(eWorldPropertyCoverReached, false);
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyLookedAround, false);
}

void CStalkerDangerGrenadePlanner::update() { inherited::update(); }
void CStalkerDangerGrenadePlanner::finalize() { inherited::finalize(); }
void CStalkerDangerGrenadePlanner::add_evaluators()
{
    add_evaluator(eWorldPropertyDanger, new CStalkerPropertyEvaluatorDangers(m_object, "danger"));
    add_evaluator(eWorldPropertyCoverActual,
        new CStalkerPropertyEvaluatorDangerUnknownCoverActual(m_object, "danger grenade : cover actual"));
    add_evaluator(
        eWorldPropertyCoverReached, new CStalkerPropertyEvaluatorMember((CPropertyStorage*)0,
                                        eWorldPropertyCoverReached, true, true, "danger grenade : cover reached"));
    add_evaluator(eWorldPropertyGrenadeExploded,
        new CStalkerPropertyEvaluatorDangerGrenadeExploded(m_object, "grenade exploded"));
    add_evaluator(
        eWorldPropertyLookedAround, new CStalkerPropertyEvaluatorMember((CPropertyStorage*)0,
                                        eWorldPropertyLookedAround, true, true, "danger grenade : looked around"));
}

void CStalkerDangerGrenadePlanner::add_actions()
{
    CStalkerActionBase* action;

    action = new CStalkerActionDangerGrenadeTakeCover(m_object, "take cover");
    add_effect(action, eWorldPropertyCoverActual, true);
    add_effect(action, eWorldPropertyCoverReached, true);
    add_operator(eWorldOperatorDangerGrenadeTakeCover, action);

    action = new CStalkerActionDangerGrenadeWaitForExplosion(m_object, "wait for explosion");
    add_condition(action, eWorldPropertyCoverActual, true);
    add_condition(action, eWorldPropertyCoverReached, true);
    add_condition(action, eWorldPropertyGrenadeExploded, false);
    add_effect(action, eWorldPropertyGrenadeExploded, true);
    add_operator(eWorldOperatorDangerGrenadeWaitForExplosion, action);

    action = new CStalkerActionDangerGrenadeTakeCoverAfterExplosion(m_object, "take cover after explosion");
    add_condition(action, eWorldPropertyGrenadeExploded, true);
    add_effect(action, eWorldPropertyCoverActual, true);
    add_effect(action, eWorldPropertyCoverReached, true);
    add_operator(eWorldOperatorDangerGrenadeTakeCoverAfterExplosion, action);

    action = new CStalkerActionDangerGrenadeLookAround(m_object, "look around");
    add_condition(action, eWorldPropertyGrenadeExploded, true);
    add_condition(action, eWorldPropertyCoverActual, true);
    add_condition(action, eWorldPropertyCoverReached, true);
    add_condition(action, eWorldPropertyLookedAround, false);
    add_effect(action, eWorldPropertyLookedAround, true);
    add_operator(eWorldOperatorDangerGrenadeLookAround, action);

    action = new CStalkerActionDangerGrenadeSearch(m_object, "search");
    add_condition(action, eWorldPropertyGrenadeExploded, true);
    add_condition(action, eWorldPropertyCoverActual, true);
    add_condition(action, eWorldPropertyCoverReached, true);
    add_condition(action, eWorldPropertyLookedAround, true);
    add_effect(action, eWorldPropertyDanger, false);
    add_operator(eWorldOperatorDangerGrenadeSearch, action);
}
