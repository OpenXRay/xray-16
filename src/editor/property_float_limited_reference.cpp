////////////////////////////////////////////////////////////////////////////
//	Module 		: property_float_limited_reference.cpp
//	Created 	: 17.12.2007
//  Modified 	: 17.12.2007
//	Author		: Dmitriy Iassenev
//	Description : float property reference implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_float_limited_reference.hpp"

property_float_limited_reference::property_float_limited_reference			(
		float& value,
		float const% increment_factor,
		float const %min,
		float const %max
	) :
	inherited				(value, increment_factor),
	m_min					(min),
	m_max					(max)
{
}

System::Object ^property_float_limited_reference::get_value		()
{
	float					value = safe_cast<float>(inherited::get_value());
	if (value < m_min)
		value				= m_min;

	if (value > m_max)
		value				= m_max;

	return					(value);
}

void property_float_limited_reference::set_value			(System::Object ^object)
{
	float					new_value = safe_cast<float>(object);

	if (new_value < m_min)
		new_value			= m_min;

	if (new_value > m_max)
		new_value			= m_max;

	inherited::set_value	(new_value);
}	