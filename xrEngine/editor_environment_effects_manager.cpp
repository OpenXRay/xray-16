#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_effects_manager.hpp"
#include "../include/editor/property_holder.hpp"
#include "property_collection.hpp"
#include "editor_environment_effects_effect.hpp"
#include "editor_environment_detail.hpp"

using editor::environment::effects::manager;
using editor::environment::effects::effect;
using editor::environment::detail::logical_string_predicate;

template <>
void property_collection<manager::effect_container_type, manager>::display_name	(u32 const& item_index, LPSTR const& buffer, u32 const& buffer_size)
{
	xr_strcpy				(buffer, buffer_size, m_container[item_index]->id());
}

template <>
editor::property_holder* property_collection<manager::effect_container_type, manager>::create	()
{
	effect*					object = xr_new<effect>(m_holder, generate_unique_id("effect_unique_id_").c_str());
	object->fill			(this);
	return					(object->object());
}

manager::manager			(::editor::environment::manager* environment) :
	m_environment			(*environment),
	m_collection			(0),
	m_changed				(true)
{
	m_collection			= xr_new<collection_type>(&m_effects, this, &m_changed);
}

manager::~manager			()
{
	xr_delete				(m_collection);
	delete_data				(m_effects_ids);
}

void manager::load			()
{
	string_path				file_name;
	CInifile*				config =
		xr_new<CInifile>(
			FS.update_path(
				file_name,
				"$game_config$",
				"environment\\effects.ltx"
			),
			TRUE,
			TRUE,
			FALSE
		);

	VERIFY					(m_effects.empty());

	typedef CInifile::Root	sections_type;
	sections_type&			sections = config->sections();
	m_effects.reserve		(sections.size());
	sections_type::const_iterator	i = sections.begin();
	sections_type::const_iterator	e = sections.end();
	for ( ; i != e; ++i) {
		effect*				object = xr_new<effect>(*this, (*i)->Name);
		object->load		(*config);
		object->fill		(m_collection);
		m_effects.push_back	(object);
	}

	xr_delete				(config);
}

void manager::save			()
{
	string_path				file_name;
	CInifile*				config =
		xr_new<CInifile>(
			FS.update_path(
				file_name,
				"$game_config$",
				"environment\\effects.ltx"
			),
			FALSE,
			FALSE,
			TRUE
		);

	effect_container_type::iterator	i = m_effects.begin();
	effect_container_type::iterator	e = m_effects.end();
	for ( ; i != e; ++i)
		(*i)->save			(*config);

	xr_delete				(config);
}

void manager::fill			(editor::property_holder* holder)
{
	VERIFY					(holder);
	holder->add_property	(
		"effects",
		"ambients",
		"this option is resposible for effects",
		m_collection
	);
}

manager::effects_ids_type const& manager::effects_ids	() const
{
	if (!m_changed)
		return				(m_effects_ids);

	m_changed				= false;

	delete_data				(m_effects_ids);

	m_effects_ids.resize	(m_effects.size());

	effect_container_type::const_iterator	i = m_effects.begin();
	effect_container_type::const_iterator	e = m_effects.end();
	effects_ids_type::iterator				j = m_effects_ids.begin();
	for ( ; i != e; ++i, ++j)
		*j					= xr_strdup((*i)->id());

	std::sort				(m_effects_ids.begin(), m_effects_ids.end(), logical_string_predicate());

	return					(m_effects_ids);
}

shared_str manager::unique_id	(shared_str const& id) const
{
	if (m_collection->unique_id(id.c_str()))
		return				(id);

	return					(m_collection->generate_unique_id(id.c_str()));
}

#endif // #ifdef INGAME_EDITOR