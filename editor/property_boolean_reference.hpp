////////////////////////////////////////////////////////////////////////////
//	Module 		: property_boolean_reference.hpp
//	Created 	: 13.12.2007
//  Modified 	: 13.12.2007
//	Author		: Dmitriy Iassenev
//	Description : boolean property reference implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_BOOLEAN_REFERENCE_HPP_INCLUDED
#define PROPERTY_BOOLEAN_REFERENCE_HPP_INCLUDED

#include "property_holder_include.hpp"

public ref class property_boolean_reference : public property_value {
public:
							property_boolean_reference	(bool& value);
	virtual					~property_boolean_reference	();
							!property_boolean_reference	();
	virtual System::Object	^get_value					();
	virtual void			set_value					(System::Object ^object);

private:
	value_holder<bool>*		m_value;
}; // ref class property_boolean

#endif // ifndef PROPERTY_BOOLEAN_REFERENCE_HPP_INCLUDED