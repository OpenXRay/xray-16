////////////////////////////////////////////////////////////////////////////
//	Module 		: property_integer_enum_value.hpp
//	Created 	: 12.12.2007
//  Modified 	: 12.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property integer enum value class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_INTEGER_ENUM_VALUE_HPP_INCLUDED
#define PROPERTY_INTEGER_ENUM_VALUE_HPP_INCLUDED

#include "property_integer.hpp"

public
ref class property_integer_enum_value : public property_integer
{
public:
    typedef XRay::Editor::property_holder_base::integer_getter_type integer_getter_type;
    typedef XRay::Editor::property_holder_base::integer_setter_type integer_setter_type;
    typedef Pair<int, System::String ^> ValuePair;

private:
    typedef property_integer inherited;
    typedef System::Collections::ArrayList collection_type;
    typedef System::Object Object;
    typedef std::pair<int, pcstr> pair;

public:
    property_integer_enum_value(
        integer_getter_type const& getter, integer_setter_type const& setter, pair* values, u32 const& value_count);
    virtual Object ^ GetValue() override;
    virtual void SetValue(Object ^ object) override;

public:
    collection_type ^ m_collection;
}; // ref class property_integer_enum_value

#endif // ifndef PROPERTY_INTEGER_ENUM_VALUE_HPP_INCLUDED
