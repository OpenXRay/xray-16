////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_sound_channels_channel.hpp
//	Created 	: 04.01.2008
//  Modified 	: 04.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment sound channels channel class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_SOUND_CHANNELS_CHANNEL_HPP_INCLUDED
#define EDITOR_WEATHER_SOUND_CHANNELS_CHANNEL_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>
#include "../include/editor/property_holder.hpp"
#include "property_collection_forward.hpp"
#include "environment.h"

namespace editor {

class property_holder_collection;

namespace environment {
namespace sound_channels {

class source;
class manager;

class channel :
	public CEnvAmbient::SSndChannel,
	public editor::property_holder_holder,
	private boost::noncopyable
{
private:
	typedef CEnvAmbient::SSndChannel	inherited;

public:
							channel		(manager const& manager, shared_str const& id);
	virtual					~channel	();
			void			load		(CInifile& config);
			void			save		(CInifile& config);
			void			fill		(editor::property_holder_collection* collection);
	inline	LPCSTR			id			() const { return m_load_section.c_str(); }
	virtual	sounds_type&	sounds		();

private:
			LPCSTR xr_stdcall id_getter	() const;
			void   xr_stdcall id_setter	(LPCSTR value);
public:
	typedef xr_vector<source*>			sound_container_type;

private:
	typedef editor::property_holder		property_holder_type;
	typedef property_collection<
				sound_container_type,
				channel
			>							collection_type;

public:
	virtual	property_holder_type*	object	();

private:
	manager const&			m_manager;
	property_holder_type*	m_property_holder;
	collection_type*		m_collection;
	sound_container_type	m_sounds;
}; // class channel
} // namespace sound_channels
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_SOUND_CHANNELS_CHANNEL_HPP_INCLUDED