////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_suns_gradient.cpp
//	Created 	: 26.01.2008
//  Modified 	: 26.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment suns gradient class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_suns_gradient.hpp"
#include "../include/editor/property_holder.hpp"
#include "editor_environment_suns_manager.hpp"
#include "editor_environment_manager.hpp"
#include "editor_environment_detail.hpp"

using editor::environment::suns::gradient;
using editor::property_holder;
using editor::property_holder_collection;
using editor::environment::suns::manager;

gradient::gradient			() :
	m_use					(false),
    m_opacity				(.0f),
    m_radius				(.0f),
    m_shader				(""),
    m_texture				("")
{
}

void gradient::load			(CInifile& config, shared_str const& section)
{
	m_use					= !!READ_IF_EXISTS(&config, r_bool,		section, "gradient",				true);
    m_opacity				=   READ_IF_EXISTS(&config, r_float,	section, "gradient_opacity",		.7f);
    m_radius				=   READ_IF_EXISTS(&config, r_float,	section, "gradient_radius",		.9f);
    m_shader				=   READ_IF_EXISTS(&config, r_string,	section, "gradient_shader",		"effects\\flare");
    m_texture				=   READ_IF_EXISTS(&config, r_string,	section, "gradient_texture",		"fx\\fx_gradient.tga");
}

bool gradient::use_getter	()
{
	return					(m_use);
}

void gradient::use_setter	(bool value)
{
	if (m_use == value)
		return;

	m_use					= value;
//	m_property_holder->clear();
//	fill_internal			();
}

void gradient::fill			(
		manager const& manager,
		property_holder* holder,
		property_holder_collection* collection
	)
{
	editor::property_holder*	properties = holder;
	VERIFY						(properties);

	typedef editor::property_holder::boolean_getter_type	boolean_getter_type;
	boolean_getter_type			boolean_getter;

	typedef editor::property_holder::boolean_setter_type	boolean_setter_type;
	boolean_setter_type			boolean_setter;

	boolean_getter.bind			(this, &gradient::use_getter);
	boolean_setter.bind			(this, &gradient::use_setter);

	properties->add_property(
		"use",
		"gradient",
		"this option is resposible for gradient usage",
		m_use,
		boolean_getter,
		boolean_setter,
		property_holder::property_read_write,
		property_holder::notify_parent_on_change,
		property_holder::no_password_char,
		property_holder::do_not_refresh_grid_on_change
	);
	properties->add_property(
		"opacity",
		"gradient",
		"this option is resposible for gradient opacity",
		m_opacity,
		m_opacity
	);
	properties->add_property(
		"radius",
		"gradient",
		"this option is resposible for gradient radius",
		m_radius,
		m_radius
	);
	properties->add_property(
		"shader",
		"gradient",
		"this option is resposible for gradient shader",
		m_shader.c_str(),
		m_shader,
		&*manager.m_environment.shader_ids().begin(),
		manager.m_environment.shader_ids().size(),
		editor::property_holder::value_editor_tree_view,
		editor::property_holder::cannot_enter_text
	);
	properties->add_property(
		"texture",
		"gradient",
		"this option is resposible for gradient texture",
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

#endif // #ifdef INGAME_EDITOR