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
#include "property_color.hpp"
#include "property_color_reference.hpp"

ref class property_editor_color;
ref class property_converter_color;

using Flobbster::Windows::Forms::PropertySpec;

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		editor::color const& default_value,
		color_getter_type const& getter,
		color_setter_type const& setter,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	PropertySpec^				spec =
		gcnew PropertySpec(
			to_string(identifier),
			Color::typeid,
			to_string(category),
			to_string(description),
			Color(default_value.r, default_value.g, default_value.b),
			property_editor_color::typeid,
			property_converter_color::typeid
		);
	m_container->add_property	(
		spec,
		gcnew property_color(
			getter,
			setter,
			spec->Attributes
		)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		editor::color const& default_value,
		editor::color& value,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	PropertySpec^				spec =
		gcnew PropertySpec(
			to_string(identifier),
			Color::typeid,
			to_string(category),
			to_string(description),
			Color(default_value.r, default_value.g, default_value.b),
			property_editor_color::typeid,
			property_converter_color::typeid
		);
	m_container->add_property	(
		spec,
		gcnew property_color_reference(
			value,
			spec->Attributes
		)
	);

	return						(nullptr);
}