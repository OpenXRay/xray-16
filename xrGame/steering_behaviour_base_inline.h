////////////////////////////////////////////////////////////////////////////
//	Module 		: steering_behaviour_base_inline.h
//	Created 	: 07.11.2007
//  Modified 	: 07.11.2007
//	Author		: Dmitriy Iassenev
//	Description : steering behaviour base class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef STEERING_BEHAVIOUR_BASE_INLINE_H_INCLUDED
#define STEERING_BEHAVIOUR_BASE_INLINE_H_INCLUDED

#define Base steering_behaviour::base

void Base::enabled			(bool const &value)
{
	m_enabled	= value;
}

bool const &Base::enabled	() const
{
	return		(m_enabled);
}

#undef Base

#endif // STEERING_BEHAVIOUR_BASE_INLINE_H_INCLUDED