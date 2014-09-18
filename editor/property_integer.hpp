////////////////////////////////////////////////////////////////////////////
//	Module 		: property_integer.hpp
//	Created 	: 07.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : integer property implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_INTEGER_HPP_INCLUDED
#define PROPERTY_INTEGER_HPP_INCLUDED

#include "property_holder_include.hpp"

public ref class property_integer : public property_value {
public:
	typedef editor::property_holder::integer_getter_type	integer_getter_type;
	typedef editor::property_holder::integer_setter_type	integer_setter_type;

public:
							property_integer	(
								integer_getter_type const &getter,
								integer_setter_type const &setter
							);
	virtual					~property_integer	();
							!property_integer	();
	virtual System::Object	^get_value			();
	virtual void			set_value			(System::Object ^object);

private:
	integer_getter_type		*m_getter;
	integer_setter_type		*m_setter;
}; // ref class property_integer

#endif // ifndef PROPERTY_INTEGER_HPP_INCLUDED