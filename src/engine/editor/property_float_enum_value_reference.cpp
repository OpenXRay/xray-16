////////////////////////////////////////////////////////////////////////////
//	Module 		: property_float_enum_value_reference.cpp
//	Created 	: 17.12.2007
//  Modified 	: 17.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property float enum value reference class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_float_enum_value_reference.hpp"

using System::String;

property_float_enum_value_reference::property_float_enum_value_reference	(
		float& value,
		pair *values,
		u32 const &value_count
	) :
	inherited				(value, .05f),
	m_collection			(gcnew collection_type())
{
	for (u32 i=0; i<value_count; ++i) {
		ValuePair^			pair = gcnew ValuePair();
		pair->first			= values[i].first;
		pair->second		= to_string(values[i].second);
		m_collection->Add	(pair);
	}
}

System::Object ^property_float_enum_value_reference::get_value		()
{
	float					value = safe_cast<float>(inherited::get_value());
	for each (ValuePair^ i in m_collection) {
		if (i->first != value)
			continue;

		return				(value);
	}

	return					(safe_cast<ValuePair^>(m_collection[0])->first);
}

void property_float_enum_value_reference::set_value					(Object ^object)
{
	String^					string_value = dynamic_cast<String^>(object);

	for each (ValuePair^ i in m_collection) {
		if (!i->second->Equals(string_value))
			continue;

		inherited::set_value(i->first);
		return;
	}

	inherited::set_value	(safe_cast<ValuePair^>(m_collection[0])->first);
}

void property_float_enum_value_reference::increment					(float const% increment)
{
}