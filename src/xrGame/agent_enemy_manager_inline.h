////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_enemy_manager_inline.h
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Agent enemy manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CAgentEnemyManager::CAgentEnemyManager						(CAgentManager *object)
{
	VERIFY				(object);
	m_object			= object;
	m_only_wounded_left	= false;
	m_is_any_wounded	= false;
}

IC	CAgentManager &CAgentEnemyManager::object					() const
{
	VERIFY				(m_object);
	return				(*m_object);
}

IC	CAgentEnemyManager::ENEMIES	&CAgentEnemyManager::enemies	()
{
	return				(m_enemies);
}
