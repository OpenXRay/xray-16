////////////////////////////////////////////////////////////////////////////
//	Module 		: property_container_converter.cpp
//	Created 	: 11.12.2007
//  Modified 	: 11.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property container converter class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_container_converter.hpp"
#include "property_container.hpp"

using System::ComponentModel::PropertyDescriptorCollection;
using System::ComponentModel::TypeDescriptor;
using System::String;
using System::Object;
using System::Array;
using System::Collections::ArrayList;
using Flobbster::Windows::Forms::PropertySpec;

PropertyDescriptorCollection^ property_container_converter::GetProperties	(
		ITypeDescriptorContext^ context,
		Object^ value,
		array<Attribute^>^ attributes
	)
{
	PropertyDescriptorCollection^	current = TypeDescriptor::GetProperties(value, attributes);
	VERIFY							(current);
	property_container^				container = dynamic_cast<property_container^>(context->Instance);
	if (container) {
		ArrayList%					properties = container->ordered_properties();
		ArrayList^					names = gcnew ArrayList();
		for each (PropertySpec^ i in properties)
			names->Add				(i->Name);

		return						(current->Sort(reinterpret_cast<array<String^>^>(names->ToArray(String::typeid))));
	}

	// here we should construct an intersection of properties sets
	// instead of just getting the properties from the first object
	Array^							objects = safe_cast<Array^>(context->Instance);
	VERIFY							(objects->Length);
	container						= safe_cast<property_container^>(objects->GetValue(0));
	ArrayList%						properties = container->ordered_properties();
	ArrayList^						names = gcnew ArrayList();
	for each (PropertySpec^ i in properties)
		names->Add					(i->Name);

	return							(current->Sort(reinterpret_cast<array<String^>^>(names->ToArray(String::typeid))));
}

bool property_container_converter::GetPropertiesSupported					(
		ITypeDescriptorContext^ context
	)
{
	return		(true);
}

bool property_container_converter::CanConvertTo								(
		ITypeDescriptorContext^ context,
		Type^ destination_type
	)
{
	if (destination_type == property_container::typeid)
		return	(true);

	return		(inherited::CanConvertTo(context, destination_type));
}

Object^ property_container_converter::ConvertTo								(
		ITypeDescriptorContext^ context,
		CultureInfo^ culture,
		Object^ value,
		Type^ destination_type
	)
{
	if (destination_type != String::typeid)
		return	(inherited::ConvertTo(context, culture, value, destination_type));

	return		("...");
}
