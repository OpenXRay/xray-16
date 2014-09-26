////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_game_selector.h
//	Created 	: 21.03.2002
//  Modified 	: 19.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Game selector
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "path_manager_params.h"

template <
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>
struct SGameLevel : public SBaseParameters<
	_dist_type,
	_index_type,
	_iteration_type
> {
	u32				m_level_id;
	_index_type		m_vertex_id;
	xr_vector<_index_type> *m_path;

	IC	SGameLevel (
			u32						level_id,
			_dist_type				max_range = _dist_type(6000),
			_iteration_type			max_iteration_count = _iteration_type(-1),
			_index_type				max_visited_node_count = _index_type(-1)
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
		m_level_id(level_id)
	{
	}

	IC	_index_type	selected_vertex_id() const
	{
		return		(m_vertex_id);
	}
};
