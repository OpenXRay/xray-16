////////////////////////////////////////////////////////////////////////////
//	Module 		: vertex_allocator_fixed.h
//	Created 	: 21.03.2002
//  Modified 	: 27.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Fixed vertex allocator
////////////////////////////////////////////////////////////////////////////

#pragma once

template <u32 reserved_vertex_count>
struct CVertexAllocatorFixed {
	template <typename _vertex>
	class CDataStorage {
	public:
		typedef _vertex								CGraphVertex;
		typedef typename CGraphVertex::_index_type	_index_type;
		typedef xr_vector<CGraphVertex>				VERTICES;

	protected:
		u32							m_vertex_count;
		VERTICES					m_vertices;

	public:
		IC							CDataStorage			();
		virtual						~CDataStorage			();
		IC		void				init					();
		IC		u32					get_visited_node_count	() const;
		IC		CGraphVertex		&create_vertex			();
	};
};

#include "vertex_allocator_fixed_inline.h"