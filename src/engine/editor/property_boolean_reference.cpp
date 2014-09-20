////////////////////////////////////////////////////////////////////////////
//	Module 		: property_boolean_reference.cpp
//	Created 	: 13.12.2007
//  Modified 	: 13.12.2007
//	Author		: Dmitriy Iassenev
//	Description : boolean property reference implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_boolean_reference.hpp"

property_boolean_reference::property_boolean_reference	(bool& value) :
	m_value					(new value_holder<bool>(value))
{
}

property_boolean_reference::~property_boolean_reference	()
{
	this->!property_boolean_reference	();
}

property_boolean_reference::!property_boolean_reference	()
{
	delete					(m_value);
}

System::Object ^property_boolean_reference::get_value	()
{
	return					(m_value->get());
}

void property_boolean_reference::set_value				(System::Object ^object)
{
	bool					value = safe_cast<bool>(object);
	m_value->set			(value);
}