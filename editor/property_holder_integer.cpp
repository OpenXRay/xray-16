////////////////////////////////////////////////////////////////////////////
//	Module 		: property_holder_integer.cpp
//	Created 	: 06.12.2007
//  Modified 	: 08.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property holder implementation class (integer properties)
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_holder.hpp"
#include "property_container.hpp"
#include "property_integer.hpp"
#include "property_integer_reference.hpp"
#include "property_integer_limited.hpp"
#include "property_integer_limited_reference.hpp"
#include "property_integer_values_value.hpp"
#include "property_integer_values_value_getter.hpp"
#include "property_integer_values_value_reference.hpp"
#include "property_integer_values_value_reference_getter.hpp"
#include "property_integer_enum_value.hpp"
#include "property_integer_enum_value_reference.hpp"

ref class property_converter_integer_enum;
ref class property_converter_integer_values;
ref class property_integer_values_value;

using Flobbster::Windows::Forms::PropertySpec;
using System::String;

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		int const &default_value,
		integer_getter_type const &getter,
		integer_setter_type const &setter,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			int::typeid,
			to_string(category),
			to_string(description),
			default_value
		),
		gcnew property_integer(
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
		int const &default_value,
		int& value,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			int::typeid,
			to_string(category),
			to_string(description),
			default_value
		),
		gcnew property_integer_reference(value)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		int const &default_value,
		integer_getter_type const &getter,
		integer_setter_type const &setter,
		int const &min_value,
		int const &max_value,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			int::typeid,
			to_string(category),
			to_string(description),
			default_value
		),
		gcnew property_integer_limited(
			getter,
			setter,
			min_value,
			max_value
		)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		int const &default_value,
		int& value,
		int const &min_value,
		int const &max_value,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			int::typeid,
			to_string(category),
			to_string(description),
			default_value
		),
		gcnew property_integer_limited_reference(
			value,
			min_value,
			max_value
		)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		int const &default_value,
		integer_getter_type const &getter,
		integer_setter_type const &setter,
		std::pair<int,LPCSTR> *values,
		u32 const &value_count,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			int::typeid,
			to_string(category),
			to_string(description),
			default_value,
			(String^)nullptr,
			property_converter_integer_enum::typeid
		),
		gcnew property_integer_enum_value(
			getter,
			setter,
			values,
			value_count
		)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		int const &default_value,
		int& value,
		std::pair<int,LPCSTR> *values,
		u32 const &value_count,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			int::typeid,
			to_string(category),
			to_string(description),
			default_value,
			(String^)nullptr,
			property_converter_integer_enum::typeid
		),
		gcnew property_integer_enum_value_reference(
			value,
			values,
			value_count
		)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		int const &default_value,
		integer_getter_type const &getter,
		integer_setter_type const &setter,
		LPCSTR const* values,
		u32 const &value_count,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			int::typeid,
			to_string(category),
			to_string(description),
			default_value,
			(String^)nullptr,
			property_converter_integer_values::typeid
		),
		gcnew property_integer_values_value(
			getter,
			setter,
			values,
			value_count
		)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		int const &default_value,
		int& value,
		LPCSTR const* values,
		u32 const &value_count,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			int::typeid,
			to_string(category),
			to_string(description),
			default_value,
			(String^)nullptr,
			property_converter_integer_values::typeid
		),
		gcnew property_integer_values_value_reference(
			value,
			values,
			value_count
		)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		int const &default_value,
		integer_getter_type const &getter,
		integer_setter_type const &setter,
		string_collection_getter_type const& values,
		string_collection_size_getter_type const& value_count,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			int::typeid,
			to_string(category),
			to_string(description),
			default_value,
			(String^)nullptr,
			property_converter_integer_values::typeid
		),
		gcnew property_integer_values_value_getter(
			getter,
			setter,
			values,
			value_count
		)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		int const &default_value,
		int& value,
		string_collection_getter_type const& values,
		string_collection_size_getter_type const& value_count,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			int::typeid,
			to_string(category),
			to_string(description),
			default_value,
			(String^)nullptr,
			property_converter_integer_values::typeid
		),
		gcnew property_integer_values_value_reference_getter(
			value,
			values,
			value_count
		)
	);

	return						(nullptr);
}