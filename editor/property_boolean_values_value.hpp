////////////////////////////////////////////////////////////////////////////
//	Module 		: property_boolean_values_value.hpp
//	Created 	: 07.12.2007
//  Modified 	: 11.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property boolean values value class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_BOOLEAN_VALUES_VALUE_HPP_INCLUDED
#define PROPERTY_BOOLEAN_VALUES_VALUE_HPP_INCLUDED

#include "property_boolean.hpp"

public ref class property_boolean_values_value : public property_boolean {
public:
	typedef editor::property_holder::boolean_getter_type	boolean_getter_type;
	typedef editor::property_holder::boolean_setter_type	boolean_setter_type;

private:
	typedef property_boolean								inherited;
	typedef System::Collections::ArrayList					collection_type;
	typedef System::Object									Object;

public:
					property_boolean_values_value	(
						boolean_getter_type const &getter,
						boolean_setter_type const &setter,
						LPCSTR values[2]
					);
	virtual void	set_value						(Object ^object) override;

public:
	collection_type	^m_collection;
}; // ref class property_boolean_values_value

#endif // ifndef PROPERTY_BOOLEAN_VALUES_VALUE_HPP_INCLUDED