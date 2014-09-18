////////////////////////////////////////////////////////////////////////////
//	Module 		: property_float_reference.cpp
//	Created 	: 17.12.2007
//  Modified 	: 17.12.2007
//	Author		: Dmitriy Iassenev
//	Description : float property reference implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_float_reference.hpp"

property_float_reference::property_float_reference			(float& value, float const% increment_factor) :
	m_value							(new value_holder<float>(value)),
	m_increment_factor				(increment_factor)
{
	VERIFY							(m_increment_factor > 0.f);
}

property_float_reference::~property_float_reference			()
{
	this->!property_float_reference	();
}

property_float_reference::!property_float_reference			()
{
	delete							(m_value);
}

System::Object ^property_float_reference::get_value	()
{
	return							(m_value->get());
}

void property_float_reference::set_value			(System::Object ^object)
{
	float							value = safe_cast<float>(object);
	m_value->set					(value);
}

void property_float_reference::increment			(float const% increment)
{
	set_value						(safe_cast<float>(get_value()) + increment*m_increment_factor);
}