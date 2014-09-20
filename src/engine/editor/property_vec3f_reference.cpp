////////////////////////////////////////////////////////////////////////////
//	Module 		: property_vec3f_reference.cpp
//	Created 	: 29.12.2007
//  Modified 	: 29.12.2007
//	Author		: Dmitriy Iassenev
//	Description : vec3f property reference implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_vec3f_reference.hpp"

using editor::vec3f;

property_vec3f_reference::property_vec3f_reference	(vec3f& value) :
	m_value							(new value_holder<vec3f>(value)),
	inherited						(value)
{
}

property_vec3f_reference::~property_vec3f_reference	()
{
	this->!property_vec3f_reference	();
}

property_vec3f_reference::!property_vec3f_reference	()
{
	delete							(m_value);
}

vec3f property_vec3f_reference::get_value_raw		()
{
	return							(m_value->get());
}

void property_vec3f_reference::set_value_raw		(vec3f value)
{
	m_value->set					(value);
}