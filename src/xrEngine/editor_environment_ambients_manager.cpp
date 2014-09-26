////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_ambients_manager.cpp
//	Created 	: 04.01.2008
//  Modified 	: 04.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment ambients manager class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_ambients_manager.hpp"
#include "ide.hpp"
#include "property_collection.hpp"
#include "editor_environment_ambients_ambient.hpp"
#include "editor_environment_detail.hpp"
#include "editor_environment_manager.hpp"

using editor::environment::ambients::manager;
using editor::environment::ambients::ambient;
using editor::environment::detail::logical_string_predicate;

template <>
void property_collection<manager::ambient_container_type, manager>::display_name	(u32 const& item_index, LPSTR const& buffer, u32 const& buffer_size)
{
	xr_strcpy				(buffer, buffer_size, m_container[item_index]->id().c_str());
}

template <>
editor::property_holder* property_collection<manager::ambient_container_type, manager>::create	()
{
	ambient*				object = xr_new<ambient>(m_holder, generate_unique_id("ambient_unique_id_").c_str());
	object->fill			(this);
	return					(object->object());
}

manager::manager			(::editor::environment::manager const& manager) :
	m_manager				(manager),
	m_property_holder		(0),
	m_collection			(0),
	m_changed				(true)
{
	m_collection			= xr_new<collection_type>(&m_ambients, this, &m_changed);
}

manager::~manager			()
{
	xr_delete				(m_collection);
	delete_data				(m_ambients);
	delete_data				(m_ambients_ids);

	if (!Device.editor())
		return;

	::ide().destroy			(m_property_holder);
}

void manager::load			()
{
	VERIFY					(m_ambients.empty());

	typedef CInifile::Root	sections_type;
	sections_type&			sections = m_manager.m_ambients_config->sections();
	m_ambients.reserve		(sections.size());
	sections_type::const_iterator	i = sections.begin();
	sections_type::const_iterator	e = sections.end();
	for ( ; i != e; ++i) {
		ambient*			object = xr_new<ambient>(*this, (*i)->Name);
		object->load		(
			*m_manager.m_ambients_config,
			*m_manager.m_sound_channels_config,
			*m_manager.m_effects_config,
			(*i)->Name
		);
		object->fill		(m_collection);
		m_ambients.push_back(object);
	}
}

void manager::save			()
{
	string_path				file_name;
	CInifile*				config =
		xr_new<CInifile>(
			FS.update_path(
				file_name,
				"$game_config$",
				"environment\\ambients.ltx"
			),
			FALSE,
			FALSE,
			TRUE
		);

	ambient_container_type::iterator	i = m_ambients.begin();
	ambient_container_type::iterator	e = m_ambients.end();
	for ( ; i != e; ++i)
		(*i)->save			(*config);

	xr_delete				(config);
}

void manager::fill			(editor::property_holder* holder)
{
	VERIFY					(holder);
	holder->add_property	(
		"ambients",
		"ambients",
		"this option is resposible for ambients",
		m_collection
	);
}

::editor::environment::effects::manager const& manager::effects_manager		() const
{
	return					(m_manager.effects());
}

::editor::environment::sound_channels::manager const& manager::sounds_manager	() const
{
	return					(m_manager.sound_channels());
}

shared_str manager::unique_id	(shared_str const& id) const
{
	if (m_collection->unique_id(id.c_str()))
		return				(id);

	return					(m_collection->generate_unique_id(id.c_str()));
}

manager::ambients_ids_type const& manager::ambients_ids	() const
{
	if (!m_changed)
		return				(m_ambients_ids);

	m_changed				= false;

	delete_data				(m_ambients_ids);

	m_ambients_ids.resize	(m_ambients.size());

	ambient_container_type::const_iterator	i = m_ambients.begin();
	ambient_container_type::const_iterator	e = m_ambients.end();
	ambients_ids_type::iterator				j = m_ambients_ids.begin();
	for ( ; i != e; ++i, ++j)
		*j					= xr_strdup((*i)->id().c_str());

	std::sort				(m_ambients_ids.begin(), m_ambients_ids.end(), logical_string_predicate());

	return					(m_ambients_ids);
}

ambient* manager::get_ambient							(shared_str const& id) const
{
	ambient_container_type::const_iterator	i = m_ambients.begin();
	ambient_container_type::const_iterator	e = m_ambients.end();
	for ( ; i != e; ++i)
		if ((*i)->id() == id)
			return			(*i);

	NODEFAULT;
#ifdef DEBUG
	return					(0);
#endif // #ifdef DEBUG
}

#endif // #ifdef INGAME_EDITOR