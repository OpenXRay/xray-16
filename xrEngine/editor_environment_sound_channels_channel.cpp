////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_sound_channels_channel.cpp
//	Created 	: 04.01.2008
//  Modified 	: 04.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment sound channels channel class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_sound_channels_channel.hpp"
#include "ide.hpp"
#include "property_collection.hpp"
#include "editor_environment_sound_channels_source.hpp"
#include "editor_environment_sound_channels_manager.hpp"

using editor::environment::sound_channels::channel;
using editor::environment::sound_channels::source;
using editor::environment::sound_channels::manager;

template <>
void property_collection<channel::sound_container_type, channel>::display_name	(u32 const& item_index, LPSTR const& buffer, u32 const& buffer_size)
{
	xr_strcpy				(buffer, buffer_size, m_container[item_index]->id());
}

template <>
editor::property_holder* property_collection<channel::sound_container_type, channel>::create	()
{
	source*					object = xr_new<source>("");
	object->fill			(this);
	return					(object->object());
}

channel::channel			(manager const& manager, shared_str const& id) :
	m_manager			(manager),
	m_property_holder	(0),
	m_collection		(0)
{
	m_load_section		= id;
	m_sound_dist		= Fvector2().set(0.f,0.f);
	m_sound_period		= Ivector4().set(0,0,0,0);
	m_collection		= xr_new<collection_type>(&m_sounds, this);
}

channel::~channel			()
{
	xr_delete			(m_collection);
	delete_data			(m_sounds);

	if (!Device.editor())
		return;

	::ide().destroy		(m_property_holder);
}

void channel::load			(CInifile& config)
{
	inherited::load		(config, m_load_section.c_str());

	VERIFY				(m_sounds.empty());
	LPCSTR sounds		= config.r_string		(m_load_section,	"sounds");
	string_path			sound;
	for (u32 i=0, n=_GetItemCount(sounds); i<n; ++i) {
		source*				object = xr_new<source>( _GetItem( sounds, i, sound ) );
		object->fill		(m_collection);
		m_sounds.push_back	(object);
	}
}

void channel::save			(CInifile& config)
{
	config.w_float		(m_load_section.c_str(), "min_distance",	m_sound_dist.x	);
	config.w_float		(m_load_section.c_str(), "max_distance",	m_sound_dist.y	);
	config.w_s32		(m_load_section.c_str(), "period0",			m_sound_period.x);
	config.w_s32		(m_load_section.c_str(), "period1",			m_sound_period.y);
	config.w_s32		(m_load_section.c_str(), "period2",			m_sound_period.z);
	config.w_s32		(m_load_section.c_str(), "period3",			m_sound_period.w);

	u32					count = 1;
	sound_container_type::const_iterator	b = m_sounds.begin(), i = b;
	sound_container_type::const_iterator	e = m_sounds.end();
	for ( ; i != e; ++i)
		count			+= xr_strlen((*i)->id()) + 2;

	LPSTR				temp = (LPSTR)_alloca(count*sizeof(char));
	*temp				= 0;
	for (i = b; i != e; ++i) {
		if (i == b) {
			xr_strcpy	(temp, count, (*i)->id());
			continue;
		}

		xr_strcat		(temp, count, ", ");
		xr_strcat		(temp, count, (*i)->id());
	}

	config.w_string		(m_load_section.c_str(),	"sounds", temp);
}

LPCSTR channel::id_getter	() const
{
	return				(m_load_section.c_str());
}

void channel::id_setter		(LPCSTR value_)
{
	shared_str			value = value_;
	if (m_load_section._get() == value._get())
		return;

	m_load_section		= m_manager.unique_id(value);
}

void channel::fill			(editor::property_holder_collection* collection)
{
	VERIFY				(!m_property_holder);
	m_property_holder	= ::ide().create_property_holder(m_load_section.c_str(), collection, this);

	typedef editor::property_holder::string_getter_type	string_getter_type;
	string_getter_type	string_getter;
	string_getter.bind	(this, &channel::id_getter);

	typedef editor::property_holder::string_setter_type	string_setter_type;
	string_setter_type	string_setter;
	string_setter.bind	(this, &channel::id_setter);

	m_property_holder->add_property	(
		"id",
		"properties",
		"this option is resposible for sound channel id",
		m_load_section.c_str(),
		string_getter,
		string_setter
	);
	m_property_holder->add_property	(
		"minimum distance",
		"properties",
		"this option is resposible for minimum distance (in meters)",
		m_sound_dist.x,
		m_sound_dist.x
	);
	m_property_holder->add_property	(
		"maximum distance",
		"properties",
		"this option is resposible for maximum distance (in meters)",
		m_sound_dist.y,
		m_sound_dist.y
	);
	m_property_holder->add_property	(
		"period 0",
		"properties",
		"this option is resposible for minimum start time interval (in seconds)",
		m_sound_period.x,
		m_sound_period.x
	);
	m_property_holder->add_property	(
		"period 1",
		"properties",
		"this option is resposible for maximum start time interval (in seconds)",
		m_sound_period.y,
		m_sound_period.y
	);
	m_property_holder->add_property	(
		"period 2",
		"properties",
		"this option is resposible for minimum pause interval (in seconds)",
		m_sound_period.z,
		m_sound_period.z
	);
	m_property_holder->add_property	(
		"period 3",
		"properties",
		"this option is resposible for maximum pause interval (in seconds)",
		m_sound_period.w,
		m_sound_period.w
	);
	m_property_holder->add_property	(
		"sounds",
		"properties",
		"this option is resposible for sound sources",
		m_collection
	);
}

channel::property_holder_type* channel::object	()
{
	return				(m_property_holder);
}

CEnvAmbient::SSndChannel::sounds_type& channel::sounds	()
{
	return				(inherited::sounds());
}

#endif // #ifdef INGAME_EDITOR