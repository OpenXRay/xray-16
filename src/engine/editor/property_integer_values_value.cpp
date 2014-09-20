////////////////////////////////////////////////////////////////////////////
//	Module 		: property_integer_values_value.cpp
//	Created 	: 07.12.2007
//  Modified 	: 11.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property integer values value class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_integer_values_value.hpp"

using System::String;
using System::Object;
using System::Collections::IList;

property_integer_values_value::property_integer_values_value	(
		integer_getter_type const &getter,
		integer_setter_type const &setter,
		LPCSTR const* values,
		u32 const &value_count
	) :
	inherited				(getter, setter),
	m_collection			(gcnew collection_type())
{
	for (u32 i=0; i<value_count; ++i)
		m_collection->Add	(to_string(values[i]));
}

Object^ property_integer_values_value::get_value				()
{
	int						value = safe_cast<int>(inherited::get_value());
	if (value < 0)
		value				= 0;

	if (value >= m_collection->Count)
		value				= (int)m_collection->Count - 1;

	return					(value);
}

void property_integer_values_value::set_value					(Object ^object)
{
	String^					string_value = dynamic_cast<String^>(object);
	int						index = m_collection->IndexOf(string_value);
	VERIFY					((index >= 0));
	inherited::set_value	(index);
}

IList^ property_integer_values_value::collection				()
{
	return					(m_collection);
}
