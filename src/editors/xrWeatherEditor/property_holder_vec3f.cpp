////////////////////////////////////////////////////////////////////////////
//	Module 		: property_holder_color.cpp
//	Created 	: 06.12.2007
//  Modified 	: 08.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property holder implementation class (color properties)
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_holder.hpp"
#include "property_container.hpp"
#include "property_vec3f.hpp"
#include "property_vec3f_reference.hpp"

ref class property_converter_vec3f;

using Flobbster::Windows::Forms::PropertySpec;
using System::String;

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		editor::vec3f const& default_value,
		vec3f_getter_type const& getter,
		vec3f_setter_type const& setter,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			Vec3f::typeid,
			to_string(category),
			to_string(description),
			Vec3f(default_value.x, default_value.y, default_value.z),
			(String^)nullptr,
			property_converter_vec3f::typeid
		),
		gcnew property_vec3f(
			getter,
			setter
		)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		editor::vec3f const& default_value,
		editor::vec3f& value,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			Vec3f::typeid,
			to_string(category),
			to_string(description),
			Vec3f(default_value.x, default_value.y, default_value.z),
			(String^)nullptr,
			property_converter_vec3f::typeid
		),
		gcnew property_vec3f_reference(value)
	);

	return						(nullptr);
}
