////////////////////////////////////////////////////////////////////////////
//	Module 		: squad_hierarchy_holder_inline.h
//	Created 	: 12.11.2001
//  Modified 	: 03.09.2004
//	Author		: Dmitriy Iassenev, Oles Shishkovtsov, Aleksandr Maksimchuk
//	Description : Squad hierarchy holder inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CSquadHierarchyHolder::CSquadHierarchyHolder									(CTeamHierarchyHolder *team)
{
	VERIFY						(team);
	m_team						= team;
#ifdef SQUAD_HIERARCHY_HOLDER_USE_LEADER
	m_leader					= 0;
#endif // SQUAD_HIERARCHY_HOLDER_USE_LEADER
	SeniorityHierarchy::assign_svector	(m_groups,max_group_count,0);
}

#ifdef SQUAD_HIERARCHY_HOLDER_USE_LEADER
IC	CEntity	*CSquadHierarchyHolder::leader											() const
{
	return						(m_leader);
}
#endif // SQUAD_HIERARCHY_HOLDER_USE_LEADER

IC	CTeamHierarchyHolder &CSquadHierarchyHolder::team								() const
{
	VERIFY						(m_team);
	return						(*m_team);
}

#ifdef SQUAD_HIERARCHY_HOLDER_USE_LEADER
IC	void CSquadHierarchyHolder::leader												(CEntity *leader)
{
	m_leader					= leader;
}
#endif // SQUAD_HIERARCHY_HOLDER_USE_LEADER

IC	const SquadHierarchyHolder::GROUP_REGISTRY &CSquadHierarchyHolder::groups		() const
{
	return						(m_groups);
}
