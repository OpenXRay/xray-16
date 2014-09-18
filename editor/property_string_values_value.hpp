////////////////////////////////////////////////////////////////////////////
//	Module 		: property_string_values_value.hpp
//	Created 	: 07.12.2007
//  Modified 	: 11.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property string values value class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_STRING_VALUES_VALUE_HPP_INCLUDED
#define PROPERTY_STRING_VALUES_VALUE_HPP_INCLUDED

#include "property_string.hpp"
#include "property_string_values_value_base.hpp"

public ref class property_string_values_value :
	public property_string,
	public property_string_values_value_base
{
private:
	typedef property_string								inherited;
	typedef property_string_values_value_base::collection_type					collection_type;

public:
	typedef editor::property_holder::string_getter_type	string_getter_type;
	typedef editor::property_holder::string_setter_type	string_setter_type;

public:
	property_string_values_value	(
		string_getter_type const &getter,
		string_setter_type const &setter,
		LPCSTR const* values,
		u32 const &value_count
	);

	virtual	collection_type	^values	() {return m_collection;};

public:
	collection_type	^m_collection;
}; // ref class property_string_values_value

#endif // ifndef PROPERTY_STRING_VALUES_VALUE_HPP_INCLUDED