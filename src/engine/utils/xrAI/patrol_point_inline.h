////////////////////////////////////////////////////////////////////////////
//	Module 		: patrol_point_inline.h
//	Created 	: 15.06.2004
//  Modified 	: 15.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Patrol point inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	const Fvector &CPatrolPoint::position						() const
{
	VERIFY				(m_initialized);
	return				(m_position);
}

IC	const u32 &CPatrolPoint::flags								() const
{
	VERIFY				(m_initialized);
	return				(m_flags);
}

IC	const shared_str &CPatrolPoint::name						() const
{
	VERIFY				(m_initialized);
	return				(m_name);
}

IC	const u32 &CPatrolPoint::level_vertex_id					(const CLevelGraph *level_graph, const CGameLevelCrossTable *cross, const CGameGraph *game_graph) const
{
	VERIFY				(m_initialized);
#ifdef DEBUG
	verify_vertex_id	(level_graph,cross,game_graph);
#endif
	return				(m_level_vertex_id);
}

IC	const GameGraph::_GRAPH_ID &CPatrolPoint::game_vertex_id	(const CLevelGraph *level_graph, const CGameLevelCrossTable *cross, const CGameGraph *game_graph) const
{
	VERIFY				(m_initialized);
#ifdef DEBUG
	verify_vertex_id	(level_graph,cross,game_graph);
#endif
	return				(m_game_vertex_id);
}

#ifdef DEBUG
IC	void CPatrolPoint::path										(const CPatrolPath *path)
{
	VERIFY				(path);
	VERIFY				(!m_path);
	m_path				= path;
}
#endif
