////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_kill_wounded_planner.h
//	Created 	: 25.05.2006
//  Modified 	: 25.05.2006
//	Author		: Dmitriy Iassenev
//	Description : Stalker kill wounded planner
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "action_planner_action_script.h"

class CAI_Stalker;

class CStalkerKillWoundedPlanner : public CActionPlannerActionScript<CAI_Stalker> {
private:
	typedef CActionPlannerActionScript<CAI_Stalker>	inherited;

private:
			void	add_evaluators				();
			void	add_actions					();

public:
					CStalkerKillWoundedPlanner	(CAI_Stalker *object = 0, LPCSTR action_name = "");
	virtual			~CStalkerKillWoundedPlanner	();
	virtual	void	setup						(CAI_Stalker *object, CPropertyStorage *storage);
	virtual void	update						();
	virtual void	initialize					();
	virtual void	execute						();
	virtual void	finalize					();
};
