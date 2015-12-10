////////////////////////////////////////////////////////////////////////////
//	Module 		: a_star.h
//	Created 	: 21.03.2002
//  Modified 	: 02.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Implementation of the A* (a-star) algorithm
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Navigation/vertex_path.h"
#include "xrAICore/Navigation/data_storage_constructor.h"
#include "xrAICore/Navigation/dijkstra.h"

template<typename _dist_type, typename TVertexData>
struct AStarVertexData
{
	template<typename TCompoundVertex>
	struct VertexData : TVertexData::template VertexData<TCompoundVertex>
    {
        typedef _dist_type _dist_type;

		_dist_type _g;
		_dist_type _h;

		_dist_type &g() { return _g; }
		_dist_type &h() { return _h; }
	};
};

template <
	typename _dist_type,
	typename _priority_queue, 
	typename _vertex_manager, 
	typename _vertex_allocator,
	bool	 euclidian_heuristics = true,
	typename _data_storage_base = CVertexPath<euclidian_heuristics>,
	typename _iteration_type = u32,
    typename TVertexData = EmptyVertexData
> class CAStar : public CDijkstra<
		_dist_type,
		_priority_queue,
		_vertex_manager,
		_vertex_allocator,
		euclidian_heuristics,
		_data_storage_base,
		_iteration_type,
        AStarVertexData<_dist_type, TVertexData>
	>
{
protected:
	typedef CDijkstra <
		_dist_type,
		_priority_queue,
		_vertex_manager,
		_vertex_allocator,
		euclidian_heuristics,
		_data_storage_base,
		_iteration_type,
        AStarVertexData<_dist_type, TVertexData>
	> inherited;
	typedef typename inherited::CGraphVertex CGraphVertex;
	typedef typename CGraphVertex::_dist_type _dist_type;
	typedef typename CGraphVertex::_index_type _index_type;

protected:
	template <typename _PathManager>
	IC		void				initialize		(_PathManager &path_manager);
	template <typename _PathManager>
	IC		bool				step			(_PathManager &path_manager);

public:
	IC							CAStar			(const u32 max_vertex_count);
	virtual						~CAStar			();
	template <typename _PathManager>
	IC		bool				find			(_PathManager &path_manager);
};

#include "xrAICore/Navigation/a_star_inline.h"