////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_ambients_manager.hpp
//	Created 	: 04.01.2008
//  Modified 	: 04.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment ambients manager class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_AMBIENTS_MANAGER_HPP_INCLUDED
#define EDITOR_WEATHER_AMBIENTS_MANAGER_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>
#include "property_collection_forward.hpp"

namespace editor {

	class property_holder;

namespace environment {

	class manager;

	namespace effects {
		class manager;
	} // namespace effects
	
	namespace sound_channels {
		class manager;
	} // namespace sound_channels

namespace ambients {

class ambient;

class manager : private boost::noncopyable {
public:
							manager			(::editor::environment::manager const& manager);
							~manager		();
			void			load			();
			void			save			();
			void			fill			(editor::property_holder* holder);
			shared_str		unique_id		(shared_str const& id) const;
			ambient*		get_ambient		(shared_str const& id) const;

public:
	effects::manager const&			effects_manager	() const;
	sound_channels::manager const&	sounds_manager	() const;

public:
	typedef xr_vector<ambient*>				ambient_container_type;
	typedef xr_vector<LPSTR>				ambients_ids_type;

public:
	ambients_ids_type const& ambients_ids	() const;

private:
	typedef editor::property_holder			property_holder_type;
	typedef property_collection<
				ambient_container_type,
				manager
			>								collection_type;

private:
	ambient_container_type					m_ambients;
	mutable ambients_ids_type				m_ambients_ids;
	::editor::environment::manager const&	m_manager;
	property_holder_type*					m_property_holder;
	collection_type*						m_collection;
	mutable bool							m_changed;
}; // class manager
} // namespace ambients
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_AMBIENTS_MANAGER_HPP_INCLUDED