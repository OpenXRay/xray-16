////////////////////////////////////////////////////////////////////////////
//	Module 		: property_holder.hpp
//	Created 	: 04.12.2007
//  Modified 	: 04.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property holder interface class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_PROPERTY_HOLDER_HPP_INCLUDED
#define EDITOR_PROPERTY_HOLDER_HPP_INCLUDED

class shared_str;

namespace editor {

#pragma pack(push,4)
struct color {
	float	r,g,b;
};

struct vec3f {
	float	x,y,z;
};
#pragma pack(pop)

class property_holder;

class property_holder_holder {
public:
	virtual	property_holder*	object					() = 0;
	virtual						~property_holder_holder	() = 0 {}
};

class property_holder_collection {
public:
	virtual	void				clear			() = 0;
	virtual	void				insert			(property_holder* holder, u32 const& position) = 0;
	virtual	void				erase			(u32 const& position) = 0;
	virtual	int					index			(property_holder* holder) = 0;
	virtual	property_holder*	item			(u32 const& position) = 0;
	virtual	u32					size			() = 0;
	virtual	void				display_name	(u32 const& item_index, char* const& buffer, u32 const& buffer_size) = 0;
	virtual	property_holder*	create			() = 0;
	virtual	void				destroy			(property_holder* holder) = 0;
}; // class propery_holder_collection

class property_value;

class property_holder {
public:
	typedef fastdelegate::FastDelegate0<bool>			boolean_getter_type;
	typedef fastdelegate::FastDelegate1<bool>			boolean_setter_type;

	typedef fastdelegate::FastDelegate0<int>			integer_getter_type;
	typedef fastdelegate::FastDelegate1<int>			integer_setter_type;

	typedef fastdelegate::FastDelegate0<float>			float_getter_type;
	typedef fastdelegate::FastDelegate1<float>			float_setter_type;

	typedef fastdelegate::FastDelegate0<LPCSTR>			string_getter_type;
	typedef fastdelegate::FastDelegate1<LPCSTR>			string_setter_type;

	typedef fastdelegate::FastDelegate0<color>			color_getter_type;
	typedef fastdelegate::FastDelegate1<color>			color_setter_type;

	typedef fastdelegate::FastDelegate0<vec3f>			vec3f_getter_type;
	typedef fastdelegate::FastDelegate1<vec3f>			vec3f_setter_type;

	typedef fastdelegate::FastDelegate0<
				property_holder_collection*
			>											collection_getter_type;

	typedef fastdelegate::FastDelegate0<LPCSTR const*>	string_collection_getter_type;
	typedef fastdelegate::FastDelegate0<u32>			string_collection_size_getter_type;

public:
	enum enter_text_enum {
		can_enter_text					= int(0),
		cannot_enter_text				= int(1),
	}; // enum can_enter_text_enum

	enum extension_action_enum {
		remove_extension				= int(0),
		keep_extension					= int(1),
	}; // enum remove_extension_enum

	enum value_editor_enum {
		value_editor_combo_box			= int(0),
		value_editor_tree_view			= int(1),
	}; // enum value_editor_enum

	enum readonly_enum {
		property_read_only				= int(0),
		property_read_write				= int(1),
	}; // enum value_editor_enum

	enum notify_parent_on_change_enum {
		notify_parent_on_change			= int(0),
		do_not_notify_parent_on_change	= int(1),
	}; // enum value_editor_enum

	enum password_char_enum {
		password_char					= int(0),
		no_password_char				= int(1),
	}; // enum value_editor_enum

