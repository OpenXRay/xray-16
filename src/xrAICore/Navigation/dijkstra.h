////////////////////////////////////////////////////////////////////////////
//  Module      : dijkstra.h
//  Created     : 21.03.2002
//  Modified    : 02.03.2004
//  Author      : Dmitriy Iassenev
//  Description : Implementation of the Dijkstra algorithm
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Navigation/vertex_path.h"
#include "xrAICore/Navigation/data_storage_constructor.h"

template <typename TDistance, typename TVertexData>
struct DijkstraVertexData
{
    template <typename TCompoundVertex>
    struct VertexData : TVertexData::template VertexData<TCompoundVertex>
    {
        using Distance = TDistance;
        
        Distance _f;
        TCompoundVertex *_back;
        
        Distance &f() { return _f; }
        const Distance &f() const { return _f; }
        TCompoundVertex *&back() { return _back; }
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
class CDijkstra
{
public:
    using Vertex = CompoundVertex<DijkstraVertexData<TDistance, TVertexData>,
        TPriorityQueue, TVertexManager, TVertexAllocator, TPathBuilder>;
    using CDataStorage = PriorityQueueConstructor<
        TPriorityQueue, TVertexManager, TPathBuilder, TVertexAllocator, Vertex>;

protected:
    using Distance = typename Vertex::Distance;
    using Index = typename Vertex::Index;

protected:
    bool m_search_started;
    CDataStorage *m_data_storage;

protected:
    inline CDijkstra(const u32 max_vertex_count);
    inline virtual ~CDijkstra();
    template <typename TPathManager>
    inline void initialize(TPathManager &path_manager);
    template <typename TPathManager>
    inline bool step(TPathManager &path_manager);
    template <typename TPathManager>
    inline void finalize(TPathManager &path_manager);

public:
    template <typename TPathManager>
    inline bool find(TPathManager &path_manager);
    inline CDataStorage &data_storage();
    inline const CDataStorage &data_storage() const;
};

#include "xrAICore/Navigation/dijkstra_inline.h"
