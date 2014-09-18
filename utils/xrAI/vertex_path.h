////////////////////////////////////////////////////////////////////////////
//	Module 		: vertex_path.h
//	Created 	: 21.03.2002
//  Modified 	: 02.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Vertex path class
////////////////////////////////////////////////////////////////////////////

#pragma once

template <bool	 bEuclidianHeuristics = true>
struct CVertexPath {

#pragma pack(push,1)
	template <template <typename _T> class T1>
	struct DataStoragePath {
		struct _vertex : public T1<_vertex> {
		};
	};
#pragma pack(pop)

	template <template <typename _T> class _vertex> 
	class CDataStorage {
	public:
		typedef typename DataStoragePath<_vertex>::_vertex	CGraphVertex;
		typedef	typename CGraphVertex::_index_type			_index_type;
	
	public:
		IC					CDataStorage		(const u32 vertex_count);
		virtual				~CDataStorage		();
		IC		void		init				();
		IC		void		assign_parent		(CGraphVertex &neighbour, CGraphVertex *parent);
		template <typename T>
		IC		void		assign_parent		(CGraphVertex &neighbour, CGraphVertex *parent, const T&);
		IC		void		update_successors	(CGraphVertex &neighbour);
		IC		void		get_node_path		(xr_vector<_index_type> &path, CGraphVertex *best);
	};
};

#include "vertex_path_inline.h"