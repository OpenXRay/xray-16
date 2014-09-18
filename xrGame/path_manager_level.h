////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_level.h
//	Created 	: 21.03.2002
//  Modified 	: 03.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Level path manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "level_graph.h"

template <
	typename _DataStorage,
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>	class CPathManager <
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
	> : public CPathManagerGeneric <
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
	typedef SBaseParameters<
		_dist_type,
		_index_type,
		_iteration_type
	> _Parameters;
	typedef typename CPathManagerGeneric <
				_Graph,
				_DataStorage,
				_Parameters,
				_dist_type,
				_index_type,
				_iteration_type
			> inherited;

protected:
	int					x1;
//	float				y1;
	int					z1;
	int					x2;
//	float				y2;
	int					z2;
	int					x3;
//	float				y3;
	int					z3;
//	float				square_size_y;
//	float				size_y;
	float				m_sqr_distance_xz;
	float				m_distance_xz;
	_Graph::CVertex		*best_node;

public:
	virtual				~CPathManager	();
	IC		void		setup			(const _Graph *graph, _DataStorage *_data_storage, xr_vector<_index_type> *_path, const _index_type	&_start_node_index, const _index_type &_goal_node_index, const _Parameters &params);
	IC		void		init			();
	IC		_dist_type	evaluate		(const _index_type &node_index1, const _index_type &node_index2, const _Graph::const_iterator &i);
	IC		_dist_type	estimate		(const _index_type &node_index) const;
	IC		bool		is_goal_reached	(const _index_type &node_index);
	IC		bool		is_limit_reached(const _iteration_type iteration_count) const;
	IC		bool		is_accessible	(const _index_type &vertex_id) const;
	IC		void		begin			(const _index_type &vertex_id, const_iterator &begin, const_iterator &end);
	IC		const _index_type get_value	(const_iterator &i) const;
};

#include "path_manager_level_inline.h"