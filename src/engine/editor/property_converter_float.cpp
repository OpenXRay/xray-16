////////////////////////////////////////////////////////////////////////////
//	Module 		: property_converter_float.cpp
//	Created 	: 23.06.2008
//  Modified 	: 23.06.2008
//	Author		: Dmitriy Iassenev
//	Description : property converter float class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_converter_float.hpp"
#include <stdio.h>

using System::Object;
using System::String;
using System::ComponentModel::PropertyDescriptorCollection;
using System::ComponentModel::PropertyDescriptor;
using System::ComponentModel::TypeDescriptor;
using System::ArgumentException;
using Flobbster::Windows::Forms::PropertyBag;
using System::Attribute;

typedef PropertyBag::PropertySpecDescriptor	PropertySpecDescriptor;

bool property_converter_float::CanConvertTo		(
		ITypeDescriptorContext^ context,
		Type^ destination_type
	)
{
	if (destination_type == float::typeid)
		return				(true);

	if (destination_type == String::typeid)
		return				(true);

	return					(inherited::CanConvertTo(context, destination_type));
}

Object^ property_converter_float::ConvertTo		(
		ITypeDescriptorContext^ context,
		CultureInfo^ culture,
		Object^ value,
		Type^ destination_type
	)
{
	if (destination_type != String::typeid)
		return					(inherited::ConvertTo(context, culture, value, destination_type));

	float						float_value= safe_cast<float>(value);
	char						temp[32];
	sprintf_s					(temp, "%.3f", float_value);
	return						( gcnew String(temp) );
}

bool property_converter_float::CanConvertFrom	(
		ITypeDescriptorContext^ context,
		Type^ source_type
	)
{
	if (source_type == String::typeid)
		return				(true);

	return					(inherited::CanConvertFrom(context, source_type));
}

Object^ property_converter_float::ConvertFrom	(
		ITypeDescriptorContext^ context,
		CultureInfo^ culture,
		Object^ value
	)
{
	String^			string = dynamic_cast<String^>(value);
	if (!value)
		return		(inherited::ConvertFrom(context, culture, value));

	try {
		return		( float::Parse(string) );
	}
	catch(...) {
        throw gcnew ArgumentException(
            "Can not convert '" + value + "' to float"
		);
	}
}