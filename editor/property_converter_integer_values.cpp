////////////////////////////////////////////////////////////////////////////
//	Module 		: property_converter_integer_values.cpp
//	Created 	: 12.12.2007
//  Modified 	: 12.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property converter integer values class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_converter_integer_values.hpp"
#include "property_integer_values_value_base.hpp"
#include "property_container.hpp"

using System::ComponentModel::TypeConverter;
typedef TypeConverter::StandardValuesCollection	StandardValuesCollection;
using System::Object;

bool property_converter_integer_values::GetStandardValuesSupported				(ITypeDescriptorContext^ context)
{
	return							(true);
}

bool property_converter_integer_values::GetStandardValuesExclusive				(ITypeDescriptorContext^ context)
{
	return							(true);
}

StandardValuesCollection ^property_converter_integer_values::GetStandardValues	(ITypeDescriptorContext^ context)
{
	property_container^				container = safe_cast<property_container^>(context->Instance);
	PropertySpecDescriptor^			descriptor = safe_cast<PropertySpecDescriptor^>(context->PropertyDescriptor);
	property_value^					raw_value = container->value(descriptor->item);
	property_integer_values_value_base^	value = safe_cast<property_integer_values_value_base^>(raw_value);
	return							(gcnew StandardValuesCollection(value->collection()));
}

Object^	property_converter_integer_values::ConvertTo							(
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
	property_integer_values_value_base^	real_value = safe_cast<property_integer_values_value_base^>(raw_value);
	int								int_value = safe_cast<int>(value);
	return							(real_value->collection()[int_value]);
}
