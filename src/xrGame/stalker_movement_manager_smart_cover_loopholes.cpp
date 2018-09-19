////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_movement_manager_smart_cover_loopholes.cpp
//	Created 	: 14.02.2008
//	Modified	: 14.02.2008
//	Author		: Dmitriy Iassenev
//	Description : stalker movement manager class with smart covers loopholes stuff
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_movement_manager_smart_cover.h"
#include "smart_cover_loophole.h"
#include "ai/stalker/ai_stalker.h"
#include "smart_cover.h"
#include "ai_space.h"
#include "xrAICore/Navigation/graph_engine.h"
#include "smart_cover_transition.hpp"
#include "smart_cover_transition_animation.hpp"
#include "sight_manager.h"
#include "smart_cover_animation_selector.h"
#include "script_game_object.h"
#include "memory_space.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "xrScriptEngine/Functor.hpp"
#include "xrCore/buffer_vector.h"

namespace smart_cover
{
shared_str transform_vertex(shared_str const& vertex_id, bool const& in);
} // namespace smart_cover

using smart_cover::loophole;
using MemorySpace::CMemoryInfo;

float stalker_movement_manager_smart_cover::enter_path(LoopholePath* result, Fvector const& position,
    u32 const level_vertex_id, smart_cover::cover const& cover, shared_str const& target_loophole_id)
{
    VERIFY(cover.get_description()->get_loophole(target_loophole_id));

    float value = flt_max;

    typedef smart_cover::cover::Loopholes Loopholes;

    Loopholes const& loopholes = cover.get_description()->loopholes();
    Loopholes::const_iterator i = loopholes.begin();
    Loopholes::const_iterator e = loopholes.end();
    for (; i != e; ++i)
    {
        if (!(*i)->enterable())
            continue;

        loophole_path(cover, (*i)->id(), target_loophole_id, m_temp_loophole_path);
        VERIFY(!m_temp_loophole_path.empty());
        shared_str const& loophole_id = m_temp_loophole_path.front();
        smart_cover::loophole const& loophole = this->loophole(cover, loophole_id);
        float new_value = cover.fov_position(loophole).distance_to(position);
        new_value += ai().graph_engine().m_string_algorithm->data_storage().get_best().g();
        if (new_value >= value)
            continue;

        value = new_value;

        if (result)
            result->swap(m_temp_loophole_path);
    }

    VERIFY(value < flt_max);
    VERIFY(!result || !result->empty());

    if (result)
        result->insert(result->begin(), smart_cover::transform_vertex("", true));

    return (value);
}

void stalker_movement_manager_smart_cover::build_enter_path()
{
    VERIFY(m_target.cover());
    smart_cover::cover const& target_cover = *m_target.cover();

    m_path.clear();

    shared_str target_loophole_id =
        smart_cover::transform_vertex(m_target.cover_loophole() ? m_target.cover_loophole()->id() : "", false);
    Fvector const& position = object().Position();
    enter_path(&m_path, position, object().ai_location().level_vertex_id(), target_cover, target_loophole_id);

    if (m_path.size() > 1)
    {
        m_current_transition = &action(target_cover, m_path[0], m_path[1]);
        m_current_transition_animation = &m_current_transition->animation();
    }
    else
    {
        m_current_transition = 0;
        m_current_transition_animation = 0;
    }
}

float stalker_movement_manager_smart_cover::exit_path_weight(u32 const& source_node, Fvector const& source_position,
    u32 const& target_node, Fvector const& target_position) const
{
    return (source_position.distance_to(target_position));
}

