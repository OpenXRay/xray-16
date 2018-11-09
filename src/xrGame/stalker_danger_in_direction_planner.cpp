////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_danger_in_direction_planner.cpp
//	Created 	: 31.05.2005
//  Modified 	: 31.05.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker danger in direction planner class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_danger_in_direction_planner.h"
#include "ai/stalker/ai_stalker.h"
#include "ai/stalker/ai_stalker_impl.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "stalker_danger_in_direction_actions.h"
#include "stalker_decision_space.h"
#include "stalker_danger_property_evaluators.h"
#include "agent_manager.h"
#include "agent_member_manager.h"

using namespace StalkerDecisionSpace;

CStalkerDangerInDirectionPlanner::CStalkerDangerInDirectionPlanner(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerDangerInDirectionPlanner::setup(CAI_Stalker* object, CPropertyStorage* storage)
{
    inherited::setup(object, storage);
    clear();
    add_evaluators();
    add_actions();
}

void CStalkerDangerInDirectionPlanner::initialize()
{
    inherited::initialize();

    object().agent_manager().member().member(&object()).cover(0);

    CScriptActionPlanner::m_storage.set_property(eWorldPropertyInCover, false);
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyLookedOut, false);
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyPositionHolded, false);
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyEnemyDetoured, false);
}

void CStalkerDangerInDirectionPlanner::update() { inherited::update(); }
void CStalkerDangerInDirectionPlanner::finalize() { inherited::finalize(); }
void CStalkerDangerInDirectionPlanner::add_evaluators()
{
    add_evaluator(eWorldPropertyDanger, new CStalkerPropertyEvaluatorDangers(m_object, "danger"));
    add_evaluator(eWorldPropertyInCover,
        new CStalkerPropertyEvaluatorMember((CPropertyStorage*)0, eWorldPropertyInCover, true, true, "in cover"));
    add_evaluator(eWorldPropertyLookedOut,
        new CStalkerPropertyEvaluatorMember((CPropertyStorage*)0, eWorldPropertyLookedOut, true, true, "looked out"));
    add_evaluator(eWorldPropertyPositionHolded, new CStalkerPropertyEvaluatorMember((CPropertyStorage*)0,
                                                    eWorldPropertyPositionHolded, true, true, "position is held"));
    add_evaluator(eWorldPropertyEnemyDetoured, new CStalkerPropertyEvaluatorMember((CPropertyStorage*)0,
                                                   eWorldPropertyEnemyDetoured, true, true, "danger is detoured"));
}

void CStalkerDangerInDirectionPlanner::add_actions()
{
    CStalkerActionBase* action;

    action = new CStalkerActionDangerInDirectionTakeCover(m_object, "take cover");
    add_condition(action, eWorldPropertyInCover, false);
    add_effect(action, eWorldPropertyInCover, true);
    add_operator(eWorldOperatorDangerInDirectionTakeCover, action);

    action = new CStalkerActionDangerInDirectionLookOut(m_object, "look out");
    add_condition(action, eWorldPropertyInCover, true);
    add_condition(action, eWorldPropertyLookedOut, false);
    add_effect(action, eWorldPropertyLookedOut, true);
    add_operator(eWorldOperatorDangerInDirectionLookOut, action);

    action = new CStalkerActionDangerInDirectionHoldPosition(m_object, "hold position");
    add_condition(action, eWorldPropertyLookedOut, true);
    add_condition(action, eWorldPropertyPositionHolded, false);
    add_effect(action, eWorldPropertyPositionHolded, true);
    add_operator(eWorldOperatorDangerInDirectionHoldPosition, action);

    action = new CStalkerActionDangerInDirectionDetour(m_object, "detour");
    add_condition(action, eWorldPropertyPositionHolded, true);
    add_condition(action, eWorldPropertyEnemyDetoured, false);
    add_effect(action, eWorldPropertyEnemyDetoured, true);
    add_operator(eWorldOperatorDangerInDirectionDetourEnemy, action);

    action = new CStalkerActionDangerInDirectionSearch(m_object, "search");
    add_condition(action, eWorldPropertyEnemyDetoured, true);
    add_effect(action, eWorldPropertyDanger, false);
    add_operator(eWorldOperatorDangerInDirectionSearchEnemy, action);
}
