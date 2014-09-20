////////////////////////////////////////////////////////////////////////////
//	Module 		: visual_memory_params.cpp
//	Created 	: 09.12.2004
//  Modified 	: 09.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Visual memory parameters
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "visual_memory_params.h"
#include "memory_space.h"

void CVisionParameters::Load	(LPCSTR section, bool not_a_stalker)
{
	m_transparency_threshold	= pSettings->r_float(section,"transparency_threshold");
	m_still_visible_time		= READ_IF_EXISTS(pSettings,r_u32,section,"still_visible_time",0);

#ifndef USE_STALKER_VISION_FOR_MONSTERS
	if (!not_a_stalker)
		return;
#endif
	m_min_view_distance			= pSettings->r_float(section,"min_view_distance");
	m_max_view_distance			= pSettings->r_float(section,"max_view_distance");
	m_visibility_threshold		= pSettings->r_float(section,"visibility_threshold");
	m_always_visible_distance	= pSettings->r_float(section,"always_visible_distance");
	m_time_quant				= pSettings->r_float(section,"time_quant");
	m_decrease_value			= pSettings->r_float(section,"decrease_value");
	m_velocity_factor			= pSettings->r_float(section,"velocity_factor");
	m_luminocity_factor			= pSettings->r_float(section,"luminocity_factor");
}
