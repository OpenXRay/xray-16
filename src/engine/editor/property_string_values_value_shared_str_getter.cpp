////////////////////////////////////////////////////////////////////////////
//	Module 		: property_string_values_value_shared_str_getter.cpp
//	Created 	: 09.01.2008
//  Modified 	: 09.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property string values value shared_str getter class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_string_values_value_shared_str_getter.hpp"

typedef property_string_values_value_shared_str_getter::collection_type	collection_type;

property_string_values_value_shared_str_getter::property_string_values_value_shared_str_getter	(
		editor::engine* engine,
		shared_str& value,
		string_collection_getter_type const& collection_getter,
		string_collection_size_getter_type const& collection_size_getter
	) :
	inherited				(engine, value),
	m_collection_getter		(new string_collection_getter_type(collection_getter)),
	m_collection_size_getter(new string_collection_size_getter_type(collection_size_getter))
{
}

property_string_values_value_shared_str_getter::~property_string_values_value_shared_str_getter	()
{
	this->!property_string_values_value_shared_str_getter	();
}

property_string_values_value_shared_str_getter::!property_string_values_value_shared_str_getter	()
{
	delete					(m_collection_getter);
	delete					(m_collection_size_getter);
}

collection_type^ property_string_values_value_shared_str_getter::values							()
{
	LPCSTR const*			values = (*m_collection_getter)();
	collection_type^		collection = gcnew collection_type();
	for (u32 i=0, n=(*m_collection_size_getter)(); i<n; ++i)
		collection->Enqueue	(to_string(values[i]));

	return					(collection);
}