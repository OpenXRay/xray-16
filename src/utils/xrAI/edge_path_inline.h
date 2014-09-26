////////////////////////////////////////////////////////////////////////////
//	Module 		: edge_path_inline.h
//	Created 	: 21.03.2002
//  Modified 	: 02.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Edge path class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template<typename _edge_type, bool bEuclidianHeuristics>\
	template <template <typename _T> class _vertex> 


#define CEdgePathBuilder		CEdgePath<_edge_type,bEuclidianHeuristics>::CDataStorage<_vertex>

TEMPLATE_SPECIALIZATION
IC	CEdgePathBuilder::CDataStorage			(const u32 vertex_count) :
	inherited					(vertex_count)
{
}

TEMPLATE_SPECIALIZATION
CEdgePathBuilder::~CDataStorage				()
{
}

TEMPLATE_SPECIALIZATION
IC	void CEdgePathBuilder::assign_parent	(CGraphVertex	&neighbour, CGraphVertex *parent)
{
	inherited::assign_parent	(neighbour,parent);
}

TEMPLATE_SPECIALIZATION
IC	void CEdgePathBuilder::assign_parent	(CGraphVertex	&neighbour, CGraphVertex *parent, const _edge_type &edge)
{
	inherited::assign_parent	(neighbour,parent);
	neighbour.edge()			= edge;
}

TEMPLATE_SPECIALIZATION
IC	void CEdgePathBuilder::get_edge_path	(xr_vector<_edge_type> &path, CGraphVertex *best, bool reverse_order)
{
	CGraphVertex			*t1 = best, *t2 = best->back();
	for (u32 i=1; t2; t1 = t2, t2 = t2->back(), ++i) ;
	u32						n = (u32)path.size(); 

	path.resize				(n + --i);
	t2						= best;

	if (!reverse_order) {
		xr_vector<_edge_type>::reverse_iterator	I = path.rbegin();
		xr_vector<_edge_type>::reverse_iterator	E = path.rend();
		for (; t2->back() ; t2 = t2->back(), ++I)
			*I = t2->edge();
	}
	else {
		xr_vector<_edge_type>::iterator	I = path.begin() + n;
		xr_vector<_edge_type>::iterator	E = path.end();
		for (; t2->back() ; t2 = t2->back(), ++I)
			*I = t2->edge();
	}
}

#undef TEMPLATE_SPECIALIZATION
#undef CEdgePathBuilder