bool stalker_movement_manager_smart_cover::test_pick(Fvector source, Fvector destination) const
{
    source.y += 2.f;
    destination.y += 2.f;
    Fvector direction = Fvector(destination).sub(source);
    float const distance = direction.magnitude();
    if (distance > EPS_L)
        direction.normalize();
    else
        return (true);

    struct parameters
    {
        float* m_range;
        CAI_Stalker* m_object;

        inline parameters(float& range, CAI_Stalker& object) : m_range(&range), m_object(&object) {}
    }; // struct parameters

    struct test_pick
    {
        static BOOL callback(collide::rq_result& result, LPVOID user_data)
        {
            parameters* const param = (parameters*)user_data;
            if (param->m_object->feel_vision_mtl_transp(result.O, result.element) < 1.f)
            {
                *param->m_range = result.range;
                return (FALSE);
            }

            return (TRUE);
        }
    };

    float range = distance;
    parameters params(range, object());
    collide::ray_defs ray_defs(source, direction, distance, CDB::OPT_CULL, collide::rqtStatic);
    Level().ObjectSpace.RayQuery(m_ray_query_storage, ray_defs, &test_pick::callback, &params, NULL, NULL);
    return (range == distance);
}

stalker_movement_manager_smart_cover::transition_action const& stalker_movement_manager_smart_cover::nearest_action(
    smart_cover::cover const& cover, shared_str const& loophole_id0, shared_str const& loophole_id1,
    Fvector const& position, Fvector& result_position, u32& result_vertex_id, EBodyState* target_body_state) const
{
    typedef smart_cover::description::TransitionGraph::CEdge edge_type;
    typedef smart_cover::description::ActionsList ActionsList;
    typedef smart_cover::transitions::action action;

    edge_type const* edge = cover.get_description()->transitions().edge(loophole_id0, loophole_id1);
    VERIFY(edge);
    ActionsList const& actions = edge->data();

    transition_action const* result = 0;
    float min_distance_sqr = flt_max;

    EBodyState result_body_state = eBodyStateDummy;
    Fmatrix const& transform = cover.get_object().XFORM();
    ActionsList::const_iterator i = actions.begin();
    ActionsList::const_iterator e = actions.end();
    for (; i != e; ++i)
    {
        if (!(*i)->applicable())
            continue;

        typedef smart_cover::transitions::action::Animations Animations;
        Animations::const_iterator I = (*i)->animations().begin();
        Animations::const_iterator E = (*i)->animations().end();
        for (; I != E; ++I)
        {
            Fvector action_position;
            transform.transform_tiny(action_position, (*I)->position());
            float const distance_sqr = action_position.distance_to_sqr(position);
            if (distance_sqr > min_distance_sqr)
                continue;

            u32 vertex_id = u32(-1);
            if ((*I)->has_animation())
            {
                if (!ai().level_graph().valid_vertex_position(action_position))
                    continue;

                vertex_id = ai().level_graph().vertex_id(action_position);
                if (!ai().level_graph().valid_vertex_id(vertex_id))
                    continue;

                float const y = ai().level_graph().vertex_plane_y(vertex_id, action_position.x, action_position.z);
                if (!fsimilar(y, action_position.y, 2.f))
                    continue;

                if (!test_pick(object().Position(), action_position))
                    continue;
            }

            if (target_body_state && ((*I)->body_state() != *target_body_state) &&
                (result_body_state == *target_body_state))
                continue;

            result_body_state = (*I)->body_state();
            min_distance_sqr = distance_sqr;
            result = *i;
            result_position = action_position;
            result_vertex_id = vertex_id;
        }
    }

    VERIFY2(
        result, make_string("cover[%s][%s], loophole[%s -> %s], body_state[%s] [%f][%f][%f]", cover.id().c_str(),
                    cover.get_description()->table_id().c_str(), loophole_id0.c_str(), loophole_id1.c_str(),
                    !target_body_state ? "" : (*target_body_state == eBodyStateStand ?
                                                      "stand" :
                                                      (*target_body_state == eBodyStateCrouch ? "crouch" : "invalid!")),
                    VPUSH(position)));
    return (*result);
}

