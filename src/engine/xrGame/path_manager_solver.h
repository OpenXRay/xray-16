////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_solver.h
//	Created 	: 21.03.2002
//  Modified 	: 03.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Solver path manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "problem_solver.h"

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	bool	 T6,
	typename T7,
	typename T8,
	typename _DataStorage,
	typename _Parameters,
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>	class CPathManager <
		CProblemSolver<T1,T2,T3,T4,T5,T6,T7,T8>,
		_DataStorage,
		_Parameters,
		_dist_type,
		_index_type,
		_iteration_type
	> : public CPathManagerGeneric <
			CProblemSolver<T1,T2,T3,T4,T5,T6,T7,T8>,
			_DataStorage,
			_Parameters,
			_dist_type,
			_index_type,
			_iteration_type
		>
{
protected:
	typedef CProblemSolver<T1,T2,T3,T4,T5,T6,T7,T8>	_Graph;
	typedef typename _Graph::_edge_type				_edge_type;

protected:
	xr_vector<_edge_type>							*m_edge_path;
	const_iterator									m_iterator;

public:
	virtual						~CPathManager	();
	IC		void				setup			(const _Graph *graph, _DataStorage *_data_storage, xr_vector<_edge_type> *_path, const _index_type	&_start_node_index, const _index_type &_goal_node_index, const _Parameters &params);
	IC		bool				is_goal_reached	(const _index_type &vertex_id) const;
	IC		const _index_type	&get_value		(const_iterator &i, bool reverse_search = typename _Graph::reverse_search) const;
	IC		const _edge_type	&edge			(const_iterator &i) const;
	IC		_dist_type			evaluate		(const _index_type &node_index1, const _index_type &node_index2, const const_iterator &i) const;
	IC		_dist_type			estimate		(const _index_type &vertex_id) const;
	IC		void				init_path		();
	template <typename T>
	IC		void				create_path		(T &vertex, _DataStorage &data_storage, bool reverse_order);
	template <typename T>
	IC		void				create_path		(T &vertex);
};

#include "path_manager_solver_inline.h"