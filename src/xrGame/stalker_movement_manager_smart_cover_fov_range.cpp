////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_movement_manager_smart_cover_fov_range.cpp
//	Created 	: 14.02.2008
//	Modified	: 14.02.2008
//	Author		: Dmitriy Iassenev
//	Description : stalker movement manager class with smart covers fov and range stuff
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_movement_manager_smart_cover.h"
#include "ai/stalker/ai_stalker.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "memory_space.h"
#include "cover_manager.h"
#include "smart_cover.h"

bool stalker_movement_manager_smart_cover::fill_enemy_position(Fvector& position) const
{
    if (m_current.cover_fire_position())
    {
        position = *m_current.cover_fire_position();
        return (true);
    }

    CEntityAlive const* enemy = object().memory().enemy().selected();
    if (!enemy)
        return (false);

    position = object().memory().memory(enemy).m_object_params.m_position;
    return (true);
}

bool stalker_movement_manager_smart_cover::enemy_in_fov() const
{
    if (!m_current.cover())
        return (false);

    Fvector position;
    if (!fill_enemy_position(position))
        return (false);

    float value;
    if (m_current.cover()->best_loophole(position, value, false, true))
        return (true);

    VERIFY(m_current.cover_loophole());
    if (!m_current.cover()->is_position_in_fov(*m_current.cover_loophole(), position))
        return (false);

    if (!m_current.cover()->is_position_in_range(*m_current.cover_loophole(), position))
        return (false);

    return (true);
}

bool stalker_movement_manager_smart_cover::in_fov(
    shared_str const& cover_id, shared_str const& loophole_id, Fvector const& object_position) const
{
    cover_type const& cover = *(ai().cover_manager().smart_cover(cover_id));
    loophole_type const& loophole = this->loophole(cover, loophole_id);
    return (cover.is_position_in_fov(loophole, object_position));
}

bool stalker_movement_manager_smart_cover::in_current_loophole_fov(Fvector const& position) const
{
    VERIFY(m_current.cover() || entering_smart_cover_with_animation());
    VERIFY(entering_smart_cover_with_animation() || m_current.cover_loophole());
    if (m_current.cover())
        return (m_current.cover()->is_position_in_fov(*m_current.cover_loophole(), position));

    smart_cover::cover const* cover = ai().cover_manager().smart_cover(m_enter_cover_id);
    VERIFY(cover);

    smart_cover::loophole const* loophole = cover->get_description()->get_loophole(m_enter_loophole_id);
    return (cover->is_position_in_fov(*loophole, position));
}

bool stalker_movement_manager_smart_cover::in_current_loophole_range(Fvector const& position) const
{
    VERIFY(m_current.cover() || entering_smart_cover_with_animation());
    VERIFY(entering_smart_cover_with_animation() || m_current.cover_loophole());
    if (m_current.cover())
        return (m_current.cover()->is_position_in_range(*m_current.cover_loophole(), position));

    smart_cover::cover const* cover = ai().cover_manager().smart_cover(m_enter_cover_id);
    VERIFY(cover);

    smart_cover::loophole const* loophole = cover->get_description()->get_loophole(m_enter_loophole_id);
    return (cover->is_position_in_range(*loophole, position));
}

bool stalker_movement_manager_smart_cover::in_range(
    shared_str const& cover_id, shared_str const& loophole_id, Fvector const& object_position) const
{
    cover_type const& cover = *(ai().cover_manager().smart_cover(cover_id));
    loophole_type const& loophole = this->loophole(cover, loophole_id);
    return (cover.is_position_in_range(loophole, object_position));
}

bool stalker_movement_manager_smart_cover::in_min_acceptable_range(
    shared_str const& cover_id, shared_str const& loophole_id, Fvector const& position, float const& min_range) const
{
    cover_type const& cover = *(ai().cover_manager().smart_cover(cover_id));
    loophole_type const& loophole = this->loophole(cover, loophole_id);
    return (cover.in_min_acceptable_range(loophole, position, min_range));
}
