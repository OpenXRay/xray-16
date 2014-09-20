////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_params_straight_line.h
//	Created 	: 21.03.2002
//  Modified 	: 04.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Straight line path manager parameters
////////////////////////////////////////////////////////////////////////////

#pragma once

template <
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>
struct SStraightLineParams : public SBaseParameters<
	_dist_type,
	_index_type,
	_iteration_type
> {
	Fvector			m_start_point;
	Fvector			m_dest_point;
	_dist_type		m_distance;

	IC	SStraightLineParams (
			const Fvector			&start_point,
			const Fvector			&dest_point,
			_dist_type				max_range = _dist_type(6000),
			_iteration_type			max_iteration_count = _iteration_type(-1),
			u32						max_visited_node_count = u32(-1)
		)
		:
		SBaseParameters<
			_dist_type,
			_index_type,
			_iteration_type
		>(
			max_range,
			max_iteration_count,
			max_visited_node_count
		),
		m_start_point(start_point),
		m_dest_point(dest_point)
	{
	}
};

