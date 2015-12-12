////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_object_location.h
//	Created 	: 27.11.2003
//  Modified 	: 27.11.2003
//	Author		: Dmitriy Iassenev
//	Description : AI object location
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CAI_ObjectLocation::CAI_ObjectLocation							()
{
	init					();
}

IC	void CAI_ObjectLocation::reinit()
{
	init					();
}

IC	const GameGraph::_GRAPH_ID CAI_ObjectLocation::game_vertex_id	() const
{
	return					(m_game_vertex_id);
}

IC	const u32 CAI_ObjectLocation::level_vertex_id					() const
{
	return					(m_level_vertex_id);
}
