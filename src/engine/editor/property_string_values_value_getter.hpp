////////////////////////////////////////////////////////////////////////////
//	Module 		: property_string_values_value_getter.hpp
//	Created 	: 09.01.2008
//  Modified 	: 09.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property string values value getter class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_STRING_VALUES_VALUE_GETTER_HPP_INCLUDED
#define PROPERTY_STRING_VALUES_VALUE_GETTER_HPP_INCLUDED

#include "property_string.hpp"
#include "property_string_values_value_base.hpp"

public ref class property_string_values_value_getter :
	public property_string,
	public property_string_values_value_base
{
private:
	typedef property_string												inherited;

public:
	typedef property_string_values_value_base::collection_type			collection_type;
	typedef editor::property_holder::string_getter_type					string_getter_type;
	typedef editor::property_holder::string_setter_type					string_setter_type;
	typedef editor::property_holder::string_collection_getter_type		string_collection_getter_type;
	typedef editor::property_holder::string_collection_size_getter_type	string_collection_size_getter_type;

public:
								property_string_values_value_getter	(
									string_getter_type const& getter,
									string_setter_type const& setter,
									string_collection_getter_type const& collection_getter,
									string_collection_size_getter_type const& collection_size_getter
								);
	virtual						~property_string_values_value_getter();
								!property_string_values_value_getter();

	virtual	collection_type^	values								();

public:
	string_collection_getter_type		*m_collection_getter;
	string_collection_size_getter_type	*m_collection_size_getter;
}; // ref class property_string_values_value_getter

#endif // ifndef PROPERTY_STRING_VALUES_VALUE_GETTER_HPP_INCLUDED