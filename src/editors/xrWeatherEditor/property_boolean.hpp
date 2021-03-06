////////////////////////////////////////////////////////////////////////////
//	Module 		: property_boolean.hpp
//	Created 	: 10.12.2007
//  Modified 	: 10.12.2007
//	Author		: Dmitriy Iassenev
//	Description : boolean property implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_BOOLEAN_HPP_INCLUDED
#define PROPERTY_BOOLEAN_HPP_INCLUDED

#include "property_holder_include.hpp"
using XRay::SdkControls::IProperty;

public
ref class property_boolean : public IProperty
{
public:
    typedef XRay::Editor::property_holder_base::boolean_getter_type boolean_getter_type;
    typedef XRay::Editor::property_holder_base::boolean_setter_type boolean_setter_type;

public:
    property_boolean(boolean_getter_type const& getter, boolean_setter_type const& setter);
    virtual ~property_boolean();
    !property_boolean();
    virtual System::Object ^ GetValue();
    virtual void SetValue(System::Object ^ object);

private:
    boolean_getter_type* m_getter;
    boolean_setter_type* m_setter;
}; // ref class property_boolean

#endif // ifndef PROPERTY_BOOLEAN_HPP_INCLUDED