stalker_movement_manager_smart_cover::transition_action const& stalker_movement_manager_smart_cover::action(
    smart_cover::cover const& cover, shared_str const& loophole_id0, shared_str const& loophole_id1) const
{
    typedef smart_cover::description::TransitionGraph::CEdge edge_type;
    typedef smart_cover::description::ActionsList ActionsList;
    typedef smart_cover::transitions::action action;

    edge_type const* edge = cover.get_description()->transitions().edge(loophole_id0, loophole_id1);
    VERIFY(edge);
    ActionsList const& actions = edge->data();

    struct applicable
    {
        IC static bool predicate(action const* const& action) { return (action->applicable()); }
    };

    ActionsList::const_iterator i = std::find_if(actions.begin(), actions.end(), &applicable::predicate);
    VERIFY(i != actions.end());
    return (**i);
}

void stalker_movement_manager_smart_cover::build_exit_path()
{
    m_path.clear();

    float value = flt_max;

    VERIFY(m_current.cover());
    smart_cover::cover const& cur_cover = *m_current.cover();

    VERIFY(m_current.cover_loophole());
    smart_cover::loophole const& cur_loophole = *m_current.cover_loophole();

    typedef smart_cover::cover::Loopholes Loopholes;
    Loopholes const& loopholes = cur_cover.get_description()->loopholes();
    Loopholes::const_iterator I = loopholes.begin();
    Loopholes::const_iterator E = loopholes.end();
    for (; I != E; ++I)
    {
        if (!(*I)->exitable())
            continue;

        shared_str const& exitable_loophole_id = (*I)->id();
        loophole_path(cur_cover, cur_loophole.id(), exitable_loophole_id, m_temp_loophole_path);
        VERIFY(!m_temp_loophole_path.empty());

        float new_value = ai().graph_engine().m_string_algorithm->data_storage().get_best().g();
        float exit_edge = cur_cover.get_description()
                              ->transitions()
                              .edge(exitable_loophole_id, smart_cover::transform_vertex("", false))
                              ->weight();
        new_value += exit_edge;

        u32 targe_vertex_id = level_dest_vertex_id();
        Fvector target_position = m_target.desired_position() ? *m_target.desired_position() :
                                                                ai().level_graph().vertex_position(targe_vertex_id);

        Fvector exit_position;
        u32 exit_vertex_id;
        //		transition_action const&	action =
        nearest_action(cur_cover, exitable_loophole_id, smart_cover::transform_vertex("", false), target_position,
            exit_position, exit_vertex_id, &m_target.m_body_state);

        new_value += exit_path_weight(exit_vertex_id, exit_position, targe_vertex_id, target_position);

        if (new_value >= value)
            continue;

        value = new_value;
        m_path.swap(m_temp_loophole_path);
    }

    VERIFY(!m_path.empty());

    m_path.push_back(smart_cover::transform_vertex("", false));

    if (m_path.size() > 1)
    {
        m_current_transition = &action(*m_current.cover(), m_path[0], m_path[1]);
        m_current_transition_animation = &m_current_transition->animation();
    }
    else
    {
        m_current_transition = 0;
        m_current_transition_animation = 0;
    }
}

