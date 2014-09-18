////////////////////////////////////////////////////////////////////////////
//	Module 		: property_integer_reference.hpp
//	Created 	: 17.12.2007
//  Modified 	: 17.12.2007
//	Author		: Dmitriy Iassenev
//	Description : integer property reference implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_INTEGER_REFERENCE_HPP_INCLUDED
#define PROPERTY_INTEGER_REFERENCE_HPP_INCLUDED

#include "property_holder_include.hpp"

public ref class property_integer_reference : public property_value {
public:
							property_integer_reference	(int& value);
	virtual					~property_integer_reference	();
							!property_integer_reference	();
	virtual System::Object	^get_value					();
	virtual void			set_value					(System::Object ^object);

private:
	value_holder<int>*		m_value;
}; // ref class property_integer_reference

#endif // ifndef PROPERTY_INTEGER_REFERENCE_HPP_INCLUDED