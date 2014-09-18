////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_manager_actions.h
//	Created 	: 25.05.2004
//  Modified 	: 25.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Agent manager actions
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "action_base.h"

class CAgentManager;

typedef CActionBase<CAgentManager>	CAgentManagerActionBase;

//////////////////////////////////////////////////////////////////////////
// CAgentManagerActionNoOrders
//////////////////////////////////////////////////////////////////////////

class CAgentManagerActionNoOrders : public CAgentManagerActionBase {
protected:
	typedef CAgentManagerActionBase inherited;

public:
						CAgentManagerActionNoOrders	(CAgentManager *object, LPCSTR action_name = "");
	virtual void		finalize					();
};

//////////////////////////////////////////////////////////////////////////
// CAgentManagerActionGatherItems
//////////////////////////////////////////////////////////////////////////

class CAgentManagerActionGatherItems : public CAgentManagerActionBase {
protected:
	typedef CAgentManagerActionBase inherited;

public:
						CAgentManagerActionGatherItems	(CAgentManager *object, LPCSTR action_name = "");
};

//////////////////////////////////////////////////////////////////////////
// CAgentManagerActionKillEnemy
//////////////////////////////////////////////////////////////////////////

class CAgentManagerActionKillEnemy : public CAgentManagerActionBase {
protected:
	typedef CAgentManagerActionBase inherited;

public:
						CAgentManagerActionKillEnemy(CAgentManager *object, LPCSTR action_name = "");
	virtual void		initialize					();
	virtual void		finalize					();
	virtual void		execute						();
};

//////////////////////////////////////////////////////////////////////////
// CAgentManagerActionReactOnDanger
//////////////////////////////////////////////////////////////////////////

class CAgentManagerActionReactOnDanger : public CAgentManagerActionBase {
protected:
	typedef CAgentManagerActionBase inherited;

public:
						CAgentManagerActionReactOnDanger(CAgentManager *object, LPCSTR action_name = "");
	virtual void		initialize						();
	virtual void		execute							();
};
