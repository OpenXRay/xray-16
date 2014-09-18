////////////////////////////////////////////////////////////////////////////
//	Module 		: property_holder_boolean.cpp
//	Created 	: 06.12.2007
//  Modified 	: 08.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property holder implementation class (boolean properties)
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_holder.hpp"
#include "property_container.hpp"
#include "property_boolean.hpp"
#include "property_boolean_reference.hpp"
#include "property_boolean_values_value.hpp"
#include "property_boolean_values_value_reference.hpp"

ref class property_converter_boolean_values;

using Flobbster::Windows::Forms::PropertySpec;
using System::String;

editor::property_value* property_holder::add_property		(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		bool const &default_value,
		boolean_getter_type const &getter,
		boolean_setter_type const &setter,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			bool::typeid,
			to_string(category),
			to_string(description),
			default_value
		),
		gcnew property_boolean(
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
		bool const& default_value,
		bool& value,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			bool::typeid,
			to_string(category),
			to_string(description),
			default_value
		),
		gcnew property_boolean_reference(value)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		bool const &default_value,
		boolean_getter_type const &getter,
		boolean_setter_type const &setter,
		LPCSTR values[2],
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			bool::typeid,
			to_string(category),
			to_string(description),
			default_value,
			(String^)nullptr,
			property_converter_boolean_values::typeid
		),
		gcnew property_boolean_values_value(
			getter,
			setter,
			values
		)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		bool const &default_value,
		bool& value,
		LPCSTR values[2],
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			bool::typeid,
			to_string(category),
			to_string(description),
			default_value,
			(String^)nullptr,
			property_converter_boolean_values::typeid
		),
		gcnew property_boolean_values_value_reference(
			value,
			values
		)
	);

	return						(nullptr);
}