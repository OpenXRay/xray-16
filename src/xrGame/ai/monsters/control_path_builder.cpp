#include "StdAfx.h"
#include "control_path_builder.h"
#include "control_manager.h"
#include "basemonster/base_monster.h"
#include "game_location_selector.h"
#include "level_location_selector.h"
#include "detail_path_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "ai_space.h"
#include "movement_manager_space.h"
#include "level_path_manager.h"
#include "Actor.h"
#include "actor_memory.h"
#include "visual_memory_manager.h"

#ifdef DEBUG
extern bool show_restrictions(CRestrictedObject* object);
#endif

CControlPathBuilder::CControlPathBuilder(CCustomMonster* monster) : CMovementManager(monster) {}
CControlPathBuilder::~CControlPathBuilder() {}
void CControlPathBuilder::load(LPCSTR section) {}
void CControlPathBuilder::reinit()
{
    // todo: remove call twice [CustomMonster reinit && control_manager reinit]
    inherited::reinit();
    inherited_com::reinit();

    m_data.use_dest_orientation = false;
    m_data.dest_orientation.set(0.f, 0.f, 0.f);

    m_data.try_min_time = true;

    m_data.target_position.set(0.f, 0.f, 0.f);
    m_data.target_node = u32(-1);

    m_data.enable = false;
    m_data.extrapolate = false;

    m_data.velocity_mask = u32(-1);
    m_data.desirable_mask = u32(-1);

    m_data.path_type = MovementManager::ePathTypeLevelPath;
    m_data.game_graph_target_vertex = u32(-1);
}

void CControlPathBuilder::update_schedule()
{
    START_PROFILE("Base Monster/Path Builder/Schedule Update");

    // the one and only reason is because of the restriction-change, so wait until
    // position and node will be in valid state
    if (m_data.path_type != MovementManager::ePathTypePatrolPath)
    {
        if (!accessible(m_data.target_position) ||
            ((m_data.target_node != u32(-1)) && (!accessible(m_data.target_node))))
        {
            return;
        }
    }

    if (m_data.reset_actuality)
        make_inactual();

    // set enabled
    enable_movement(m_data.enable);

    // set params only if enable params
    if (m_data.enable)
    {
        detail().set_path_type(eDetailPathTypeSmooth);

        // установить direction
        detail().set_use_dest_orientation(m_data.use_dest_orientation);
        if (m_data.use_dest_orientation)
            detail().set_dest_direction(m_data.dest_orientation);

        detail().set_try_min_time(m_data.try_min_time);

        extrapolate_path(m_data.extrapolate);

        detail().set_velocity_mask(m_data.velocity_mask);
        detail().set_desirable_mask(m_data.desirable_mask);

        set_path_type(m_data.path_type);
        if (m_data.path_type == MovementManager::ePathTypeGamePath)
        {
            // check if we have alife task
            if (m_data.game_graph_target_vertex != u32(-1))
            {
                set_game_dest_vertex(GameGraph::_GRAPH_ID(m_data.game_graph_target_vertex));
                game_selector().set_selection_type(eSelectionTypeMask);
            }
            else
                // else just wandering through the game graph
                game_selector().set_selection_type(eSelectionTypeRandomBranching);
        }
        else if (m_data.path_type != MovementManager::ePathTypePatrolPath)
        {
            // set target
            // TODO: make it VERIFY
            //			VERIFY3(m_data.target_node != u32(-1), "Error: Object set wrong path params! Object
            // name:",*inherited_com::m_object->cName());
            if (m_data.target_node == u32(-1))
                return;

            detail().set_dest_position(m_data.target_position);
            set_level_dest_vertex(m_data.target_node);
        }
    }

    update_path();

    m_man->notify(ControlCom::eventPathUpdated, 0);

    STOP_PROFILE;
}

void CControlPathBuilder::on_travel_point_change(const u32& previous_travel_point_index)
{
    inherited::on_travel_point_change(previous_travel_point_index);
    m_man->notify(ControlCom::eventTravelPointChange, 0);
}

void CControlPathBuilder::on_build_path() { m_man->notify(ControlCom::eventPathBuilt, 0); }
//////////////////////////////////////////////////////////////////////////
// Special Build Path
//////////////////////////////////////////////////////////////////////////
bool CControlPathBuilder::build_special(const Fvector& target, u32 node, u32 vel_mask)
{
    if (!accessible(target))
        return false;

    if (node == u32(-1))
    {
        // нода в прямой видимости?
        restrictions().add_border(object().Position(), target);
        node = ai().level_graph().check_position_in_direction(
            object().ai_location().level_vertex_id(), object().Position(), target);
        restrictions().remove_border();

        if (!ai().level_graph().valid_vertex_id(node) || !accessible(node))
            return false;
    }

    enable_movement(true);

    detail().set_velocity_mask(vel_mask);
    detail().set_desirable_mask(vel_mask);

    detail().set_try_min_time(false);
    detail().set_use_dest_orientation(false);

    detail().set_path_type(eDetailPathTypeSmooth);
    set_path_type(MovementManager::ePathTypeLevelPath);

    detail().set_dest_position(target);
    set_level_dest_vertex(node);

    set_build_path_at_once();
    update_path();

    // check if path built successfully
    if (!path_completed() && (detail().time_path_built() >= Device.dwTimeGlobal))
        return true;

    return false;
}

