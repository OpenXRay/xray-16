////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_danger_grenade_actions.h
//	Created 	: 31.05.2005
//  Modified 	: 31.05.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker danger grenade actions classes
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stalker_combat_actions.h"

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerGrenadeTakeCover
//////////////////////////////////////////////////////////////////////////

class CStalkerActionDangerGrenadeTakeCover : public CStalkerActionCombatBase {
protected:
	typedef CStalkerActionCombatBase inherited;

public:
						CStalkerActionDangerGrenadeTakeCover	(CAI_Stalker *object, LPCSTR action_name = "");
	virtual void		initialize									();
	virtual void		execute										();
	virtual void		finalize									();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerGrenadeWaitForExplosion
//////////////////////////////////////////////////////////////////////////

class CStalkerActionDangerGrenadeWaitForExplosion : public CStalkerActionCombatBase {
protected:
	typedef CStalkerActionCombatBase inherited;

public:
						CStalkerActionDangerGrenadeWaitForExplosion		(CAI_Stalker *object, LPCSTR action_name = "");
	virtual void		initialize									();
	virtual void		execute										();
	virtual void		finalize									();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerGrenadeTakeCoverAfterExplosion
//////////////////////////////////////////////////////////////////////////

class CStalkerActionDangerGrenadeTakeCoverAfterExplosion : public CStalkerActionCombatBase {
protected:
	typedef CStalkerActionCombatBase inherited;

private:
	bool				m_direction_sight;

public:
						CStalkerActionDangerGrenadeTakeCoverAfterExplosion	(CAI_Stalker *object, LPCSTR action_name = "");
	virtual void		initialize									();
	virtual void		execute										();
	virtual void		finalize									();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerGrenadeLookAround
//////////////////////////////////////////////////////////////////////////

class CStalkerActionDangerGrenadeLookAround : public CStalkerActionCombatBase {
protected:
	typedef CStalkerActionCombatBase inherited;

public:
						CStalkerActionDangerGrenadeLookAround	(CAI_Stalker *object, LPCSTR action_name = "");
	virtual void		initialize									();
	virtual void		execute										();
	virtual void		finalize									();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerGrenadeSearch
//////////////////////////////////////////////////////////////////////////

class CStalkerActionDangerGrenadeSearch : public CStalkerActionCombatBase {
protected:
	typedef CStalkerActionCombatBase inherited;

public:
						CStalkerActionDangerGrenadeSearch	(CAI_Stalker *object, LPCSTR action_name = "");
	virtual void		initialize								();
	virtual void		execute									();
	virtual void		finalize								();
};
