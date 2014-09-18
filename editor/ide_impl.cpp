////////////////////////////////////////////////////////////////////////////
//	Module 		: ide_impl.cpp
//	Created 	: 04.12.2007
//  Modified 	: 04.12.2007
//	Author		: Dmitriy Iassenev
//	Description : IDE implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "ide_impl.hpp"
#include "window_ide.h"
#include "window_view.h"
#include "window_levels.h"
#include "window_weather.h"
#include "window_weather_editor.h"
#include "../include/editor/engine.hpp"
#include "property_holder.hpp"
#include "property_container.hpp"
#include "property_holder_include.hpp"

using editor::window_ide;

ide_impl::ide_impl				(editor::engine* engine) :
	m_engine			(engine),
	m_window			(nullptr),
	m_paused			(false),
	m_in_idle			(false)
{
}

ide_impl::~ide_impl				()
{
}

void ide_impl::window			(window_ide ^window)
{
	m_window			= window;
}

window_ide ^ide_impl::window	()
{
	return				(m_window);
}

void ide_impl::on_idle_start	()
{
	VERIFY				(!m_in_idle);
	m_in_idle			= true;
}

void ide_impl::on_idle			()
{
	VERIFY				(m_in_idle);

	m_window->weather_editor().on_idle	();
	m_window->view().on_idle			();
}

void ide_impl::on_idle_end		()
{
	VERIFY				(m_in_idle);
	m_in_idle			= false;
}

bool ide_impl::idle				() const
{
	return				(m_in_idle);
}

HWND ide_impl::main_handle		()
{
	return				((HWND)m_window->Handle.ToInt32());
}

HWND ide_impl::view_handle		()
{
	return				((HWND)m_window->view().draw_handle().ToInt32());
}

void ide_impl::run				()
{
	Application::Run	(m_window);
}

void ide_impl::on_load_finished	()
{
	m_window->view().on_load_finished			();
	m_window->weather_editor().on_load_finished	();
}

void ide_impl::pause			()
{
	m_window->view().pause		();
}

editor::property_holder* ide_impl::create_property_holder	(
		LPCSTR display_name,
		editor::property_holder_collection* collection,
		editor::property_holder_holder* holder
	)
{
	return				(new ::property_holder(m_engine, display_name, collection, holder));
}

void ide_impl::destroy										(editor::property_holder *&property_holder)
{
	delete				(property_holder);
	property_holder		= 0;
}

void ide_impl::environment_levels							(property_holder *property_holder)
{
	::property_holder*	properties = dynamic_cast<::property_holder*>(property_holder);
	VERIFY				(properties);
	m_window->levels().property_grid()->SelectedObject	= properties->container();
}

void ide_impl::environment_weathers							(property_holder *property_holder)
{
	::property_holder*	properties = dynamic_cast<::property_holder*>(property_holder);
	VERIFY				(properties);
	m_window->weather().property_grid()->SelectedObject	= properties->container();
}

void ide_impl::weather_editor_setup							(
		weathers_getter_type const& weathers_getter,
		weathers_size_getter_type const& weathers_size_getter,
		frames_getter_type const& frames_getter,
		frames_size_getter_type const& frames_size_getter
	)
{
	m_window->weather_editor().weathers_ids	(
		weathers_getter,
		weathers_size_getter,
		frames_getter,
		frames_size_getter
	);
}