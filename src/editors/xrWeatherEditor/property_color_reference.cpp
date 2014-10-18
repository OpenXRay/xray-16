////////////////////////////////////////////////////////////////////////////
//	Module 		: property_color_reference.cpp
//	Created 	: 17.12.2007
//  Modified 	: 17.12.2007
//	Author		: Dmitriy Iassenev
//	Description : color property reference implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_color_reference.hpp"

using editor::color;

property_color_reference::property_color_reference	(
		color& value,
		array<System::Attribute^>^ attributes
	) :
	m_value							(new value_holder<color>(value)),
	inherited						(value, attributes)
{
}

property_color_reference::~property_color_reference	()
{
	this->!property_color_reference	();
}

property_color_reference::!property_color_reference	()
{
	delete							(m_value);
}

color property_color_reference::get_value_raw		()
{
	return							(m_value->get());
}

void property_color_reference::set_value_raw		(color value)
{
	m_value->set					(value);
}