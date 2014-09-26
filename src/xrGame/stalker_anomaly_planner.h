////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_anomaly_planner.h
//	Created 	: 25.03.2004
//  Modified 	: 27.09.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker anomaly planner
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "action_planner_action_script.h"

class CAI_Stalker;

class CStalkerAnomalyPlanner : public CActionPlannerActionScript<CAI_Stalker> {
private:
	typedef CActionPlannerActionScript<CAI_Stalker> inherited;

public:
						CStalkerAnomalyPlanner	(CAI_Stalker *object = 0, LPCSTR action_name = "");
	virtual				~CStalkerAnomalyPlanner	();
	virtual	void		setup					(CAI_Stalker *object, CPropertyStorage *storage);
	virtual	void		update					();
			void		add_evaluators			();
			void		add_actions				();
};
