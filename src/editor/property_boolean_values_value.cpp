////////////////////////////////////////////////////////////////////////////
//	Module 		: property_boolean_values_value.cpp
//	Created 	: 07.12.2007
//  Modified 	: 11.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property boolean values value class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_boolean_values_value.hpp"

using System::String;

property_boolean_values_value::property_boolean_values_value	(
		boolean_getter_type const &getter,
		boolean_setter_type const &setter,
		LPCSTR values[2]
	) :
	inherited				(getter, setter),
	m_collection			(gcnew collection_type())
{
	for (u32 i=0; i<2; ++i)
		m_collection->Add	(to_string(values[i]));
}

void property_boolean_values_value::set_value					(Object ^object)
{
	String^					string_value = dynamic_cast<String^>(object);
	int						index = m_collection->IndexOf(string_value);
	VERIFY					((index < 2));
	VERIFY					((index >= 0));
	inherited::set_value	((index == 1));
}