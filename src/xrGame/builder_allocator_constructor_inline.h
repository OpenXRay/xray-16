////////////////////////////////////////////////////////////////////////////
//	Module 		: builder_allocator_constructor_inline.h
//	Created 	: 21.03.2002
//  Modified 	: 28.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Builder allocator constructor inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template <typename _1,typename _2>\
	template <template <typename _T> class _vertex> 

#define CConstructorBuilderAllocator CBuilderAllocatorConstructor<_1,_2>::CDataStorage<_vertex>

TEMPLATE_SPECIALIZATION
IC	CConstructorBuilderAllocator::CDataStorage	(const u32 vertex_count) :
	CDataStorageBase			(vertex_count),
	CDataStorageAllocator		()
{
}

TEMPLATE_SPECIALIZATION
CConstructorBuilderAllocator::~CDataStorage		()
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