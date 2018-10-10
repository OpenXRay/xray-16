////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_object_location.h
//	Created 	: 27.11.2003
//  Modified 	: 27.11.2003
//	Author		: Dmitriy Iassenev
//	Description : AI object location
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Navigation/game_graph_space.h"

namespace LevelGraph
{
class CVertex;
};

class CAI_ObjectLocation
{
public:
    using _GRAPH_ID = GameGraph::_GRAPH_ID;
    using CVertex = GameGraph::CVertex;
    using CLevelVertex = LevelGraph::CVertex;

private:
    u32 m_level_vertex_id;
    _GRAPH_ID m_game_vertex_id;

public:
    IC CAI_ObjectLocation();
    IC void init();
    IC void reinit();
    IC void game_vertex(CVertex const* game_vertex);
    IC void game_vertex(_GRAPH_ID const& game_vertex_id);
    IC const CVertex* game_vertex() const;
    IC const _GRAPH_ID game_vertex_id() const;
    IC void level_vertex(CLevelVertex const* level_vertex);
    IC void level_vertex(u32 const& level_vertex_id);
    IC const CLevelVertex* level_vertex() const;
    IC const u32 level_vertex_id() const;
};

#include "ai_object_location_inline.h"
