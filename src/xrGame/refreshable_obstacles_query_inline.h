////////////////////////////////////////////////////////////////////////////
//	Module 		: refreshable_obstacles_query_inline .h
//	Created 	: 16.05.2007
//  Modified 	: 16.05.2007
//	Author		: Dmitriy Iassenev
//	Description : refreshable obstacles query inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	refreshable_obstacles_query::refreshable_obstacles_query()
{
	m_last_update_time	= 0;
}

IC	const float &refreshable_obstacles_query::refresh_radius()
{
	static const float	m_update_objects_radius			= 2.f;
	static const float	m_large_update_objects_radius	= 100.f;

	if (Device.dwTimeGlobal < (m_update_objects_radius + m_large_update_check_time))
		return			(m_update_objects_radius);

	m_last_update_time	= Device.dwTimeGlobal;
	return				(m_large_update_objects_radius);
}