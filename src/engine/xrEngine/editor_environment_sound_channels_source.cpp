////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_sound_channels_source.cpp
//	Created 	: 04.01.2008
//  Modified 	: 04.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment sound channels source class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_sound_channels_source.hpp"
#include "ide.hpp"
#include "editor_environment_detail.hpp"

using editor::environment::sound_channels::source;

source::source				(shared_str const& source) :
	m_source			(source),
	m_property_holder	(0)
{
}

source::~source				()
{
	if (!Device.editor())
		return;

	::ide().destroy		(m_property_holder);
}

void source::fill			(editor::property_holder_collection* collection)
{
	VERIFY				(!m_property_holder);
	m_property_holder	= ::ide().create_property_holder(m_source.c_str(), collection, this);

	m_property_holder->add_property(
		"sound",
		"properties",
		"this option is resposible for sound",
		m_source.c_str(),
		m_source,
		".ogg",
		"Sound files (*.ogg)|*.ogg",
		detail::real_path("$game_sounds$", "").c_str(),
		"Select sound...",
		editor::property_holder::cannot_enter_text,
		editor::property_holder::remove_extension
	);
}

source::property_holder_type* source::object()
{
	return				(m_property_holder);
}

#endif // #ifdef INGAME_EDITOR