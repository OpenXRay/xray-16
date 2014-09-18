////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_game_vertex.h
//	Created 	: 05.07.2006
//  Modified 	: 05.07.2006
//	Author		: Dmitriy Iassenev
//	Description : path manager parameters for game vertex path manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "path_manager_params.h"
#include "game_graph_space.h"

template <
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>
struct SGameVertex : public SBaseParameters<
	_dist_type,
	_index_type,
	_iteration_type
> {
	typedef GameGraph::TERRAIN_VECTOR	VERTEX_TYPES;

	const VERTEX_TYPES	*m_vertex_types;
	_index_type		m_vertex_id;
	xr_vector<_index_type> *m_path;

	IC	SGameVertex (
			const VERTEX_TYPES		&vertex_types,
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
		)
	{
		m_vertex_types	= &vertex_types;
	}

	IC	_index_type	selected_vertex_id() const
	{
		return		(m_vertex_id);
	}
};
