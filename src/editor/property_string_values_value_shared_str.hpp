////////////////////////////////////////////////////////////////////////////
//	Module 		: property_string_values_value_shared_str.hpp
//	Created 	: 07.12.2007
//  Modified 	: 11.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property string values value class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_STRING_VALUES_VALUE_SHARED_STR_HPP_INCLUDED
#define PROPERTY_STRING_VALUES_VALUE_SHARED_STR_HPP_INCLUDED

#include "property_string_shared_str.hpp"
#include "property_string_values_value_base.hpp"

public ref class property_string_values_value_shared_str :
	public property_string_shared_str,
	public property_string_values_value_base
{
private:
	typedef property_string_shared_str		inherited;
	typedef property_string_values_value_base::collection_type		collection_type;

public:
							property_string_values_value_shared_str	(
								editor::engine* engine,
								shared_str& value,
								LPCSTR const* values,
								u32 const &value_count
							);

	virtual	collection_type	^values									() {return m_collection;};

public:
	collection_type	^m_collection;
}; // ref class property_string_values_value_shared_str

#endif // ifndef PROPERTY_STRING_VALUES_VALUE_SHARED_STR_HPP_INCLUDED