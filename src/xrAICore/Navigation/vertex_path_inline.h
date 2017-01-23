////////////////////////////////////////////////////////////////////////////
//  Module      : vertex_path_inline.h
//  Created     : 21.03.2002
//  Modified    : 02.03.2004
//  Author      : Dmitriy Iassenev
//  Description : Vertex path class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION\
    template <bool EuclidianHeuristics>\
    template <typename TCompoundVertex>

#define CVertexPathBuilder CVertexPath<EuclidianHeuristics>::CDataStorage<TCompoundVertex>

TEMPLATE_SPECIALIZATION
inline CVertexPathBuilder::CDataStorage(const u32 /*vertex_count*/)
{}

TEMPLATE_SPECIALIZATION
inline CVertexPathBuilder::~CDataStorage()
{}

TEMPLATE_SPECIALIZATION
inline void CVertexPathBuilder::init()
{}

TEMPLATE_SPECIALIZATION
inline void CVertexPathBuilder::assign_parent(Vertex &neighbour, Vertex *parent)
{ neighbour.back() = parent; }

TEMPLATE_SPECIALIZATION
template <typename T>
inline void CVertexPathBuilder::assign_parent(Vertex &neighbour, Vertex *parent, const T&)
{ assign_parent(neighbour, parent); }

TEMPLATE_SPECIALIZATION
inline void CVertexPathBuilder::update_successors(Vertex &/*tpNeighbour*/)
{ NODEFAULT; }

TEMPLATE_SPECIALIZATION
inline void CVertexPathBuilder::get_node_path(xr_vector<Index> &path, Vertex *best)
{
    Vertex *t1 = best, *t2 = best->back();
    u32 i;
    for (i = 1; t2; t1 = t2, t2 = t2->back(), i++);
    path.resize(i);
    t1 = best;
    path[--i] = best->index();
    t2 = t1->back();
    auto it = path.rbegin();
    for (it++; t2; t2 = t2->back(), it++)
        *it = t2->index();
}

#undef TEMPLATE_SPECIALIZATION
#undef CVertexPathBuilder
