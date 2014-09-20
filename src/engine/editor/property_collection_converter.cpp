////////////////////////////////////////////////////////////////////////////
//	Module 		: property_collection_converter.cpp
//	Created 	: 24.12.2007
//  Modified 	: 25.12.2007
//	Author		: Dmitriy Iassenev
//	Description : collection property converter implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_collection_converter.hpp"

using System::Object;

bool property_collection_converter::CanConvertTo(ITypeDescriptorContext^ context, Type^ type)
{
	return	(type == System::String::typeid);
}
	
Object^ property_collection_converter::ConvertTo(
		ITypeDescriptorContext^ context,
		CultureInfo^ culture,
		Object^ value,
		Type^ type
	)
{
	return	("< ... >");
}
