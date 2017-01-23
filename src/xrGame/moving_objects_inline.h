////////////////////////////////////////////////////////////////////////////
//	Module 		: moving_objects_inline.h
//	Created 	: 27.03.2007
//  Modified 	: 27.03.2007
//	Author		: Dmitriy Iassenev
//	Description : moving objects inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef MOVING_OBJECTS_INLINE_H
#define MOVING_OBJECTS_INLINE_H

IC	const moving_objects::COLLISIONS &moving_objects::collisions	() const
{
	return	(m_previous_collisions);
}

#endif // MOVING_OBJECTS_INLINE_H