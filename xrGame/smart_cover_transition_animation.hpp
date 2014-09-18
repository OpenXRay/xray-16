////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_transition_animation.hpp
//	Created 	: 20.12.2007
//	Author		: Alexander Dudin
//	Description : Animation transition class for smart_cover
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_TRANSITION_ANIMATION_HPP_INCLUDED
#define SMART_COVER_TRANSITION_ANIMATION_HPP_INCLUDED

#include "debug_make_final.hpp"
#include "ai_monster_space.h"

namespace smart_cover {

namespace transitions {

class animation_action :
	private debug::make_final<animation_action>,
	private boost::noncopyable 
{
private:
	Fvector									m_position;
	shared_str								m_animation_id;
	MonsterSpace::EBodyState				m_body_state;
	MonsterSpace::EMovementType				m_movement_type;

public:
											animation_action(
												Fvector const& position,
												shared_str const& animation_id,
												MonsterSpace::EBodyState const& body_state,
												MonsterSpace::EMovementType const& movement_type
											);
	IC	bool								has_animation	() const;
	IC	Fvector const&						position		() const;
	IC	shared_str const&					animation_id	() const;
	IC	MonsterSpace::EBodyState const&		body_state		() const;
	IC	MonsterSpace::EMovementType const&	movement_type	() const;
};

} // namespace transitions

} // namespace smart_cover

#include "smart_cover_transition_animation_inline.hpp"

#endif // SMART_COVER_TRANSITION_ANIMATION_HPP_INCLUDED