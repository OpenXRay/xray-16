////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_object_location_impl.h
//	Created 	: 27.11.2003
//  Modified 	: 27.11.2003
//	Author		: Dmitriy Iassenev
//	Description : AI object location implementation
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ai_object_location.h"
#include "ai_space.h"
#include "game_graph.h"
#include "level_graph.h"

IC	void CAI_ObjectLocation::init								()
{
	if (ai().get_level_graph())
		ai().level_graph().set_invalid_vertex	(m_level_vertex_id);
	else
		m_level_vertex_id	= u32(-1);

	if (ai().get_game_graph())
		ai().game_graph().set_invalid_vertex	(m_game_vertex_id);
	else
		m_game_vertex_id						= GameGraph::_GRAPH_ID(-1);
}

IC	void CAI_ObjectLocation::game_vertex						(CVertex  const *game_vertex)
{
	VERIFY				(ai().game_graph().valid_vertex_id(ai().game_graph().vertex_id(game_vertex)));
	m_game_vertex_id	= ai().game_graph().vertex_id(game_vertex);
}

IC	void CAI_ObjectLocation::game_vertex						(_GRAPH_ID const &game_vertex_id)
{
	VERIFY				(ai().game_graph().valid_vertex_id(game_vertex_id));
	m_game_vertex_id	= game_vertex_id;
}

IC	const CGameGraph::CVertex *CAI_ObjectLocation::game_vertex	() const
{
	VERIFY				(ai().game_graph().valid_vertex_id(m_game_vertex_id));
	return				(ai().game_graph().vertex(m_game_vertex_id));
}

IC	void CAI_ObjectLocation::level_vertex						(CLevelVertex  const *level_vertex)
{
	VERIFY				(ai().level_graph().valid_vertex_id(ai().level_graph().vertex_id(level_vertex)));
	m_level_vertex_id	= ai().level_graph().vertex_id(level_vertex);
}

IC	void CAI_ObjectLocation::level_vertex						(u32 const &level_vertex_id)
{
	VERIFY				(ai().level_graph().valid_vertex_id(level_vertex_id));
	m_level_vertex_id	= level_vertex_id;
}

IC	const CLevelGraph::CVertex *CAI_ObjectLocation::level_vertex() const
{
	VERIFY				(ai().level_graph().valid_vertex_id(m_level_vertex_id));
	return				(ai().level_graph().vertex(m_level_vertex_id));
}
