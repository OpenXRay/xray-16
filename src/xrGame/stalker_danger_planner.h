////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_danger_planner.h
//	Created 	: 11.02.2005
//  Modified 	: 11.02.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker danger planner
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "action_planner_action_script.h"

class CAI_Stalker;

class CStalkerDangerPlanner : public CActionPlannerActionScript<CAI_Stalker> {
private:
	typedef CActionPlannerActionScript<CAI_Stalker> inherited;

protected:
			void		add_evaluators			();
			void		add_actions				();

public:
						CStalkerDangerPlanner	(CAI_Stalker *object = 0, LPCSTR action_name = "");
	virtual	void		setup					(CAI_Stalker *object, CPropertyStorage *storage);
	virtual void		initialize				();
	virtual void		update					();
	virtual void		finalize				();
};

#include "stalker_danger_planner_inline.h"