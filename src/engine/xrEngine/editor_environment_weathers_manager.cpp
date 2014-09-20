////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_manager.cpp
//	Created 	: 04.01.2008
//  Modified 	: 04.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment manager class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_weathers_manager.hpp"
#include "editor_environment_detail.hpp"
#include "ide.hpp"
#include "../include/editor/property_holder.hpp"
#include "../xrServerEntities/object_broker.h"
#include "editor_environment_weathers_weather.hpp"
#include "editor_environment_weathers_time.hpp"
#include "property_collection.hpp"
#include "editor_environment_manager.hpp"

using editor::environment::weathers::manager;
using editor::environment::weathers::weather;
using editor::environment::detail::logical_string_predicate;

template <>
void property_collection<manager::weather_container_type, manager>::display_name	(u32 const& item_index, LPSTR const& buffer, u32 const& buffer_size)
{
	xr_strcpy						(buffer, buffer_size, m_container[item_index]->id().c_str());
}

template <>
editor::property_holder* property_collection<manager::weather_container_type, manager>::create	()
{
	weather*						object = xr_new<weather>(&m_holder.m_manager, generate_unique_id("weather_unique_id_").c_str());
	object->fill					(this);
	return							(object->object());
}

manager::manager			(editor::environment::manager* manager) :
	m_manager						(*manager),
	m_collection					(0),
	m_changed						(true)
{
	m_collection					= xr_new<collection_type>(&m_weathers, this, &m_changed);
}

manager::~manager			()
{
	xr_delete						(m_collection);
	delete_data						(m_weathers);

	delete_data						(m_weather_ids);
	delete_data						(m_times_ids);
}

void manager::load			()
{
	typedef xr_vector<LPSTR>		file_list_type;
	file_list_type*					file_list = FS.file_list_open("$game_weathers$","");
	VERIFY							(file_list);
	
	file_list_type::const_iterator	i = file_list->begin();
	file_list_type::const_iterator	e = file_list->end();
	for ( ; i != e; ++i) {
		u32							length = xr_strlen(*i);
		if (length <= 4)
			continue;

		if ((*i)[length - 4] != '.')
			continue;

		if ((*i)[length - 3] != 'l')
			continue;

		if ((*i)[length - 2] != 't')
			continue;

		if ((*i)[length - 1] != 'x')
			continue;

		u32							new_length = length - 4;
		LPSTR						identifier = (LPSTR)_alloca((new_length + 1)*sizeof(char));
		Memory.mem_copy				(identifier, *i, new_length*sizeof(char));
		identifier[new_length]		= 0;
		weather*					object = xr_new<weather>(&m_manager, identifier);
		object->load				();
		object->fill				(m_collection);
		m_weathers.push_back		(object);
	}

	FS.file_list_close				(file_list);
}

void manager::save					()
{
	weather_container_type::iterator	i =m_weathers.begin();
	weather_container_type::iterator	e =m_weathers.end();
	for ( ; i != e; ++i)
		(*i)->save					();
}

LPCSTR const* manager::weathers_getter	() const
{
	return							(&*weather_ids().begin());
}

u32 manager::weathers_size_getter		() const
{
	return							(weather_ids().size());
}

struct predicate {
	shared_str		value;

	inline			predicate	(LPCSTR const& value_) :
		value						(value_)
	{
	}

	inline	bool	operator()	(weather const* weather) const
	{
		return						(value._get() == weather->id()._get());
	}
}; // struct predicate

LPCSTR const* manager::frames_getter	(LPCSTR weather_id) const
{
	delete_data						(m_times_ids);

	weather_container_type::const_iterator	found =
		std::find_if(
			m_weathers.begin(),
			m_weathers.end(),
			predicate(weather_id)
		);

	if (found == m_weathers.end())
		return						(0);

	typedef weather::container_type	container_type;
	container_type const&			times = (*found)->times();

	m_times_ids.resize				(times.size());
	times_ids_type::iterator		j = m_times_ids.begin();
	container_type::const_iterator	i = times.begin();
	container_type::const_iterator	e = times.end();
	for ( ; i != e; ++i, ++j)
		*j							= xr_strdup((*i)->id().c_str());

	return							(&*m_times_ids.begin());
}

u32 manager::frames_size_getter			(LPCSTR weather_id) const
{
	weather_container_type::const_iterator	found =
		std::find_if(
			m_weathers.begin(),
			m_weathers.end(),
			predicate(weather_id)
		);

	if (found == m_weathers.end())
		return						(0);

#pragma todo("Dima to Dima: dangerous scheme: it depends on the call sequence (frames_getter should be called berfore frames_size_getter to get correct results)")
	return							(m_times_ids.size());
}

