////////////////////////////////////////////////////////////////////////////
//	Module 		: edge_path.h
//	Created 	: 21.03.2002
//  Modified 	: 02.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Edge path class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "vertex_path.h"

template <
	typename	_edge_type,
	bool		bEuclidianHeuristics = true
>
struct CEdgePath {

	template <template <typename _T> class T1>
	struct DataStorageEdgePath {
		template <typename T2>
		struct _vertex : public T1<T2> {
			_edge_type	_edge;

			IC	_edge_type	&edge()
			{
				return	(_edge);
			}
		};
	};

	template <template <typename _T> class _vertex> 
	class CDataStorage : public CVertexPath<bEuclidianHeuristics>::CDataStorage<DataStorageEdgePath<_vertex>::_vertex> {
	public:
		typedef typename CVertexPath<
			bEuclidianHeuristics
			>::CDataStorage<
				DataStorageEdgePath<_vertex>::_vertex
			>											inherited;
		typedef typename inherited::CGraphVertex		CGraphVertex;
		typedef	typename CGraphVertex::_index_type		_index_type;
	
	public:
		IC					CDataStorage		(const u32 vertex_count);
		virtual				~CDataStorage		();
		IC		void		assign_parent		(CGraphVertex &neighbour, CGraphVertex *parent);
		IC		void		assign_parent		(CGraphVertex &neighbour, CGraphVertex *parent, const _edge_type &edge);
		IC		void		get_edge_path		(xr_vector<_edge_type>  &path, CGraphVertex *best, bool reverse_order = false);
	};
};

#include "edge_path_inline.h"