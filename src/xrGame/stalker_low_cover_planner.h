////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_low_cover_planner.h
//	Created 	: 04.09.2007
//  Modified 	: 04.09.2007
//	Author		: Dmitriy Iassenev
//	Description : Stalker low cover planner
////////////////////////////////////////////////////////////////////////////

#ifndef STALKER_LOW_COVER_PLANNER_H_INCLUDED
#define STALKER_LOW_COVER_PLANNER_H_INCLUDED

#include "action_planner_action_script.h"

class CAI_Stalker;

class stalker_low_cover_planner : public CActionPlannerActionScript<CAI_Stalker> {
private:
	typedef CActionPlannerActionScript<CAI_Stalker>	inherited;

private:
			void	add_evaluators				();
			void	add_actions					();

public:
					stalker_low_cover_planner	(CAI_Stalker *object = 0, LPCSTR action_name = "");
	virtual	void	setup						(CAI_Stalker *object, CPropertyStorage *storage);
	virtual void	update						();
	virtual void	initialize					();
	virtual void	execute						();
	virtual void	finalize					();
};

#endif // STALKER_LOW_COVER_PLANNER_H_INCLUDED