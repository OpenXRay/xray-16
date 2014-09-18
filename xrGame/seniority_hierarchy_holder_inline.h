////////////////////////////////////////////////////////////////////////////
//	Module 		: seniority_hierarchy_holder_inline.h
//	Created 	: 12.11.2001
//  Modified 	: 03.09.2004
//	Author		: Dmitriy Iassenev, Oles Shishkovtsov, Aleksandr Maksimchuk
//	Description : Seniority hierarchy holder inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CSeniorityHierarchyHolder::CSeniorityHierarchyHolder								()
{
	SeniorityHierarchy::assign_svector	(m_teams,max_team_count,0);
}

IC	const CSeniorityHierarchyHolder::TEAM_REGISTRY &CSeniorityHierarchyHolder::teams	() const
{
	return								(m_teams);
}