void manager::fill					(property_holder_type* holder)
{
	VERIFY							(holder);
	holder->add_property			(
		"weathers",
		"weathers",
		"this option is resposible for weathers",
		m_collection
	);

	typedef ::editor::ide::weathers_getter_type			weathers_getter_type;
	weathers_getter_type			weathers_getter;
	weathers_getter.bind			(this, &manager::weathers_getter);

	typedef ::editor::ide::weathers_size_getter_type	weathers_size_getter_type;
	weathers_size_getter_type		weathers_size_getter;
	weathers_size_getter.bind		(this, &manager::weathers_size_getter);

	typedef ::editor::ide::frames_getter_type			frames_getter_type;
	frames_getter_type				frames_getter;
	frames_getter.bind				(this, &manager::frames_getter);

	typedef ::editor::ide::frames_size_getter_type		frames_size_getter_type;
	frames_size_getter_type			frames_size_getter;
	frames_size_getter.bind			(this, &manager::frames_size_getter);

	::ide().weather_editor_setup	(
		weathers_getter,
		weathers_size_getter,
		frames_getter,
		frames_size_getter
	);
}

manager::weather_ids_type const& manager::weather_ids	() const
{
	if (!m_changed)
		return						(m_weather_ids);

	m_changed						= false;

	delete_data						(m_weather_ids);

	m_weather_ids.resize			(m_weathers.size());

	weather_container_type::const_iterator	i = m_weathers.begin();
	weather_container_type::const_iterator	e = m_weathers.end();
	weather_ids_type::iterator		j = m_weather_ids.begin();
	for ( ; i != e; ++i, ++j)
		*j							= xr_strdup((*i)->id().c_str());

	std::sort						(m_weather_ids.begin(), m_weather_ids.end(), logical_string_predicate());

	return							(m_weather_ids);
}

shared_str manager::unique_id			(shared_str const& id) const
{
	if (m_collection->unique_id(id.c_str()))
		return						(id);

	return							(m_collection->generate_unique_id(id.c_str()));
}

bool manager::save_current_blend		(char* buffer, u32 const& buffer_size)
{
	CInifile		temp(0, FALSE, FALSE, FALSE);

	using editor::environment::weathers::time;
	time*			frame = static_cast<time*>(m_manager.CurrentEnv);
	frame->save		(temp);

	CMemoryWriter	writer;
	temp.save_as	(writer);
	if (writer.size() > buffer_size)
		return		(false);

	writer.w_u8		(0);
	writer.seek		(0);
	Memory.mem_copy	(buffer, writer.pointer(), writer.size());
	return			(true);
}

bool manager::paste_current_time_frame	(char const* buffer, u32 const& buffer_size)
{
	if (!m_manager.Current[0])
		return							(false);

	weather_container_type::iterator	i = m_weathers.begin();
	weather_container_type::iterator	e = m_weathers.end();
	for ( ; i != e; ++i) {
		if (m_manager.CurrentWeatherName._get() != (*i)->id()._get())
			continue;

		return						((*i)->paste_time_frame(m_manager.Current[0]->m_identifier, buffer, buffer_size));
	}

	return							(false);
}

bool manager::paste_target_time_frame	(char const* buffer, u32 const& buffer_size)
{
	if (!m_manager.Current[1])
		return							(false);

	weather_container_type::iterator	i = m_weathers.begin();
	weather_container_type::iterator	e = m_weathers.end();
	for ( ; i != e; ++i) {
		if (m_manager.CurrentWeatherName._get() != (*i)->id()._get())
			continue;

		return						((*i)->paste_time_frame(m_manager.Current[1]->m_identifier, buffer, buffer_size));
	}

	return							(false);
}

bool manager::add_time_frame			(char const* buffer, u32 const& buffer_size)
{
	weather_container_type::iterator	i = m_weathers.begin();
	weather_container_type::iterator	e = m_weathers.end();
	for ( ; i != e; ++i) {
		if (m_manager.CurrentWeatherName._get() != (*i)->id()._get())
			continue;

		return							((*i)->add_time_frame(buffer, buffer_size));
	}

	return								(false);
}

void manager::reload_current_time_frame	()
{
	if (!m_manager.Current[0])
		return;

	weather_container_type::iterator	i = m_weathers.begin();
	weather_container_type::iterator	e = m_weathers.end();
	for ( ; i != e; ++i) {
		if (m_manager.CurrentWeatherName._get() != (*i)->id()._get())
			continue;

		(*i)->reload_time_frame			(m_manager.Current[0]->m_identifier);
		return;
	}
}

void manager::reload_target_time_frame	()
{
	if (!m_manager.Current[1])
		return;

	weather_container_type::iterator	i = m_weathers.begin();
	weather_container_type::iterator	e = m_weathers.end();
	for ( ; i != e; ++i) {
		if (m_manager.CurrentWeatherName._get() != (*i)->id()._get())
			continue;

		(*i)->reload_time_frame			(m_manager.Current[1]->m_identifier);
		return;
	}
}

void manager::reload_current_weather	()
{
	weather_container_type::iterator	i = m_weathers.begin();
	weather_container_type::iterator	e = m_weathers.end();
	for ( ; i != e; ++i) {
		if (m_manager.CurrentWeatherName._get() != (*i)->id()._get())
			continue;

		(*i)->reload					();
		return;
	}
}

void manager::reload					()
{
	delete_data							(m_weathers);
	load								();
}

#endif // #ifdef INGAME_EDITOR