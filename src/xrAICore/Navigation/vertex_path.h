////////////////////////////////////////////////////////////////////////////
//  Module      : vertex_path.h
//  Created     : 21.03.2002
//  Modified    : 02.03.2004
//  Author      : Dmitriy Iassenev
//  Description : Vertex path class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/xr_types.h"
#include "xrCommon/xr_vector.h"

template <bool EuclidianHeuristics = true>
struct CVertexPath
{
#pragma pack(push, 1)
    template <typename TCompoundVertex>
    struct VertexData
    {
    };
#pragma pack(pop)

    template <typename TCompoundVertex>
    class CDataStorage
    {
    public:
        using Vertex = TCompoundVertex;
        using Index = typename Vertex::Index;

    public:
        inline CDataStorage(const u32 vertex_count);
        inline virtual ~CDataStorage();
        inline void init();
        inline void assign_parent(Vertex& neighbour, Vertex* parent);
        template <typename T>
        inline void assign_parent(Vertex& neighbour, Vertex* parent, const T&);
        inline void update_successors(Vertex& neighbour);
        inline void get_node_path(xr_vector<Index>& path, Vertex* best);
    };
};

#include "xrAICore/Navigation/vertex_path_inline.h"
