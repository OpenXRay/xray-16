////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_movement_manager_obstacles_path.cpp
//	Created 	: 18.04.2007
//  Modified 	: 18.04.2007
//	Author		: Dmitriy Iassenev
//	Description : Stalker movement manager: dynamic obstacles avoidance: build path
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_movement_manager_obstacles.h"
#include "stalker_movement_manager_space.h"
#include "ai_space.h"
#include "ai/stalker/ai_stalker.h"
#include "restricted_object_obstacle.h"
#include "level_path_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "moving_objects.h"
#include "detail_path_manager.h"
#include "level_path_builder.h"
#include "ai_obstacle.h"

#ifndef MASTER_GOLD
#include "ai_debug.h"
#endif // MASTER_GOLD

static const float check_time_delta = 1.f;

bool stalker_movement_manager_obstacles::simulate_path_navigation()
{
    Fvector current_position = object().Position();
    Fvector previous_position = current_position;
    u32 current_travel_point = 0;
    while (!detail().completed(current_position, !detail().state_patrol_path(), current_travel_point))
    {
        m_static_obstacles.on_before_query();
        m_static_obstacles.query(current_position, previous_position);

        if (!m_static_obstacles.process_query(false))
        {
            m_last_fail_time = Device.dwTimeGlobal;
            m_failed_to_build_path = true;
            restore_current_state();
            return (false);
        }

        if (m_static_obstacles.need_path_to_rebuild())
            return (false);

        //		float						dist_to_target;
        //		Fvector						dir_to_target;
        //		float						distance;
        //		current_position			=
        // path_position(1.f,current_position,check_time_delta,current_travel_point,distance,dist_to_target,dir_to_target);
        previous_position = current_position;
        current_position = predict_position(check_time_delta, current_position, current_travel_point, 1.f);
    }

    return (true);
}

void stalker_movement_manager_obstacles::save_current_state()
{
    m_saved_state = false;

    if (level_path().path().empty())
        return;

    if (level_path().path().back() != level_path_builder().dest_vertex_id())
        return;

    if (detail().path().empty())
        return;

    if (detail().dest_vertex_id() != level_path_builder().dest_vertex_id())
        return;

    m_saved_state = true;
    m_level_path.swap(level_path_path());
    m_detail_current_index = detail().path().empty() ? u32(-1) : detail().curr_travel_point_index();
    m_detail_path.swap(detail().path());
#ifdef DEBUG
    m_detail_key_points.swap(detail().key_points());
#endif // DEBUG
    m_detail_last_patrol_point = detail().last_patrol_point();
    m_saved_current_iteration.copy(m_static_obstacles.current_iteration());
}

void stalker_movement_manager_obstacles::restore_current_state()
{
    if (!m_saved_state)
        return;

    m_level_path.swap(level_path_path());
    m_detail_path.swap(detail().path());
    detail().m_current_travel_point = m_detail_current_index;
#ifdef DEBUG
    m_detail_key_points.swap(detail().key_points());
#endif // DEBUG
    detail().last_patrol_point(m_detail_last_patrol_point);
    m_saved_current_iteration.swap(m_static_obstacles.current_iteration());
}

IC void stalker_movement_manager_obstacles::remove_query_objects(const Fvector& position, const float& radius)
{
    m_static_obstacles.inactive_query().remove_objects(position, radius);
    m_static_obstacles.active_query().remove_objects(position, radius);
}

void stalker_movement_manager_obstacles::build_level_path()
{
#ifndef MASTER_GOLD
    if (!psAI_Flags.test(aiObstaclesAvoiding))
    {
        inherited::build_level_path();
        return;
    }
#endif // MASTER_GOLD

#ifdef DEBUG
    CTimer timer;
    timer.Start();
#endif // DEBUG

    if (m_last_dest_vertex_id != level_path().dest_vertex_id())
        remove_query_objects(object().Position(), 5.f);

    m_last_fail_time = 0;

    m_failed_to_build_path = false;
    //	Msg								("[%6d] m_failed_to_build_path = %s
    //(stalker_movement_manager_obstacles::build_level_path)",Device.dwTimeGlobal,m_failed_to_build_path ? "true" :
    //"false");

    save_current_state();
    m_static_obstacles.inactive_query().copy(m_static_obstacles.active_query());
    m_static_obstacles.inactive_query().update_objects(object().Position(), 10000.f);

#ifndef MASTER_GOLD
    if (!psAI_Flags.test(aiObstaclesAvoidingStatic))
        m_dynamic_obstacles.inactive_query().copy(m_dynamic_obstacles.active_query());
#endif // MASTER_GOLD

    bool pure_search_tried = false;
    bool pure_search_result = false;

    do
    {
        if (m_failed_to_build_path)
            break;

        inherited::build_level_path();

        if (level_path().failed())
        {
            if (!pure_search_tried)
            {
                pure_search_tried = true;

                m_static_obstacles.clear();
                m_saved_current_iteration.clear();

                level_path().invalidate_failed_info();

                inherited::build_level_path();

                pure_search_result = !level_path().failed();
            }

            if (!pure_search_result)
            {
#ifndef MASTER_GOLD
                Msg("! level_path().failed() during navigation");
#endif // #ifndef MASTER_GOLD
                break;
            }
        }
    } while (!simulate_path_navigation());

    m_last_dest_vertex_id = level_path().dest_vertex_id();
    //	Msg								("[%6d][%6d][%s][%f]
    // build_level_path",Device.dwFrame,Device.dwTimeGlobal,*object().cName(),timer.GetElapsed_sec()*1000.f);
}
