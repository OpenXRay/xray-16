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
class CObject;

class CAgentManagerPlanner : public CActionPlanner<CAgentManager> {
private:
	typedef CActionPlanner<CAgentManager>	inherited;

public:
	virtual void		setup				(CAgentManager *object);
			void		add_evaluators		();
			void		add_actions			();
			void		remove_links		(CObject *object);
};
