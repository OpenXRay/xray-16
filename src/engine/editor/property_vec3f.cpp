////////////////////////////////////////////////////////////////////////////
//	Module 		: property_vec3f.cpp
//	Created 	: 29.12.2007
//  Modified 	: 29.12.2007
//	Author		: Dmitriy Iassenev
//	Description : vec3f property implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_vec3f.hpp"

using editor::vec3f;

property_vec3f::property_vec3f		(
		vec3f_getter_type const &getter,
		vec3f_setter_type const &setter
	) :
	m_getter				(new vec3f_getter_type(getter)),
	m_setter				(new vec3f_setter_type(setter)),
	inherited				(getter())
{
}

property_vec3f::~property_vec3f		()
{
	this->!property_vec3f	();
}

property_vec3f::!property_vec3f		()
{
	delete					(m_getter);
	delete					(m_setter);
}

vec3f property_vec3f::get_value_raw	()
{
	return					((*m_getter)());
}

void property_vec3f::set_value_raw	(vec3f value)
{
	(*m_setter)				(value);
}