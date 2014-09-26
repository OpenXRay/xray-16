////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_corpse_manager_inline.h
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Agent corpse manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CAgentCorpseManager::CAgentCorpseManager	(CAgentManager *object)
{
	VERIFY						(object);
	m_object					= object;
}

IC	CAgentManager &CAgentCorpseManager::object	() const
{
	VERIFY						(m_object);
	return						(*m_object);
}

IC	void CAgentCorpseManager::register_corpse	(CAI_Stalker *corpse)
{
	MEMBER_CORPSES::iterator	I = std::find(m_corpses.begin(),m_corpses.end(),corpse);
	VERIFY2						(I == m_corpses.end(),"Cannot register corpse more than a time!");
	m_corpses.push_back			(CMemberCorpse(corpse,0,Device.dwTimeGlobal));
}

IC	CAgentCorpseManager::MEMBER_CORPSES &CAgentCorpseManager::corpses	()
{
	return						(m_corpses);
}

IC	void CAgentCorpseManager::clear				()
{
	m_corpses.clear				();
}
