////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_danger_in_direction_planner.h
//	Created 	: 31.05.2005
//  Modified 	: 31.05.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker danger in direction planner class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "action_planner_action_script.h"

class CAI_Stalker;

class CStalkerDangerInDirectionPlanner : public CActionPlannerActionScript<CAI_Stalker> {
private:
	typedef CActionPlannerActionScript<CAI_Stalker> inherited;

protected:
			void		add_evaluators						();
			void		add_actions							();

public:
						CStalkerDangerInDirectionPlanner	(CAI_Stalker *object = 0, LPCSTR action_name = "");
	virtual	void		setup								(CAI_Stalker *object, CPropertyStorage *storage);
	virtual void		initialize							();
	virtual void		update								();
	virtual void		finalize							();
};
