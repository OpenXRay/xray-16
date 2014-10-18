////////////////////////////////////////////////////////////////////////////
//	Module 		: property_string_values_value_base.hpp
//	Created 	: 20.12.2007
//  Modified 	: 20.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property string values value base class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_STRING_VALUES_VALUE_BASE_HPP_INCLUDED
#define PROPERTY_STRING_VALUES_VALUE_BASE_HPP_INCLUDED

public interface class property_string_values_value_base {
public:
	typedef System::Collections::Generic::Queue<System::String^>	collection_type;

public:
	virtual	collection_type	^values	() = 0;
}; // ref class property_string_values_value

#endif // ifndef PROPERTY_STRING_VALUES_VALUE_BASE_HPP_INCLUDED