////////////////////////////////////////////////////////////////////////////
//	Module 		: property_integer_reference.cpp
//	Created 	: 17.12.2007
//  Modified 	: 17.12.2007
//	Author		: Dmitriy Iassenev
//	Description : integer property reference implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_integer_reference.hpp"

property_integer_reference::property_integer_reference(int& value) : m_value(xr_new<value_holder<int>>(value)) {}
property_integer_reference::~property_integer_reference() { this->!property_integer_reference(); }
property_integer_reference::!property_integer_reference() { delete (m_value); }
System::Object ^ property_integer_reference::GetValue() { return (m_value->get()); }
void property_integer_reference::SetValue(System::Object ^ object)
{
    int value = safe_cast<int>(object);
    m_value->set(value);
}
