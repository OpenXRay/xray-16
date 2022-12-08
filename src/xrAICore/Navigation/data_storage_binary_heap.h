////////////////////////////////////////////////////////////////////////////
//  Module      : data_storage_binary_heap.h
//  Created     : 21.03.2002
//  Modified    : 26.02.2004
//  Author      : Dmitriy Iassenev
//  Description : Binary heap data storage
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/xr_types.h"

struct CDataStorageBinaryHeap
{
    template <typename TCompoundVertex>
    struct VertexData
    {
    };

    template <typename TManagerDataStorage>
    class CDataStorage : public TManagerDataStorage
    {
    public:
        using Inherited = TManagerDataStorage;
        using Vertex = typename Inherited::Vertex;
        using Distance = typename Vertex::Distance;
        using Index = typename Vertex::Index;

        struct VertexPredicate
        {
            bool operator()(Vertex* a, Vertex* b) { return a->f() > b->f(); }
        };

    protected:
        Vertex** m_heap;
        Vertex** m_heap_head;
        Vertex** m_heap_tail;

    public:
        inline CDataStorage(const u32 vertex_count);
        inline virtual ~CDataStorage();
        inline void init();
        inline bool is_opened_empty() const;
        inline void add_opened(Vertex& vertex);
        inline void decrease_opened(Vertex& vertex, const Distance value);
        inline void remove_best_opened();
        inline void add_best_closed();
        inline Vertex& get_best() const;
    };
};

#include "xrAICore/Navigation/data_storage_binary_heap_inline.h"
