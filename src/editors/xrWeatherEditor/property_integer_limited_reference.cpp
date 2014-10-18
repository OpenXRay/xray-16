////////////////////////////////////////////////////////////////////////////
//	Module 		: property_integer_limited_reference.cpp
//	Created 	: 17.12.2007
//  Modified 	: 17.12.2007
//	Author		: Dmitriy Iassenev
//	Description : limited integer property reference implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_integer_limited_reference.hpp"

property_integer_limited_reference::property_integer_limited_reference	(
		int& value,
		int const %min,
		int const %max
	) :
	inherited				(value),
	m_min					(min),
	m_max					(max)
{
}

System::Object ^property_integer_limited_reference::GetValue			()
{
	int						value = safe_cast<int>(inherited::GetValue());
	if (value < m_min)
		value				= m_min;

	if (value > m_max)
		value				= m_max;

	return					(value);
}

void property_integer_limited_reference::SetValue						(System::Object ^object)
{
	int						new_value = safe_cast<int>(object);

	if (new_value < m_min)
		new_value			= m_min;

	if (new_value > m_max)
		new_value			= m_max;

	inherited::SetValue	(new_value);
}