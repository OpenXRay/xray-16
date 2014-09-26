////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_levels_manager.cpp
//	Created 	: 28.12.2007
//  Modified 	: 28.12.2007
//	Author		: Dmitriy Iassenev
//	Description : editor environment levels manager class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR

#include "editor_environment_levels_manager.hpp"
#include "editor_environment_weathers_manager.hpp"
#include "../include/editor/property_holder.hpp"
#include "../include/editor/ide.hpp"
#include "ide.hpp"

using editor::environment::levels::manager;

static LPCSTR s_default_weather_id	= "[default]";
static LPCSTR s_level_section_id	= "levels";

manager::manager					(::editor::environment::weathers::manager* weathers) :
	m_weathers						(*weathers),
	m_property_holder				(0)
{
}

manager::~manager					()
{
	VERIFY							(m_config_single);
	CInifile::Destroy				(m_config_single);
	m_config_single					= 0;

	VERIFY							(m_config_mp);
	CInifile::Destroy				(m_config_mp);
	m_config_mp						= 0;

	if (!Device.editor())
		return;

	::ide().destroy					(m_property_holder);
}

void manager::fill_levels			(CInifile& config, LPCSTR prefix, LPCSTR category)
{
	string_path						section_id;
	xr_strcpy						(section_id, "level_maps_");
	xr_strcat						(section_id, prefix);
	CInifile::Items const&			section = config.r_section(section_id).Data;

	CInifile::Items::const_iterator	i = section.begin();
	CInifile::Items::const_iterator	e = section.end();
	for ( ; i != e; ++i) {
		if (!(*i).first.size())
			continue;

		VERIFY						(config.section_exist((*i).first));
		if (!config.line_exist((*i).first, "weathers")) {
			m_levels.insert			(
				std::make_pair(
					(*i).first.c_str(),
					std::make_pair(
						category,
						s_default_weather_id
					)
				)
			);
			continue;
		}

		LPCSTR						weather_id = config.r_string((*i).first, "weathers");
		m_levels.insert				(
			std::make_pair(
				(*i).first.c_str(),
				std::make_pair(
					category,
					weather_id
				)
			)
		);
	}
}

void manager::load					()
{
	string_path						file_name;

	m_config_single					=
		CInifile::Create(
			FS.update_path(
				file_name,
				"$game_config$",
				"game_maps_single.ltx"
			),
			false
		);

	m_config_mp						=
		CInifile::Create(
			FS.update_path(
				file_name,
				"$game_config$",
				"game_maps_mp.ltx"
			),
			false
		);

	VERIFY							(m_levels.empty());
	fill_levels						(*m_config_single,	"single",	"single");
	fill_levels						(*m_config_mp,		"mp",		"multiplayer");
}

LPCSTR const* manager::collection	()
{
	return							(&*m_weathers.weather_ids().begin());
}

u32 manager::collection_size		()
{
	return							(m_weathers.weather_ids().size());
}

void manager::fill					()
{
	VERIFY							(!m_property_holder);
	m_property_holder				= ::ide().create_property_holder("levels");

	typedef editor::property_holder::string_collection_getter_type	collection_getter_type;
	collection_getter_type			collection_getter;
	collection_getter.bind			(this, &manager::collection);

	typedef editor::property_holder::string_collection_size_getter_type	collection_size_getter_type;
	collection_size_getter_type		collection_size_getter;
	collection_size_getter.bind		(this, &manager::collection_size);

	levels_container_type::iterator	i = m_levels.begin();
	levels_container_type::iterator	e = m_levels.end();
	for ( ; i != e; ++i) {
		string_path					description;
		xr_strcpy					(description, "weather for level ");
		xr_strcat					(description, (*i).first.c_str());
		m_property_holder->add_property	(
			(*i).first.c_str(),
			(*i).second.first,
			description,
			(*i).second.second.c_str(),
			(*i).second.second,
			collection_getter,
			collection_size_getter,
			editor::property_holder::value_editor_combo_box,
			editor::property_holder::cannot_enter_text
		);
	}

	::ide().environment_levels		(m_property_holder);
}

#endif // #ifdef INGAME_EDITOR