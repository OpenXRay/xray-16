////////////////////////////////////////////////////////////////////////////
//	Module 		: property_boolean.hpp
//	Created 	: 10.12.2007
//  Modified 	: 10.12.2007
//	Author		: Dmitriy Iassenev
//	Description : boolean property implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_BOOLEAN_HPP_INCLUDED
#define PROPERTY_BOOLEAN_HPP_INCLUDED

#include "property_holder_include.hpp"

public ref class property_boolean : public property_value {
public:
	typedef editor::property_holder::boolean_getter_type	boolean_getter_type;
	typedef editor::property_holder::boolean_setter_type	boolean_setter_type;

public:
							property_boolean	(
								boolean_getter_type const &getter,
								boolean_setter_type const &setter
							);
	virtual					~property_boolean	();
							!property_boolean	();
	virtual System::Object	^get_value			();
	virtual void			set_value			(System::Object ^object);

private:
	boolean_getter_type		*m_getter;
	boolean_setter_type		*m_setter;
}; // ref class property_boolean

#endif // ifndef PROPERTY_BOOLEAN_HPP_INCLUDED