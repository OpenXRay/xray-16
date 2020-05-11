////////////////////////////////////////////////////////////////////////////
//	Module 		: property_integer.cpp
//	Created 	: 07.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : integer property implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_integer.hpp"

property_integer::property_integer(integer_getter_type const& getter, integer_setter_type const& setter)
    : m_getter(xr_new<integer_getter_type>(getter)), m_setter(xr_new<integer_setter_type>(setter))
{
}

property_integer::~property_integer() { this->!property_integer(); }
property_integer::!property_integer()
{
    delete (m_getter);
    delete (m_setter);
}

System::Object ^ property_integer::GetValue() { return ((*m_getter)()); }
void property_integer::SetValue(System::Object ^ object) { (*m_setter)(safe_cast<int>(object)); }
