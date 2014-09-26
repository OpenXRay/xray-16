////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_script.h
//	Created 	: 25.02.2003
//  Modified 	: 19.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation script
////////////////////////////////////////////////////////////////////////////

#pragma once

class CStalkerAnimationScript {
private:
	Fmatrix					m_transform;
	Fmatrix const*			m_transform_ptr;
	MotionID				m_animation;
	bool					m_hand_usage;
	bool					m_use_movement_controller;
	bool					m_local_animation;

public:
	IC						CStalkerAnimationScript	(const MotionID &animation, bool hand_usage, bool use_movement_controller, Fmatrix const* transform = 0, bool local_animation = true);
	IC						CStalkerAnimationScript	(CStalkerAnimationScript const& object);
	IC	const MotionID&		animation				() const;
	IC	const bool&			hand_usage				() const;
	IC	const bool&			use_movement_controller	() const;
	IC	const bool&			local_animation			() const;
	IC	Fmatrix const&		transform				(CObject const& object) const;
	IC	bool				has_transform			() const;
};

#include "stalker_animation_script_inline.h"
