////////////////////////////////////////////////////////////////////////////
//	Module 		: property_string_values_value_shared_str.cpp
//	Created 	: 11.12.2007
//  Modified 	: 11.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property string values value class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_string_values_value_shared_str.hpp"

property_string_values_value_shared_str::property_string_values_value_shared_str	(
		editor::engine* engine,
		shared_str& value,
		LPCSTR const* values,
		u32 const &value_count
	) :
	inherited				(engine, value),
	m_collection			(gcnew collection_type())
{
	for (u32 i=0; i<value_count; ++i)
		m_collection->Enqueue	(to_string(values[i]));
}
