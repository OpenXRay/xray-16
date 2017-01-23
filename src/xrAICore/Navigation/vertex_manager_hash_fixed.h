////////////////////////////////////////////////////////////////////////////
//  Module      : vertex_manager_hash_fixed.h
//  Created     : 21.03.2002
//  Modified    : 05.03.2004
//  Author      : Dmitriy Iassenev
//  Description : Hash fixed vertex manager
////////////////////////////////////////////////////////////////////////////

#pragma once

template <
    typename TPathId,
    typename TIndex,
    u32 HashSize,
    u32 FixSize
>
struct CVertexManagerHashFixed
{
    template<typename TCompoundVertex>
    struct VertexData
    {
        using Index = TIndex;

        Index _index;
        bool _opened;

        const Index &index() const { return _index; }
        Index &index() { return _index; }
        bool &opened() { return _opened; }
        bool opened() const { return _opened; }
    };

    template <
        typename TPathBuilder,
        typename TVertexAllocator,
        typename TCompoundVertex
    > 
    class CDataStorage :
        public TPathBuilder::template CDataStorage<TCompoundVertex>,
        public TVertexAllocator::template CDataStorage<TCompoundVertex>
    {
    public:
        using CDataStorageBase = typename TPathBuilder::template CDataStorage<TCompoundVertex>;
        using CDataStorageAllocator = typename TVertexAllocator::template CDataStorage<TCompoundVertex>;
        using Vertex = TCompoundVertex;
        using Index = TIndex;
        using PathId = TPathId;

#pragma pack(push, 1)
        struct IndexVertex
        {
            Vertex *m_vertex;
            IndexVertex *m_next;
            IndexVertex *m_prev;
            u32 m_hash;
            PathId m_path_id;
        };
#pragma pack(pop)

    protected:
        PathId m_current_path_id;
        IndexVertex *m_vertices;
        IndexVertex **m_hash;
        u32 m_vertex_count;

    public:
        inline CDataStorage(const u32 vertex_count);
        inline virtual ~CDataStorage();
        inline void init();
        inline bool is_opened(const Vertex &vertex) const;
        inline bool is_visited(const Index &vertex_id) const;
        inline bool is_closed(const Vertex &vertex) const;
        inline Vertex &get_node(const Index &vertex_id) const;
        inline Vertex &create_vertex(Vertex &vertex, const Index &vertex_id);
        inline void add_opened(Vertex &vertex);
        inline void add_closed(Vertex &vertex);
        inline PathId current_path_id() const;
        inline u32 hash_index(const Index &vertex_id) const;
    };
};

#include "vertex_manager_hash_fixed_inline.h"
