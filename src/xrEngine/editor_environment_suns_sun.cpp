////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_suns_sun.cpp
//	Created 	: 13.12.2007
//  Modified 	: 04.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment suns sun class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_suns_sun.hpp"
#include "../include/editor/ide.hpp"
#include "editor_environment_manager.hpp"
#include "ide.hpp"
#include "editor_environment_detail.hpp"
#include "editor_environment_suns_manager.hpp"

using editor::environment::suns::sun;
using editor::environment::suns::flare;
using editor::environment::suns::manager;
using editor::property_holder;

sun::sun					(manager const& manager, shared_str const &id) :
	m_manager				(manager),
	m_id					(id),
    m_use					(false),
    m_ignore_color			(false),
    m_radius				(0.f),
    m_shader				(""),
    m_texture				(""),
	m_property_holder		(0)
{
}

sun::~sun					()
{
	if (!Device.editor())
		return;

	::ide().destroy			(m_property_holder);
}

void sun::load				(CInifile& config)
{
    m_use					= !!READ_IF_EXISTS(&config, r_bool,		m_id, "sun",				true);
    m_ignore_color			= !!READ_IF_EXISTS(&config, r_bool,		m_id, "sun_ignore_color",	false);
    m_radius				=   READ_IF_EXISTS(&config, r_float,	m_id, "sun_radius",			.15f);
    m_shader				=   READ_IF_EXISTS(&config, r_string,	m_id, "sun_shader",			"effects\\sun");
    m_texture				=   READ_IF_EXISTS(&config, r_string,	m_id, "sun_texture",		"fx\\fx_sun.tga");
}

void sun::save				(CInifile& config)
{
   config.w_bool			(m_id.c_str(), "sun",				m_use				);
   config.w_bool			(m_id.c_str(), "sun_ignore_color",	m_ignore_color		);
   config.w_float			(m_id.c_str(), "sun_radius",		m_radius			);
   config.w_string			(m_id.c_str(), "sun_shader",		m_shader.c_str()	);
   config.w_string			(m_id.c_str(), "sun_texture",		m_texture.c_str()	);
}

LPCSTR sun::id_getter		() const
{
	return					(m_id.c_str());
}

void sun::id_setter			(LPCSTR value_)
{
	shared_str				value = value_;
	if (m_id._get() == value._get())
		return;

	m_id					= m_manager.unique_id(value);
}

void sun::fill				(editor::property_holder_collection* collection)
{
	VERIFY					(!m_property_holder);
	m_property_holder		= ::ide().create_property_holder(m_id.c_str(), collection, this);
	editor::property_holder*properties = m_property_holder;
	VERIFY					(properties);

	typedef editor::property_holder::string_getter_type	string_getter_type;
	string_getter_type		string_getter;
	string_getter.bind		(this, &sun::id_getter);

	typedef editor::property_holder::string_setter_type	string_setter_type;
	string_setter_type		string_setter;
	string_setter.bind		(this, &sun::id_setter);

	properties->add_property(
		"id",
		"common",
		"this option is resposible for sun identifier",
		m_id.c_str(),
		string_getter,
		string_setter
	);
	properties->add_property(
		"use",
		"sun",
		"this option is resposible for sun usage",
		m_use,
		m_use
	);
	properties->add_property(
		"ignore color",
		"sun",
		"this option is resposible for sun ignore color",
		m_ignore_color,
		m_ignore_color
	);
	properties->add_property(
		"radius",
		"sun",
		"this option is resposible for sun radius",
		m_radius,
		m_radius
	);
	properties->add_property(
		"shader",
		"sun",
		"this option is resposible for sun shader",
		m_shader.c_str(),
		m_shader,
		&*m_manager.m_environment.shader_ids().begin(),
		m_manager.m_environment.shader_ids().size(),
		editor::property_holder::value_editor_tree_view,
		editor::property_holder::cannot_enter_text
	);
	properties->add_property(
		"texture",
		"sun",
		"this option is resposible for sun texture",
		m_texture.c_str(),
		m_texture,
		".dds",
		"Texture files (*.dds)|*.dds",
		detail::real_path("$game_textures$", "").c_str(),
		"Select texture...",
		editor::property_holder::cannot_enter_text,
		editor::property_holder::remove_extension
	);
}

property_holder* sun::object()
{
	return					(m_property_holder);
}

#endif // #ifdef INGAME_EDITOR