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

System::Object ^property_float_reference::GetValue	()
{
	return							(m_value->get());
}

void property_float_reference::SetValue			(System::Object ^object)
{
	float							value = safe_cast<float>(object);
	m_value->set					(value);
}

void property_float_reference::Increment			(float value)
{
	SetValue						(safe_cast<float>(GetValue()) + value*m_increment_factor);
}