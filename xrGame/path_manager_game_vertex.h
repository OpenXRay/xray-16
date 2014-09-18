////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_game_vertex.h
//	Created 	: 05.07.2006
//  Modified 	: 05.07.2006
//	Author		: Dmitriy Iassenev
//	Description : Game vertex path manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "game_graph.h"
#include "path_manager_params_game_vertex.h"

template <
	typename _DataStorage,
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>	class CPathManager <
		CGameGraph,
		_DataStorage,
		SGameVertex<
			_dist_type,
			_index_type,
			_iteration_type
		>,
		_dist_type,
		_index_type,
		_iteration_type
	> : public CPathManager <
			CGameGraph,
			_DataStorage,
			SBaseParameters<
				_dist_type,
				_index_type,
				_iteration_type
			>,
			_dist_type,
			_index_type,
			_iteration_type
		>
{
protected:
	typedef CGameGraph _Graph;
	typedef SGameVertex<
		_dist_type,
		_index_type,
		_iteration_type
	> _Parameters;
	typedef typename CPathManager <
				_Graph,
				_DataStorage,
				SBaseParameters<
					_dist_type,
					_index_type,
					_iteration_type
				>,
				_dist_type,
				_index_type,
				_iteration_type
			> inherited;

protected:
	_Parameters		*m_evaluator;
	bool			m_start_is_accessible;

public:
	IC		void		setup			(const _Graph *graph, _DataStorage *_data_storage, xr_vector<_index_type> *_path, const _index_type	&_start_node_index, const _index_type &_goal_node_index, _Parameters &params);
	IC		bool		is_accessible	(const _index_type &vertex_id) const;
};

#include "path_manager_game_vertex_inline.h"