////////////////////////////////////////////////////////////////////////////
//	Module 		: danger_location_inline.h
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Danger location inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	bool CDangerLocation::operator==	(const Fvector &position) const
{
	return		(!!this->position().similar(position));
}

IC	bool CDangerLocation::operator==	(const CObject *object) const
{
	return		(false);
}

IC	const CDangerLocation::flags &CDangerLocation::mask	() const
{
	return		(m_mask);
}
