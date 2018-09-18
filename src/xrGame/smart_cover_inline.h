////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_inline.h
//	Created 	: 16.08.2007
//	Author		: Alexander Dudin
//	Description : Smart cover class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_H_INLINE_INCLUDED
#define SMART_COVER_H_INLINE_INCLUDED

#define cover smart_cover::cover

IC smart_cover::object const& cover::get_object() const { return (m_object); }
IC cover::Loopholes const& cover::loopholes() const { return (m_loopholes); }
IC Fvector cover::fov_position(loophole const& loophole) const
{
    Fvector position;
    m_object.XFORM().transform_tiny(position, loophole.fov_position());
    return (position);
}

IC Fvector cover::fov_direction(loophole const& loophole) const
{
    Fvector direction;
    m_object.XFORM().transform_dir(direction, loophole.fov_direction());
    direction.normalize();
    return (direction);
}

IC Fvector cover::danger_fov_direction(loophole const& loophole) const
{
    Fvector direction;
    m_object.XFORM().transform_dir(direction, loophole.danger_fov_direction());
    direction.normalize();
    return (direction);
}

IC Fvector cover::enter_direction(loophole const& loophole) const
{
    Fvector direction;
    m_object.XFORM().transform_dir(direction, loophole.enter_direction());
    direction.normalize();
    return (direction);
}

IC Fvector cover::position(Fvector const& position) const
{
    Fvector pos;
    m_object.XFORM().transform_tiny(pos, position);
    return (pos);
}

IC cover::DescriptionPtr const& cover::get_description() const { return (m_description); }
IC shared_str const& cover::id() const { return (m_id); }
IC bool cover::is_combat_cover() const { return (m_is_combat_cover); }
IC bool cover::can_fire() const { return (m_is_combat_cover || m_can_fire); }
#undef cover

#endif // SMART_COVER_H_INLINE_INCLUDED
