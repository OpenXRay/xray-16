////////////////////////////////////////////////////////////////////////////
//	Module 		: property_vec3f_base.hpp
//	Created 	: 29.12.2007
//  Modified 	: 29.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property vec3f base class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_VEC3F_BASE_HPP_INCLUDED
#define PROPERTY_VEC3F_BASE_HPP_INCLUDED

#include "property_holder_include.hpp"
#include "property_container_holder.hpp"

ref class property_container;
ref class property_vec3f_base;

class vec3f_components
{
public:
    vec3f_components(property_vec3f_base ^ holder);

    float x_getter();
    void x_setter(float);

    float y_getter();
    void y_setter(float);

    float z_getter();
    void z_setter(float);

private:
    gcroot<property_vec3f_base ^> m_holder;
}; // class vec3f_components

public
value struct Vec3f
{
public:
    inline Vec3f(float x_, float y_, float z_)
    {
        x = x_;
        y = y_;
        z = z_;
    }

    property float x;
    property float y;
    property float z;
}; // value struct  Vec3f

public
ref class property_vec3f_base abstract : public XRay::SdkControls::IProperty, public property_container_holder
{
public:
    property_vec3f_base(XRay::Editor::vec3f const % vec3f);
    virtual ~property_vec3f_base();
    !property_vec3f_base();
    void x(float value);
    void y(float value);
    void z(float value);

public:
    virtual System::Object ^ GetValue();
    virtual void SetValue(System::Object ^ object);

public:
    virtual XRay::Editor::vec3f get_value_raw() = 0;
    virtual void set_value_raw(XRay::Editor::vec3f vec3f) = 0;

private:
    property_container ^ m_container;
    vec3f_components* m_components;
}; // ref class property_vec3f_base abstract

#endif // ifndef PROPERTY_VEC3F_BASE_HPP_INCLUDED
