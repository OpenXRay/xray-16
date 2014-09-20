////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_effects_manager.hpp
//	Created 	: 28.12.2007
//  Modified 	: 28.12.2007
//	Author		: Dmitriy Iassenev
//	Description : editor environment effects manager class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_EFFECTS_MANAGER_HPP_INCLUDED
#define EDITOR_WEATHER_EFFECTS_MANAGER_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>
#include "property_collection_forward.hpp"

namespace editor {

class property_holder;

namespace environment {

class manager;

namespace effects {

class effect;

class manager : private boost::noncopyable {
public:
							manager			(::editor::environment::manager* environment);
							~manager		();
			void			load			();
			void			save			();
			void			fill			(editor::property_holder* holder);
			shared_str		unique_id		(shared_str const& id) const;

public:
	inline	::editor::environment::manager&	environment	() const {return m_environment;}

public:
	typedef xr_vector<effect*>				effect_container_type;
	typedef xr_vector<LPSTR>				effects_ids_type;

public:
	effects_ids_type const&	effects_ids		() const;

private:
	typedef editor::property_holder			property_holder_type;
	typedef property_collection<
				effect_container_type,
				manager
			>								collection_type;

private:
	effect_container_type					m_effects;
	mutable effects_ids_type				m_effects_ids;
	::editor::environment::manager&			m_environment;
	property_holder_type*					m_property_holder;
	collection_type*						m_collection;
	mutable bool							m_changed;
}; // class effects_manager

} // namespace effects
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_EFFECTS_MANAGER_HPP_INCLUDED