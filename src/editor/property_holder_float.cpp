////////////////////////////////////////////////////////////////////////////
//	Module 		: property_holder_float.cpp
//	Created 	: 06.12.2007
//  Modified 	: 08.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property holder implementation class (float properties)
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_holder.hpp"
#include "property_container.hpp"
#include "property_float.hpp"
#include "property_float_reference.hpp"
#include "property_float_limited.hpp"
#include "property_float_limited_reference.hpp"
#include "property_float_enum_value.hpp"
#include "property_float_enum_value_reference.hpp"

ref class property_converter_float_enum;
ref class property_converter_float;

using Flobbster::Windows::Forms::PropertySpec;
using System::String;

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		float const &default_value,
		float_getter_type const &getter,
		float_setter_type const &setter,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			float::typeid,
			to_string(category),
			to_string(description),
			default_value,
			(String^)nullptr,
			property_converter_float::typeid
		),
		gcnew property_float(
			getter,
			setter,
			.05f
		)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		float const &default_value,
		float& value,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			float::typeid,
			to_string(category),
			to_string(description),
			default_value,
			(String^)nullptr,
			property_converter_float::typeid
		),
		gcnew property_float_reference(value, .05f)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		float const &default_value,
		float_getter_type const &getter,
		float_setter_type const &setter,
		float const &min_value,
		float const &max_value,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			float::typeid,
			to_string(category),
			to_string(description),
			default_value,
			(String^)nullptr,
			property_converter_float::typeid
		),
		gcnew property_float_limited(
			getter,
			setter,
			(max_value - min_value)*.0025f,
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
		float const &default_value,
		float& value,
		float const &min_value,
		float const &max_value,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			float::typeid,
			to_string(category),
			to_string(description),
			default_value,
			(String^)nullptr,
			property_converter_float::typeid
		),
		gcnew property_float_limited_reference(
			value,
			(max_value - min_value)*.0025f,
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
		float const &default_value,
		float_getter_type const &getter,
		float_setter_type const &setter,
		std::pair<float,LPCSTR> *values,
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
			float::typeid,
			to_string(category),
			to_string(description),
			default_value,
			(String^)nullptr,
			property_converter_float_enum::typeid
		),
		gcnew property_float_enum_value(
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
		float const &default_value,
		float& value,
		std::pair<float,LPCSTR> *values,
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
			float::typeid,
			to_string(category),
			to_string(description),
			default_value,
			(String^)nullptr,
			property_converter_float_enum::typeid
		),
		gcnew property_float_enum_value_reference(
			value,
			values,
			value_count
		)
	);

	return						(nullptr);
}