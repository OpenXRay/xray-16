////////////////////////////////////////////////////////////////////////////
//	Module 		: data_storage_constructor.h
//	Created 	: 21.03.2002
//  Modified 	: 28.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Data storage constructor
////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename T>				class CEmptyClassTemplate	{};
template <typename T1, typename T2> class CEmptyClassTemplate2	{};

#include "xrAICore/Navigation/manager_builder_allocator_constructor.h"

template <
	typename _algorithm, 
	typename _manager, 
	typename _builder, 
	typename _allocator,
	template <typename _T> class _vertex = CEmptyClassTemplate
>
struct CDataStorageConstructor : 
    public _algorithm::template CDataStorage<
    CManagerBuilderAllocatorConstructor<_manager, _builder, _allocator>, _vertex>
{
    typedef typename _algorithm::template CDataStorage<
		CManagerBuilderAllocatorConstructor<_manager, _builder, _allocator>, _vertex> inherited; 

	typedef typename inherited::CGraphVertex	CGraphVertex;
	typedef typename CGraphVertex::_index_type	_index_type;

	IC	CDataStorageConstructor (const u32 vertex_count) :
		inherited(vertex_count)
	{
	}
};
