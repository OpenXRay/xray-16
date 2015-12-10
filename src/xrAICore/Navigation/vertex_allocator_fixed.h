////////////////////////////////////////////////////////////////////////////
//  Module      : vertex_allocator_fixed.h
//  Created     : 21.03.2002
//  Modified    : 27.02.2004
//  Author      : Dmitriy Iassenev
//  Description : Fixed vertex allocator
////////////////////////////////////////////////////////////////////////////

#pragma once

template <u32 ReserveSize>
struct CVertexAllocatorFixed
{
    template<typename TCompoundVertex>
    struct VertexData
    {};

    template <typename TCompoundVertex>
    class CDataStorage
    {
    public:
        using Vertex = TCompoundVertex;
        using Index = typename Vertex::Index;
        using VertexContainer = xr_vector<Vertex>;

    protected:
        u32 m_vertex_count;
        VertexContainer m_vertices;

    public:
        inline CDataStorage();
        inline virtual ~CDataStorage();
        inline void init();
        inline u32 get_visited_node_count() const;
        inline Vertex &create_vertex();
    };
};

#include "vertex_allocator_fixed_inline.h"
