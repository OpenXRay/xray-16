////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_base_inline.h
//	Created 	: 17.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction base inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef DEBUG
IC	bool CSpaceRestrictionBase::correct	() const
{
	return	(m_correct);
}
#endif