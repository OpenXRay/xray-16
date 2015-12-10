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
// instantiated in CDijkstra
template <
	typename _algorithm, // CDataStorageBucketList|CDataStorageBinaryHeap
	typename _manager, // CVertexManagerFixed|CVertexManagerHashFixed
	typename _builder, // CEdgePath|CVertexPath
	typename _allocator, // CVertexAllocatorFixed
	template <typename _T> class _vertex = CEmptyClassTemplate // _Vertex -- dijkstra vertex
>
struct CDataStorageConstructor : // CDataStorageBucketList::CDataStorage<CManagerBuilderAllocatorConstructor<manager, path, allocator> >
    public _algorithm::template CDataStorage<
    CManagerBuilderAllocatorConstructor<_manager, _builder, _allocator>, _vertex>
{
    typedef typename _algorithm::template CDataStorage<
		CManagerBuilderAllocatorConstructor<_manager, _builder, _allocator>, _vertex> inherited; 

	typedef typename inherited::CGraphVertex	CGraphVertex;
	typedef typename CGraphVertex::_index_type	_index_type;

	CDataStorageConstructor (const u32 vertex_count) :
	    inherited(vertex_count)
	{}
};
