////////////////////////////////////////////////////////////////////////////
//	Module 		: moving_objects_impl.h
//	Created 	: 14.05.2007
//  Modified 	: 14.05.2007
//	Author		: Dmitriy Iassenev
//	Description : moving objects implementation dependant inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef MOVING_OBJECTS_IMPL_H
#define MOVING_OBJECTS_IMPL_H

static const float step_to_check			= .5f;
static const float time_to_check			= 1.0f;
static const float wait_radius_factor		= 2.0f;
static const u32   inertia_time_ms			= 500;
static const float additional_radius		= 2.f;
static const float max_linear_velocity		= 10.f;

IC	bool moving_objects::collided			(const CObject *object, const Fvector &position, const float &radius) const
{
	return	(object->Position().distance_to(position) <= (object->Radius() + radius));
}

#endif // MOVING_OBJECTS_IMPL_H