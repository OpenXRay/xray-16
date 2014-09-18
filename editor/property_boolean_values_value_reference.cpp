////////////////////////////////////////////////////////////////////////////
//	Module 		: property_boolean_values_value_reference.cpp
//	Created 	: 17.12.2007
//  Modified 	: 17.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property boolean values value reference class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_boolean_values_value_reference.hpp"

using System::String;

property_boolean_values_value_reference::property_boolean_values_value_reference(
		bool& value,
		LPCSTR values[2]
	) :
	inherited				(value),
	m_collection			(gcnew collection_type())
{
	for (u32 i=0; i<2; ++i)
		m_collection->Add	(to_string(values[i]));
}

void property_boolean_values_value_reference::set_value							(Object ^object)
{
	String^					string_value = dynamic_cast<String^>(object);
	int						index = m_collection->IndexOf(string_value);
	VERIFY					((index < 2));
	VERIFY					((index >= 0));
	inherited::set_value	((index == 1));
}