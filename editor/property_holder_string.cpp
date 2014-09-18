////////////////////////////////////////////////////////////////////////////
//	Module 		: property_holder_string.cpp
//	Created 	: 06.12.2007
//  Modified 	: 09.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property holder implementation class (string properties)
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_holder.hpp"
#include "property_container.hpp"
#include "property_string.hpp"
#include "property_string_shared_str.hpp"
#include "property_string_values_value.hpp"
#include "property_string_values_value_getter.hpp"
#include "property_string_values_value_shared_str.hpp"
#include "property_string_values_value_shared_str_getter.hpp"
#include "property_file_name_value.hpp"
#include "property_file_name_value_shared_str.hpp"

ref class property_converter_tree_values;
ref class property_converter_string_values;
ref class property_converter_string_values_no_enter;
ref class property_editor_file_name;
ref class property_editor_tree_values;

using Flobbster::Windows::Forms::PropertySpec;
using System::String;

//	NotifyParentPropertyAttribute
//	PasswordPropertyTextAttribute
//	RefreshPropertiesAttribute
//	spec->Attributes			= 
//		gcnew array<System::Attribute^>{
//				gcnew System::ComponentModel::ReadOnlyAttribute(true)
//		

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		LPCSTR default_value,
		string_getter_type const &getter,
		string_setter_type const &setter,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			String::typeid,
			to_string(category),
			to_string(description),
			to_string(default_value)
		),
		gcnew property_string(
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
		LPCSTR default_value,
		shared_str& value,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			String::typeid,
			to_string(category),
			to_string(description),
			to_string(default_value)
		),
		gcnew property_string_shared_str(
			m_engine,
			value
		)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		LPCSTR default_value,
		string_getter_type const &getter,
		string_setter_type const &setter,
		LPCSTR default_extension,
		LPCSTR file_mask,
		LPCSTR default_folder,
		LPCSTR caption,
		enter_text_enum const& can_enter_text,
		extension_action_enum const& extension_action,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		(can_enter_text == property_holder::can_enter_text)? 
		gcnew PropertySpec(
			to_string(identifier),
			String::typeid,
			to_string(category),
			to_string(description),
			to_string(default_value),
			property_editor_file_name::typeid,
			(String^)nullptr
		) :
		gcnew PropertySpec(
			to_string(identifier),
			String::typeid,
			to_string(category),
			to_string(description),
			to_string(default_value),
			property_editor_file_name::typeid,
			property_converter_tree_values::typeid
		),
		gcnew property_file_name_value(
			getter,
			setter,
			to_string(default_extension),
			to_string(file_mask),
			to_string(default_folder),
			to_string(caption),
			extension_action == remove_extension
		)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		LPCSTR default_value,
		shared_str& value,
		LPCSTR default_extension,
		LPCSTR file_mask,
		LPCSTR default_folder,
		LPCSTR caption,
		enter_text_enum const& can_enter_text,
		extension_action_enum const& extension_action,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		(can_enter_text == property_holder::can_enter_text)? 
		gcnew PropertySpec(
			to_string(identifier),
			String::typeid,
			to_string(category),
			to_string(description),
			to_string(default_value),
			property_editor_file_name::typeid,
			(String^)nullptr
		) :
		gcnew PropertySpec(
			to_string(identifier),
			String::typeid,
			to_string(category),
			to_string(description),
			to_string(default_value),
			property_editor_file_name::typeid,
			property_converter_tree_values::typeid
		),
		gcnew property_file_name_value_shared_str(
			m_engine,
			value,
			to_string(default_extension),
			to_string(file_mask),
			to_string(default_folder),
			to_string(caption),
			extension_action == remove_extension
		)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		LPCSTR default_value,
		string_getter_type const &getter,
		string_setter_type const &setter,
		LPCSTR const* values,
		u32 const &value_count,
		value_editor_enum const& value_editor,
		enter_text_enum const& can_enter_text,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	PropertySpec	^spec = nullptr;

	if (value_editor == value_editor_tree_view) {
		if (can_enter_text == property_holder::can_enter_text)
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					property_editor_tree_values::typeid,
					(String^)nullptr
				);
		else
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					property_editor_tree_values::typeid,
					property_converter_tree_values::typeid
				);
	}
	else {
		VERIFY		(value_editor == value_editor_combo_box);
		if (can_enter_text == property_holder::can_enter_text)
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					(String^)nullptr,
					property_converter_string_values::typeid
				);
		else
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					(String^)nullptr,
					property_converter_string_values_no_enter::typeid
				);
	}
	m_container->add_property	(
		spec,
		gcnew property_string_values_value(
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
		LPCSTR default_value,
		shared_str& value,
		LPCSTR const* values,
		u32 const &value_count,
		value_editor_enum const& value_editor,
		enter_text_enum const& can_enter_text,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	PropertySpec	^spec = nullptr;

	if (value_editor == value_editor_tree_view) {
		if (can_enter_text == property_holder::can_enter_text)
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					property_editor_tree_values::typeid,
					(String^)nullptr
				);
		else
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					property_editor_tree_values::typeid,
					property_converter_tree_values::typeid
				);
	}
	else {
		VERIFY		(value_editor == value_editor_combo_box);
		if (can_enter_text == property_holder::can_enter_text)
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					(String^)nullptr,
					property_converter_string_values::typeid
				);
		else
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					(String^)nullptr,
					property_converter_string_values_no_enter::typeid
				);
	}
	m_container->add_property	(
		spec,
		gcnew property_string_values_value_shared_str(
			m_engine,
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
		LPCSTR default_value,
		string_getter_type const &getter,
		string_setter_type const &setter,
		string_collection_getter_type const& values,
		string_collection_size_getter_type const& value_count,
		value_editor_enum const& value_editor,
		enter_text_enum const& can_enter_text,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	PropertySpec	^spec = nullptr;

	if (value_editor == value_editor_tree_view) {
		if (can_enter_text == property_holder::can_enter_text)
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					property_editor_tree_values::typeid,
					(String^)nullptr
				);
		else
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					property_editor_tree_values::typeid,
					property_converter_tree_values::typeid
				);
	}
	else {
		VERIFY		(value_editor == value_editor_combo_box);
		if (can_enter_text == property_holder::can_enter_text)
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					(String^)nullptr,
					property_converter_string_values::typeid
				);
		else
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					(String^)nullptr,
					property_converter_string_values_no_enter::typeid
				);
	}
	m_container->add_property	(
		spec,
		gcnew property_string_values_value_getter(
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
		LPCSTR default_value,
		shared_str& value,
		string_collection_getter_type const& values,
		string_collection_size_getter_type const& value_count,
		value_editor_enum const& value_editor,
		enter_text_enum const& can_enter_text,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	PropertySpec	^spec = nullptr;

	if (value_editor == value_editor_tree_view) {
		if (can_enter_text == property_holder::can_enter_text)
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					property_editor_tree_values::typeid,
					(String^)nullptr
				);
		else
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					property_editor_tree_values::typeid,
					property_converter_tree_values::typeid
				);
	}
	else {
		VERIFY		(value_editor == value_editor_combo_box);
		if (can_enter_text == property_holder::can_enter_text)
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					(String^)nullptr,
					property_converter_string_values::typeid
				);
		else
			spec	= 
				gcnew PropertySpec(
					to_string(identifier),
					String::typeid,
					to_string(category),
					to_string(description),
					to_string(default_value),
					(String^)nullptr,
					property_converter_string_values_no_enter::typeid
				);
	}
	m_container->add_property	(
		spec,
		gcnew property_string_values_value_shared_str_getter(
			m_engine,
			value,
			values,
			value_count
		)
	);

	return						(nullptr);
}
