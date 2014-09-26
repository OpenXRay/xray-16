////////////////////////////////////////////////////////////////////////////
//	Module 		: property_property_container.cpp
//	Created 	: 11.12.2007
//  Modified 	: 11.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property container property implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_property_container.hpp"
#include "property_holder.hpp"

using System::Object;

property_property_container::property_property_container(property_holder* object) :
	m_object	(object)
{
}

Object^ property_property_container::get_value			()
{
	return		(m_object->container());
}

void property_property_container::set_value				(Object ^object)
{
	NODEFAULT;
}