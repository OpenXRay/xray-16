////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_level_evaluator.h
//	Created 	: 21.03.2002
//  Modified 	: 03.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Level evaluator path manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "path_manager_level.h"
#include "path_manager_params_level_evaluator.h"

template <
	typename _DataStorage,
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>	class CPathManager <
		CLevelGraph,
		_DataStorage,
		CAbstractVertexEvaluator,
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
	typedef CAbstractVertexEvaluator _Parameters;
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

public:
	virtual				~CPathManager	();
	IC		void		setup			(const _Graph *graph, _DataStorage *_data_storage, xr_vector<_index_type> *_path, const _index_type	&_start_node_index, const _index_type &_goal_node_index, _Parameters &params);
	IC		_dist_type	estimate		(const _index_type &node_index) const;
	IC		bool		is_goal_reached	(const _index_type &node_index);
	IC		void		finalize		();
};

#include "path_manager_level_evaluator_inline.h"