	enum refresh_grid_on_change_enum {
		refresh_grid_on_change			= int(0),
		do_not_refresh_grid_on_change	= int(1),
	}; // enum value_editor_enum
						 
public:
	virtual	property_holder_holder*	holder	() = 0;
	virtual	void	clear				() = 0;
	virtual	property_value*	add_property		(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						bool const &default_value,
						boolean_getter_type const &getter,
						boolean_setter_type const &setter,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property		(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						bool const &default_value,
						bool& value,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property		(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						bool const &default_value,
						boolean_getter_type const &getter,
						boolean_setter_type const &setter,
						LPCSTR values[2],
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property		(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						bool const &default_value,
						bool& value,
						LPCSTR values[2],
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						int const &default_value,
						integer_getter_type const &getter,
						integer_setter_type const &setter,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						int const &default_value,
						int& value,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						int const &default_value,
						integer_getter_type const &getter,
						integer_setter_type const &setter,
						int const &min_value,
						int const &max_value,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						int const &default_value,
						int& value,
						int const &min_value,
						int const &max_value,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						int const &default_value,
						integer_getter_type const &getter,
						integer_setter_type const &setter,
						std::pair<int,LPCSTR> *values,
						u32 const &value_count,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						int const &default_value,
						int& value,
						std::pair<int,LPCSTR> *values,
						u32 const &value_count,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						int const &default_value,
						integer_getter_type const &getter,
						integer_setter_type const &setter,
						LPCSTR const* values,
						u32 const &value_count,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						int const &default_value,
						int& value,
						LPCSTR const* values,
						u32 const &value_count,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						int const &default_value,
						integer_getter_type const &getter,
						integer_setter_type const &setter,
						string_collection_getter_type const& values,
						string_collection_size_getter_type const& value_count,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						int const &default_value,
						int& value,
						string_collection_getter_type const& values,
						string_collection_size_getter_type const& value_count,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						float const &default_value,
						float_getter_type const &getter,
						float_setter_type const &setter,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						float const &default_value,
						float& value,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						float const &default_value,
						float_getter_type const &getter,
						float_setter_type const &setter,
						float const &min_value,
						float const &max_value,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						float const &default_value,
						float& value,
						float const &min_value,
						float const &max_value,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						float const &default_value,
						float_getter_type const &getter,
						float_setter_type const &setter,
						std::pair<float,LPCSTR> *values,
						u32 const &value_count,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						float const &default_value,
						float& value,
						std::pair<float,LPCSTR> *values,
						u32 const &value_count,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						LPCSTR default_value,
						string_getter_type const &getter,
						string_setter_type const &setter,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						LPCSTR default_value,
						shared_str& value,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
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
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
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
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
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
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						LPCSTR default_value,
						shared_str& value,
						LPCSTR const* values,
						u32 const &value_count,
						value_editor_enum const& value_editor,
						enter_text_enum const& can_enter_text,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
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
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						LPCSTR default_value,
						shared_str& value,
						string_collection_getter_type const& values,
						string_collection_size_getter_type const& value_count,
						value_editor_enum const& value_editor,
						enter_text_enum const& can_enter_text,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						color const& default_value,
						color_getter_type const& getter,
						color_setter_type const& setter,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						color const& default_value,
						color& result,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						vec3f const& default_value,
						vec3f_getter_type const& getter,
						vec3f_setter_type const& setter,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						vec3f const& default_value,
						vec3f& result,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						property_holder* value,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						property_holder_collection* collection,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
	// probably, dummy method, should be removed sometimes
	virtual	property_value*	add_property(
						LPCSTR identifier,
						LPCSTR category,
						LPCSTR description,
						collection_getter_type const& collection_getter,
						readonly_enum const& read_only = property_read_write,
						notify_parent_on_change_enum const& notify_parent = do_not_notify_parent_on_change,
						password_char_enum const& password = no_password_char,
						refresh_grid_on_change_enum const& refresh_grid = do_not_refresh_grid_on_change
					) = 0;
}; // class property_holder

class property_value {
public:
	virtual	void		attribute	(property_holder::readonly_enum const& read_only) = 0;
	virtual	void		attribute	(property_holder::notify_parent_on_change_enum const& notify_parent) = 0;
	virtual	void		attribute	(property_holder::password_char_enum const& password_char) = 0;
	virtual	void		attribute	(property_holder::refresh_grid_on_change_enum const& refresh_grid) = 0;
}; // class property_value

} // namespace editor

#endif // ifndef EDITOR_PROPERTY_HOLDER_HPP_INCLUDED