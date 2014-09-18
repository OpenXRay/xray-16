////////////////////////////////////////////////////////////////////////////
//	Module 		: property_string_values_value_getter.cpp
//	Created 	: 09.01.2008
//  Modified 	: 09.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property string values value getter class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_string_values_value_getter.hpp"

typedef property_string_values_value_getter::collection_type				collection_type;

property_string_values_value_getter::property_string_values_value_getter	(
		string_getter_type const& getter,
		string_setter_type const& setter,
		string_collection_getter_type const& collection_getter,
		string_collection_size_getter_type const& collection_size_getter
	) :
	inherited				(getter, setter),
	m_collection_getter		(new string_collection_getter_type(collection_getter)),
	m_collection_size_getter(new string_collection_size_getter_type(collection_size_getter))
{
}

property_string_values_value_getter::~property_string_values_value_getter	()
{
	this->!property_string_values_value_getter	();
}

property_string_values_value_getter::!property_string_values_value_getter	()
{
	delete					(m_collection_getter);
	delete					(m_collection_size_getter);
}

collection_type^ property_string_values_value_getter::values				()
{
	LPCSTR const*			values = (*m_collection_getter)();
	collection_type^		collection = gcnew collection_type();
	for (u32 i=0, n=(*m_collection_size_getter)(); i<n; ++i)
		collection->Enqueue	(to_string(values[i]));

	return					(collection);
}