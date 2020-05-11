////////////////////////////////////////////////////////////////////////////
//	Module 		: property_boolean.cpp
//	Created 	: 10.12.2007
//  Modified 	: 10.12.2007
//	Author		: Dmitriy Iassenev
//	Description : boolean property implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_boolean.hpp"

property_boolean::property_boolean(boolean_getter_type const& getter, boolean_setter_type const& setter)
    : m_getter(xr_new<boolean_getter_type>(getter)), m_setter(xr_new<boolean_setter_type>(setter))
{
}

property_boolean::~property_boolean() { this->!property_boolean(); }
property_boolean::!property_boolean()
{
    delete (m_getter);
    delete (m_setter);
}

System::Object ^ property_boolean::GetValue() { return ((*m_getter)()); }
void property_boolean::SetValue(System::Object ^ object) { (*m_setter)(safe_cast<bool>(object)); }
