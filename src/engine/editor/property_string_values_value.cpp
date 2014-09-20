////////////////////////////////////////////////////////////////////////////
//	Module 		: property_string_values_value.cpp
//	Created 	: 11.12.2007
//  Modified 	: 11.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property string values value class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_string_values_value.hpp"

property_string_values_value::property_string_values_value	(
		string_getter_type const &getter,
		string_setter_type const &setter,
		LPCSTR const* values,
		u32 const &value_count
	) :
	inherited				(getter, setter),
	m_collection			(gcnew collection_type())
{
	for (u32 i=0; i<value_count; ++i)
		m_collection->Enqueue	(to_string(values[i]));
}
