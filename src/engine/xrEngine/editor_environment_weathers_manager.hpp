////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_weathers_manager.hpp
//	Created 	: 04.01.2008
//  Modified 	: 11.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment weathers manager class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_ENVIRONMENT_WEATHERS_MANAGER_HPP_INCLUDED
#define EDITOR_ENVIRONMENT_WEATHERS_MANAGER_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>
#include "property_collection_forward.hpp"

namespace editor {

class property_holder;

namespace environment {

	class manager;

namespace weathers {

class weather;

class manager : private boost::noncopyable {
public:
							manager				(environment::manager* manager);
							~manager			();
			void			load				();
			void			save				();
			void			fill				(::editor::property_holder* property_holder);
			shared_str		unique_id			(shared_str const& id) const;
			bool	save_current_blend			(char* buffer, u32 const& buffer_size);
			bool	paste_current_time_frame	(char const* buffer, u32 const& buffer_size);
			bool	paste_target_time_frame		(char const* buffer, u32 const& buffer_size);
			void	reload_current_time_frame	();
			void	reload_target_time_frame	();
			void	reload_current_weather		();
			void	reload						();
			bool	add_time_frame				(char const* buffer, u32 const& buffer_size);

public:
	typedef xr_vector<LPCSTR>					weather_ids_type;
	typedef xr_vector<LPCSTR>					times_ids_type;
	typedef xr_vector<weather*>					weather_container_type;

public:
	weather_ids_type const& weather_ids			() const;

private:
	typedef editor::property_holder				property_holder_type;
	typedef property_collection<
				weather_container_type,
				manager
			>									collection_type;

private:
	LPCSTR const* xr_stdcall weathers_getter	() const;
	u32 xr_stdcall weathers_size_getter			() const;
	LPCSTR const* xr_stdcall frames_getter		(LPCSTR weather_id) const;
	u32 xr_stdcall frames_size_getter			(LPCSTR weather_id) const;

private:
	weather_container_type						m_weathers;
	mutable weather_ids_type					m_weather_ids;
	collection_type*							m_collection;
	mutable times_ids_type						m_times_ids;

public:
	environment::manager&						m_manager;
	mutable bool								m_changed;
}; // class manager
} // namespace weathers
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_ENVIRONMENT_WEATHERS_MANAGER_HPP_INCLUDED