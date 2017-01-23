////////////////////////////////////////////////////////////////////////////
//  Module      : edge_path_inline.h
//  Created     : 21.03.2002
//  Modified    : 02.03.2004
//  Author      : Dmitriy Iassenev
//  Description : Edge path class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION\
    template <typename TEdge, bool EuclidianHeuristics>\
    template <typename TCompoundVertex>

#define CEdgePathBuilder CEdgePath<TEdge, EuclidianHeuristics>::CDataStorage<TCompoundVertex>

TEMPLATE_SPECIALIZATION
inline CEdgePathBuilder::CDataStorage(const u32 vertex_count) :
    Inherited(vertex_count)
{}

TEMPLATE_SPECIALIZATION
CEdgePathBuilder::~CDataStorage()
{}

TEMPLATE_SPECIALIZATION
inline void CEdgePathBuilder::assign_parent(Vertex    &neighbour, Vertex *parent)
{ Inherited::assign_parent(neighbour, parent); }

TEMPLATE_SPECIALIZATION
inline void CEdgePathBuilder::assign_parent(Vertex    &neighbour, Vertex *parent, const TEdge &edge)
{
    Inherited::assign_parent(neighbour, parent);
    neighbour.edge() = edge;
}

TEMPLATE_SPECIALIZATION
inline void CEdgePathBuilder::get_edge_path(xr_vector<TEdge> &path, Vertex *best, bool reverse_order)
{
    Vertex *t1 = best, *t2 = best->back();
    u32 i;
    for (i=1; t2; t1 = t2, t2 = t2->back(), i++);
    u32 n = (u32)path.size();
    i--;
    path.resize(n+i);
    t2 = best;
    if (!reverse_order)
    {
        auto it = path.rbegin();
        for (; t2->back() ; t2 = t2->back(), it++)
            *it = t2->edge();
    }
    else
    {
        auto it = path.begin()+n;
        for (; t2->back(); t2 = t2->back(), it++)
            *it = t2->edge();
    }
}

#undef TEMPLATE_SPECIALIZATION
#undef CEdgePathBuilder
