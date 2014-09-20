////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_alife_task_actions.h
//	Created 	: 25.10.2004
//  Modified 	: 25.10.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker alife task action classes
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stalker_base_action.h"

class CAI_Trader;

//////////////////////////////////////////////////////////////////////////
// CStalkerActionSolveZonePuzzle
//////////////////////////////////////////////////////////////////////////

class CStalkerActionSolveZonePuzzle : public CStalkerActionBase {
protected:
	typedef CStalkerActionBase inherited;

protected:
	u32					m_stop_weapon_handling_time;

public:
						CStalkerActionSolveZonePuzzle	(CAI_Stalker *object, LPCSTR action_name = "");
	virtual void		initialize						();
	virtual void		execute							();
	virtual void		finalize						();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionSmartTerrain
//////////////////////////////////////////////////////////////////////////

class CStalkerActionSmartTerrain : public CStalkerActionBase {
protected:
	typedef CStalkerActionBase inherited;

public:
						CStalkerActionSmartTerrain		(CAI_Stalker *object, LPCSTR action_name = "");
	virtual void		initialize						();
	virtual void		execute							();
	virtual void		finalize						();
};
