////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_suns_blend.cpp
//	Created 	: 26.01.2008
//  Modified 	: 26.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment suns blend class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_suns_blend.hpp"
#include "../include/editor/property_holder.hpp"

using editor::environment::suns::blend;
using editor::environment::suns::manager;

blend::blend		() :
    m_down_time	(0.f),
    m_rise_time	(0.f),
    m_time		(0.f)
{
}

void blend::load	(CInifile& config, shared_str const& section)
{
    m_down_time	=   READ_IF_EXISTS(&config, r_float,	section, "blend_down_time",	60.f);
    m_rise_time	=   READ_IF_EXISTS(&config, r_float,	section, "blend_rise_time",	60.f);
    m_time		=   READ_IF_EXISTS(&config, r_float,	section, "blend_time",		.1f);
}

void blend::fill	(manager const& manager, editor::property_holder* holder, editor::property_holder_collection* collection)
{
	editor::property_holder*	properties = holder;
	VERIFY						(properties);

	properties->add_property	(
		"down time",
		"blend",
		"this option is resposible for the blend down time",
		m_down_time,
		m_down_time
	);
	properties->add_property	(
		"rise time",
		"blend",
		"this option is resposible for the blend rise time",
		m_rise_time,
		m_rise_time
	);
	properties->add_property	(
		"time",
		"blend",
		"this option is resposible for the blend time",
		m_time,
		m_time
	);
}

#endif // #ifdef INGAME_EDITOR