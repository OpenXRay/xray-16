////////////////////////////////////////////////////////////////////////////
//	Module 		: seniority_hierarchy_holder.h
//	Created 	: 12.11.2001
//  Modified 	: 03.09.2004
//	Author		: Dmitriy Iassenev, Oles Shishkovtsov, Aleksandr Maksimchuk
//	Description : Seniority hierarchy holder
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "seniority_hierarchy_space.h"

class CTeamHierarchyHolder;

class CSeniorityHierarchyHolder {
private:
	enum {max_team_count = 64};

private:
	typedef svector<CTeamHierarchyHolder*,max_team_count> TEAM_REGISTRY;

private:
	TEAM_REGISTRY					m_teams;

public:
	IC								CSeniorityHierarchyHolder	();
	virtual							~CSeniorityHierarchyHolder	();
			CTeamHierarchyHolder	&team						(u32 team_id);
	IC		const TEAM_REGISTRY		&teams						() const;
};

#include "seniority_hierarchy_holder_inline.h"