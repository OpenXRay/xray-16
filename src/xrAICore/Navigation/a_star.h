////////////////////////////////////////////////////////////////////////////
//  Module      : a_star.h
//  Created     : 21.03.2002
//  Modified    : 02.03.2004
//  Author      : Dmitriy Iassenev
//  Description : Implementation of the A* (a-star) algorithm
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Navigation/vertex_path.h"
#include "xrAICore/Navigation/data_storage_constructor.h"
#include "xrAICore/Navigation/dijkstra.h"

template <typename TDistance, typename TVertexData>
struct AStarVertexData
{
    template <typename TCompoundVertex>
    struct VertexData : TVertexData::template VertexData<TCompoundVertex>
    {
        using Distance = TDistance;

        Distance _g;
        Distance _h;

        Distance &g() { return _g; }
        Distance &h() { return _h; }
    };
};

template <
    typename TDistance,
    typename TPriorityQueue,
    typename TVertexManager,
    typename TVertexAllocator,
    bool EuclidianHeuristics = true,
    typename TPathBuilder = CVertexPath<EuclidianHeuristics>,
    typename TIteration = u32,
    typename TVertexData = EmptyVertexData
>
class CAStar : public CDijkstra<
    TDistance,
    TPriorityQueue,
    TVertexManager,
    TVertexAllocator,
    EuclidianHeuristics,
    TPathBuilder,
    TIteration,
    AStarVertexData<TDistance, TVertexData>
>
{
protected:
    using Inherited = CDijkstra<
        TDistance,
        TPriorityQueue,
        TVertexManager,
        TVertexAllocator,
        EuclidianHeuristics,
        TPathBuilder,
        TIteration,
        AStarVertexData<TDistance, TVertexData>
    >;

protected:
    template <typename TPathManager>
    inline void initialize(TPathManager &path_manager);
    template <typename TPathManager>
    inline bool step(TPathManager &path_manager);

public:
    inline CAStar(const u32 max_vertex_count);
    inline virtual ~CAStar();
    template <typename TPathManager>
    inline bool find(TPathManager &path_manager);
};

#include "xrAICore/Navigation/a_star_inline.h"
