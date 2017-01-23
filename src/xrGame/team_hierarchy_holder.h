////////////////////////////////////////////////////////////////////////////
//	Module 		: team_hierarchy_holder.h
//	Created 	: 12.11.2001
//  Modified 	: 03.09.2004
//	Author		: Dmitriy Iassenev, Oles Shishkovtsov, Aleksandr Maksimchuk
//	Description : Team hierarchy holder
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "seniority_hierarchy_space.h"

class CSquadHierarchyHolder;
class CSeniorityHierarchyHolder;

class CTeamHierarchyHolder {
private:
	enum {max_squad_count = 256};

private:
	typedef svector<CSquadHierarchyHolder*,max_squad_count> SQUAD_REGISTRY;

private:
	CSeniorityHierarchyHolder			*m_seniority_manager;
	mutable SQUAD_REGISTRY				m_squads;

public:
	IC									CTeamHierarchyHolder	(CSeniorityHierarchyHolder *m_seniority_manager);
	virtual								~CTeamHierarchyHolder	();
			CSquadHierarchyHolder		&squad					(u32 squad_id) const;
	IC		CSeniorityHierarchyHolder	&team					() const;
	IC		const SQUAD_REGISTRY		&squads					() const;
};

#include "team_hierarchy_holder_inline.h"