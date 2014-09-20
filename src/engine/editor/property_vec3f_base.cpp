////////////////////////////////////////////////////////////////////////////
//	Module 		: property_vec3f_base.cpp
//	Created 	: 29.12.2007
//  Modified 	: 29.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property vec3f base class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_vec3f_base.hpp"
#include "property_container.hpp"
#include "property_float_limited.hpp"

using System::Object;
using System::String;
using Flobbster::Windows::Forms::PropertySpec;
using System::Collections::DictionaryEntry;

ref class property_converter_float;

vec3f_components::vec3f_components			(property_vec3f_base^ holder) :
	m_holder			(holder)
{
}

float vec3f_components::x_getter			()
{
	return				(m_holder->get_value_raw().x);
}

void vec3f_components::x_setter			(float value)
{
	m_holder->x			(value);
}

float vec3f_components::y_getter			()
{
	return				(m_holder->get_value_raw().y);
}

void vec3f_components::y_setter			(float value)
{
	m_holder->y			(value);
}

float vec3f_components::z_getter			()
{
	return				(m_holder->get_value_raw().z);
}

void vec3f_components::z_setter				(float value)
{
	m_holder->z			(value);
}

property_vec3f_base::property_vec3f_base	(editor::vec3f const% vec3f)
{
	m_container			= gcnew property_container(nullptr, this);
	m_components		= new vec3f_components(this);

	typedef editor::property_holder::float_getter_type	float_getter_type;
	typedef editor::property_holder::float_setter_type	float_setter_type;

	float_getter_type	getter;
	float_setter_type	setter;

	getter.bind			(m_components, &vec3f_components::x_getter);
	setter.bind			(m_components, &vec3f_components::x_setter);
	m_container->add_property	(
		gcnew PropertySpec(
			"x",
			float::typeid,
			"components",
			"X component",
			vec3f.x,
			(String^)nullptr,
			property_converter_float::typeid
		),
		gcnew property_float(
			getter,
			setter,
			.01f
		)
	);

	getter.bind			(m_components, &vec3f_components::y_getter);
	setter.bind			(m_components, &vec3f_components::y_setter);
	m_container->add_property	(
		gcnew PropertySpec(
			"y",
			float::typeid,
			"components",
			"Y component",
			vec3f.y,
			(String^)nullptr,
			property_converter_float::typeid
		),
		gcnew property_float(
			getter,
			setter,
			.01f
		)
	);

	getter.bind			(m_components, &vec3f_components::z_getter);
	setter.bind			(m_components, &vec3f_components::z_setter);
	m_container->add_property	(
		gcnew PropertySpec(
			"z",
			float::typeid,
			"components",
			"Z component",
			vec3f.z,
			(String^)nullptr,
			property_converter_float::typeid
		),
		gcnew property_float(
			getter,
			setter,
			.01f
		)
	);
}

property_vec3f_base::~property_vec3f_base	()
{
	this->!property_vec3f_base();
}

property_vec3f_base::!property_vec3f_base	()
{
	delete				(m_container);
}

Object^ property_vec3f_base::get_value		()
{
	return				(m_container);
}

void property_vec3f_base::set_value			(Object ^object)
{
	Vec3f				vec3f = safe_cast<Vec3f>(object);
	editor::vec3f		value;
	value.x				= vec3f.x;
	value.y				= vec3f.y;
	value.z				= vec3f.z;
	set_value_raw		(value);
}

void property_vec3f_base::x					(float value)
{
	editor::vec3f		current = get_value_raw();
	current.x			= value;
	set_value_raw		(current);
}

void property_vec3f_base::y					(float value)
{
	editor::vec3f		current = get_value_raw();
	current.y			= value;
	set_value_raw		(current);
}

void property_vec3f_base::z					(float value)
{
	editor::vec3f		current = get_value_raw();
	current.z			= value;
	set_value_raw		(current);
}