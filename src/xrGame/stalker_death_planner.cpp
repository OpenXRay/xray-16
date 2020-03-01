////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_death_planner.cpp
//	Created 	: 25.03.2004
//  Modified 	: 27.09.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker death planner
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_death_planner.h"
#include "stalker_death_actions.h"
#include "stalker_decision_space.h"
#include "stalker_property_evaluators.h"
#include "ai/stalker/ai_stalker.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"

using namespace StalkerDecisionSpace;

CStalkerDeathPlanner::CStalkerDeathPlanner(CAI_Stalker* object, LPCSTR action_name) : inherited(object, action_name) {}
CStalkerDeathPlanner::~CStalkerDeathPlanner() {}
void CStalkerDeathPlanner::setup(CAI_Stalker* object, CPropertyStorage* storage)
{
    inherited::setup(object, storage);

    CScriptActionPlanner::m_storage.set_property(eWorldPropertyDead, false);

    clear();
    add_evaluators();
    add_actions();
}

void CStalkerDeathPlanner::add_evaluators()
{
    add_evaluator(eWorldPropertyPuzzleSolved, new CStalkerPropertyEvaluatorConst(false, "resurrecting"));
    add_evaluator(eWorldPropertyDead,
        new CStalkerPropertyEvaluatorMember((CPropertyStorage*)0, eWorldPropertyDead, true, true, "completely dead"));
}

void CStalkerDeathPlanner::add_actions()
{
    CStalkerActionBase* action;

    action = new CStalkerActionDead(m_object, "dying");
    add_condition(action, eWorldPropertyDead, false);
    add_effect(action, eWorldPropertyDead, true);
    add_operator(eWorldOperatorDying, action);

    action = new CStalkerActionBase(m_object, "dead");
    add_condition(action, eWorldPropertyDead, true);
    add_effect(action, eWorldPropertyPuzzleSolved, true);
    add_operator(eWorldOperatorDead, action);
}
