////////////////////////////////////////////////////////////////////////////
//	Module 		: data_storage_constructor.h
//	Created 	: 21.03.2002
//  Modified 	: 28.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Data storage constructor
////////////////////////////////////////////////////////////////////////////

#pragma once

struct EmptyVertexData
{};

template<typename... Components>
struct CompoundVertex : Components::template VertexData<CompoundVertex<Components...>>...
{};

template<
	typename TStorage, // CDataStorageBucketList|CDataStorageBinaryHeap
	typename TVertexManager, // CVertexManagerFixed|CVertexManagerHashFixed
	typename TPathBuilder, // CEdgePath|CVertexPath
	typename TVertexAllocator, // CVertexAllocatorFixed
	typename TCompoundVertex,
    typename TManagerDataStorage = typename TVertexManager::template
        CDataStorage<TPathBuilder, TVertexAllocator, TCompoundVertex>,
    typename TDataStorageBase = typename TStorage::template CDataStorage<TManagerDataStorage>
>
struct CDataStorageConstructor : public TDataStorageBase
{
    typedef TDataStorageBase inherited;
	typedef TCompoundVertex CGraphVertex;
	typedef typename CGraphVertex::_index_type _index_type;

	CDataStorageConstructor(const u32 vertex_count) : inherited(vertex_count)
	{}
    void init() { inherited::init(); }
    CGraphVertex &create_vertex(const _index_type &vertex_id)
    { return inherited::create_vertex(inherited::CDataStorageAllocator::create_vertex(), vertex_id); }
};
