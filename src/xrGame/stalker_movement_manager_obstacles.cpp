////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_movement_manager_obstacles.cpp
//	Created 	: 27.03.2007
//  Modified 	: 27.03.2007
//	Author		: Dmitriy Iassenev
//	Description : Stalker movement manager: dynamic obstacles avoidance
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_movement_manager_obstacles.h"
#include "stalker_movement_manager_space.h"
#include "ai_space.h"
#include "ai/stalker/ai_stalker.h"
#include "restricted_object_obstacle.h"
#include "level_path_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "detail_path_manager.h"
#include "stalker_animation_manager.h"
#include "doors_actor.h"
#include "doors_manager.h"
#include "level_path_builder.h"

#ifndef MASTER_GOLD
#include "ai_debug.h"
#endif // MASTER_GOLD

static const u32 fail_check_time = 1000;

stalker_movement_manager_obstacles::stalker_movement_manager_obstacles(CAI_Stalker* object)
    : inherited(object), m_last_dest_vertex_id(u32(-1)), m_last_fail_time(0), m_failed_to_build_path(false)
{
    m_doors_actor = new doors::actor(*object);
    m_static_obstacles.construct(this, m_failed_to_build_path);
    m_dynamic_obstacles.construct(this, m_failed_to_build_path);
}

stalker_movement_manager_obstacles::~stalker_movement_manager_obstacles() { xr_delete(m_doors_actor); }
void stalker_movement_manager_obstacles::Load(LPCSTR section)
{
    inherited::Load(section);

    level_path_builder().use_delay_after_fail(false);
}

CRestrictedObject* stalker_movement_manager_obstacles::create_restricted_object()
{
    m_restricted_object =
        new CRestrictedObjectObstacle(&object(), m_static_obstacles.active_query(), m_dynamic_obstacles.active_query());

    return (m_restricted_object);
}

void stalker_movement_manager_obstacles::rebuild_path()
{
    level_path().make_inactual();

    // building level path
    set_build_path_at_once();
    update_path();

    // building detail path
    set_build_path_at_once();
    update_path();
}

bool stalker_movement_manager_obstacles::apply_border(const obstacles_query& query)
{
    u32 start_vertex_id = object().ai_location().level_vertex_id();
    u32 dest_vertex_id = level_path().dest_vertex_id();
    CLevelGraph& graph = ai().level_graph();

    restricted_object().CRestrictedObject::add_border(start_vertex_id, dest_vertex_id);

    AREA::const_iterator B = query.area().begin(), I = B;
    AREA::const_iterator E = query.area().end();
    for (; I != E; ++I)
    {
        if ((*I == dest_vertex_id))
            continue;

        if (*I == start_vertex_id)
            continue;

        graph.set_mask_no_check(*I);
    }

    return (true);
}

void stalker_movement_manager_obstacles::remove_border(const obstacles_query& query)
{
    restricted_object().CRestrictedObject::remove_border();
    CLevelGraph& graph = ai().level_graph();
    AREA::const_iterator I = query.area().begin();
    AREA::const_iterator E = query.area().end();
    for (; I != E; ++I)
        graph.clear_mask_no_check(*I);
}

bool stalker_movement_manager_obstacles::can_build_restricted_path(const obstacles_query& query)
{
    if (!apply_border(query))
    {
        VERIFY(!restricted_object().applied());
        return (false);
    }

    typedef SBaseParameters<float, u32, u32> evaluator_type;

    m_failed_to_build_path = !ai().graph_engine().search(ai().level_graph(), object().ai_location().level_vertex_id(),
        level_path().dest_vertex_id(), &m_temp_path, evaluator_type(type_max<_dist_type>, _iteration_type(-1), 4096));

    remove_border(query);

    VERIFY(!restricted_object().applied());

    return (!m_failed_to_build_path);
}

void stalker_movement_manager_obstacles::move_along_path_impl(
    CPHMovementControl* movement_control, Fvector& dest_position, float time_delta)
{
#ifndef MASTER_GOLD
    if (psAI_Flags.test(aiObstaclesAvoidingStatic))
#endif // MASTER_GOLD
    {
        m_dynamic_obstacles.update();
        if (!m_dynamic_obstacles.movement_enabled())
        {
            float desirable_speed = old_desirable_speed();
            set_desirable_speed(0.f);

            inherited::move_along_path(movement_control, dest_position, time_delta);

            set_desirable_speed(desirable_speed);
            return;
        }
    }

    m_static_obstacles.update();

    if (
#ifndef MASTER_GOLD
        (!psAI_Flags.test(aiObstaclesAvoidingStatic) && m_dynamic_obstacles.need_path_to_rebuild()) ||
#endif // MASTER_GOLD
        m_static_obstacles.need_path_to_rebuild())
        rebuild_path();

    inherited::move_along_path(movement_control, dest_position, time_delta);
}

