////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_description_inline.h
//	Created 	: 29.08.2007
//	Author		: Alexander Dudin
//	Description : Smart cover description class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_DESCRIPTION_INLINE_H_INCLUDED
#define SMART_COVER_DESCRIPTION_INLINE_H_INCLUDED

namespace smart_cover{

IC	shared_str const &description::table_id						() const
{
	return	(m_table_id);
}

IC	description::Loopholes const &description::loopholes		() const
{
	return	(m_loopholes);
}

IC	description::TransitionGraph const &description::transitions() const
{
	return	(m_transitions);
}

} // namespace smart_cover

#endif //SMART_COVER_DESCRIPTION_INLINE_H_INCLUDED