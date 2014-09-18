////////////////////////////////////////////////////////////////////////////
//	Module 		: property_float.cpp
//	Created 	: 07.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : float property implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_float.hpp"

property_float::property_float			(
		float_getter_type const &getter,
		float_setter_type const &setter,
		float const% increment_factor
	) :
	m_getter				(new float_getter_type(getter)),
	m_setter				(new float_setter_type(setter)),
	m_increment_factor		(increment_factor)
{
}

property_float::~property_float			()
{
	this->!property_float	();
}

property_float::!property_float			()
{
	delete					(m_getter);
	delete					(m_setter);
}

System::Object ^property_float::get_value	()
{
	return					((*m_getter)());
}

void property_float::set_value			(System::Object ^object)
{
	(*m_setter)				(safe_cast<float>(object));
}

void property_float::increment			(float const% increment)
{
	set_value				(safe_cast<float>(get_value()) + increment*m_increment_factor);
}