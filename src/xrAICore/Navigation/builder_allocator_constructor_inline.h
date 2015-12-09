////////////////////////////////////////////////////////////////////////////
//	Module 		: builder_allocator_constructor_inline.h
//	Created 	: 21.03.2002
//  Modified 	: 28.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Builder allocator constructor inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template <template <typename _T> class _vertex, typename _1,typename _2> 

#define CConstructorBuilderAllocator BuilderAllocatorDataStorage<_vertex,_1,_2>

TEMPLATE_SPECIALIZATION
IC	CConstructorBuilderAllocator::BuilderAllocatorDataStorage(const u32 vertex_count) :
	CDataStorageBase			(vertex_count),
	CDataStorageAllocator		()
{
}

TEMPLATE_SPECIALIZATION
CConstructorBuilderAllocator::~BuilderAllocatorDataStorage()
{
}

TEMPLATE_SPECIALIZATION
IC	void CConstructorBuilderAllocator::init		()
{
	CDataStorageBase::init		();
	CDataStorageAllocator::init ();
}

#undef TEMPLATE_SPECIALIZATION
#undef CConstructorBuilderAllocator