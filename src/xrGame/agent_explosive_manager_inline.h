////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_explosive_manager_inline.h
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Agent explosive manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CAgentExplosiveManager::CAgentExplosiveManager							(CAgentManager *object)
{
	VERIFY		(object);
	m_object	= object;
}

IC	CAgentManager &CAgentExplosiveManager::object							() const
{
	VERIFY		(m_object);
	return		(*m_object);
}

IC	CAgentExplosiveManager::EXPLOSIVES &CAgentExplosiveManager::explosives	()
{
	return		(m_explosives);
}
