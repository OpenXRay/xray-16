////////////////////////////////////////////////////////////////////////////
//	Module 		: movement_manager.cpp
//	Created 	: 02.10.2001
//  Modified 	: 12.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Movement manager
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "movement_manager.h"
#include "movement_manager_space.h"
#include "game_location_selector.h"
#include "level_location_selector.h"
#include "game_path_manager.h"
#include "level_path_manager.h"
#include "detail_path_manager.h"
#include "patrol_path_manager.h"
#include "xrMessages.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "CustomMonster.h"
#include "location_manager.h"
#include "level_path_builder.h"
#include "detail_path_builder.h"
#include "xrEngine/profiler.h"
#include "mt_config.h"
#include "xrNetServer/NET_Messages.h"

// Lain: added
#include "steering_behaviour.h"

using namespace MovementManager;

const float verify_distance = 15.f;

CMovementManager::CMovementManager(CCustomMonster* object)
{
    VERIFY(object);
    m_object = object;
}

CMovementManager::~CMovementManager()
{
    xr_delete(m_base_game_selector);
    xr_delete(m_base_level_selector);

    xr_delete(m_restricted_object);
    xr_delete(m_location_manager);

    xr_delete(m_game_location_selector);
    xr_delete(m_game_path_manager);
    xr_delete(m_level_path_manager);
    xr_delete(m_detail_path_manager);
    xr_delete(m_patrol_path_manager);

    xr_delete(m_level_path_builder);
    xr_delete(m_detail_path_builder);
}

void CMovementManager::Load(LPCSTR section)
{
    m_restricted_object = create_restricted_object();
    m_location_manager = new CLocationManager(m_object);

    m_base_game_selector = new CGameVertexParams(locations().vertex_types());
    m_base_level_selector = new CBaseParameters();

    m_game_location_selector = new CGameLocationSelector(m_restricted_object, m_location_manager);
    m_game_path_manager = new CGamePathManager(m_restricted_object);
    m_level_path_manager = new CLevelPathManager(m_restricted_object);
    m_detail_path_manager = new CDetailPathManager(m_restricted_object);
    m_patrol_path_manager = new CPatrolPathManager(m_restricted_object, m_object);

    m_level_path_builder = new CLevelPathBuilder(this);
    m_detail_path_builder = new CDetailPathBuilder(this);

    extrapolate_path(false);

    m_wait_for_distributed_computation = false;

    locations().Load(section);
}

void CMovementManager::reinit()
{
    m_speed = 0.f;
    m_path_type = ePathTypeNoPath;
    m_path_state = ePathStateDummy;
    m_path_actuality = true;
    m_speed = 0.f;
    m_old_desirable_speed = 0.f;
    m_build_at_once = false;

    enable_movement(true);
    game_selector().reinit(&ai().game_graph());
    detail().reinit();
    game_path().reinit(&ai().game_graph());
    level_path().reinit(ai().get_level_graph());
    patrol().reinit();

    game_selector().set_dest_path(game_path().m_path);
}

void CMovementManager::reload(LPCSTR section) { locations().reload(section); }
BOOL CMovementManager::net_Spawn(CSE_Abstract* data) { return (restrictions().net_Spawn(data)); }
void CMovementManager::net_Destroy()
{
    level_path_builder().remove();
    detail_path_builder().remove();
    restrictions().net_Destroy();
}

CMovementManager::EPathType CMovementManager::path_type() const
{
    VERIFY(m_path_type != MovementManager::ePathTypeDummy);
    return (m_path_type);
}

void CMovementManager::set_game_dest_vertex(const GameGraph::_GRAPH_ID& game_vertex_id)
{
    game_path().set_dest_vertex(game_vertex_id);
    m_path_actuality = m_path_actuality && game_path().actual();
}

GameGraph::_GRAPH_ID CMovementManager::game_dest_vertex_id() const
{
    return (GameGraph::_GRAPH_ID(game_path().dest_vertex_id()));
}

void CMovementManager::set_level_dest_vertex(u32 const& level_vertex_id)
{
    VERIFY2(restrictions().accessible(level_vertex_id), *object().cName());
    level_path().set_dest_vertex(level_vertex_id);
    m_path_actuality = m_path_actuality && level_path().actual();
}

