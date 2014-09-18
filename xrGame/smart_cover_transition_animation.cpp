////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_transition_animation.cpp
//	Created 	: 20.12.2007
//	Author		: Alexander Dudin
//	Description : Animation transition class for smart_cover
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_transition_animation.hpp"


using smart_cover::transitions::animation_action;

animation_action::animation_action			(
		Fvector const& position,
		shared_str const& animation_id,
		MonsterSpace::EBodyState const& body_state,
		MonsterSpace::EMovementType const& movement_type
	) :
	m_position		(position),
	m_animation_id	(animation_id),
	m_body_state	(body_state),
	m_movement_type	(movement_type)
{
}