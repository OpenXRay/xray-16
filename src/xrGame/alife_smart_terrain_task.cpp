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

//#define ENABLE_MISSING_PATROL_PATH_WORKAROUNDS

#ifdef ENABLE_MISSING_PATROL_PATH_WORKAROUNDS
// workarounds for mods
static const CPatrolPath* TryToNotCrash(const shared_str& patrol_path_name)
{
    const CPatrolPath* patrol_path = nullptr;

    const size_t sz = patrol_path_name.size();
    pcstr path_name = patrol_path_name.c_str();
    pcstr path_name_end = path_name + sz;
    Msg("! Cannot find %s. Executing workarounds...", path_name);
    do
    {
        // Imagine we have missing patrol path mil_monster_burrow_29_home_3
        // Try to fallback to mil_monster_burrow_29_home_2
        char const* almostEnd = strrchr(path_name, '_');
        if (!almostEnd || (++almostEnd == path_name_end))
            break;
        int number = strtol(almostEnd, nullptr, 10);
        if (number == 0 || errno == ERANGE)
            break;
        xr_string temp(path_name, almostEnd);
        temp += std::to_string(--number);

        patrol_path = const_cast<CPatrolPathStorage&>(ai().patrol_paths()).add_alias_if_exist(temp.c_str(), patrol_path_name);
    } while (false);

    if (!patrol_path)
    {
        Msg("! ... Workarounds didn't help.", path_name);
        // Let it crash.
        patrol_path = ai().patrol_paths().path(patrol_path_name); // the call is the same as in GetPatrolPath, but with assertion enabled
    }
    return patrol_path;
}

static const CPatrolPath* GetPatrolPath(const shared_str& patrol_path_name)
{
    const CPatrolPath* patrol_path = ai().patrol_paths().path(patrol_path_name, true); // assertion on error disabled
    VERIFY(patrol_path);
    if (!patrol_path) // engage workarounds
        patrol_path = TryToNotCrash(patrol_path_name);
    return patrol_path;
}

#else
static const CPatrolPath* GetPatrolPath(const shared_str& patrol_path_name)
{
    return ai().patrol_paths().path(patrol_path_name);
}
#endif // ENABLE_MISSING_PATROL_PATH_WORKAROUNDS

void CALifeSmartTerrainTask::setup_patrol_point(const shared_str& patrol_path_name, const u32& patrol_point_index)
{
    VERIFY(!m_patrol_point);

    const CPatrolPath* patrol_path = GetPatrolPath(patrol_path_name);
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
