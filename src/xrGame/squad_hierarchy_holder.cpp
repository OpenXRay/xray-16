////////////////////////////////////////////////////////////////////////////
//	Module 		: squad_hierarchy_holder.cpp
//	Created 	: 12.11.2001
//  Modified 	: 03.09.2004
//	Author		: Dmitriy Iassenev, Oles Shishkovtsov, Aleksandr Maksimchuk
//	Description : Squad hierarchy holder
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "squad_hierarchy_holder.h"
#include "group_hierarchy_holder.h"
#include "Common/object_broker.h"
#include "seniority_hierarchy_space.h"
#include "memory_space.h"

CSquadHierarchyHolder::~CSquadHierarchyHolder() { delete_data(m_groups); }
CGroupHierarchyHolder& CSquadHierarchyHolder::group(u32 group_id) const
{
    VERIFY3(group_id < max_group_count, "Group id is invalid : ", *SeniorityHierarchy::to_string(group_id));
    if (!m_groups[group_id])
        m_groups[group_id] = new CGroupHierarchyHolder(const_cast<CSquadHierarchyHolder*>(this));
    return (*m_groups[group_id]);
}

#ifdef SQUAD_HIERARCHY_HOLDER_USE_LEADER
void CSquadHierarchyHolder::update_leader()
{
    m_leader = 0;
    GROUP_REGISTRY::const_iterator I = m_groups.begin();
    GROUP_REGISTRY::const_iterator E = m_groups.end();
    for (; I != E; ++I)
        if (*I && (*I)->leader())
        {
            leader((*I)->leader());
            break;
        }
}
#endif // SQUAD_HIERARCHY_HOLDER_USE_LEADER