void stalker_movement_manager_smart_cover::build_exit_path_to_cover()
{
    m_path.clear();

    float value = flt_max;
    smart_cover::transitions::action const* selected_action = 0;

    VERIFY(m_current.cover());
    smart_cover::cover const& current_cover = *m_current.cover();

    VERIFY(m_current.cover_loophole());
    smart_cover::loophole const& current_loophole = *m_current.cover_loophole();

    VERIFY(m_target.cover());
    smart_cover::cover const& target_cover = *m_target.cover();

    VERIFY(m_target.cover_loophole());
    smart_cover::loophole const& target_loophole = *m_target.cover_loophole();

    Fvector target_position;
    target_cover.get_object().XFORM().transform_tiny(target_position, target_loophole.fov_position());

    typedef smart_cover::cover::Loopholes Loopholes;
    Loopholes const& loopholes = current_cover.get_description()->loopholes();
    Loopholes::const_iterator I = loopholes.begin();
    Loopholes::const_iterator E = loopholes.end();
    for (; I != E; ++I)
    {
        if (!(*I)->exitable())
            continue;

        shared_str const& exitable_loophole_id = (*I)->id();
        loophole_path(current_cover, current_loophole.id(), exitable_loophole_id, m_temp_loophole_path);
        VERIFY(!m_temp_loophole_path.empty());

        float new_value = ai().graph_engine().m_string_algorithm->data_storage().get_best().g();
        float exit_edge = current_cover.get_description()
                              ->transitions()
                              .edge(exitable_loophole_id, smart_cover::transform_vertex("", false))
                              ->weight();
        new_value += exit_edge;

        Fvector exit_position;
        u32 exit_vertex_id;
        EBodyState exit_body_state = eBodyStateStand;
        smart_cover::transitions::action const& current_action = nearest_action(current_cover, exitable_loophole_id,
            smart_cover::transform_vertex("", false), target_position, exit_position, exit_vertex_id, &exit_body_state);

        buffer_vector<shared_str> temp(_alloca(sizeof(u32) * m_temp_loophole_path.size()), m_temp_loophole_path.size(),
            m_temp_loophole_path.begin(), m_temp_loophole_path.end());
        new_value += enter_path(0, exit_position, exit_vertex_id, target_cover,
            (target_loophole.enterable() ? target_loophole : nearest_enterable_loophole()).id());

        if (new_value >= value)
            continue;

        selected_action = &current_action;
        value = new_value;
        m_path.assign(temp.begin(), temp.end());
    }

    VERIFY(!m_path.empty());

    m_path.push_back(smart_cover::transform_vertex("", false));

    if (m_path.size() > 1)
    {
        VERIFY(selected_action);
        m_current_transition = selected_action;
        m_current_transition_animation = &m_current_transition->animation();
    }
    else
    {
        m_current_transition = 0;
        m_current_transition_animation = 0;
    }
}

void stalker_movement_manager_smart_cover::actualize_path()
{
    VERIFY(m_current.cover() || m_target.cover());

    if (!m_current.cover())
    {
        build_enter_path();
        return;
    }

    if (!m_target.cover())
    {
        build_exit_path();
        return;
    }

    if (m_current.cover() != m_target.cover())
    {
        build_exit_path_to_cover();
        return;
    }

    VERIFY(m_current.cover());
    shared_str current_loophole_id =
        smart_cover::transform_vertex(m_current.cover_loophole() ? m_current.cover_loophole()->id() : "", true);

    shared_str target_loophole_id;
    if (m_current.cover() == m_target.cover())
        target_loophole_id = m_target.cover_loophole_id();
    else
        target_loophole_id = "";

    loophole_path(*m_current.cover(), current_loophole_id, target_loophole_id, m_path);

    VERIFY(!m_path.empty());

    if (m_path.size() > 1)
    {
        m_current_transition = &action(*m_current.cover(), m_path[0], m_path[1]);
        m_current_transition_animation = &m_current_transition->animation();
    }
    else
    {
        m_current_transition = 0;
        m_current_transition_animation = 0;
    }
}

void stalker_movement_manager_smart_cover::try_actualize_path()
{
    if (m_path.empty())
    {
        actualize_path();
        return;
    }

    shared_str current_loophole_id =
        smart_cover::transform_vertex(m_current.cover() ? m_current.cover_loophole()->id() : "", true);
    if (m_path.front() != current_loophole_id)
    {
        actualize_path();
        return;
    }

    shared_str target_loophole_id = smart_cover::transform_vertex(
        (m_target.cover() == m_current.cover()) ? m_target.cover_loophole()->id() : "", false);
    if (m_path.back() == target_loophole_id)
        return;

    actualize_path();
}

