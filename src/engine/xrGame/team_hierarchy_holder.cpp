////////////////////////////////////////////////////////////////////////////
//	Module 		: team_hierarchy_holder.cpp
//	Created 	: 12.11.2001
//  Modified 	: 03.09.2004
//	Author		: Dmitriy Iassenev, Oles Shishkovtsov, Aleksandr Maksimchuk
//	Description : Team hierarchy holder
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "team_hierarchy_holder.h"
#include "squad_hierarchy_holder.h"
#include "object_broker.h"
#include "seniority_hierarchy_space.h"

CTeamHierarchyHolder::~CTeamHierarchyHolder			()
{
	delete_data				(m_squads);
}

CSquadHierarchyHolder &CTeamHierarchyHolder::squad	(u32 squad_id) const
{
	VERIFY3					(squad_id < max_squad_count,"Squad id is invalid : ",*SeniorityHierarchy::to_string(squad_id));
	if (!m_squads[squad_id])
		m_squads[squad_id]	= xr_new<CSquadHierarchyHolder>(const_cast<CTeamHierarchyHolder*>(this));
	return					(*m_squads[squad_id]);
}
