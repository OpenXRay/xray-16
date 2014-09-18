////////////////////////////////////////////////////////////////////////////
//	Module 		: moving_objects.cpp
//	Created 	: 28.03.2007
//  Modified 	: 28.03.2007
//	Author		: Dmitriy Iassenev
//	Description : moving objects
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "moving_object.h"
#include "ai_space.h"
#include "moving_objects.h"

moving_object::moving_object			(const CEntityAlive *object)
{
	VERIFY				(object);
	m_object			= object;
	
	m_action			= action_move;
	m_action_position	= Fvector().set(flt_max,flt_max,flt_max);
	m_action_frame		= 0;
	m_action_time		= 0;

	update_position		();

	ai().moving_objects().register_object	(this);
}

moving_object::~moving_object			()
{
	ai().moving_objects().unregister_object	(this);
}

void moving_object::on_object_move		()
{
	ai().moving_objects().on_object_move	(this);
}

void moving_object::update_position		()
{
	m_position			= m_object->Position();
}

Fvector moving_object::predict_position	(const float &time_to_check) const
{
	return				(object().predict_position(time_to_check));
}

Fvector moving_object::target_position	() const
{
	return				(object().target_position());
}

void moving_object::ignore				(const CObject *object)
{
	m_ignored_object	= object;
}

bool moving_object::ignored				(const CObject *object)
{
	if (object == m_object)
		return			(true);

	if (object == m_ignored_object)
		return			(true);

	return				(false);
}
