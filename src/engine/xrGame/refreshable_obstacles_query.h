////////////////////////////////////////////////////////////////////////////
//	Module 		: refreshable_obstacles_query.h
//	Created 	: 16.05.2007
//  Modified 	: 16.05.2007
//	Author		: Dmitriy Iassenev
//	Description : refreshable obstacles query
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "obstacles_query.h"

class refreshable_obstacles_query : public obstacles_query {
private:
	typedef obstacles_query								inherited;

private:
	static const u32	m_large_update_check_time		= 1000;

private:
	u32					m_last_update_time;

public:
	IC					refreshable_obstacles_query		();
	IC		const float	&refresh_radius					();
};

#include "refreshable_obstacles_query_inline.h"