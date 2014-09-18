////////////////////////////////////////////////////////////////////////////
//	Module 		: visual_memory_params.h
//	Created 	: 09.12.2004
//  Modified 	: 09.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Visual memory parameters
////////////////////////////////////////////////////////////////////////////

#pragma once

struct CVisionParameters {
	float						m_min_view_distance;
	float						m_max_view_distance;
	float						m_visibility_threshold;
	float						m_always_visible_distance;
	float						m_time_quant;
	float						m_decrease_value;
	float						m_velocity_factor;
	float						m_transparency_threshold;
	float						m_luminocity_factor;
	u32							m_still_visible_time;

			void	Load		(LPCSTR section, bool not_a_stalker);
};
