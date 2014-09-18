////////////////////////////////////////////////////////////////////////////
//	Module 		: group_hierarchy_holder_inline.h
//	Created 	: 12.11.2001
//  Modified 	: 03.09.2004
//	Author		: Dmitriy Iassenev, Oles Shishkovtsov, Aleksandr Maksimchuk
//	Description : Group hierarchy holder inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CGroupHierarchyHolder::CGroupHierarchyHolder			(CSquadHierarchyHolder *squad)
{
	VERIFY				(squad);
	m_squad				= squad;
#ifdef SQUAD_HIERARCHY_HOLDER_USE_LEADER
	m_leader			= 0;
#endif // SQUAD_HIERARCHY_HOLDER_USE_LEADER
	m_visible_objects	= 0;
	m_sound_objects		= 0;
	m_hit_objects		= 0;
	m_agent_manager		= 0;
	m_dwLastActionTime	= 0;
	m_dwLastAction		= 0;
	m_dwActiveCount		= 0;
	m_dwAliveCount		= 0;
	m_dwStandingCount	= 0;
}

IC	CAgentManager &CGroupHierarchyHolder::agent_manager		() const
{
	VERIFY				(m_agent_manager);
	return				(*m_agent_manager);
}

IC	CAgentManager *CGroupHierarchyHolder::get_agent_manager	() const
{
	return				(m_agent_manager);
}

IC	const GroupHierarchyHolder::MEMBER_REGISTRY &CGroupHierarchyHolder::members	() const
{
	return				(m_members);
}

IC	CSquadHierarchyHolder &CGroupHierarchyHolder::squad		() const
{
	VERIFY				(m_squad);
	return				(*m_squad);
}

#ifdef SQUAD_HIERARCHY_HOLDER_USE_LEADER
IC	CEntity	*CGroupHierarchyHolder::leader					() const
{
	return				(m_leader);
}
#endif // SQUAD_HIERARCHY_HOLDER_USE_LEADER

IC	GroupHierarchyHolder::VISIBLE_OBJECTS &CGroupHierarchyHolder::visible_objects	() const
{
	VERIFY				(m_visible_objects);
	return				(*m_visible_objects);
}

IC	GroupHierarchyHolder::SOUND_OBJECTS &CGroupHierarchyHolder::sound_objects		() const
{
	VERIFY				(m_sound_objects);
	return				(*m_sound_objects);
}

IC	GroupHierarchyHolder::HIT_OBJECTS &CGroupHierarchyHolder::hit_objects			() const
{
	VERIFY				(m_hit_objects);
	return				(*m_hit_objects);
}
