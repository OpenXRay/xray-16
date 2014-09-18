////////////////////////////////////////////////////////////////////////////
//	Module 		: property_converter_vec3f.cpp
//	Created 	: 29.12.2007
//  Modified 	: 29.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property converter vec3f class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_converter_vec3f.hpp"
#include "property_vec3f.hpp"
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

typedef PropertyBag::PropertySpecDescriptor	PropertySpecDescriptor;

PropertyDescriptorCollection^ property_converter_vec3f::GetProperties	(
		ITypeDescriptorContext^ context,
		Object^ value,
		array<Attribute^>^ attributes
	)
{
	PropertyDescriptorCollection^	current = TypeDescriptor::GetProperties(value, attributes);
	VERIFY							(current);
	VERIFY							((current->Count == 3));

	array<String^>^					names = gcnew cli::array<String^>(current->Count);
	names[0]						= "x";
	names[1]						= "y";
	names[2]						= "z";

	return							(current->Sort(names));
}

bool property_converter_vec3f::GetPropertiesSupported					(ITypeDescriptorContext^ context)
{
	return					(true);
}

bool property_converter_vec3f::CanConvertTo		(
		ITypeDescriptorContext^ context,
		Type^ destination_type
	)
{
	if (destination_type == Vec3f::typeid)
		return				(true);

	if (destination_type == String::typeid)
		return				(false);

	return					(inherited::CanConvertTo(context, destination_type));
}

Object^ property_converter_vec3f::ConvertTo		(
		ITypeDescriptorContext^ context,
		CultureInfo^ culture,
		Object^ value,
		Type^ destination_type
	)
{
	if (destination_type == String::typeid) {
		property_container^	container = safe_cast<property_container^>(value);
		editor::vec3f		vec3f = safe_cast<property_vec3f_base%>(container->container_holder()).get_value_raw();
		return		(
			"" +  property_converter_float().ConvertTo(context, culture, vec3f.x, String::typeid) +
			" " + property_converter_float().ConvertTo(context, culture, vec3f.y, String::typeid) +
			" " + property_converter_float().ConvertTo(context, culture, vec3f.z, String::typeid) +
			""
		);
	}

	if (destination_type == Vec3f::typeid) {
		property_container^	container = safe_cast<property_container^>(value);
		editor::vec3f		vec3f = safe_cast<property_vec3f_base%>(container->container_holder()).get_value_raw();
		return		(
			Vec3f(
				vec3f.x,
				vec3f.y,
				vec3f.z
			)
		);
	}

	return					(inherited::ConvertTo(context, culture, value, destination_type));
}

bool property_converter_vec3f::CanConvertFrom	(
		ITypeDescriptorContext^ context,
		Type^ source_type
	)
{
	if (source_type == String::typeid)
		return				(true);

	return					(inherited::CanConvertFrom(context, source_type));
}

Object^ property_converter_vec3f::ConvertFrom	(
		ITypeDescriptorContext^ context,
		CultureInfo^ culture,
		Object^ value
	)
{
	String^			string = dynamic_cast<String^>(value);
	if (!value)
		return		(inherited::ConvertFrom(context, culture, value));

	try {
		int			comma = string->IndexOf(" ");
		String		^real_value = string->Substring(0, comma);
		Vec3f		vec3f;
		vec3f.x		= float::Parse(real_value);

		string		= string->Substring(comma + 1, string->Length - comma - 1);
		comma		= string->IndexOf(" ");
		real_value	= string->Substring(0, comma);
		vec3f.y		= float::Parse(real_value);

		string		= string->Substring(comma + 1, string->Length - comma - 1);
		vec3f.z		= float::Parse(string);

		return		(vec3f);
	}
	catch(...) {
        throw gcnew ArgumentException(
            "Can not convert '" + value + "' to vec3f"
		);
	}
}