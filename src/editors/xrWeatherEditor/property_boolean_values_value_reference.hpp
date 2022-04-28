////////////////////////////////////////////////////////////////////////////
//	Module 		: property_boolean_values_value_reference.hpp
//	Created 	: 17.12.2007
//  Modified 	: 17.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property boolean values value reference class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_BOOLEAN_VALUES_VALUE_REFERENCE_HPP_INCLUDED
#define PROPERTY_BOOLEAN_VALUES_VALUE_REFERENCE_HPP_INCLUDED

#include "property_boolean_reference.hpp"

public
ref class property_boolean_values_value_reference : public property_boolean_reference
{
public:
    typedef XRay::Editor::property_holder_base::boolean_getter_type boolean_getter_type;
    typedef XRay::Editor::property_holder_base::boolean_setter_type boolean_setter_type;

private:
    typedef property_boolean_reference inherited;
    typedef System::Collections::ArrayList collection_type;
    typedef System::Object Object;

public:
    property_boolean_values_value_reference(bool& value, pcstr values[2]);
    virtual void SetValue(Object ^ object) override;

public:
    collection_type ^ m_collection;
}; // ref class property_boolean_values_value_reference

#endif // ifndef PROPERTY_BOOLEAN_VALUES_VALUE_REFERENCE_HPP_INCLUDED
