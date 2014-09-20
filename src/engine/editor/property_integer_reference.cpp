////////////////////////////////////////////////////////////////////////////
//	Module 		: property_integer_reference.cpp
//	Created 	: 17.12.2007
//  Modified 	: 17.12.2007
//	Author		: Dmitriy Iassenev
//	Description : integer property reference implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_integer_reference.hpp"

property_integer_reference::property_integer_reference			(int& value) :
	m_value		(new value_holder<int>(value))
{
}

property_integer_reference::~property_integer_reference			()
{
	this->!property_integer_reference	();
}

property_integer_reference::!property_integer_reference			()
{
	delete		(m_value);
}

System::Object ^property_integer_reference::get_value	()
{
	return		(m_value->get());
}

void property_integer_reference::set_value			(System::Object ^object)
{
	int			value = safe_cast<int>(object);
	m_value->set(value);
}