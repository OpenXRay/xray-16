////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_get_distance_planner.h
//	Created 	: 25.07.2007
//  Modified 	: 25.07.2007
//	Author		: Dmitriy Iassenev
//	Description : Stalker get distance planner
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "action_planner_action_script.h"

class CAI_Stalker;

class CStalkerGetDistancePlanner : public CActionPlannerActionScript<CAI_Stalker> {
private:
	typedef CActionPlannerActionScript<CAI_Stalker>	inherited;

private:
			void	add_evaluators				();
			void	add_actions					();

public:
					CStalkerGetDistancePlanner	(CAI_Stalker *object = 0, LPCSTR action_name = "");
	virtual			~CStalkerGetDistancePlanner	();
	virtual	void	setup						(CAI_Stalker *object, CPropertyStorage *storage);
};
