////////////////////////////////////////////////////////////////////////////
//	Module 		: rat_state_base_inline.h
//	Created 	: 31.08.2007
//  Modified 	: 31.08.2007
//	Author		: Dmitriy Iassenev
//	Description : rat state base class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef RAT_STATE_BASE_INLINE_H_INCLUDED
#define RAT_STATE_BASE_INLINE_H_INCLUDED

IC	rat_state_base::rat_state_base	() :
	m_object(0)
{
}

IC	CAI_Rat &rat_state_base::object	() const
{
	VERIFY	(m_object);
	return	(*m_object);
}

#endif // RAT_STATE_BASE_INLINE_H_INCLUDED