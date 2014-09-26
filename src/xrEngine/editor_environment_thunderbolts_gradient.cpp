////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_thunderbolts_gradient.cpp
//	Created 	: 04.01.2008
//  Modified 	: 10.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment thunderbolts gradient class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_thunderbolts_gradient.hpp"
#include "ide.hpp"
#include "editor_environment_manager.hpp"
#include "editor_environment_detail.hpp"

using editor::environment::thunderbolts::gradient;

gradient::gradient	() :
	m_property_holder	(0)
{
}

gradient::~gradient	()
{
	if (!Device.editor())
		return;

	::ide().destroy		(m_property_holder);
}

void gradient::load	(CInifile& config, shared_str const& section_id, LPCSTR prefix)
{
	string_path			temp;
	shader				= config.r_string	(section_id, strconcat(sizeof(temp), temp, prefix, "_shader"));
	texture				= config.r_string	(section_id, strconcat(sizeof(temp), temp, prefix, "_texture"));
	fOpacity			= config.r_float	(section_id, strconcat(sizeof(temp), temp, prefix, "_opacity"));
	fRadius				= config.r_fvector2	(section_id, strconcat(sizeof(temp), temp, prefix, "_radius"));
	m_pFlare->CreateShader	(*shader,*texture);
}

void gradient::save	(CInifile& config, shared_str const& section_id, LPCSTR prefix)
{
	string_path			temp;
	config.w_string		(section_id.c_str(), strconcat(sizeof(temp), temp, prefix, "_shader" ),	shader.c_str()	);
	config.w_string		(section_id.c_str(), strconcat(sizeof(temp), temp, prefix, "_texture"),	texture.c_str()	);
	config.w_float		(section_id.c_str(), strconcat(sizeof(temp), temp, prefix, "_opacity"),	fOpacity		);
	config.w_fvector2	(section_id.c_str(), strconcat(sizeof(temp), temp, prefix, "_radius" ),	fRadius			);
}

LPCSTR gradient::shader_getter	() const
{
	return				(shader.c_str());
}

void gradient::shader_setter	(LPCSTR	value)
{
	shader					= value;
	m_pFlare->CreateShader	(*shader,*texture);
}

LPCSTR gradient::texture_getter	() const
{
	return				(texture.c_str());
}

void gradient::texture_setter	(LPCSTR	value)
{
	texture				= value;
	m_pFlare->CreateShader	(*shader,*texture);
}

void gradient::fill	(
		::editor::environment::manager& environment,
		LPCSTR name,
		LPCSTR description,
		::editor::property_holder& holder
	)
{
	VERIFY							(!m_property_holder);
	m_property_holder				= ::ide().create_property_holder(name);

	holder.add_property				(
		name,
		"gradient",
		description,
		m_property_holder
	);

	m_property_holder->add_property	(
		"opacity",
		"properties",
		"this option is resposible for thunderbolt gradient opacity",
		fOpacity,
		fOpacity
	);
	m_property_holder->add_property	(
		"minimum radius",
		"properties",
		"this option is resposible for thunderbolt gradient minimum radius",
		fRadius.x,
		fRadius.x
	);
	m_property_holder->add_property	(
		"maximum _radius",
		"properties",
		"this option is resposible for thunderbolt gradient maximum radius",
		fRadius.y,
		fRadius.y
	);

	typedef ::editor::property_holder::string_getter_type	string_getter_type;
	string_getter_type	string_getter;
	string_getter.bind	(this, &gradient::shader_getter);

	typedef ::editor::property_holder::string_setter_type	string_setter_type;
	string_setter_type	string_setter;
	string_setter.bind	(this, &gradient::shader_setter);

	m_property_holder->add_property	(
		"shader",
		"properties",
		"this option is resposible for thunderbolt gradient shader",
		shader.c_str(),
		string_getter,
		string_setter,
		&*environment.shader_ids().begin(),
		environment.shader_ids().size(),
		editor::property_holder::value_editor_tree_view,
		editor::property_holder::cannot_enter_text
	);

	string_getter.bind	(this, &gradient::texture_getter);
	string_setter.bind	(this, &gradient::texture_setter);
	m_property_holder->add_property	(
		"texture",
		"",
		"this option is resposible for thunderbolt gradient texture",
		texture.c_str(),
		texture,
		".dds",
		"Texture files (*.dds)|*.dds",
		detail::real_path("$game_textures$", "").c_str(),
		"Select texture...",
		editor::property_holder::cannot_enter_text,
		editor::property_holder::remove_extension
	);
}
#endif // #ifdef INGAME_EDITOR
