////////////////////////////////////////////////////////////////////////////
//	Module 		: property_converter_boolean_values.cpp
//	Created 	: 07.12.2007
//  Modified 	: 11.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property converter boolean values class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_converter_boolean_values.hpp"
#include "property_boolean_values_value.hpp"
#include "property_container.hpp"

using System::ComponentModel::TypeConverter;
typedef TypeConverter::StandardValuesCollection	StandardValuesCollection;
using System::Object;

bool property_converter_boolean_values::GetStandardValuesSupported				(ITypeDescriptorContext^ context)
{
	return							(true);
}

bool property_converter_boolean_values::GetStandardValuesExclusive				(ITypeDescriptorContext^ context)
{
	return							(true);
}

StandardValuesCollection ^property_converter_boolean_values::GetStandardValues	(ITypeDescriptorContext^ context)
{
	property_container^				container = safe_cast<property_container^>(context->Instance);
	PropertySpecDescriptor^			descriptor = safe_cast<PropertySpecDescriptor^>(context->PropertyDescriptor);
	property_value^					raw_value = container->value(descriptor->item);
	property_boolean_values_value^	value = safe_cast<property_boolean_values_value^>(raw_value);
	return							(gcnew StandardValuesCollection(value->m_collection));
}

Object^	property_converter_boolean_values::ConvertTo							(
		ITypeDescriptorContext^ context,
		CultureInfo^ culture,
		Object^ value,
		Type^ destination_type
	)
{
	if (!context)
		return						(inherited::ConvertTo(context, culture, value, destination_type));

	if (!context->Instance)
		return						(inherited::ConvertTo(context, culture, value, destination_type));

	if (!context->PropertyDescriptor)
		return						(inherited::ConvertTo(context, culture, value, destination_type));

	if (destination_type != System::String::typeid)
		return						(inherited::ConvertTo(context, culture, value, destination_type));

	if (dynamic_cast<System::String^>(value))
		return						(value);

	property_container^				container = safe_cast<property_container^>(context->Instance);
	PropertySpecDescriptor^			descriptor = safe_cast<PropertySpecDescriptor^>(context->PropertyDescriptor);
	property_value^					raw_value = container->value(descriptor->item);
	property_boolean_values_value^	real_value = safe_cast<property_boolean_values_value^>(raw_value);
	bool							bool_value = safe_cast<bool>(value);
	return							(real_value->m_collection[bool_value ? 1 : 0]);
}
