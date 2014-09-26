////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_script_inline.h
//	Created 	: 25.02.2003
//  Modified 	: 19.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation script inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CStalkerAnimationScript::CStalkerAnimationScript				(
		const MotionID &animation,
		bool hand_usage,
		bool use_movement_controller,
		Fmatrix const* transform,
		bool local_animation
	) :
	m_animation					(animation),
	m_hand_usage				(hand_usage),
	m_use_movement_controller	(use_movement_controller),
	m_local_animation			(local_animation)
{
	if (transform) {
		m_transform				= *transform;
		m_transform_ptr			= &m_transform;
		return;
	}

	m_transform_ptr				= 0;
	m_transform.i.set			(flt_max, flt_max, flt_max);
	m_transform.j.set			(flt_max, flt_max, flt_max);
	m_transform.k.set			(flt_max, flt_max, flt_max);
	m_transform.c.set			(flt_max, flt_max, flt_max);
}

IC CStalkerAnimationScript::CStalkerAnimationScript					(CStalkerAnimationScript const& object)
{
	m_transform					= object.m_transform;
	m_transform_ptr				= object.m_transform_ptr ? &m_transform : 0;
	m_animation					= object.m_animation;
	m_hand_usage				= object.m_hand_usage;
	m_use_movement_controller	= object.m_use_movement_controller;
	m_local_animation			= object.m_local_animation;
}

IC	const MotionID &CStalkerAnimationScript::animation				() const
{
	return						(m_animation);
}

IC	const bool &CStalkerAnimationScript::hand_usage					() const
{
	return						(m_hand_usage);
}

IC	const bool &CStalkerAnimationScript::use_movement_controller	() const
{
	return						(m_use_movement_controller);
}

IC	const bool &CStalkerAnimationScript::local_animation			() const
{
	return						(m_local_animation);
}

IC	Fmatrix const& CStalkerAnimationScript::transform				(CObject const& object) const
{
	if (m_transform_ptr)
		return					(*m_transform_ptr);

	return						(object.XFORM());
}

IC	bool CStalkerAnimationScript::has_transform						() const
{
	return						( !!m_transform_ptr );
}