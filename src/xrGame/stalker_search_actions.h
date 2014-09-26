////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_search_actions.h
//	Created 	: 25.03.2004
//  Modified 	: 08.10.2007
//	Author		: Dmitriy Iassenev
//	Description : stalker search enemy action classes
////////////////////////////////////////////////////////////////////////////

#ifndef STALKER_SEARCH_ACTIONS_H_INCLUDED
#define STALKER_SEARCH_ACTIONS_H_INCLUDED

#include "stalker_combat_action_base.h"

class CCoverPoint;

//////////////////////////////////////////////////////////////////////////
// CStalkerActionReachEnemyLocation
//////////////////////////////////////////////////////////////////////////

class CStalkerActionReachEnemyLocation : public CStalkerActionCombatBase {
private:
	CPropertyStorage	*m_combat_storage;
	u32					m_last_hit_time;

protected:
	typedef CStalkerActionCombatBase inherited;

public:
					CStalkerActionReachEnemyLocation(CAI_Stalker *object, CPropertyStorage *combat_storage, LPCSTR action_name = "");
	virtual void	initialize						();
	virtual void	execute							();
	virtual void	finalize						();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionReachAmbushLocation
//////////////////////////////////////////////////////////////////////////

class CStalkerActionReachAmbushLocation : public CStalkerActionCombatBase {
private:
	CPropertyStorage	*m_combat_storage;
	u32					m_last_hit_time;

protected:
	typedef CStalkerActionCombatBase inherited;

public:
					CStalkerActionReachAmbushLocation	(CAI_Stalker *object, CPropertyStorage *combat_storage, LPCSTR action_name = "");
	virtual void	initialize						();
	virtual void	execute							();
	virtual void	finalize						();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionHoldAmbushLocation
//////////////////////////////////////////////////////////////////////////

class CStalkerActionHoldAmbushLocation : public CStalkerActionCombatBase {
private:
	CPropertyStorage	*m_combat_storage;
	u32					m_last_hit_time;

protected:
	typedef CStalkerActionCombatBase inherited;

public:
					CStalkerActionHoldAmbushLocation(CAI_Stalker *object, CPropertyStorage *combat_storage, LPCSTR action_name = "");
	virtual void	initialize						();
	virtual void	execute							();
	virtual void	finalize						();
};

#endif // STALKER_SEARCH_ACTIONS_H_INCLUDED