void stalker_movement_manager_obstacles::move_along_path(
    CPHMovementControl* movement_control, Fvector& dest_position, float time_delta)
{
    VERIFY(m_doors_actor);

    if (!ai().doors().actualize_doors_state(*m_doors_actor, old_desirable_speed()))
    {
        //		Msg							( "%6d stalker %s waits for the some door to be open/closed",
        //Device.dwTimeGlobal,
        // object().cName().c_str() );
        float desirable_speed = old_desirable_speed();
        set_desirable_speed(0.f);

        inherited::move_along_path(movement_control, dest_position, time_delta);

        set_desirable_speed(desirable_speed);
        return;
    }

//	Msg								( "%6d stalker %s is going", Device.dwTimeGlobal, object().cName().c_str() );

#ifndef MASTER_GOLD
    if (!psAI_Flags.test(aiObstaclesAvoiding))
    {
        inherited::move_along_path(movement_control, dest_position, time_delta);
        return;
    }
#endif // MASTER_GOLD

    if (Device.dwTimeGlobal < (m_last_fail_time + fail_check_time))
    {
        inherited::move_along_path(movement_control, dest_position, time_delta);
        return;
    }

    if (!move_along_path())
    {
        inherited::move_along_path(movement_control, dest_position, time_delta);
        return;
    }

    move_along_path_impl(movement_control, dest_position, time_delta);
}

const float& stalker_movement_manager_obstacles::prediction_speed() const
{
    return (object().animation().target_speed());
}

void stalker_movement_manager_obstacles::remove_links(IGameObject* object)
{
    inherited::remove_links(object);
    m_static_obstacles.remove_links(object);
    m_dynamic_obstacles.remove_links(object);
}

static float get_distance(Fvector const& a_first, Fvector const& a_second, Fvector const& b_first,
    Fvector const& b_second, float const safe_distance)
{
    Fvector2 intersection;
    switch (ai().level_graph().intersect(a_first.x, a_first.z, a_second.x, a_second.z, b_first.x, b_first.z, b_second.x,
        b_second.z, &intersection.x, &intersection.y))
    {
    case LevelGraph::eLineIntersectionEqual: return 0.f;

    case LevelGraph::eLineIntersectionCollinear:
    {
        Fvector2 const as_af =
            Fvector2().sub(Fvector2().set(a_second.x, a_second.z), Fvector2().set(a_first.x, a_first.z));
        Fvector2 const bf_af =
            Fvector2().sub(Fvector2().set(b_first.x, b_first.z), Fvector2().set(a_first.x, a_first.z));
        float const as_af_magnitude = as_af.magnitude();
        float const bf_af_magnitude = bf_af.magnitude();
        Fvector2 const as_af_dir = Fvector2(as_af).div(as_af_magnitude);
        float const signed_distance = as_af_dir.dotproduct(bf_af);
        float const distance = _sqrt(_sqr(bf_af_magnitude) + _sqr(signed_distance));
        if (distance >= safe_distance)
            return -1.f;

        return 0.f;
    }

    case LevelGraph::eLineIntersectionIntersect: return intersection.distance_to(Fvector2().set(a_first.x, a_first.z));

    default: NODEFAULT;
    }

#ifdef DEBUG
    return -1.f;
#endif // #ifdef DEBUG
}

float stalker_movement_manager_obstacles::is_going_through(
    Fmatrix const& matrix, Fvector const& vector, float const max_distance) const
{
    if (!actual())
        return -1.f;

    if (path().empty())
        return -1.f;

    if (detail().curr_travel_point_index() >= detail().path().size() - 1)
        return -1.f;

    Fvector start_position = matrix.c;
    Fvector stop_position;
    matrix.transform_tiny(stop_position, vector);

    float current_distance = 0.f;
    float min_distance = flt_max;
    typedef xr_vector<STravelPathPoint> detail_path_type;
    detail_path_type::const_iterator i = detail().path().begin() + detail().curr_travel_point_index() + 1;
    detail_path_type::const_iterator e = detail().path().end();
    for (; i != e; ++i)
    {
        float const distance = get_distance((*(i - 1)).position, (*i).position, start_position, stop_position, .35f);
        min_distance = distance > -1.f ? std::min(min_distance, distance) : min_distance;

        current_distance += (*(i - 1)).position.distance_to((*i).position);
        if (current_distance > max_distance)
            return min_distance == flt_max ? -1.f : min_distance;
    }

    return min_distance == flt_max ? -1.f : min_distance;
}

void stalker_movement_manager_obstacles::on_death()
{
    VERIFY(m_doors_actor);
    xr_delete(m_doors_actor);
}
