////////////////////////////////////////////////////////////////////////////
//	Module 		: property_storage_inline.h
//	Created 	: 29.03.2004
//  Modified 	: 29.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Property storage class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	void CPropertyStorage::set_property	(const _condition_type &condition_id, const _value_type &value)
{
	CConditionStorage::iterator			I = std::find(m_storage.begin(),m_storage.end(),condition_id);
	if (m_storage.end() != I)
		(*I).m_value					= value;
	else
		m_storage.push_back				(CConditionValue(condition_id,value));
}

IC	const CPropertyStorage::_value_type	&CPropertyStorage::property	(const _condition_type &condition_id) const
{
	CConditionStorage::const_iterator	I = std::find(m_storage.begin(),m_storage.end(),condition_id);
	THROW								(m_storage.end() != I);
	return								((*I).m_value);
}

IC	void CPropertyStorage::clear		()
{
	m_storage.clear						();
}
