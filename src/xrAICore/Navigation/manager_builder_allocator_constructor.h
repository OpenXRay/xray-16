////////////////////////////////////////////////////////////////////////////
//	Module 		: vertex_builder_allocator_constructor.h
//	Created 	: 21.03.2002
//  Modified 	: 28.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Manager builder allocator constructor
////////////////////////////////////////////////////////////////////////////

#pragma once

template <
	typename _manager, 
	typename _builder, // CVertexPath
	typename _allocator
>
struct CManagerBuilderAllocatorConstructor {
	template <
		template <typename T> class _vertex = CEmptyClassTemplate,
		template <typename T1, typename T2> class _index_vertex = CEmptyClassTemplate2
	>
	class CDataStorage : 
	    public _manager::template CDataStorage<
			_builder,
            _allocator,
			_vertex,
			_index_vertex
		>
	{
	public:
	    typedef typename _manager::template CDataStorage<_builder, _allocator, _vertex, _index_vertex> inherited;
		typedef typename inherited::CDataStorageAllocator inherited_allocator;
		typedef typename inherited::CGraphVertex	CGraphVertex;
		typedef typename CGraphVertex::_index_type	_index_type;

	public:
        CDataStorage(const u32 vertex_count) :
            inherited(vertex_count)
        {}
        virtual ~CDataStorage() {}
        void init() { inherited::init(); }
        CGraphVertex &create_vertex(const _index_type &vertex_id)
        { return inherited::create_vertex(inherited_allocator::create_vertex(), vertex_id); }
	};
};
