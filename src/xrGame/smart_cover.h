////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover.h
//	Created 	: 16.08.2007
//	Author		: Alexander Dudin
//	Description : Smart cover class
////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef SMART_COVER_H_INCLUDED
#define SMART_COVER_H_INCLUDED

#include "Common/Noncopyable.hpp"
#include "smart_cover_description.h"
#include "cover_point.h"
#include "smart_cover_loophole.h"
#include "smart_cover_object.h"

namespace smart_cover
{
class storage;

struct loophole_data
{
    typedef std::pair<shared_str, u32> Action;
    typedef xr_vector<Action> ActionVertices;

    ActionVertices m_action_vertices;
    u32 m_level_vertex_id;
};

class cover final : public CCoverPoint, private Noncopyable
{
public:
    typedef intrusive_ptr<smart_cover::description, detail::intrusive_base_time> DescriptionPtr;
    typedef std::pair<loophole const*, loophole_data> Vertex;
    typedef xr_vector<Vertex> Vertices;
    typedef description::Loopholes Loopholes;

private:
    typedef CCoverPoint inherited;

private:
    DescriptionPtr m_description;
    Vertices m_vertices;
    Loopholes m_loopholes;
    object const& m_object;
    shared_str m_id;
    bool m_is_combat_cover;
    bool m_can_fire;

private:
    void vertex(loophole const& loophole, loophole_data& loophole_data);

public:
    cover(object const& object, DescriptionPtr description, bool is_combat_cover, bool can_fire,
        luabind::adl::object const& loopholes);
    ~cover();
    IC Loopholes const& loopholes() const;
    IC object const& get_object() const;
    IC Fvector fov_position(loophole const& loophole) const;
    IC Fvector fov_direction(loophole const& loophole) const;
    IC Fvector danger_fov_direction(loophole const& loophole) const;
    IC Fvector enter_direction(loophole const& loophole) const;
    u32 const& level_vertex_id(loophole const& loophole) const;
    IC Fvector position(Fvector const& position) const;
    u32 const& action_level_vertex_id(loophole const& loophole, shared_str const& action_id) const;
    loophole* best_loophole(
        Fvector const& position, float& value, bool const& use_default_behaviour, bool is_smart_cover_entered) const;
    IC DescriptionPtr const& get_description() const;
    void evaluate_loophole(
        Fvector const& position, loophole*& source, loophole*& result, float& value, bool is_smart_cover_entered) const;
    IC shared_str const& id() const;
    void evaluate_loophole_for_default_usage(
        Fvector const& position, smart_cover::loophole*& source, smart_cover::loophole*& result, float& value) const;
    bool is_position_in_fov(loophole const& source, Fvector const& position) const;
    bool is_position_in_danger_fov(loophole const& source, Fvector const& position) const;
    bool is_position_in_range(loophole const& source, Fvector const& position) const;
    bool in_min_acceptable_range(loophole const& source, Fvector const& position, float const& min_range) const;
    IC bool is_combat_cover() const;
    IC bool can_fire() const;

#ifdef DEBUG
private:
    bool loophole_path(shared_str const& source_raw, shared_str const& target_raw) const;
    void check_loopholes_connectivity() const;
#endif // DEBUG
};

} // namespace smart_cover

#include "smart_cover_inline.h"

#endif // SMART_COVER_H_INCLUDED
