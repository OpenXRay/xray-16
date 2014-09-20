////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_animation_selector_inline.h
//	Created 	: 07.09.2007
//	Author		: Alexander Dudin
//	Description : Animation selector for smart covers inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_ANIMATION_SELECTOR_INLINE_H_INCLUDED
#define SMART_COVER_ANIMATION_SELECTOR_INLINE_H_INCLUDED

namespace smart_cover {

IC	CPropertyStorage *animation_selector::property_storage	()
{
	return				(m_storage);
}

IC	animation_planner &animation_selector::planner			()
{
	return				(*m_planner);
}

} // namespace smart_cover

#endif // SMART_COVER_ANIMATION_SELECTOR_INLINE_H_INCLUDED