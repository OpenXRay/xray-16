////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_alife_actions.h
//	Created 	: 25.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker alife action classes
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stalker_base_action.h"

//////////////////////////////////////////////////////////////////////////
// CStalkerActionGatherItems
//////////////////////////////////////////////////////////////////////////

class CStalkerActionGatherItems : public CStalkerActionBase {
protected:
	typedef CStalkerActionBase inherited;

public:
						CStalkerActionGatherItems	(CAI_Stalker *object, LPCSTR action_name = "");
	virtual void		initialize					();
	virtual void		execute						();
	virtual void		finalize					();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionNoALife
//////////////////////////////////////////////////////////////////////////

class CStalkerActionNoALife : public CStalkerActionBase {
protected:
	typedef CStalkerActionBase inherited;

protected:
	u32					m_stop_weapon_handling_time;

public:
						CStalkerActionNoALife		(CAI_Stalker *object, LPCSTR action_name = "");
	virtual void		initialize					();
	virtual void		execute						();
	virtual void		finalize					();
};
