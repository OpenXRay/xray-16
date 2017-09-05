////////////////////////////////////////////////////////////////////////////
//	Module 		: property_float.hpp
//	Created 	: 07.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : float property implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_FLOAT_HPP_INCLUDED
#define PROPERTY_FLOAT_HPP_INCLUDED

#include "property_holder_include.hpp"

public
ref class property_float : public XRay::SdkControls::IProperty, public XRay::SdkControls::IIncrementable
{
public:
    typedef editor::property_holder_base::float_getter_type float_getter_type;
    typedef editor::property_holder_base::float_setter_type float_setter_type;

public:
    property_float(float_getter_type const& getter, float_setter_type const& setter, float const % increment_factor);
    virtual ~property_float();
    !property_float();
    virtual System::Object ^ GetValue();
    virtual void SetValue(System::Object ^ object);
    virtual void Increment(float value);

private:
    float_getter_type* m_getter;
    float_setter_type* m_setter;
    float m_increment_factor;
}; // ref class property_float

#endif // ifndef PROPERTY_FLOAT_HPP_INCLUDED
