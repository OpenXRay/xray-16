////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_action_inline.h
//	Created 	: 03.09.2007
//	Author		: Alexander Dudin
//	Description : Inline functions for action class
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_ACTION_INLINE_H_INCLUDED
#define SMART_COVER_ACTION_INLINE_H_INCLUDED

namespace smart_cover {

IC	bool		const	&action::movement			() const
{
	return			(m_movement);
}

IC	Fvector		const	&action::target_position	() const
{
	return			(m_target_position);
}

IC	action::Animations const &action::animations	(shared_str const& cover_id, shared_str const &id) const
{
	AnimationList::const_iterator found = m_animations.find(id);
	VERIFY2			(found != m_animations.end(), make_string("can't find animation %s in smart cover %s", id.c_str(), cover_id.c_str()));
	return			(*found->second);
}

} // namespace smart_cover

#endif // SMART_COVER_ACTION_INLINE_H_INCLUDED