////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_monster_patrol_path_manager.cpp
//	Created 	: 01.11.2005
//  Modified 	: 22.11.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife monster patrol path manager class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_monster_patrol_path_manager.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "ai_space.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path_storage.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path.h"
#include "xrAICore/Navigation/PatrolPath/patrol_point.h"
#include "xrAICore/Navigation/game_graph.h"

CALifeMonsterPatrolPathManager::CALifeMonsterPatrolPathManager(object_type* object)
{
    VERIFY(object);
    m_object = object;

    m_path = 0;

    m_actual = true;
    m_completed = true;

    m_current_vertex_index = u32(-1);
    m_previous_vertex_index = u32(-1);
    m_start_vertex_index = u32(-1);

    start_type(ePatrolStartTypeNearest);
    route_type(ePatrolRouteTypeContinue);
    use_randomness(true);
}

void CALifeMonsterPatrolPathManager::path(const shared_str& path_name) { path(ai().patrol_paths().path(path_name)); }
const CALifeMonsterPatrolPathManager::_GRAPH_ID& CALifeMonsterPatrolPathManager::target_game_vertex_id() const
{
    return (path().vertex(m_current_vertex_index)->data().game_vertex_id());
}

const u32& CALifeMonsterPatrolPathManager::target_level_vertex_id() const
{
    return (path().vertex(m_current_vertex_index)->data().level_vertex_id());
}

const Fvector& CALifeMonsterPatrolPathManager::target_position() const
{
    return (path().vertex(m_current_vertex_index)->data().position());
}

void CALifeMonsterPatrolPathManager::select_nearest()
{
    m_current_vertex_index = u32(-1);
    Fvector global_position = ai().game_graph().vertex(object().get_object().m_tGraphID)->game_point();
    float best_distance = flt_max;
    CPatrolPath::const_vertex_iterator I = path().vertices().begin();
    CPatrolPath::const_vertex_iterator E = path().vertices().end();
    for (; I != E; ++I)
    {
        if ((*I).second->data().game_vertex_id() == object().get_object().m_tGraphID)
        {
            m_current_vertex_index = (*I).second->vertex_id();
            break;
        }

        float distance =
            global_position.distance_to(ai().game_graph().vertex((*I).second->data().game_vertex_id())->game_point());

        if (distance >= best_distance)
            continue;

        best_distance = distance;
        m_current_vertex_index = (*I).second->vertex_id();
    }

    VERIFY(m_current_vertex_index < path().vertices().size());
}

void CALifeMonsterPatrolPathManager::actualize()
{
    m_current_vertex_index = u32(-1);
    m_previous_vertex_index = u32(-1);
    m_actual = true;
    m_completed = false;

    switch (start_type())
    {
    case ePatrolStartTypeFirst:
    {
        m_current_vertex_index = 0;
        break;
    }
    case ePatrolStartTypeLast:
    {
        m_current_vertex_index = path().vertices().size() - 1;
        break;
    }
    case ePatrolStartTypeNearest:
    {
        select_nearest();
        break;
    }
    case ePatrolStartTypePoint:
    {
        m_current_vertex_index = m_start_vertex_index;
        break;
    }
    case ePatrolStartTypeNext:
    // we advisedly do not process this case since it is far-fetched
    default: NODEFAULT;
    };

    VERIFY(path().vertices().size() > m_current_vertex_index);
}

bool CALifeMonsterPatrolPathManager::location_reached() const
{
    if (object().get_object().m_tGraphID != target_game_vertex_id())
        return (false);

    if (object().get_object().m_tNodeID != target_level_vertex_id())
        return (false);

    return (true);
}

void CALifeMonsterPatrolPathManager::navigate()
{
    const CPatrolPath::CVertex& vertex = *path().vertex(m_current_vertex_index);

    typedef CPatrolPath::CVertex::EDGES EDGES;
    EDGES::const_iterator I = vertex.edges().begin(), B = I;
    EDGES::const_iterator E = vertex.edges().end();

    u32 branching_factor = 0;
    for (; I != E; ++I)
    {
        if (*I == m_previous_vertex_index)
            continue;

        ++branching_factor;
    }

    if (!branching_factor)
    {
        switch (route_type())
        {
        case ePatrolRouteTypeStop:
        {
            VERIFY(!m_completed);
            m_completed = true;
            break;
        };
        case ePatrolRouteTypeContinue:
        {
            if (vertex.edges().empty())
            {
                VERIFY(!m_completed);
                m_completed = true;
                break;
            }

            VERIFY(vertex.edges().size() == 1);
            VERIFY(vertex.edges().front().vertex_id() == m_previous_vertex_index);
            std::swap(m_current_vertex_index, m_previous_vertex_index);
            break;
        };
        default: NODEFAULT;
        };
    }

    u32 chosen = use_randomness() ? object().get_object().randI(branching_factor) : 0;
    u32 branch = 0;
    for (I = B; I != E; ++I)
    {
        if (*I == m_previous_vertex_index)
            continue;

        if (chosen == branch)
            break;

        ++branch;
    }

    VERIFY(I != E);
    m_previous_vertex_index = m_current_vertex_index;
    m_current_vertex_index = (*I).vertex_id();
}

void CALifeMonsterPatrolPathManager::update()
{
    if (!m_path)
        return;

    if (completed())
        return;

    if (!actual())
        actualize();

    if (!location_reached())
        return;

    navigate();
}
