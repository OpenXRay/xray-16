////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover.cpp
//	Created 	: 16.08.2007
//	Author		: Alexander Dudin
//	Description : Smart cover class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover.h"
#include "smart_cover_storage.h"
#include "smart_cover_object.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "smart_cover_action.h"
#include "ai_space.h"
#include "xrAICore/Navigation/level_graph.h"
#include "xrAICore/Navigation/graph_engine.h"

namespace hash_fixed_vertex_manager
{
IC u32 to_u32(shared_str const& string)
{
    const str_value* get = string._get();
    return (*(u32 const*)&get);
}

} // namespace hash_fixed_vertex_manager

namespace smart_cover
{
shared_str transform_vertex(shared_str const& vertex_id, bool const& in);
} // namespace smart_cover

using smart_cover::cover;
using smart_cover::description;
using smart_cover::transform_vertex;

cover::cover(smart_cover::object const& object, DescriptionPtr description, bool const is_combat_cover,
    bool const can_fire, luabind::object const& loopholes_availability)
    : inherited(object.Position(), object.ai_location().level_vertex_id()), m_object(object),
      m_description(description), m_id(m_object.cName()), m_is_combat_cover(is_combat_cover), m_can_fire(can_fire)
{
    m_is_smart_cover = 1;

    if (loopholes_availability)
    {
        m_loopholes.reserve(m_description->loopholes().size());
        for (const auto& it : m_description->loopholes())
        {
            for (luabind::iterator i(loopholes_availability), e; i != e; ++i)
            {
                pcstr const loophole_id = luabind::object_cast<pcstr>(i.key());
                if (xr_strcmp(loophole_id, it->id()))
                    continue;

                if (!luabind::object_cast<bool>(*i))
                    break;

                m_loopholes.push_back(it);
                break;
            }
        }
    }
    else
    {
        m_loopholes.reserve(m_description->loopholes().size());

        for (const auto& it : m_description->loopholes())
            m_loopholes.push_back(it);
    }

    CLevelGraph const& graph = ai().level_graph();
    m_vertices.resize(loopholes().size());
    Vertices::iterator i = m_vertices.begin();
    Loopholes::const_iterator J = loopholes().begin();
    Loopholes::const_iterator K = loopholes().end();
    for (; J != K; ++J, ++i)
    {
        Fvector position = this->fov_position(**J);
        position.y += 2.0f;
        u32 level_vertex_id = graph.vertex_id(position);
        VERIFY2(graph.valid_vertex_id(level_vertex_id), make_string("invalid vertex id: smart cover[%s], loophole [%s]",
                                                            m_object.cName().c_str(), (*J)->id().c_str()));
        vertex(**J, (*i).second);
        const_cast<loophole*&>((*i).first) = *J;
    }

#ifdef DEBUG
    check_loopholes_connectivity();
#endif // DEBUG
}

cover::~cover() {}
void cover::vertex(smart_cover::loophole const& loophole, smart_cover::loophole_data& loophole_data)
{
    CLevelGraph const& graph = ai().level_graph();
    Fvector pos = fov_position(loophole);
    pos.y += 2.0f;
    loophole_data.m_level_vertex_id = graph.vertex_id(pos);
    VERIFY2(graph.valid_vertex_id(loophole_data.m_level_vertex_id),
        make_string(
            "invalid vertex id: smart cover[%s], loophole [%s]", m_object.cName().c_str(), loophole.id().c_str()));

    typedef loophole::ActionList::const_iterator const_iterator;
    const_iterator I = loophole.actions().begin();
    const_iterator E = loophole.actions().end();
    for (; I != E; ++I)
        if ((*I).second->movement())
        {
            Fvector pos = position((*I).second->target_position());
            pos.y += 2.0f;
            u32 level_vertex_id = graph.vertex_id(pos);
            VERIFY2(graph.valid_vertex_id(level_vertex_id),
                make_string("invalid vertex id: loophole [%s]", loophole.id().c_str()));
            loophole_data.m_action_vertices.push_back(std::make_pair((*I).first, level_vertex_id));
        }
}

u32 const& cover::action_level_vertex_id(smart_cover::loophole const& loophole, shared_str const& action_id) const
{
    auto found = std::find_if(m_vertices.begin(), m_vertices.end(),
        [&loophole] (cover::Vertex const& vertex) { return loophole.id()._get() == vertex.first->id()._get(); });
    VERIFY(found != m_vertices.end());

    auto found2 = std::find_if(
        found->second.m_action_vertices.begin(), found->second.m_action_vertices.end(),
        [action_id] (smart_cover::loophole_data::Action const& action) { return (action_id._get() == action.first._get()); });
    VERIFY(found2 != found->second.m_action_vertices.end());
    VERIFY(ai().level_graph().valid_vertex_id(found2->second));

    return (found2->second);
}

smart_cover::loophole* cover::best_loophole(
    Fvector const& position, float& value, bool const& use_default_behaviour, bool is_smart_cover_entered) const
{
    value = flt_max;

    loophole* result = 0;
    Loopholes::const_iterator I = loopholes().begin();
    Loopholes::const_iterator E = loopholes().end();
    for (; I != E; ++I)
    {
        loophole* loophole = (*I);
        if (use_default_behaviour)
            evaluate_loophole_for_default_usage(position, loophole, result, value);
        else
            evaluate_loophole(position, loophole, result, value, is_smart_cover_entered);
    }

    return (result);
}

