////////////////////////////////////////////////////////////////////////////
//	Module 		: vertex_builder_allocator_constructor_inline.h
//	Created 	: 21.03.2002
//  Modified 	: 28.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Manager builder allocator constructor inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template<\
		typename _manager,\
		typename _builder,\
		typename _allocator,\
		template <\
			typename _builder,\
			typename _allocator\
		>\
		class	 _builder_allocator_constructor\
	>\
	template <\
		template <typename T> class _vertex,\
		template <typename T1, typename T2> class _index_vertex\
	>

#define CConstructorManagerBuilderAllocator \
	CManagerBuilderAllocatorConstructor<\
		_manager,\
		_builder,\
		_allocator,\
		_builder_allocator_constructor\
	>::CDataStorage<_vertex,_index_vertex>

TEMPLATE_SPECIALIZATION
IC	CConstructorManagerBuilderAllocator::CDataStorage	(const u32 vertex_count) :
		inherited				(vertex_count)
{
}

TEMPLATE_SPECIALIZATION
CConstructorManagerBuilderAllocator::~CDataStorage	()
{
}

TEMPLATE_SPECIALIZATION
IC	void CConstructorManagerBuilderAllocator::init	()
{
	inherited::init				();
}

TEMPLATE_SPECIALIZATION
IC	typename CConstructorManagerBuilderAllocator::CGraphVertex &CConstructorManagerBuilderAllocator::create_vertex	(const _index_type &vertex_id)
{
	return						(inherited::create_vertex(inherited_allocator::create_vertex(),vertex_id));
}

#undef TEMPLATE_SPECIALIZATION
#undef CConstructorManagerBuilderAllocator