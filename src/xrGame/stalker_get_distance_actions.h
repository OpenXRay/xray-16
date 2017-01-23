////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_get_distance_actions.h
//	Created 	: 25.07.2007
//  Modified 	: 25.07.2007
//	Author		: Dmitriy Iassenev
//	Description : Stalker get distance action classes
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stalker_combat_actions.h"

//////////////////////////////////////////////////////////////////////////
// CStalkerActionRunToCover
//////////////////////////////////////////////////////////////////////////

class CStalkerActionRunToCover : public CStalkerActionCombatBase {
protected:
	typedef CStalkerActionCombatBase inherited;

public:
						CStalkerActionRunToCover	(CAI_Stalker *object, LPCSTR action_name = "");
	virtual void		initialize					();
	virtual void		execute						();
	virtual void		finalize					();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionWaitInCover
//////////////////////////////////////////////////////////////////////////

class CStalkerActionWaitInCover : public CStalkerActionCombatBase {
protected:
	typedef CStalkerActionCombatBase inherited;

public:
						CStalkerActionWaitInCover	(CAI_Stalker *object, LPCSTR action_name = "");
	virtual void		initialize					();
	virtual void		execute						();
	virtual void		finalize					();
};