u32 CMovementManager::level_dest_vertex_id() const { return (level_path().dest_vertex_id()); }
const xr_vector<DetailPathManager::STravelPathPoint>& CMovementManager::path() const { return (detail().path()); }
void CMovementManager::update_path()
{
    START_PROFILE("Build Path::update")

    if (!enabled() || wait_for_distributed_computation())
        return;

    if (!game_path().evaluator())
        game_path().set_evaluator(base_game_params());

    if (!level_path().evaluator())
        level_path().set_evaluator(base_level_params());

#pragma todo("Optimize this in case of slowdown or not intended behaviour")
    if (!restrictions().actual())
    {
        m_path_actuality = false;
    }

    restrictions().actual(true);

    if (!actual())
    {
        game_path().make_inactual();
        level_path().make_inactual();
        patrol().make_inactual();
        switch (m_path_type)
        {
        case ePathTypeGamePath:
        {
            m_path_state = ePathStateSelectGameVertex;
            break;
        }
        case ePathTypeLevelPath:
        {
            m_path_state = ePathStateBuildLevelPath;
            if (!restrictions().accessible(level_path().dest_vertex_id()))
            {
                Fvector temp;
                level_path().set_dest_vertex(restrictions().accessible_nearest(
                    ai().level_graph().vertex_position(level_path().dest_vertex_id()), temp));
                detail().set_dest_position(temp);
            }
            else
            {
                if (!restrictions().accessible(detail().dest_position()))
                {
                    detail().set_dest_position(ai().level_graph().vertex_position(level_path().dest_vertex_id()));
                }
            }
            break;
        }
        case ePathTypePatrolPath:
        {
            //				Msg				("[%6d][%s] actuality is false",Device.dwFrame,*object().cName());
            m_path_state = ePathStateSelectPatrolPoint;
            break;
        }
        case ePathTypeNoPath:
        {
            m_path_state = ePathStateDummy;
            break;
        }
        default: NODEFAULT;
        }
        m_path_actuality = true;
    }

    switch (m_path_type)
    {
    case ePathTypeGamePath:
    {
        process_game_path();
        break;
    }
    case ePathTypeLevelPath:
    {
        process_level_path();
        break;
    }
    case ePathTypePatrolPath:
    {
        process_patrol_path();
        break;
    }
    case ePathTypeNoPath: { break;
    }
    default: NODEFAULT;
    }

#ifdef USE_FREE_IN_RESTRICTIONS
    if (restrictions().accessible(object().Position()))
        verify_detail_path();
#endif // USE_FREE_IN_RESTRICTIONS

    m_build_at_once = false;

    STOP_PROFILE
}

bool CMovementManager::actual_all() const
{
    if (!m_path_actuality)
        return (false);
    switch (m_path_type)
    {
    case ePathTypeGamePath: return (game_path().actual() && level_path().actual() && detail().actual());
    case ePathTypeLevelPath: return (level_path().actual() && detail().actual());
    case ePathTypePatrolPath: return (patrol().actual() && level_path().actual() && detail().actual());
    case ePathTypeNoPath: return (detail().actual());
    default: NODEFAULT;
    }
#ifdef DEBUG
    return (true);
#endif
}

void CMovementManager::teleport(u32 game_vertex_id)
{
    NET_Packet net_packet;
    GameGraph::_GRAPH_ID _game_vertex_id = (GameGraph::_GRAPH_ID)game_vertex_id;
    u32 _level_vertex_id = ai().game_graph().vertex(_game_vertex_id)->level_vertex_id();
    Fvector position = ai().game_graph().vertex(_game_vertex_id)->level_point();
    object().u_EventGen(net_packet, GE_TELEPORT_OBJECT, object().ID());
    net_packet.w(&_game_vertex_id, sizeof(_game_vertex_id));
    net_packet.w(&_level_vertex_id, sizeof(_level_vertex_id));
    net_packet.w_vec3(position);
    Level().Send(net_packet, net_flags(TRUE, TRUE));
}

void CMovementManager::clear_path()
{
    m_detail_path_manager->m_path.clear();
    detail_path_builder().remove();
}

bool CMovementManager::distance_to_destination_greater(const float& distance_to_check) const
{
    if (path().size() < 2)
        return (true);

    if (path_completed())
        return (true);

    float accumulator = 0.f;
    for (u32 i = detail().curr_travel_point_index(), n = detail().path().size() - 1; i < n; ++i)
    {
        accumulator += detail().path()[i].position.distance_to(detail().path()[i + 1].position);
        if (accumulator >= distance_to_check)
            return (true);
    }

    return (false);
}