//////////////////////////////////////////////////////////////////////////
// Services
//////////////////////////////////////////////////////////////////////////

bool CControlPathBuilder::is_path_end(float dist_to_end)
{
    if (!is_path_built())
        return false;
    // if (path_completed())			return true;
    if (!is_moving_on_path())
        return true;

    u32 cur_point_idx = detail().curr_travel_point_index();
    u32 path_size = detail().path().size();
    if (path_size < 2)
        return true;
    if (cur_point_idx + 1 >= path_size)
        return true;

    // count distance from current object position to the path end
    float cur_dist_to_end =
        object().Position().distance_to(detail().path()[detail().curr_travel_point_index() + 1].position);
    for (u32 i = detail().curr_travel_point_index() + 1; i < detail().path().size() - 1; i++)
    {
        cur_dist_to_end += detail().path()[i].position.distance_to(detail().path()[i + 1].position);
        if (cur_dist_to_end > dist_to_end)
            break;
    }

    if ((cur_dist_to_end < dist_to_end))
        return true;
    return false;
}

bool CControlPathBuilder::valid_destination(const Fvector& pos, u32 node)
{
    return (ai().level_graph().valid_vertex_id(node) && ai().level_graph().valid_vertex_position(pos) &&
        ai().level_graph().inside(node, pos));
}

bool CControlPathBuilder::valid_and_accessible(Fvector& pos, u32 node)
{
    if (!valid_destination(pos, node) || !accessible(node))
        return false;

    fix_position(Fvector().set(pos), node, pos);
    return true;
}

void CControlPathBuilder::fix_position(const Fvector& pos, u32 node, Fvector& res_pos)
{
    VERIFY(accessible(node));
    VERIFY(ai().level_graph().inside(node, pos));

    res_pos.set(pos);
    res_pos.y = ai().level_graph().vertex_plane_y(node, res_pos.x, res_pos.z);

    if (!accessible(res_pos))
    {
        u32 level_vertex_id = restrictions().accessible_nearest(Fvector().set(res_pos), res_pos);

#ifdef DEBUG
        if (level_vertex_id != node)
        {
            Msg("! src_node[%d] res_node[%d] src_pos[%f,%f,%f] res_pos[%f,%f,%f]", node, level_vertex_id, VPUSH(pos),
                VPUSH(res_pos));
        }
        VERIFY3((level_vertex_id == node) || show_restrictions(m_restricted_object),
            "Invalid restrictions (see log for details) for object ", *(CControl_Com::m_object->cName()));
#endif
    }
}

bool CControlPathBuilder::is_moving_on_path()
{
    return (!detail().completed(inherited_com::m_object->Position()) && enabled());
}

bool CControlPathBuilder::get_node_in_radius(
    u32 src_node, float min_radius, float max_radius, u32 attempts, u32& dest_node)
{
    Fvector vertex_position = ai().level_graph().vertex_position(src_node);

    for (u32 i = 0; i < attempts; i++)
    {
        Fvector dir;
        dir.random_dir();
        dir.normalize();

        Fvector new_pos;
        new_pos.mad(vertex_position, dir, Random.randF(min_radius, max_radius));

        restrictions().add_border(vertex_position, new_pos);
        dest_node = ai().level_graph().check_position_in_direction(src_node, vertex_position, new_pos);
        restrictions().remove_border();
        if (dest_node != u32(-1) && accessible(dest_node))
            return true;
    }
    return false;
}

void CControlPathBuilder::make_inactual()
{
    // switch once
    enable_movement(!enabled());
    // switch twice
    enable_movement(!enabled());
}

extern CActor* g_actor;

bool CControlPathBuilder::can_use_distributed_computations(u32 option) const
{
    if (!g_actor)
        return true;

    VERIFY(Actor());
    VERIFY(inherited_com::m_object);
    if (Actor()->memory().visual().visible_right_now(inherited_com::m_object))
        return false;
    return inherited::can_use_distributed_computations(option);
}

u32 CControlPathBuilder::find_nearest_vertex(
    const u32& level_vertex_id, const Fvector& target_position, const float& range)
{
    xr_vector<u32> temp;

    ai().graph_engine().search(ai().level_graph(), level_vertex_id, level_vertex_id, &temp,
        GraphEngineSpace::CNearestVertexParameters(target_position, range));

    VERIFY(!temp.empty());
    VERIFY(temp.size() == 1);
    return (temp.front());
}

bool CControlPathBuilder::is_path_built() { return actual_all(); }
