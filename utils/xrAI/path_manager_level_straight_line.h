////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_level_straight_line.h
//	Created 	: 21.03.2002
//  Modified 	: 03.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Level straight line path manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "path_manager_level.h"

template <
	typename _DataStorage,
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>	class CPathManager <
		CLevelGraph,
		_DataStorage,
		SStraightLineParams<
			_dist_type,
			_index_type,
			_iteration_type
		>,
		_dist_type,
		_index_type,
		_iteration_type
	> : public CPathManager <
			CLevelGraph,
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
	typedef CLevelGraph _Graph;
	typedef SStraightLineParams<
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
	_Parameters			*m_parameters;

public:
	virtual				~CPathManager	();
	IC		void		setup			(const _Graph *graph, _DataStorage *_data_storage, xr_vector<_index_type> *_path, const _index_type	&_start_node_index, const _index_type &_goal_node_index, _Parameters &params);
	template <typename T>
	IC		void		create_path		(T &vertex);
};

#include "path_manager_level_straight_line_inline.h"