#ifdef USE_FREE_IN_RESTRICTIONS
void CMovementManager::verify_detail_path()
{
    if (detail().path().empty() || !detail().actual() || detail().completed(detail().dest_position()))
        return;

    if (restrictions().out_restrictions().size())
        return;

    float distance = 0.f;
    for (u32 i = detail().curr_travel_point_index() + 1, n = detail().path().size(); i < n; ++i)
    {
        if (!restrictions().accessible(detail().path()[i].position, EPS_L))
        {
            m_path_actuality = false;
            return;
        }

        distance += detail().path()[i].position.distance_to(detail().path()[i - 1].position);
        if (distance >= verify_distance)
            break;
    }
}
#endif // USE_FREE_IN_RESTRICTIONS

void CMovementManager::on_restrictions_change()
{
    //	Msg								("[%6d][%s][on_restrictions_change]",Device.dwTimeGlobal,*object().cName());
    m_path_actuality = false;
    level_path_builder().remove();
    detail_path_builder().remove();
    level_path().on_restrictions_change();
}

bool CMovementManager::can_use_distributed_computations(u32 option) const
{
    return (!m_build_at_once && g_mt_config.test(option) && !object().getDestroy());
}

void CMovementManager::on_frame(CPHMovementControl* movement_control, Fvector& dest_position)
{
    if (enabled() && (m_path_state != ePathStatePathVerification) && (m_path_state != ePathStatePathCompleted))
        update_path();

    move_along_path(movement_control, dest_position, object().client_update_fdelta());
}

void CMovementManager::on_travel_point_change(const u32& previous_travel_point_index)
{
    detail().on_travel_point_change(previous_travel_point_index);
}

void CMovementManager::enable_movement(bool enabled)
{
    //	m_path_actuality					= m_path_actuality && (m_enabled == enabled);
    if (!enabled && m_enabled)
        m_on_disable_object_position = object().Position();
    else
    {
        if (enabled && !m_enabled && !object().Position().similar(m_on_disable_object_position))
            m_path_actuality = false;
    }

    m_enabled = enabled;
}

CRestrictedObject* CMovementManager::create_restricted_object() { return (new CRestrictedObject(m_object)); }
CMovementManager::CLevelPathManager::PATH& CMovementManager::level_path_path() { return (level_path().m_path); }
void CMovementManager::build_level_path()
{
    //	CTimer								timer;
    //	timer.Start							();
    level_path_builder().process_impl();
    //	static int i=0;
    //	Msg									("[%6d][%6d][%4d][%f]
    // build_level_path",Device.dwTimeGlobal,Device.dwFrame,++i,timer.GetElapsed_sec()*1000.f);
}

Fvector CMovementManager::predict_position(
    const float& time_delta, const Fvector& start_position, u32& current_travel_point, const float& velocity) const
{
    typedef xr_vector<DetailPathManager::STravelPathPoint> PATH;
    const PATH& path = detail().path();
    if (path.empty())
        return (start_position);

    float distance_to_check = velocity * time_delta;

    const u32& path_size = path.size();
    if (current_travel_point == path_size - 1)
        return (path.back().position);

    {
        const Fvector& next = path[current_travel_point + 1].position;
        float distance = start_position.distance_to(next);
        if (distance >= distance_to_check)
        {
            if (next.similar(start_position))
                return (next);

            Fvector result;
            result.sub(next, start_position);
            result.normalize();
            result.mul(distance_to_check);
            result.add(start_position);
            return (result);
        }

        distance_to_check -= distance;
        ++current_travel_point;
    }

    while (current_travel_point < (path_size - 1))
    {
        const Fvector& current = path[current_travel_point].position;
        const Fvector& next = path[current_travel_point + 1].position;
        float distance = current.distance_to(next);
        if (distance > distance_to_check)
            break;

        distance_to_check -= distance;
        ++current_travel_point;
    }

    if (current_travel_point == path_size - 1)
        return (path.back().position);

    const Fvector& current = path[current_travel_point].position;
    const Fvector& next = path[current_travel_point + 1].position;

    VERIFY(current.distance_to(next) > distance_to_check);

    if (next.similar(current))
        return (next);

    Fvector direction = Fvector().sub(next, current);
    direction.normalize();
    direction.mul(distance_to_check);
    direction.add(current);
    return (direction);
}

Fvector CMovementManager::predict_position(const float& time_delta) const
{
    u32 travel_point = detail().m_current_travel_point;
    return (predict_position(time_delta, object().Position(), travel_point, prediction_speed()));
}

const float& CMovementManager::prediction_speed() const { return (old_desirable_speed()); }
Fvector CMovementManager::target_position() const
{
    if (detail().path().empty())
        return (object().Position());

    return (detail().path()[detail().last_patrol_point()].position);
}
