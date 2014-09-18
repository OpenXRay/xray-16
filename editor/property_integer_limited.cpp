////////////////////////////////////////////////////////////////////////////
//	Module 		: property_integer_limited.cpp
//	Created 	: 07.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : limited integer property implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_integer_limited.hpp"

property_integer_limited::property_integer_limited	(
		integer_getter_type const &getter,
		integer_setter_type const &setter,
		int const %min,
		int const %max
	) :
	inherited				(getter, setter),
	m_min					(min),
	m_max					(max)
{
}

System::Object ^property_integer_limited::get_value	()
{
	int						value = safe_cast<int>(inherited::get_value());
	if (value < m_min)
		value				= m_min;

	if (value > m_max)
		value				= m_max;

	return					(value);
}

void property_integer_limited::set_value			(System::Object ^object)
{
	int						new_value = safe_cast<int>(object);

	if (new_value < m_min)
		new_value			= m_min;

	if (new_value > m_max)
		new_value			= m_max;

	inherited::set_value	(new_value);
}