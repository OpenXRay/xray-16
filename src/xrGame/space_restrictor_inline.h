////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restrictor_inline.h
//	Created 	: 17.08.2004
//  Modified 	: 17.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restrictor inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CSpaceRestrictor::CSpaceRestrictor		()
{
	m_space_restrictor_type = RestrictionSpace::eRestrictorTypeNone;
}

IC	bool CSpaceRestrictor::actual			() const
{
	return							(m_actuality);
}

IC	void CSpaceRestrictor::actual			(bool value) const
{
	m_actuality						= value;
}

IC RestrictionSpace::ERestrictorTypes CSpaceRestrictor::restrictor_type	() const
{
	return RestrictionSpace::ERestrictorTypes(m_space_restrictor_type);
}