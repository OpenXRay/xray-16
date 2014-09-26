////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_manager_inline.h
//	Created 	: 17.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef DEBUG
IC	const CSpaceRestrictionManager::SPACE_RESTRICTIONS &CSpaceRestrictionManager::restrictions	() const
{
	return								(m_space_restrictions);
}
#endif

template <typename T1, typename T2>
IC	void CSpaceRestrictionManager::add_border					(ALife::_OBJECT_ID id, T1 p1, T2 p2)
{
	CRestrictionPtr						client_restriction = restriction(id);
	if (client_restriction)
		client_restriction->add_border	(p1,p2);
}
