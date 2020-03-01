////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_monster_patrol_path_manager.h
//	Created 	: 01.11.2005
//  Modified 	: 22.11.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife monster patrol path manager class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Navigation/game_graph_space.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path.h"

class CMovementManagerHolder;
class CPatrolPath;

class CALifeMonsterPatrolPathManager
{
public:
    typedef CMovementManagerHolder object_type;
    typedef GameGraph::_GRAPH_ID _GRAPH_ID;

private:
    object_type* m_object;

private:
    const CPatrolPath* m_path;
    EPatrolStartType m_start_type;
    EPatrolRouteType m_route_type;
    bool m_use_randomness;
    u32 m_start_vertex_index;

private:
    bool m_actual;
    bool m_completed;
    u32 m_current_vertex_index;
    u32 m_previous_vertex_index;

private:
    void select_nearest();
    void actualize();
    bool location_reached() const;
    void navigate();

public:
    CALifeMonsterPatrolPathManager(object_type* object);
    void update();
    void path(const shared_str& path_name);

public:
    IC object_type& object() const;
    IC void path(const CPatrolPath* path);
    IC void path(LPCSTR path_name);
    IC void start_type(const EPatrolStartType& start_type);
    IC void route_type(const EPatrolRouteType& route_type);
    IC const EPatrolStartType& start_type() const;
    IC const EPatrolRouteType& route_type() const;
    IC bool actual() const;
    IC bool completed() const;
    IC const CPatrolPath& path() const;
    IC void start_vertex_index(const u32& start_vertex_index);
    IC bool use_randomness() const;
    IC void use_randomness(const bool& use_randomness);
    const _GRAPH_ID& target_game_vertex_id() const;
    const u32& target_level_vertex_id() const;
    const Fvector& target_position() const;
};

#include "alife_monster_patrol_path_manager_inline.h"
