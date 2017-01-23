////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_danger_unknown_actions.h
//	Created 	: 31.05.2005
//  Modified 	: 31.05.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker danger unknown actions classes
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stalker_combat_actions.h"

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerUnknownTakeCover
//////////////////////////////////////////////////////////////////////////

class CStalkerActionDangerUnknownTakeCover : public CStalkerActionCombatBase {
protected:
	typedef CStalkerActionCombatBase inherited;

private:
	bool				m_direction_sight;

public:
						CStalkerActionDangerUnknownTakeCover	(CAI_Stalker *object, LPCSTR action_name = "");
	virtual void		initialize								();
	virtual void		execute									();
	virtual void		finalize								();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerUnknownLookAround
//////////////////////////////////////////////////////////////////////////

class CStalkerActionDangerUnknownLookAround : public CStalkerActionCombatBase {
protected:
	typedef CStalkerActionCombatBase inherited;

public:
						CStalkerActionDangerUnknownLookAround	(CAI_Stalker *object, LPCSTR action_name = "");
	virtual void		initialize								();
	virtual void		execute									();
	virtual void		finalize								();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerUnknownSearch
//////////////////////////////////////////////////////////////////////////

class CStalkerActionDangerUnknownSearch : public CStalkerActionCombatBase {
protected:
	typedef CStalkerActionCombatBase inherited;

public:
						CStalkerActionDangerUnknownSearch	(CAI_Stalker *object, LPCSTR action_name = "");
	virtual void		initialize							();
	virtual void		execute								();
	virtual void		finalize							();
};
