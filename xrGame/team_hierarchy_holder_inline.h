////////////////////////////////////////////////////////////////////////////
//	Module 		: team_hierarchy_holder_inline.h
//	Created 	: 12.11.2001
//  Modified 	: 03.09.2004
//	Author		: Dmitriy Iassenev, Oles Shishkovtsov, Aleksandr Maksimchuk
//	Description : Team hierarchy holder inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CTeamHierarchyHolder::CTeamHierarchyHolder									(CSeniorityHierarchyHolder *seniority_manager)
{
	VERIFY								(seniority_manager);
	m_seniority_manager					= seniority_manager;
	SeniorityHierarchy::assign_svector	(m_squads,max_squad_count,0);
}

IC	CSeniorityHierarchyHolder &CTeamHierarchyHolder::team						() const
{
	VERIFY								(m_seniority_manager);
	return								(*m_seniority_manager);
}

IC	const CTeamHierarchyHolder::SQUAD_REGISTRY &CTeamHierarchyHolder::squads	() const
{
	return								(m_squads);
}
