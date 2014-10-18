////////////////////////////////////////////////////////////////////////////
//	Module 		: property_holder.hpp
//	Created 	: 06.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property holder implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_HOLDER_HPP_INCLUDED
#define PROPERTY_HOLDER_HPP_INCLUDED

#include "property_holder_include.hpp"

ref class property_container;
ref class property_holder_converter;

namespace editor {
	class engine;
	class property_holder_collection;
} // namespace editor 

class property_holder : public editor::property_holder {
public:
						property_holder		(
							editor::engine* engine,
							LPCSTR display_name,
							editor::property_holder_collection* collection,
							editor::property_holder_holder* holder
						);
	virtual				~property_holder	();
			void		on_dispose			();
	property_container	^container			();
	editor::engine&		engine				();

public:
	virtual	editor::property_holder_holder*	holder	();
	virtual	void	clear				();
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						bool const &default_value,
						bool& value,
						readonly_enum const& read_only,
						notify_parent_on_change_enum const& notify_parent,
						password_char_enum const& password,
						refresh_grid_on_change_enum const& refresh_grid
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						int const &default_value,
						int& value,
						readonly_enum const& read_only,
						notify_parent_on_change_enum const& notify_parent,
						password_char_enum const& password,
						refresh_grid_on_change_enum const& refresh_grid
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						float const &default_value,
						float& value,
						readonly_enum const& read_only,
						notify_parent_on_change_enum const& notify_parent,
						password_char_enum const& password,
						refresh_grid_on_change_enum const& refresh_grid
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						LPCSTR default_value,
						shared_str& value,
						readonly_enum const& read_only,
						notify_parent_on_change_enum const& notify_parent,
						password_char_enum const& password,
						refresh_grid_on_change_enum const& refresh_grid
					);
	virtual	editor::property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						LPCSTR default_value,
						string_getter_type const &getter,
						string_setter_type const &setter,
						LPCSTR default_extension,	// ".dds",
						LPCSTR file_mask,			// "Texture files (*.dds)|*.dds",
						LPCSTR default_folder,		// "R:\\development\\priquel\\resources\\gamedata\\textures\\sky",
						LPCSTR caption,				// "Select texture..."
						enter_text_enum const& can_enter_text,
						extension_action_enum const& remove_extension,
						readonly_enum const& read_only,
						notify_parent_on_change_enum const& notify_parent,
						password_char_enum const& password,
						refresh_grid_on_change_enum const& refresh_grid
					);
	virtual	editor::property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						LPCSTR default_value,
						shared_str& value,
						LPCSTR default_extension,	// ".dds",
						LPCSTR file_mask,			// "Texture files (*.dds)|*.dds",
						LPCSTR default_folder,		// "R:\\development\\priquel\\resources\\gamedata\\textures\\sky",
						LPCSTR caption,				// "Select texture..."
						enter_text_enum const& can_enter_text,
						extension_action_enum const& remove_extension,
						readonly_enum const& read_only,
						notify_parent_on_change_enum const& notify_parent,
						password_char_enum const& password,
						refresh_grid_on_change_enum const& refresh_grid
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						editor::color const& default_value,
						editor::color& result,
						readonly_enum const& read_only,
						notify_parent_on_change_enum const& notify_parent,
						password_char_enum const& password,
						refresh_grid_on_change_enum const& refresh_grid
					);
	virtual	editor::property_value*	add_property(
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
					);
	virtual	editor::property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						editor::vec3f const& default_value,
						editor::vec3f& result,
						readonly_enum const& read_only,
						notify_parent_on_change_enum const& notify_parent,
						password_char_enum const& password,
						refresh_grid_on_change_enum const& refresh_grid
					);
	virtual	editor::property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						editor::property_holder* value,
						readonly_enum const& read_only,
						notify_parent_on_change_enum const& notify_parent,
						password_char_enum const& password,
						refresh_grid_on_change_enum const& refresh_grid
					);
	virtual	editor::property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						editor::property_holder_collection* collection,
						readonly_enum const& read_only,
						notify_parent_on_change_enum const& notify_parent,
						password_char_enum const& password,
						refresh_grid_on_change_enum const& refresh_grid
					);
	virtual	editor::property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						collection_getter_type const& collection_getter,
						readonly_enum const& read_only,
						notify_parent_on_change_enum const& notify_parent,
						password_char_enum const& password,
						refresh_grid_on_change_enum const& refresh_grid
					);

public:
	typedef editor::property_holder_collection	collection_type;

	System::String^	display_name();
	collection_type*collection	();

private:
	gcroot<property_container^>		m_container;
	gcroot<System::String^>			m_display_name;
	collection_type*				m_collection;
	editor::engine*					m_engine;
	editor::property_holder_holder* m_holder;
	bool							m_disposing;
}; // class property_holder

#endif // ifndef PROPERTY_HOLDER_HPP_INCLUDED