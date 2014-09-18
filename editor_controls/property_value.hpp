////////////////////////////////////////////////////////////////////////////
//	Module 		: property_value.hpp
//	Created 	: 07.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property value
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_VALUE_HPP_INCLUDED
#define PROPERTY_VALUE_HPP_INCLUDED

public interface class property_value {
public:
	virtual System::Object	^get_value		() = 0;
	virtual void			set_value		(System::Object ^object) = 0;
}; // interface class property_value

#endif // ifndef PROPERTY_VALUE_HPP_INCLUDED