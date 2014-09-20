////////////////////////////////////////////////////////////////////////////
//	Module 		: property_string.cpp
//	Created 	: 07.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : string property implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_string.hpp"

property_string::property_string			(
		string_getter_type const &getter,
		string_setter_type const &setter
	) :
	m_getter				(new string_getter_type(getter)),
	m_setter				(new string_setter_type(setter))
{
}

property_string::~property_string			()
{
	this->!property_string	();
}

property_string::!property_string			()
{
	delete					(m_getter);
	delete					(m_setter);
}

System::Object ^property_string::get_value	()
{
	return					(to_string((*m_getter)()));
}

void property_string::set_value			(System::Object ^object)
{
	LPSTR					result = to_string(safe_cast<System::String^>(object));
	(*m_setter)				(result);
	free					(result);
}