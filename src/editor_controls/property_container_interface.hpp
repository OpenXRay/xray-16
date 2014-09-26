////////////////////////////////////////////////////////////////////////////
//	Module 		: property_container_interface.hpp
//	Created 	: 23.01.2008
//  Modified 	: 23.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property container interface class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_CONTAINER_INTERFACE_HPP_INCLUDED
#define PROPERTY_CONTAINER_INTERFACE_HPP_INCLUDED

interface class property_value;

public interface class property_container_interface {
public:
	typedef Flobbster::Windows::Forms::PropertySpec	PropertySpec;

public:
	virtual	property_value^	value	(PropertySpec^ description) = 0;
}; // interface class property_container_interface

#endif // ifndef PROPERTY_CONTAINER_INTERFACE_HPP_INCLUDED