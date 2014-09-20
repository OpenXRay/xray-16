////////////////////////////////////////////////////////////////////////////
//	Module 		: property_file_name_value.cpp
//	Created 	: 07.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property file name value class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_file_name_value.hpp"

using System::String;

property_file_name_value::property_file_name_value	(
		string_getter_type const &getter,
		string_setter_type const &setter,
		System::String^	DefaultExt_,
		System::String^	Filter_,
		System::String^	InitialDirectory_,
		System::String^	Title_,
		bool			remove_extension
	) :
	inherited			(getter, setter),
	DefaultExt			(DefaultExt_),
	Filter				(Filter_),
	InitialDirectory	(InitialDirectory_),
	Title				(Title_),
	m_remove_extension	(remove_extension)
{
}

String^ property_file_name_value::default_extension	()
{
	return				(DefaultExt);
}

String^ property_file_name_value::filter			()
{
	return				(Filter);
}

String^ property_file_name_value::initial_directory	()
{
	return				(InitialDirectory);
}

String^ property_file_name_value::title				()
{
	return				(Title);
}

bool property_file_name_value::remove_extension		()
{
	return				(m_remove_extension);
}