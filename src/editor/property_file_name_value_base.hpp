////////////////////////////////////////////////////////////////////////////
//	Module 		: property_file_name_value_base.hpp
//	Created 	: 19.12.2007
//  Modified 	: 19.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property file name value base class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_FILE_NAME_VALUE_BASE_HPP_INCLUDED
#define PROPERTY_FILE_NAME_VALUE_BASE_HPP_INCLUDED

interface class property_file_name_value_base : public property_value {
	virtual	System::String^	default_extension	() = 0;
	virtual	System::String^	filter				() = 0;
	virtual	System::String^	initial_directory	() = 0;
	virtual	System::String^	title				() = 0;
	virtual	bool			remove_extension	() = 0;
}; // interface class property_file_name_value_base

#endif // ifndef PROPERTY_FILE_NAME_VALUE_BASE_HPP_INCLUDED