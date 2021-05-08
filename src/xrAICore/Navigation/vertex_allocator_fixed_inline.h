////////////////////////////////////////////////////////////////////////////
//  Module      : vertex_allocator_fixed_inline.h
//  Created     : 21.03.2002
//  Modified    : 27.02.2004
//  Author      : Dmitriy Iassenev
//  Description : Fixed vertex allocator inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <u32 ReserveSize>  \
    template <typename TCompoundVertex>

#define CFixedVertexAllocator CVertexAllocatorFixed<ReserveSize>::CDataStorage<TCompoundVertex>

TEMPLATE_SPECIALIZATION
inline CFixedVertexAllocator::CDataStorage() : m_vertex_count(u32(-1)) { m_vertices.resize(ReserveSize); }
TEMPLATE_SPECIALIZATION
inline CFixedVertexAllocator::~CDataStorage() {}
TEMPLATE_SPECIALIZATION
inline void CFixedVertexAllocator::init() { m_vertex_count = 0; }
TEMPLATE_SPECIALIZATION
inline u32 CFixedVertexAllocator::get_visited_node_count() const { return m_vertex_count; }
TEMPLATE_SPECIALIZATION
inline TCompoundVertex& CFixedVertexAllocator::create_vertex()
{
    VERIFY(m_vertex_count < ReserveSize - 1);
    return *(m_vertices.begin() + m_vertex_count++);
}

#undef TEMPLATE_SPECIALIZATION
#undef CFixedVertexAllocator
