////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_object.h
//	Created 	: 28.08.2007
//  Modified 	: 28.08.2007
//	Author		: Dmitriy Iassenev
//	Description : smart cover object class
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_OBJECT_INLINE_H_INCLUDED
#define SMART_COVER_OBJECT_INLINE_H_INCLUDED

IC	float const	&smart_cover::object::enter_min_enemy_distance	() const
{
	return	(m_enter_min_enemy_distance);
}

IC	float const	&smart_cover::object::exit_min_enemy_distance	() const
{
	return	(m_exit_min_enemy_distance);
}

IC	smart_cover::cover const& smart_cover::object::cover		() const
{
	VERIFY	(m_cover);
	return	(*m_cover);
}

#endif // SMART_COVER_OBJECT_INLINE_H_INCLUDED