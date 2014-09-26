////////////////////////////////////////////////////////////////////////////
//	Module 		: property_string_values_value_shared_str_getter.hpp
//	Created 	: 09.01.2008
//  Modified 	: 09.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property string values value shared_str getter class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_STRING_VALUES_VALUE_SHARED_STR_GETTER_HPP_INCLUDED
#define PROPERTY_STRING_VALUES_VALUE_SHARED_STR_GETTER_HPP_INCLUDED

#include "property_string_shared_str.hpp"
#include "property_string_values_value_base.hpp"

public ref class property_string_values_value_shared_str_getter :
	public property_string_shared_str,
	public property_string_values_value_base
{
private:
	typedef property_string_shared_str										inherited;

public:
	typedef property_string_values_value_base::collection_type				collection_type;
	typedef editor::property_holder::string_getter_type						string_getter_type;
	typedef editor::property_holder::string_setter_type						string_setter_type;
	typedef editor::property_holder::string_collection_getter_type			string_collection_getter_type;
	typedef editor::property_holder::string_collection_size_getter_type		string_collection_size_getter_type;

public:
								property_string_values_value_shared_str_getter	(
									editor::engine* engine,
									shared_str& value,
									string_collection_getter_type const& collection_getter,
									string_collection_size_getter_type const& collection_size_getter
								);
	virtual						~property_string_values_value_shared_str_getter	();
								!property_string_values_value_shared_str_getter	();
	virtual	collection_type^	values											();

public:
	string_collection_getter_type		*m_collection_getter;
	string_collection_size_getter_type	*m_collection_size_getter;
}; // ref class property_string_values_value_shared_str_getter

#endif // ifndef PROPERTY_STRING_VALUES_VALUE_SHARED_STR_GETTER_HPP_INCLUDED