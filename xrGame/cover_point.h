////////////////////////////////////////////////////////////////////////////
//	Module 		: cover_point.h
//	Created 	: 24.03.2004
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Cover point class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"

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

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CCoverPoint)
#undef script_type_list
#define script_type_list save_type_list(CCoverPoint)

#include "cover_point_inline.h"