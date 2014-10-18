////////////////////////////////////////////////////////////////////////////
//	Module 		: property_file_name_value.hpp
//	Created 	: 07.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property file name value class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_FILE_NAME_VALUE_HPP_INCLUDED
#define PROPERTY_FILE_NAME_VALUE_HPP_INCLUDED

#include "property_string.hpp"
#include "property_file_name_value_base.hpp"

public ref class property_file_name_value :
	public property_string,
	public property_file_name_value_base
{
private:
	typedef property_string									inherited;

public:
	typedef editor::property_holder::string_getter_type		string_getter_type;
	typedef editor::property_holder::string_setter_type		string_setter_type;

public:
							property_file_name_value	(
								string_getter_type const &getter,
								string_setter_type const &setter,
								System::String^	DefaultExt_,
								System::String^	Filter_,
								System::String^	InitialDirectory_,
								System::String^	Title_,
								bool			remove_extension
							);

	virtual	System::String^	default_extension			();
	virtual	System::String^	filter						();
	virtual	System::String^	initial_directory			();
	virtual	System::String^	title						();
	virtual	bool			remove_extension			();

public:
	System::String^			DefaultExt;
	System::String^			Filter;
	System::String^			InitialDirectory;
	System::String^			Title;
	bool					m_remove_extension;
}; // ref class property_file_name_value

#endif // ifndef PROPERTY_FILE_NAME_VALUE_HPP_INCLUDED