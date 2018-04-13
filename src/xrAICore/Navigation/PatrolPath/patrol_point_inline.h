////////////////////////////////////////////////////////////////////////////
//	Module 		: patrol_point_inline.h
//	Created 	: 15.06.2004
//  Modified 	: 15.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Patrol point inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

inline bool CPatrolPoint::operator==(const CPatrolPoint& rhs) const
{
    R_ASSERT(!"not implemented");
    return false;
}

IC const Fvector& CPatrolPoint::position() const
{
#ifdef DEBUG
    VERIFY(m_initialized);
#endif
    return (m_position);
}

IC const u32& CPatrolPoint::flags() const
{
#ifdef DEBUG
    VERIFY(m_initialized);
#endif
    return (m_flags);
}

IC const shared_str& CPatrolPoint::name() const
{
#ifdef DEBUG
    VERIFY(m_initialized);
#endif
    return (m_name);
}

IC const u32& CPatrolPoint::level_vertex_id(
    const CLevelGraph* level_graph, const CGameLevelCrossTable* cross, const CGameGraph* game_graph) const
{
#ifdef DEBUG
    VERIFY(m_initialized);
    verify_vertex_id(level_graph, cross, game_graph);
#endif
    return (m_level_vertex_id);
}

IC const GameGraph::_GRAPH_ID& CPatrolPoint::game_vertex_id(
    const CLevelGraph* level_graph, const CGameLevelCrossTable* cross, const CGameGraph* game_graph) const
{
#ifdef DEBUG
    VERIFY(m_initialized);
    verify_vertex_id(level_graph, cross, game_graph);
#endif
    return (m_game_vertex_id);
}

#ifdef DEBUG
IC void CPatrolPoint::path(const CPatrolPath* path)
{
    VERIFY(path);
    VERIFY(!m_path);
    m_path = path;
}
#endif
