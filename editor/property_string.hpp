////////////////////////////////////////////////////////////////////////////
//	Module 		: property_string.hpp
//	Created 	: 07.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : string property implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_STRING_HPP_INCLUDED
#define PROPERTY_STRING_HPP_INCLUDED

#include "property_holder_include.hpp"

public ref class property_string : public property_value {
public:
	typedef editor::property_holder::string_getter_type	string_getter_type;
	typedef editor::property_holder::string_setter_type	string_setter_type;

public:
							property_string	(
								string_getter_type const &getter,
								string_setter_type const &setter
							);
	virtual					~property_string();
							!property_string();
	virtual System::Object	^get_value		();
	virtual void			set_value		(System::Object ^object);

private:
	string_getter_type		*m_getter;
	string_setter_type		*m_setter;
}; // ref class property_string

#endif // ifndef PROPERTY_STRING_HPP_INCLUDED