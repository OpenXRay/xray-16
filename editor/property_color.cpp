////////////////////////////////////////////////////////////////////////////
//	Module 		: property_color.cpp
//	Created 	: 10.12.2007
//  Modified 	: 10.12.2007
//	Author		: Dmitriy Iassenev
//	Description : color property implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_color.hpp"

using editor::color;

property_color::property_color		(
		color_getter_type const &getter,
		color_setter_type const &setter,
		array<System::Attribute^>^ attributes
	) :
	m_getter				(new color_getter_type(getter)),
	m_setter				(new color_setter_type(setter)),
	inherited				(getter(), attributes)
{
}

property_color::~property_color		()
{
	this->!property_color	();
}

property_color::!property_color		()
{
	delete					(m_getter);
	delete					(m_setter);
}

color property_color::get_value_raw	()
{
	return					((*m_getter)());
}

void property_color::set_value_raw	(color value)
{
	(*m_setter)				(value);
}