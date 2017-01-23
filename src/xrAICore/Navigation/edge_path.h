////////////////////////////////////////////////////////////////////////////
//  Module      : edge_path.h
//  Created     : 21.03.2002
//  Modified    : 02.03.2004
//  Author      : Dmitriy Iassenev
//  Description : Edge path class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Navigation/vertex_path.h"

template <typename TEdge, bool EuclidianHeuristics = true>
struct CEdgePath
{
    template <typename TCompoundVertex>
    struct VertexData : CVertexPath<EuclidianHeuristics>::template VertexData<TCompoundVertex>
    {
        TEdge _edge;
        TEdge &edge() { return _edge; }
    };

    template <typename TCompoundVertex>
    class CDataStorage : public CVertexPath<EuclidianHeuristics>::template CDataStorage<TCompoundVertex>
    {
    public:
        using Inherited = typename CVertexPath<EuclidianHeuristics>::template CDataStorage<TCompoundVertex>;
        using Vertex = TCompoundVertex;
        using Index = typename Vertex::Index;
    
    public:
        inline CDataStorage(const u32 vertex_count);
        inline virtual ~CDataStorage();
        inline void assign_parent(Vertex &neighbour, Vertex *parent);
        inline void assign_parent(Vertex &neighbour, Vertex *parent, const TEdge &edge);
        inline void get_edge_path(xr_vector<TEdge> &path, Vertex *best, bool reverse_order = false);
    };
};

#include "xrAICore/Navigation/edge_path_inline.h"
