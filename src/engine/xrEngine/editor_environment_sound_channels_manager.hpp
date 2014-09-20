////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_sound_channels_manager.hpp
//	Created 	: 04.01.2008
//  Modified 	: 04.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment sound channels manager class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_SOUND_CHANNELS_MANAGER_HPP_INCLUDED
#define EDITOR_WEATHER_SOUND_CHANNELS_MANAGER_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>
#include "property_collection_forward.hpp"

namespace editor {

class property_holder;

namespace environment {
namespace sound_channels {

class channel;

class manager : private boost::noncopyable {
public:
							manager		();
							~manager	();
			void			load		();
			void			save		();
			void			fill		(editor::property_holder* holder);
			shared_str		unique_id	(shared_str const& id) const;

public:
	typedef xr_vector<channel*>			channel_container_type;
	typedef xr_vector<LPSTR>			channels_ids_type;

public:
	channels_ids_type const&channels_ids() const;

private:
	typedef editor::property_holder		property_holder_type;
	typedef property_collection<
				channel_container_type,
				manager
			>							collection_type;

private:
	channel_container_type				m_channels;
	mutable channels_ids_type			m_channels_ids;
	property_holder_type*				m_property_holder;
	collection_type*					m_collection;
	mutable bool						m_changed;
}; // class manager
} // namespace sound_channels
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_SOUND_CHANNELS_MANAGER_HPP_INCLUDED