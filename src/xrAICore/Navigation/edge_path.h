////////////////////////////////////////////////////////////////////////////
//	Module 		: edge_path.h
//	Created 	: 21.03.2002
//  Modified 	: 02.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Edge path class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Navigation/vertex_path.h"

template<typename _edge_type, bool bEuclidianHeuristics = true>
struct CEdgePath
{
	template<typename TCompoundVertex>
	struct VertexData : CVertexPath<bEuclidianHeuristics>::template VertexData<TCompoundVertex>
    {
        _edge_type _edge;
		_edge_type &edge() { return _edge; }
	};

	template<typename TCompoundVertex>
    class CDataStorage : public CVertexPath<bEuclidianHeuristics>::template CDataStorage<TCompoundVertex>
    {
	public:
		typedef typename CVertexPath<bEuclidianHeuristics>::template CDataStorage<TCompoundVertex> inherited;
		typedef TCompoundVertex CGraphVertex;
		typedef	typename CGraphVertex::_index_type _index_type;
	
	public:
		IC					CDataStorage		(const u32 vertex_count);
		virtual				~CDataStorage		();
		IC		void		assign_parent		(CGraphVertex &neighbour, CGraphVertex *parent);
		IC		void		assign_parent		(CGraphVertex &neighbour, CGraphVertex *parent, const _edge_type &edge);
		IC		void		get_edge_path		(xr_vector<_edge_type>  &path, CGraphVertex *best, bool reverse_order = false);
	};
};

#include "xrAICore/Navigation/edge_path_inline.h"