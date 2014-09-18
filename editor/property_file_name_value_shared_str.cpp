////////////////////////////////////////////////////////////////////////////
//	Module 		: property_file_name_value_shared_str.cpp
//	Created 	: 19.12.2007
//  Modified 	: 19.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property file name value shared_str class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_file_name_value_shared_str.hpp"

using System::String;

property_file_name_value_shared_str::property_file_name_value_shared_str	(
		editor::engine* engine,
		shared_str& value,
		System::String^	DefaultExt_,
		System::String^	Filter_,
		System::String^	InitialDirectory_,
		System::String^	Title_,
		bool			remove_extension
	) :
	inherited			(engine, value),
	DefaultExt			(DefaultExt_),
	Filter				(Filter_),
	InitialDirectory	(InitialDirectory_),
	Title				(Title_),
	m_remove_extension	(remove_extension)
{
}

String^ property_file_name_value_shared_str::default_extension				()
{
	return				(DefaultExt);
}

String^ property_file_name_value_shared_str::filter							()
{
	return				(Filter);
}

String^ property_file_name_value_shared_str::initial_directory				()
{
	return				(InitialDirectory);
}

String^ property_file_name_value_shared_str::title							()
{
	return				(Title);
}

bool property_file_name_value_shared_str::remove_extension					()
{
	return				(m_remove_extension);
}