////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_smart_terrain_task.cpp
//	Created 	: 20.09.2005
//  Modified 	: 20.09.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife smart terrain task
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_smart_terrain_task.h"
#include "ai_space.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path_storage.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path.h"
#include "xrAICore/Navigation/PatrolPath/patrol_point.h"

#include "xrAICore/Navigation/level_graph.h"
#include "xrAICore/Navigation/game_graph.h"

void CALifeSmartTerrainTask::setup_patrol_point(const shared_str& patrol_path_name, const u32& patrol_point_index)
{
    VERIFY(!m_patrol_point);

    const CPatrolPath* patrol_path = ai().patrol_paths().path(patrol_path_name);
    VERIFY(patrol_path);

    m_patrol_point = &patrol_path->vertex(patrol_point_index)->data();
    VERIFY(m_patrol_point);
}

GameGraph::_GRAPH_ID CALifeSmartTerrainTask::game_vertex_id() const
{
    if (m_game_vertex_id == GameGraph::_GRAPH_ID(-1))
    {
        VERIFY3(ai().game_graph().valid_vertex_id(patrol_point().game_vertex_id()), *m_patrol_path_name,
            *m_patrol_point->name());
        return (patrol_point().game_vertex_id());
    }
    else
    {
        VERIFY(ai().game_graph().valid_vertex_id(m_game_vertex_id));
        return m_game_vertex_id;
    }
}

u32 CALifeSmartTerrainTask::level_vertex_id() const
{
    if (m_level_vertex_id == u32(-1))
    {
        VERIFY3(ai().game_graph().valid_vertex_id(patrol_point().game_vertex_id()), *m_patrol_path_name,
            *m_patrol_point->name());
        return (patrol_point().level_vertex_id());
    }
    else
    {
        VERIFY2(ai().game_graph().valid_vertex_id(m_game_vertex_id),
            make_string("Vertex [%d] is not valid!!!", m_game_vertex_id));
        return m_level_vertex_id;
    }
}

Fvector CALifeSmartTerrainTask::position() const
{
    if (m_level_vertex_id == u32(-1))
    {
        return (patrol_point().position());
    }
    else
    {
        if (ai().game_graph().vertex(m_game_vertex_id)->level_id() == ai().level_graph().level_id())
            return (ai().level_graph().vertex_position(m_level_vertex_id));
        else
            return ai().game_graph().vertex(m_game_vertex_id)->level_point();
    }
}
