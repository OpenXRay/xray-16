////////////////////////////////////////////////////////////////////////////
//  Module      : vertex_manager_fixed.h
//  Created     : 21.03.2002
//  Modified    : 01.03.2004
//  Author      : Dmitriy Iassenev
//  Description : Fixed vertex manager
////////////////////////////////////////////////////////////////////////////

#pragma once

template <
    typename TPathId,
    typename TIndex,
    u8 Mask
>
struct CVertexManagerFixed
{
    template<typename TCompoundVertex>
    struct VertexData
    {
        using Index = TIndex;
        Index _index : 8*sizeof(Index)-Mask;
        Index _opened : Mask;

        Index index() const { return _index; }
        Index opened() const { return _opened; }
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
            PathId m_path_id;
            Vertex *m_vertex;
        };
#pragma pack(pop)

    protected:
        PathId m_current_path_id;
        u32 m_max_node_count;
        IndexVertex *m_indexes;

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
    };
};

#include "vertex_manager_fixed_inline.h"
