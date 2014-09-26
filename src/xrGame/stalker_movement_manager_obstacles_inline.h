////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_movement_manager_obstacles_inline.h
//	Created 	: 27.12.2003
//	Modified	: 13.02.2008
//	Author		: Dmitriy Iassenev
//	Description : stalker movement manager class with obstacles avoiding inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef STALKER_MOVEMENT_MANAGER_OBSTACLES_INLINE_H_INCLUDED
#define STALKER_MOVEMENT_MANAGER_OBSTACLES_INLINE_H_INCLUDED

IC	CRestrictedObjectObstacle &stalker_movement_manager_obstacles::restricted_object() const
{
	VERIFY	(m_restricted_object);
	return	(*m_restricted_object);
}

#endif // #ifndef STALKER_MOVEMENT_MANAGER_OBSTACLES_INLINE_H_INCLUDED