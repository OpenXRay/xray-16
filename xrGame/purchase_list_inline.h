////////////////////////////////////////////////////////////////////////////
//	Module 		: purchase_list_inline.h
//	Created 	: 12.01.2006
//  Modified 	: 12.01.2006
//	Author		: Dmitriy Iassenev
//	Description : purchase list class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	float CPurchaseList::deficit							(const shared_str &section) const
{
	DEFICITS::const_iterator	I = m_deficits.find(section);
    if (I != m_deficits.end())
		return					((*I).second);

	return						(1.f);
}

IC	const CPurchaseList::DEFICITS &CPurchaseList::deficits	() const
{
	return						(m_deficits);
}

IC	void CPurchaseList::deficit								(const shared_str &section, const float &deficit)
{
	DEFICITS::iterator			I = m_deficits.find(section);
	if (I != m_deficits.end()) {
		(*I).second				= deficit;
		return;
	}

	m_deficits.insert			(std::make_pair(section,deficit));
}
