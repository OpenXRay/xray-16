////////////////////////////////////////////////////////////////////////////
//	Module 		: vertex_allocator_fixed_inline.h
//	Created 	: 21.03.2002
//  Modified 	: 27.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Fixed vertex allocator inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template<u32 reserved_vertex_count>\
	template<typename _vertex>

#define CFixedVertexAllocator	CVertexAllocatorFixed<reserved_vertex_count>::CDataStorage<_vertex>

TEMPLATE_SPECIALIZATION
IC	CFixedVertexAllocator::CDataStorage					()
{
	u32						memory_usage = 0;
	u32						byte_count;

	byte_count				= (reserved_vertex_count)*sizeof(CGraphVertex);
//	m_vertices				= xr_alloc<CGraphVertex>(reserved_vertex_count);
//	ZeroMemory				(m_vertices,byte_count);
	m_vertices.resize		(reserved_vertex_count);
	memory_usage			+= byte_count;
}

TEMPLATE_SPECIALIZATION
CFixedVertexAllocator::~CDataStorage					()
{
//	xr_free					(m_vertices);
}

TEMPLATE_SPECIALIZATION
IC	void CFixedVertexAllocator::init					()
{
	m_vertex_count			= 0;
}

TEMPLATE_SPECIALIZATION
IC	u32  CFixedVertexAllocator::get_visited_node_count	() const
{
	return					(m_vertex_count);
}

TEMPLATE_SPECIALIZATION
IC	typename CFixedVertexAllocator::CGraphVertex &CFixedVertexAllocator::create_vertex		()
{
	VERIFY					(m_vertex_count < reserved_vertex_count - 1);
	return					(*(m_vertices.begin() + m_vertex_count++));
}

#undef TEMPLATE_SPECIALIZATION
#undef CFixedVertexAllocator