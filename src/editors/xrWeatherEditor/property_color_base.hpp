////////////////////////////////////////////////////////////////////////////
//	Module 		: property_color_base.hpp
//	Created 	: 27.12.2007
//  Modified 	: 27.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property color base class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_COLOR_BASE_HPP_INCLUDED
#define PROPERTY_COLOR_BASE_HPP_INCLUDED

#include "property_holder_include.hpp"
#include "property_container_holder.hpp"

ref class property_container;
ref class property_color_base;

class color_components
{
public:
    color_components(property_color_base ^ holder);

    float red_getter();
    void red_setter(float);

    float green_getter();
    void green_setter(float);

    float blue_getter();
    void blue_setter(float);

private:
    gcroot<property_color_base ^> m_holder;
}; // class color_components

public
value struct Color
{
public:
    inline Color(float red, float green, float blue)
    {
        r = red;
        g = green;
        b = blue;
    }

    property float r;
    property float g;
    property float b;
}; // value struct Color

public
ref class property_color_base abstract : public XRay::SdkControls::IProperty,
                                         public property_container_holder,
                                         public XRay::SdkControls::IMouseListener,
                                         public XRay::SdkControls::IIncrementable
{
public:
    typedef System::Attribute Attribute;

public:
    property_color_base(XRay::Editor::color const % color, array<System::Attribute ^> ^ attributes);
    virtual ~property_color_base();
    !property_color_base();
    void red(float value);
    void green(float value);
    void blue(float value);

public:
    virtual System::Object ^ GetValue();
    virtual void SetValue(System::Object ^ object);
    virtual void OnDoubleClick(XRay::SdkControls::PropertyGrid ^ property_grid);
    virtual void Increment(float increment);

public:
    virtual XRay::Editor::color get_value_raw() = 0;
    virtual void set_value_raw(XRay::Editor::color color) = 0;

private:
    property_container ^ m_container;
    color_components* m_components;
    array<System::Attribute ^> ^ m_attributes;
}; // ref class property_color_base abstract

#endif // ifndef PROPERTY_COLOR_BASE_HPP_INCLUDED