loophole const& stalker_movement_manager_smart_cover::nearest_enterable_loophole()
{
    VERIFY(!m_current.cover());
    VERIFY(!m_current.cover_loophole());
    VERIFY(m_target.cover());
    VERIFY(m_target.cover_loophole());

    try_actualize_path();

    VERIFY(!m_path.empty());
    VERIFY(m_path.size() > 1);
    VERIFY(m_path[0]._get() == smart_cover::transform_vertex("", true)._get());

    return (loophole(*m_target.cover(), m_path[1]));
}

shared_str const& stalker_movement_manager_smart_cover::next_loophole_id()
{
    VERIFY2(m_current.cover(),
        make_string("[%s][%s] -> [%s][%s], [%d]", m_current.cover() ? m_current.cover()->id().c_str() : "<world>",
            m_current.cover() ? m_current.cover_loophole()->id().c_str() : "<no loophole>",
            m_target.cover() ? m_target.cover()->id().c_str() : "<world>",
            m_target.cover() ? m_target.cover_loophole()->id().c_str() : "<no loophole>", m_path.size()));

    try_actualize_path();

    VERIFY(!m_path.empty());
    VERIFY(m_path.size() > 1);

    return (m_path[1]);
}

void stalker_movement_manager_smart_cover::go_next_loophole()
{
    try_actualize_path();

    VERIFY(!m_path.empty());

    //	VERIFY						(m_path.size() > 1);
    if (m_path.size() == 1)
    {
        VERIFY(m_current.cover());
        VERIFY(m_current.cover_loophole()->id() == m_path[0]);
        return;
    }

    if (m_path[0]._get() == smart_cover::transform_vertex("", true)._get())
    {
        VERIFY(m_target.cover());
        VERIFY(!m_current.cover());
#ifdef DEBUG
        Msg("setting up cover (direct from target): %s (%s)", m_target.cover_id().c_str(), m_enter_cover_id.c_str());
#endif // #ifdef DEBUG
        m_current.cover_id(m_target.cover_id());
        m_current.cover_loophole_id(m_path[1]);
        return;
    }

    VERIFY(m_current.cover());

    if (m_path[1]._get() == smart_cover::transform_vertex("", false)._get())
    {
        VERIFY(m_path.size() == 2);
#ifdef DEBUG
        Msg("exiting from cover: %s", m_current.cover_id().c_str());
#endif // #ifdef DEBUG
        m_current.cover_id("");
        on_smart_cover_exit();
        return;
    }

    m_current.cover_loophole_id(m_path[1]);
}

static IC bool exit_loophole(shared_str const& loophole_id)
{
    return (loophole_id._get() == smart_cover::transform_vertex("", false)._get());
}

void stalker_movement_manager_smart_cover::non_animated_change_loophole()
{
    VERIFY(m_current.cover());
    setup_movement_params();
    if (!m_non_animated_loophole_change)
        return;

    VERIFY(m_current.cover());
    inherited::update(m_current);

    VERIFY(m_current.cover());
    cover_type const& cover = *m_current.cover();
    shared_str const& loophole_id = next_loophole_id();

    if (!exit_loophole(loophole_id))
    {
        if (!target_approached(m_apply_loophole_direction_distance))
            object().sight().setup(CSightAction(SightManager::eSightTypePathDirection));
        else
        {
            loophole_type const& loophole = this->loophole(cover, loophole_id);
            Fvector direction = cover.enter_direction(loophole);
            object().sight().setup(CSightAction(SightManager::eSightTypeDirection, direction, true));
        }
    }

    if (!path_completed())
        return;

    if (!object().sight().current_action().target_reached())
        return;

    go_next_loophole();

    if (!m_current.cover())
        return;

    m_animation_selector->on_animation_end();
    m_animation_selector->planner().update();
}

