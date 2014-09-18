////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_params_flooder.h
//	Created 	: 21.03.2002
//  Modified 	: 04.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Flooder path manager parameters
////////////////////////////////////////////////////////////////////////////

#pragma once

template <
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>
struct SFlooder  : public SBaseParameters<
	_dist_type,
	_index_type,
	_iteration_type
> {
	u32	m_dummy;
	IC	SFlooder (
			_dist_type				max_range = _dist_type(6000),
			_iteration_type			max_iteration_count = _iteration_type(-1),
			u32						max_visited_node_count = 65530
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
		)
	{
	}
};