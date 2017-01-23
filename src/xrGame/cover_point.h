////////////////////////////////////////////////////////////////////////////
//	Module 		: cover_point.h
//	Created 	: 24.03.2004
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Cover point class
////////////////////////////////////////////////////////////////////////////

#pragma once

class CCoverPoint {
public:
	Fvector				m_position;
	u32					m_level_vertex_id : 31;
	u32					m_is_smart_cover  :  1;

public:
	IC					CCoverPoint		(Fvector const &point, u32 const &level_vertex_id);
	IC	Fvector	const	&position		() const;
	IC	u32				level_vertex_id	() const;
	IC	bool			operator==		(CCoverPoint const &point) const;
};

#include "cover_point_inline.h"
