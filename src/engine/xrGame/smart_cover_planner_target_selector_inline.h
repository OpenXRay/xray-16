////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_planner_target_selector_inline.h
//	Created 	: 18.09.2007
//	Author		: Alexander Dudin
//	Description : Target selector for smart covers animation planner inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_PLANNER_TARGET_SELECTOR_INLINE_H_INCLUDED
#define SMART_COVER_PLANNER_TARGET_SELECTOR_INLINE_H_INCLUDED

namespace smart_cover {

IC target_selector::callback_type const& target_selector::callback	() const
{
	return	(m_script_callback);
}

} // namespace smart_cover

#endif // SMART_COVER_PLANNER_TARGET_SELECTOR_INLINE_H_INCLUDED