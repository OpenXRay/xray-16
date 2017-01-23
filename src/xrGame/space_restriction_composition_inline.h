////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_composition_inline.h
//	Created 	: 17.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction composition inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CSpaceRestrictionComposition::CSpaceRestrictionComposition	(CSpaceRestrictionHolder *space_restriction_holder, shared_str space_restrictors)
{
	VERIFY						(space_restriction_holder);
	m_space_restriction_holder	= space_restriction_holder;
	m_space_restrictors			= space_restrictors;
#ifdef DEBUG
	check_restrictor_type		();
#endif // DEBUG
	++g_restriction_checker;
}

IC	shared_str	CSpaceRestrictionComposition::name					() const
{
	return						(m_space_restrictors);
}

IC	bool CSpaceRestrictionComposition::shape					() const
{
	return						(false);
}

IC	bool CSpaceRestrictionComposition::default_restrictor		() const
{
	return						(false);
}
