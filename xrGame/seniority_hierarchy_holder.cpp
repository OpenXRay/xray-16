////////////////////////////////////////////////////////////////////////////
//	Module 		: seniority_hierarchy_holder.cpp
//	Created 	: 12.11.2001
//  Modified 	: 03.09.2004
//	Author		: Dmitriy Iassenev, Oles Shishkovtsov, Aleksandr Maksimchuk
//	Description : Seniority hierarchy holder
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "seniority_hierarchy_holder.h"
#include "team_hierarchy_holder.h"
#include "object_broker.h"

CSeniorityHierarchyHolder::~CSeniorityHierarchyHolder	()
{
	delete_data				(m_teams);
}

CTeamHierarchyHolder &CSeniorityHierarchyHolder::team	(u32 team_id)
{
	VERIFY3					(team_id < max_team_count,"Team id is invalid : ",*SeniorityHierarchy::to_string(team_id));
	if (!m_teams[team_id])
		m_teams[team_id]	= xr_new<CTeamHierarchyHolder>(this);
	return					(*m_teams[team_id]);
}
