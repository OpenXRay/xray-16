////////////////////////////////////////////////////////////////////////////
//	Module 		: property_converter_color.cpp
//	Created 	: 11.12.2007
//  Modified 	: 11.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property converter color class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_converter_color.hpp"
#include "property_color.hpp"
#include "property_container.hpp"
#include "property_converter_float.hpp"

using System::Object;
using System::String;
using System::ComponentModel::PropertyDescriptorCollection;
using System::ComponentModel::PropertyDescriptor;
using System::ComponentModel::TypeDescriptor;
using System::ArgumentException;
using Flobbster::Windows::Forms::PropertyBag;
using System::Attribute;

typedef PropertyBag::PropertySpecDescriptor PropertySpecDescriptor;

PropertyDescriptorCollection ^ property_converter_color::GetProperties(
                                   ITypeDescriptorContext ^ context, Object ^ value, array<Attribute ^> ^ attributes)
{
    PropertyDescriptorCollection ^ current = TypeDescriptor::GetProperties(value, attributes);
    VERIFY(current);
    VERIFY((current->Count == 3));

    array<String ^> ^ names = gcnew cli::array<String ^>(current->Count);
    names[0] = "red";
    names[1] = "green";
    names[2] = "blue";

    return (current->Sort(names));
}

bool property_converter_color::GetPropertiesSupported(ITypeDescriptorContext ^ context) { return (true); }
bool property_converter_color::CanConvertTo(ITypeDescriptorContext ^ context, Type ^ destination_type)
{
    if (destination_type == Color::typeid)
        return (true);

    if (destination_type == String::typeid)
        return (true);

    return (inherited::CanConvertTo(context, destination_type));
}

Object ^ property_converter_color::ConvertTo(
             ITypeDescriptorContext ^ context, CultureInfo ^ culture, Object ^ value, Type ^ destination_type)
{
    if (destination_type == String::typeid)
    {
        property_container ^ container = safe_cast<property_container ^>(value);
        XRay::Editor::color color = safe_cast<property_color_base %>(container->container_holder()).get_value_raw();
        return ("" + property_converter_float().ConvertTo(context, culture, color.r, String::typeid) + " " +
            property_converter_float().ConvertTo(context, culture, color.g, String::typeid) + " " +
            property_converter_float().ConvertTo(context, culture, color.b, String::typeid) + "");
    }

    if (destination_type == Color::typeid)
    {
        property_container ^ container = safe_cast<property_container ^>(value);
        XRay::Editor::color color = safe_cast<property_color_base %>(container->container_holder()).get_value_raw();
        return (Color(color.r, color.g, color.b));
    }

    return (inherited::ConvertTo(context, culture, value, destination_type));
}

bool property_converter_color::CanConvertFrom(ITypeDescriptorContext ^ context, Type ^ source_type)
{
    if (source_type == String::typeid)
        return (true);

    return (inherited::CanConvertFrom(context, source_type));
}

Object ^ property_converter_color::ConvertFrom(ITypeDescriptorContext ^ context, CultureInfo ^ culture, Object ^ value)
{
    if (!value)
        return (inherited::ConvertFrom(context, culture, value));

    String ^ string = dynamic_cast<String ^>(value);
    try
    {
        int comma = string->IndexOf(" ");
        String ^ real_value = string->Substring(0, comma);
        Color color;
        color.r = float ::Parse(real_value);

        string = string->Substring(comma + 1, string->Length - comma - 1);
        comma = string->IndexOf(" ");
        real_value = string->Substring(0, comma);
        color.g = float ::Parse(real_value);

        string = string->Substring(comma + 1, string->Length - comma - 1);
        color.b = float ::Parse(string);

        return (color);
    }
    catch (...)
    {
        throw gcnew ArgumentException("Can not convert '" + value + "' to color");
    }
}
