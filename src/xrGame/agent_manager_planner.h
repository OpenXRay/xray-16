////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_manager_planner.h
//	Created 	: 24.05.2004
//  Modified 	: 02.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Agent manager planner
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "action_planner.h"

class CAgentManager;
class IGameObject;

class CAgentManagerPlanner : public CActionPlanner<CAgentManager>
{
protected:
    using inherited = CActionPlanner<CAgentManager>;

public:
    virtual void setup(CAgentManager* object);
    void add_evaluators();
    void add_actions();
    void remove_links(IGameObject* object);
};
