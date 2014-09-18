////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_ambients_ambient.cpp
//	Created 	: 04.01.2008
//  Modified 	: 04.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment ambients ambient class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_ambients_ambient.hpp"
#include "ide.hpp"
#include "property_collection.hpp"
#include "editor_environment_ambients_effect_id.hpp"
#include "editor_environment_ambients_sound_id.hpp"
#include "editor_environment_ambients_manager.hpp"
#include "editor_environment_sound_channels_channel.hpp"
#include "editor_environment_effects_effect.hpp"

using editor::environment::ambients::ambient;
using editor::environment::ambients::manager;
using editor::environment::ambients::effect_id;
using editor::environment::ambients::sound_id;
using editor::environment::sound_channels::channel;
using editor::environment::effects::effect;

template <>
void property_collection<ambient::effect_container_type, ambient>::display_name	(u32 const& item_index, LPSTR const& buffer, u32 const& buffer_size)
{
	xr_strcpy				(buffer, buffer_size, m_container[item_index]->id().c_str());
}

template <>
editor::property_holder* property_collection<ambient::effect_container_type, ambient>::create	()
{
	effect_id*				object = xr_new<effect_id>(m_holder.effects_manager(), "");
	object->fill			(this);
	return					(object->object());
}

template <>
void property_collection<ambient::sound_container_type, ambient>::display_name	(u32 const& item_index, LPSTR const& buffer, u32 const& buffer_size)
{
	xr_strcpy				(buffer, buffer_size, m_container[item_index]->id().c_str());
}

template <>
editor::property_holder* property_collection<ambient::sound_container_type, ambient>::create	()
{
	sound_id*				object = xr_new<sound_id>(m_holder.sounds_manager(), "");
	object->fill			(this);
	return					(object->object());
}

ambient::ambient			(manager const& manager, shared_str const& id) :
	m_manager				(manager),
	m_property_holder		(0),
	m_effects_collection	(0),
	m_sounds_collection		(0)
{
	m_load_section			= id;
	m_effects_collection	= xr_new<effect_collection_type>(&m_effects_ids, this);
	m_sounds_collection		= xr_new<sound_collection_type>(&m_sound_channels_ids, this);
}

ambient::~ambient			()
{
	delete_data				(m_effects_ids);
	xr_delete				(m_effects_collection);

	delete_data				(m_sound_channels_ids);
	xr_delete				(m_sounds_collection);

	if (!Device.editor())
		return;

	::ide().destroy			(m_property_holder);
}

void ambient::load			(
		CInifile& ambients_config,
		CInifile& sound_channels_config,
		CInifile& effects_config,
		const shared_str& section
	)
{
	VERIFY					(m_load_section == section);
	inherited::load			(
		ambients_config,
		sound_channels_config,
		effects_config,
		m_load_section
	);

	{
		VERIFY				(m_effects_ids.empty());
		LPCSTR				effects_string = READ_IF_EXISTS(&ambients_config, r_string, m_load_section,	"effects", "");
		for (u32 i=0, n=_GetItemCount(effects_string); i<n; ++i) {
			string_path		temp;
			effect_id*		object = xr_new<effect_id>( m_manager.effects_manager(), _GetItem( effects_string, i, temp ) );
			object->fill	(m_effects_collection);
			m_effects_ids.push_back	(object);
		}
	}

	{
		VERIFY				(m_sound_channels_ids.empty());
		LPCSTR				sounds_string = READ_IF_EXISTS(&ambients_config, r_string, m_load_section,	"sound_channels", "");
		for (u32 i=0, n=_GetItemCount(sounds_string); i<n; ++i) {
			string_path		temp;
			sound_id*		object = xr_new<sound_id>( m_manager.sounds_manager(), _GetItem( sounds_string, i, temp ) );
			object->fill	(m_sounds_collection);
			m_sound_channels_ids.push_back	(object);
		}
	}
}