void stalker_movement_manager_smart_cover::setup_movement_params()
{
    VERIFY(m_current.cover());
    smart_cover::cover const& cover = *m_current.cover();

    try_actualize_path();
    VERIFY(!m_path.empty());

    if (m_path.size() == 1)
    {
        m_non_animated_loophole_change = false;
        return;
    }

    shared_str const& loophole_id = m_path[1];
    m_current.m_movement_type = eMovementTypeRun;

    if (exit_loophole(loophole_id))
        return;

    m_current.m_body_state = current_transition_animation().body_state();

    loophole_type const& loophole = this->loophole(cover, loophole_id);

    u32 level_vertex_id = cover.level_vertex_id(loophole);
    VERIFY(restrictions().accessible(level_vertex_id));

    Fvector position = cover.fov_position(loophole);
    VERIFY(restrictions().accessible(position));

    CMovementManager::set_level_dest_vertex(level_vertex_id);
    m_current.desired_position(&position);
}

struct loophole_id_predicate
{
    shared_str m_id;

    IC loophole_id_predicate(shared_str const& id) : m_id(id) {}
    IC bool operator()(smart_cover::loophole* loophole) const { return (loophole->id()._get() == m_id._get()); }
};

loophole const& stalker_movement_manager_smart_cover::loophole(
    smart_cover::cover const& cover, shared_str const& loophole_id) const
{
    typedef smart_cover::cover::Loopholes Loopholes;
    Loopholes const& loopholes = cover.get_description()->loopholes();
    Loopholes::const_iterator i = std::find_if(loopholes.begin(), loopholes.end(), loophole_id_predicate(loophole_id));

    VERIFY2(i != loopholes.end(),
        make_string("loophole [%s] not present in smart_cover [%s]", loophole_id.c_str(), cover.id().c_str()));
    return (**i);
}

float const& stalker_movement_manager_smart_cover::idle_min_time() const
{
    return (animation_selector().planner().idle_min_time());
}

void stalker_movement_manager_smart_cover::idle_min_time(float const& value)
{
    animation_selector().planner().idle_min_time(value);
}

float const& stalker_movement_manager_smart_cover::idle_max_time() const
{
    return (animation_selector().planner().idle_max_time());
}

void stalker_movement_manager_smart_cover::idle_max_time(float const& value)
{
    animation_selector().planner().idle_max_time(value);
}

float const& stalker_movement_manager_smart_cover::lookout_min_time() const
{
    return (animation_selector().planner().lookout_min_time());
}

void stalker_movement_manager_smart_cover::lookout_min_time(float const& value)
{
    animation_selector().planner().lookout_min_time(value);
}

float const& stalker_movement_manager_smart_cover::lookout_max_time() const
{
    return (animation_selector().planner().lookout_max_time());
}

void stalker_movement_manager_smart_cover::lookout_max_time(float const& value)
{
    animation_selector().planner().lookout_max_time(value);
}

void stalker_movement_manager_smart_cover::start_non_animated_loophole_change()
{
    object().movement().unbind_global_selector();
    object().movement().non_animated_loophole_change(true);
    object().movement().non_animated_change_loophole();
}

void stalker_movement_manager_smart_cover::stop_non_animated_loophole_change()
{
    object().movement().non_animated_loophole_change(false);
    object().movement().bind_global_selector();
}

Fvector stalker_movement_manager_smart_cover::position_to_cover_from() const
{
    Fvector const* cover_fire_position = m_target.cover_fire_position();
    if (cover_fire_position)
        return (*cover_fire_position);

    CGameObject const* fire_object = m_target.cover_fire_object();
    if (fire_object)
    {
        if (!object().g_Alive())
            return (fire_object->Position());

        CMemoryInfo info = object().memory().memory(fire_object);
        if (info.m_visual_info | info.m_sound_info | info.m_hit_info)
            return (info.m_object_params.m_position);

        return (fire_object->Position());
    }

    CEntityAlive const* enemy = object().memory().enemy().selected();
    if (!enemy)
        return (object().Position());

    VERIFY(enemy);
    return (object().memory().memory(enemy).m_object_params.m_position);
}
