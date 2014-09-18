////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_manager_actions.cpp
//	Created 	: 25.05.2004
//  Modified 	: 25.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Agent manager actions
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "agent_manager_actions.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "agent_location_manager.h"
#include "agent_corpse_manager.h"
#include "agent_explosive_manager.h"
#include "agent_enemy_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "sight_action.h"
#include "inventory.h"

//////////////////////////////////////////////////////////////////////////
// CAgentManagerActionNoOrders
//////////////////////////////////////////////////////////////////////////

CAgentManagerActionNoOrders::CAgentManagerActionNoOrders	(CAgentManager *object, LPCSTR action_name) :
	inherited		(object,action_name)
{
}

void CAgentManagerActionNoOrders::finalize			()
{
	inherited::finalize				();
	m_object->corpse().clear		();
}

//////////////////////////////////////////////////////////////////////////
// CAgentManagerActionGatherItems
//////////////////////////////////////////////////////////////////////////

CAgentManagerActionGatherItems::CAgentManagerActionGatherItems	(CAgentManager *object, LPCSTR action_name) :
	inherited		(object,action_name)
{
}

//////////////////////////////////////////////////////////////////////////
// CAgentManagerActionKillEnemy
//////////////////////////////////////////////////////////////////////////

CAgentManagerActionKillEnemy::CAgentManagerActionKillEnemy	(CAgentManager *object, LPCSTR action_name) :
	inherited		(object,action_name)
{
}

void CAgentManagerActionKillEnemy::initialize		()
{
	inherited::initialize						();
	
	m_object->location().clear					();
}

void CAgentManagerActionKillEnemy::finalize			()
{
	inherited::finalize							();
	
//	m_object->enemy().distribute_enemies		();
}

void CAgentManagerActionKillEnemy::execute			()
{
	inherited::execute							();

	m_object->enemy().distribute_enemies		();
	m_object->explosive().react_on_explosives	();
	m_object->corpse().react_on_member_death	();
}

//////////////////////////////////////////////////////////////////////////
// CAgentManagerActionReactOnDanger
//////////////////////////////////////////////////////////////////////////

CAgentManagerActionReactOnDanger::CAgentManagerActionReactOnDanger	(CAgentManager *object, LPCSTR action_name) :
	inherited		(object,action_name)
{
}

void CAgentManagerActionReactOnDanger::initialize		()
{
	inherited::initialize			();

	m_object->location().clear		();
}

void CAgentManagerActionReactOnDanger::execute			()
{
	inherited::execute							();

	m_object->explosive().react_on_explosives	();
	m_object->corpse().react_on_member_death	();
}
