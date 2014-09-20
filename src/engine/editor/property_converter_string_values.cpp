////////////////////////////////////////////////////////////////////////////
//	Module 		: property_converter_string_values.cpp
//	Created 	: 11.12.2007
//  Modified 	: 11.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property converter string values class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_converter_string_values.hpp"
#include "property_string_values_value_base.hpp"
#include "property_container.hpp"

using System::ComponentModel::ITypeDescriptorContext;
using System::ComponentModel::TypeConverter;
typedef TypeConverter::StandardValuesCollection	StandardValuesCollection;

bool property_converter_string_values::GetStandardValuesSupported				(ITypeDescriptorContext^ context)
{
	return							(true);
}

bool property_converter_string_values::GetStandardValuesExclusive				(ITypeDescriptorContext^ context)
{
	return							(true);
}

StandardValuesCollection ^property_converter_string_values::GetStandardValues	(ITypeDescriptorContext^ context)
{
	property_container^				container = safe_cast<property_container^>(context->Instance);
	PropertySpecDescriptor^			descriptor = safe_cast<PropertySpecDescriptor^>(context->PropertyDescriptor);
	property_value^					raw_value = container->value(descriptor->item);
	property_string_values_value_base^	value = safe_cast<property_string_values_value_base^>(raw_value);
	return							(gcnew StandardValuesCollection(value->values()));
}

bool property_converter_string_values::CanConvertFrom							(ITypeDescriptorContext^ context, Type^ source_type)
{
	return							(false);
}
