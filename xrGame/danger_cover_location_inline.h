////////////////////////////////////////////////////////////////////////////
//	Module 		: danger_cover_location_inline.h
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Danger cover location inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CDangerCoverLocation::CDangerCoverLocation	(const CCoverPoint *cover, u32 level_time, u32 interval, float radius, const squad_mask_type &mask)
{
	VERIFY			(cover);
	m_cover			= cover;
	m_level_time	= level_time;
	m_interval		= interval;
	m_radius		= radius;
	m_mask.assign	(mask);
}
