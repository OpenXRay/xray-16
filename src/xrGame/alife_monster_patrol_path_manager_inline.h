////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_monster_patrol_path_manager_inline.h
//	Created 	: 01.11.2005
//  Modified 	: 22.11.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife monster patrol path manager class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC CALifeMonsterPatrolPathManager::object_type& CALifeMonsterPatrolPathManager::object() const
{
    VERIFY(m_object);
    return (*m_object);
}

IC void CALifeMonsterPatrolPathManager::path(const CPatrolPath* path)
{
    m_actual = m_actual && (m_path == path);
    m_path = path;
}

IC void CALifeMonsterPatrolPathManager::path(LPCSTR path_name) { path(shared_str(path_name)); }
IC bool CALifeMonsterPatrolPathManager::actual() const { return (m_actual); }
IC bool CALifeMonsterPatrolPathManager::completed() const { return (actual() && m_completed); }
IC void CALifeMonsterPatrolPathManager::start_type(const EPatrolStartType& start_type) { m_start_type = start_type; }
IC void CALifeMonsterPatrolPathManager::route_type(const EPatrolRouteType& route_type) { m_route_type = route_type; }
IC const EPatrolStartType& CALifeMonsterPatrolPathManager::start_type() const
{
    return (m_start_type);
}

IC const EPatrolRouteType& CALifeMonsterPatrolPathManager::route_type() const
{
    return (m_route_type);
}

IC const CPatrolPath& CALifeMonsterPatrolPathManager::path() const
{
    VERIFY(m_path);
    return (*m_path);
}

IC void CALifeMonsterPatrolPathManager::start_vertex_index(const u32& start_vertex_index)
{
    m_start_vertex_index = start_vertex_index;
}

IC bool CALifeMonsterPatrolPathManager::use_randomness() const { return (m_use_randomness); }
IC void CALifeMonsterPatrolPathManager::use_randomness(const bool& use_randomness)
{
    m_use_randomness = use_randomness;
}