void ambient::save			(CInifile& config)
{
	u32						count = 1;
	LPSTR					temp = 0;
	{
		sound_container_type::const_iterator	b = m_sound_channels_ids.begin(), i = b;
		sound_container_type::const_iterator	e = m_sound_channels_ids.end();
		for ( ; i != e; ++i)
			count			+= (*i)->id().size() + 2;

		temp				= (LPSTR)_alloca(count*sizeof(char));
		*temp				= 0;
		for (i = b; i != e; ++i) {
			if (i == b) {
				xr_strcpy	(temp, count, (*i)->id().c_str());
				continue;
			}

			xr_strcat		(temp, count, ", ");
			xr_strcat		(temp, count, (*i)->id().c_str());
		}
	}

	config.w_string			(m_load_section.c_str(), "sound_channels",	  temp);
	config.w_float			(m_load_section.c_str(), "min_effect_period", float(m_effect_period.x)/1000.f);
	config.w_float			(m_load_section.c_str(), "max_effect_period", float(m_effect_period.y)/1000.f);

	{
		count				= 1;
		effect_container_type::const_iterator	b = m_effects_ids.begin(), i = b;
		effect_container_type::const_iterator	e = m_effects_ids.end();
		for ( ; i != e; ++i)
			count			+= (*i)->id().size() + 2;

		temp				= (LPSTR)_alloca(count*sizeof(char));
		*temp				= 0;
		for (i = b; i != e; ++i) {
			if (i == b) {
				xr_strcpy	(temp, count, (*i)->id().c_str());
				continue;
			}

			xr_strcat		(temp, count, ", ");
			xr_strcat		(temp, count, (*i)->id().c_str());
		}
	}
	config.w_string			(m_load_section.c_str(), "effects", temp);
}

LPCSTR ambient::id_getter						() const
{
	return					(m_load_section.c_str());
}

void ambient::id_setter							(LPCSTR value_)
{
	shared_str				value = value_;
	if (m_load_section._get() == value._get())
		return;

	m_load_section			= m_manager.unique_id(value);
}

void ambient::fill			(editor::property_holder_collection* collection)
{
	VERIFY					(!m_property_holder);
	m_property_holder		= ::ide().create_property_holder(m_load_section.c_str(), collection, this);

	typedef editor::property_holder::string_getter_type	string_getter_type;
	string_getter_type				string_getter;
	string_getter.bind				(this, &ambient::id_getter);

	typedef editor::property_holder::string_setter_type	string_setter_type;
	string_setter_type				string_setter;
	string_setter.bind				(this, &ambient::id_setter);

	m_property_holder->add_property	(
		"id",
		"properties",
		"this option is resposible for ambient identifier",
		m_load_section.c_str(),
		string_getter,
		string_setter
	);
	m_property_holder->add_property	(
		"minimum period",
		"effects",
		"this option is resposible for minimum effect period (in seconds)",
		m_effect_period.x,
		m_effect_period.x
	);
	m_property_holder->add_property	(
		"maximum period",
		"effects",
		"this option is resposible for maximum effect period (in seconds)",
		m_effect_period.y,
		m_effect_period.y
	);
	m_property_holder->add_property	(
		"effects",
		"effects",
		"this option is resposible for maximum effects",
		m_effects_collection
	);
	m_property_holder->add_property	(
		"sound channels",
		"sounds",
		"this option is resposible for sound channels",
		m_sounds_collection
	);
}

ambient::property_holder_type* ambient::object	()
{
	return					(m_property_holder);
}

::editor::environment::effects::manager const& ambient::effects_manager		() const
{
	return					(m_manager.effects_manager());
}

::editor::environment::sound_channels::manager const& ambient::sounds_manager	() const
{
	return					(m_manager.sounds_manager());
}

ambient::SEffect* ambient::create_effect			(CInifile& config, LPCSTR id)
{
	effect*					result = xr_new<effect>(m_manager.effects_manager(), id);
	result->load			(config);
	result->fill			(m_effects_collection);
	return					(result);
}

ambient::SSndChannel* ambient::create_sound_channel	(CInifile& config, LPCSTR id)
{
	channel*				result = xr_new<channel>(m_manager.sounds_manager(), id);
	result->load			(config);
	result->fill			(m_sounds_collection);
	return					(result);
}

CEnvAmbient::EffectVec& ambient::effects							()
{
	return					(inherited::effects());
}

CEnvAmbient::SSndChannelVec& ambient::get_snd_channels			()
{
	return					(inherited::get_snd_channels());
}

#endif // #ifdef INGAME_EDITOR