void cover::evaluate_loophole(Fvector const& position, smart_cover::loophole*& source, smart_cover::loophole*& result,
    float& value, bool const is_smart_cover_entered) const
{
    VERIFY(source);
    VERIFY2(_valid(position), make_string("[%f][%f][%f]", VPUSH(position)));

    if (!source->usable())
        return;

    Fvector fov_position = this->fov_position(*source);
    VERIFY2(_valid(fov_position), make_string("[%f][%f][%f]", VPUSH(fov_position)));
    float const distance_to_target = fov_position.distance_to(position);
    if (distance_to_target > source->range())
        return;

    float const min_enemy_distance =
        is_smart_cover_entered ? object().enter_min_enemy_distance() : object().exit_min_enemy_distance();
    if (distance_to_target <= min_enemy_distance)
        return;

    Fvector direction = Fvector().sub(position, fov_position);
    VERIFY2(_valid(direction), make_string("[%f][%f][%f]", VPUSH(direction)));
    if (direction.magnitude() < 1.f)
        return;

    direction.normalize();
    float cos_alpha = this->fov_direction(*source).dotproduct(direction);

    float alpha = _abs(acosf(cos_alpha));
    if (alpha >= source->fov() / 2)
        return;

    if (alpha >= value)
        return;

    value = 2.f * alpha / source->fov();
    result = source;
}

void cover::evaluate_loophole_for_default_usage(
    Fvector const& position, smart_cover::loophole*& source, smart_cover::loophole*& result, float& value) const
{
    VERIFY(source);

    if (!source->usable())
        return;

    Fvector fov_position = this->fov_position(*source);
    Fvector direction = Fvector().sub(position, fov_position);
    direction.normalize_safe();
    float cos_alpha = this->fov_direction(*source).dotproduct(direction);
    float alpha = acosf(cos_alpha);
    if (alpha >= value)
        return;

    value = alpha;
    result = source;
}

struct loophole_predicate
{
    smart_cover::loophole const* m_loophole;

    IC loophole_predicate(smart_cover::loophole const* loophole) : m_loophole(loophole) {}
    IC bool operator()(cover::Vertex const& vertex) const { return (vertex.first == m_loophole); }
};

u32 const& cover::level_vertex_id(smart_cover::loophole const& loophole) const
{
    Vertices::const_iterator I = std::find_if(m_vertices.begin(), m_vertices.end(), loophole_predicate(&loophole));
    VERIFY(I != m_vertices.end());
    return ((*I).second.m_level_vertex_id);
}

#ifdef DEBUG
bool cover::loophole_path(shared_str const& source_raw, shared_str const& target_raw) const
{
    shared_str source = transform_vertex(source_raw, true);
    shared_str target = transform_vertex(target_raw, false);

    typedef GraphEngineSpace::CBaseParameters CBaseParameters;
    CBaseParameters parameters(u32(-1), u32(-1), u32(-1));
    bool result = ai().graph_engine().search(m_description->transitions(), source, target, 0, parameters);

    VERIFY2(result, make_string("failde to find loophole path [%s]->[%s] in cover [%s]", source.c_str(), target.c_str(),
                        m_description->table_id().c_str()));
    return (result);
}

void cover::check_loopholes_connectivity() const
{
    VERIFY(!loopholes().empty());

    shared_str enter = transform_vertex("", true);
    shared_str exit = transform_vertex("", false);

    Loopholes::const_iterator I = loopholes().begin();
    Loopholes::const_iterator E = loopholes().end();
    for (; I != E; ++I)
    {
        shared_str const& lhs = (*I)->id();
        Loopholes::const_iterator J = I + 1;
        for (; J != E; ++J)
        {
            shared_str const& rhs = (*J)->id();
            VERIFY2(loophole_path(lhs, rhs), make_string("failed to find path [%s -> %s] in smart_cover [%s]",
                                                 lhs.c_str(), rhs.c_str(), m_description->table_id().c_str()));
            VERIFY2(loophole_path(rhs, lhs), make_string("failed to find path [%s -> %s] in smart_cover [%s]",
                                                 rhs.c_str(), lhs.c_str(), m_description->table_id().c_str()));
        }

        VERIFY2(loophole_path(lhs, exit), make_string("failed to find path [%s -> %s] in smart_cover [%s]", lhs.c_str(),
                                              exit.c_str(), m_description->table_id().c_str()));
        VERIFY2(loophole_path(enter, lhs), make_string("failed to find path [%s -> %s] in smart_cover [%s]",
                                               enter.c_str(), lhs.c_str(), m_description->table_id().c_str()));
    }
}
#endif // DEBUG

static bool in_fov(
    Fvector const& position, Fvector const& fov_position, Fvector const& fov_direction, float const fov_angle)
{
    Fvector direction = Fvector().sub(position, fov_position);
    if (direction.magnitude() < 1.f)
        return (false);

    direction.normalize();
    float cos_alpha = fov_direction.dotproduct(direction);
    float alpha = acosf(cos_alpha);
    if (alpha >= fov_angle / 2)
        return (false);

    return (true);
}

bool cover::is_position_in_fov(smart_cover::loophole const& source, Fvector const& position) const
{
    return in_fov(position, fov_position(source), fov_direction(source), source.fov());
}

bool cover::is_position_in_danger_fov(smart_cover::loophole const& source, Fvector const& position) const
{
    return in_fov(position, fov_position(source), danger_fov_direction(source), source.danger_fov());
}

bool cover::is_position_in_range(smart_cover::loophole const& source, Fvector const& position) const
{
    Fvector fov_position = this->fov_position(source);
    if (fov_position.distance_to(position) > source.range())
        return (false);

    return (true);
}

bool cover::in_min_acceptable_range(
    smart_cover::loophole const& source, Fvector const& position, float const& min_range) const
{
    Fvector fov_position = this->fov_position(source);
    if (fov_position.distance_to(position) < min_range)
        return (false);

    return (true);
}
