////////////////////////////////////////////////////////////////////////////
//	Module 		: patrol_point.cpp
//	Created 	: 15.06.2004
//  Modified 	: 15.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Patrol point
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "patrol_point.h"
#include "Navigation/level_graph.h"
#include "Navigation/level_graph.h"
#include "Navigation/game_level_cross_table.h"
#include "Navigation/game_graph.h"
#include "Common/object_broker.h"
#include "xrScriptEngine/DebugMacros.hpp"
#include "AISpaceBase.hpp"
#ifdef DEBUG
#include "Navigation/PatrolPath/patrol_path.h"
#endif

CPatrolPoint::CPatrolPoint(const CPatrolPath* path)
    : m_flags(0), m_level_vertex_id(u32(-1)), m_game_vertex_id(GameGraph::_GRAPH_ID(-1))
#ifdef DEBUG
    , m_initialized(false), m_path(path)
#endif
{
}

#ifdef DEBUG
void CPatrolPoint::verify_vertex_id(
    const CLevelGraph* level_graph, const CGameLevelCrossTable* cross, const CGameGraph* game_graph) const
{
    if (!level_graph)
        return;

    if (level_graph->valid_vertex_id(m_level_vertex_id))
    {
        return;
    }

    VERIFY(m_path);
    string1024 temp;
    xr_sprintf(temp, "\n! Patrol point %s in path %s is not on the level graph vertex!", *m_name, *m_path->m_name);
    THROW2(level_graph->valid_vertex_id(m_level_vertex_id), temp);
}
#endif

IC void CPatrolPoint::correct_position(
    const CLevelGraph* level_graph, const CGameLevelCrossTable* cross, const CGameGraph* game_graph)
{
    if (!level_graph || !level_graph->valid_vertex_position(position()) ||
        !level_graph->valid_vertex_id(m_level_vertex_id))
        return;

    if (!level_graph->inside(level_vertex_id(level_graph, cross, game_graph), position()))
        m_position = level_graph->vertex_position(level_vertex_id(level_graph, cross, game_graph));

    m_game_vertex_id = cross->vertex(level_vertex_id(level_graph, cross, game_graph)).game_vertex_id();
}

CPatrolPoint::CPatrolPoint(const CLevelGraph* level_graph, const CGameLevelCrossTable* cross,
    const CGameGraph* game_graph, const CPatrolPath* path, const Fvector& position, u32 level_vertex_id, u32 flags,
    shared_str name)
    : m_name(name)
{
#ifdef DEBUG
    VERIFY(path);
    m_path = path;
#endif
    m_position = position;
    m_level_vertex_id = level_vertex_id;
    m_flags = flags;
#ifdef DEBUG
    m_initialized = true;
#endif
    correct_position(level_graph, cross, game_graph);
}

CPatrolPoint& CPatrolPoint::load_raw(
    const CLevelGraph* level_graph, const CGameLevelCrossTable* cross, const CGameGraph* game_graph, IReader& stream)
{
    stream.r_fvector3(m_position);
    m_flags = stream.r_u32();
    stream.r_stringZ(m_name);
    if (level_graph && level_graph->valid_vertex_position(m_position))
    {
        Fvector position = m_position;
        position.y += .15f;
        m_level_vertex_id = level_graph->vertex_id(position);
    }
    else
        m_level_vertex_id = u32(-1);
#ifdef DEBUG
    m_initialized = true;
#endif
    correct_position(level_graph, cross, game_graph);
    return (*this);
}

void CPatrolPoint::load(IReader& stream)
{
    load_data(m_name, stream);
    load_data(m_position, stream);
    load_data(m_flags, stream);
    load_data(m_level_vertex_id, stream);
    load_data(m_game_vertex_id, stream);

#ifdef DEBUG
    m_initialized = true;
#endif
}

void CPatrolPoint::save(IWriter& stream)
{
    save_data(m_name, stream);
    save_data(m_position, stream);
    save_data(m_flags, stream);
    save_data(m_level_vertex_id, stream);
    save_data(m_game_vertex_id, stream);
}

const u32& CPatrolPoint::level_vertex_id() const
{
    const CGameGraph& gameGraph = GEnv.AISpace->game_graph();
    const CLevelGraph& levelGraph = GEnv.AISpace->level_graph();
    if (gameGraph.vertex(m_game_vertex_id)->level_id() == levelGraph.level_id())
        return level_vertex_id(&levelGraph, &gameGraph.cross_table(), &gameGraph);
    return m_level_vertex_id;
}

const GameGraph::_GRAPH_ID& CPatrolPoint::game_vertex_id() const
{
    const CGameGraph& gameGraph = GEnv.AISpace->game_graph();
    const CLevelGraph& levelGraph = GEnv.AISpace->level_graph();
    const CGameGraph::CGameVertex* vertex = gameGraph.vertex(m_game_vertex_id);
    VERIFY2(vertex,
        make_string(
            "invalid game vertex id[%d] (level_vertex_id[%d]) for patrol point[%s] in path[%s] in position[%f][%f][%f]",
            m_game_vertex_id, m_level_vertex_id, m_name.c_str(), m_path->m_name.c_str(), VPUSH(m_position)));
    if (vertex->level_id() == levelGraph.level_id())
        return game_vertex_id(&levelGraph, &gameGraph.cross_table(), &gameGraph);
    return m_game_vertex_id;
}
