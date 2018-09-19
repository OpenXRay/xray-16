////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_movement_params.cpp
//	Created 	: 23.12.2005
//  Modified 	: 23.12.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker movement parameters class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_movement_params.h"
#include "ai_monster_space.h"
#include "movement_manager_space.h"
#include "detail_path_manager_space.h"
#include "ai_space.h"
#include "cover_manager.h"
#include "smart_cover.h"
#include "smart_cover_description.h"
#include "stalker_movement_manager_smart_cover.h"

static const u32 time_before_selection = 2000;

stalker_movement_params::stalker_movement_params()
    : m_manager(0), m_body_state(MonsterSpace::eBodyStateStand), m_movement_type(MonsterSpace::eMovementTypeStand),
      m_mental_state(MonsterSpace::eMentalStateDanger), m_path_type(MovementManager::ePathTypeNoPath),
      m_detail_path_type(DetailPathManager::eDetailPathTypeSmooth),

      m_desired_position_impl(Fvector().set(flt_max, flt_max, flt_max)), m_desired_position(0),

      m_desired_direction_impl(Fvector().set(flt_max, flt_max, flt_max)), m_desired_direction(0),

      m_cover_id(""), m_cover(0), m_cover_loophole_id(""), m_cover_loophole(0), m_cover_fire_object(0),

      m_cover_fire_position_impl(Fvector().set(flt_max, flt_max, flt_max)), m_cover_fire_position(0),

      m_selected_loophole_actual(false), m_last_selection_time(0), m_cover_selected_loophole(0)
{
}

stalker_movement_params& stalker_movement_params::operator=(stalker_movement_params const& rhs)
{
    m_body_state = rhs.m_body_state;
    m_movement_type = rhs.m_movement_type;
    m_mental_state = rhs.m_mental_state;
    m_path_type = rhs.m_path_type;
    m_detail_path_type = rhs.m_detail_path_type;

    m_desired_position_impl = rhs.m_desired_position_impl;
    m_desired_position = rhs.m_desired_position ? &m_desired_position_impl : 0;

    m_desired_direction_impl = rhs.m_desired_direction_impl;
    m_desired_direction = rhs.m_desired_direction ? &m_desired_direction_impl : 0;

    m_cover_id = rhs.m_cover_id;
    m_cover_loophole_id = rhs.m_cover_loophole_id;
    m_cover_fire_object = rhs.m_cover_fire_object;

    m_cover_fire_position_impl = rhs.m_cover_fire_position_impl;
    m_cover_fire_position = rhs.m_cover_fire_position ? &m_cover_fire_position_impl : 0;

    m_selected_loophole_actual = rhs.m_selected_loophole_actual;
    m_last_selection_time = rhs.m_last_selection_time;
    m_cover_selected_loophole = rhs.m_cover_selected_loophole;

    return (*this);
}

bool stalker_movement_params::equal_to_target(stalker_movement_params const& target) const
{
    if (m_detail_path_type != target.m_detail_path_type)
        return (false);

    if (m_path_type != target.m_path_type)
        return (false);

    if (m_mental_state != target.m_mental_state)
        return (false);

    if (m_movement_type != target.m_movement_type)
        return (false);

    if (m_body_state != target.m_body_state)
        return (false);

    if (!m_desired_direction_impl.similar(target.m_desired_direction_impl))
        return (false);

    if (!m_desired_position_impl.similar(target.m_desired_position_impl))
        return (false);

    if (m_cover_id != target.m_cover_id)
        return (false);

    if (m_cover_fire_object != target.m_cover_fire_object)
        return (false);

    if (!m_cover_fire_position_impl.similar(target.m_cover_fire_position_impl))
        return (false);

    if (!target.m_cover_loophole)
    {
        if (m_cover_loophole != target.m_cover_selected_loophole)
            return (false);
    }
    else
    {
        if (m_cover_loophole != target.m_cover_loophole)
            return (false);
    }

    return (true);
}

void stalker_movement_params::cover_id(shared_str const& cover_id)
{
    if (m_cover_id == cover_id)
        return;

    m_cover_id = cover_id;

    cover_loophole_id("");
    VERIFY(!m_cover_loophole);

    m_selected_loophole_actual = false;
    m_cover_selected_loophole = 0;

    if (!cover_id.size())
    {
        m_cover = 0;
        return;
    }

    m_cover = ai().cover_manager().smart_cover(cover_id);
}

struct loophole_id_predicate
{
    shared_str m_id;

    IC loophole_id_predicate(shared_str const& id) : m_id(id) {}
    IC bool operator()(smart_cover::loophole* loophole) const { return (loophole->id()._get() == m_id._get()); }
}; // struct loophole_id_predicate

void stalker_movement_params::cover_loophole_id(shared_str const& loophole_id)
{
    cover_fire_object(0);
    cover_fire_position(0);

    if (m_cover_loophole_id == loophole_id)
        return;

    m_cover_loophole_id = loophole_id;
    m_selected_loophole_actual = false;
    m_cover_selected_loophole = 0;

    if (!loophole_id.size())
    {
        m_cover_loophole = 0;
        return;
    }

    VERIFY(m_cover);

    typedef smart_cover::cover::Loopholes Loopholes;
    Loopholes const& loopholes = m_cover->get_description()->loopholes();
    Loopholes::const_iterator i = std::find_if(loopholes.begin(), loopholes.end(), loophole_id_predicate(loophole_id));

    VERIFY2(i != loopholes.end(),
        make_string("loophole [%s] not present in smart_cover [%s]", loophole_id.c_str(), m_cover_id.c_str()));

    m_cover_loophole = *i;
}

void stalker_movement_params::actualize_loophole() const
{
    if (m_selected_loophole_actual)
    {
        if (!m_cover || !m_cover_selected_loophole || m_cover->get_description()->get_loophole(m_cover_selected_loophole->id()))
        {
            if (m_last_selection_time + time_before_selection > Device.dwTimeGlobal)
                return;
        }
    }

    m_selected_loophole_actual = true;
    m_last_selection_time = Device.dwTimeGlobal;

    float value;
    Fvector position = m_manager->position_to_cover_from();
    m_cover_selected_loophole =
        m_cover->best_loophole(position, value, true, m_manager->current_params().cover() == m_cover);
}

LPCSTR stalker_movement_params::cover_loophole_id() const
{
    VERIFY(m_cover);

    if (m_cover_loophole)
    {
        VERIFY(m_cover_loophole_id == m_cover_loophole->id());
        VERIFY(m_cover->get_description()->get_loophole(m_cover_loophole_id));
        return (m_cover_loophole_id.c_str());
    }

    actualize_loophole();
    VERIFY(!m_cover_selected_loophole || m_cover->get_description()->get_loophole(m_cover_selected_loophole->id()));
    return (m_cover_selected_loophole ? m_cover_selected_loophole->id().c_str() : "");
}

smart_cover::loophole const* stalker_movement_params::cover_loophole() const
{
    VERIFY(m_cover);

    if (m_cover_loophole)
    {
        VERIFY(m_cover_loophole_id == m_cover_loophole->id());
        VERIFY(m_cover->get_description()->get_loophole(m_cover_loophole_id));
        return (m_cover_loophole);
    }

    actualize_loophole();
    VERIFY(!m_cover_selected_loophole || m_cover->get_description()->get_loophole(m_cover_selected_loophole->id()));
    return (m_cover_selected_loophole);
}
