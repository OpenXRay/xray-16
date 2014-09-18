////////////////////////////////////////////////////////////////////////////
//	Module 		: cover_point_inline.h
//	Created 	: 24.03.2004
//  Modified 	: 24.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Cover point class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CCoverPoint::CCoverPoint				(const Fvector &point, u32 level_vertex_id) :
	m_position			(point),
	m_level_vertex_id	(level_vertex_id)
{
}

IC	const Fvector &CCoverPoint::position	() const
{
	return				(m_position);
}

IC	u32	CCoverPoint::level_vertex_id		() const
{
	return				(m_level_vertex_id);
}

IC	bool CCoverPoint::operator==			(const CCoverPoint &point) const
{
	return				(!!position().similar(point.position()));
}
