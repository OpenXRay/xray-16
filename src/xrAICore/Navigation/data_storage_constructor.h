////////////////////////////////////////////////////////////////////////////
//	Module 		: data_storage_constructor.h
//	Created 	: 21.03.2002
//  Modified 	: 28.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Data storage constructor
////////////////////////////////////////////////////////////////////////////

#pragma once
struct EmptyVertexData {};
template <typename T>				class CEmptyClassTemplate	{};
template <typename T1, typename T2> class CEmptyClassTemplate2	{};

template <
	typename _manager, 
	typename _builder, // CVertexPath
	typename _allocator
>
struct CManagerBuilderAllocatorConstructor
{
	template<typename TCompoundVertex>
	class CDataStorage :
	    public _manager::template CDataStorage<_builder, _allocator, TCompoundVertex>
	{
	public:
	    typedef typename _manager::template CDataStorage<_builder, _allocator, TCompoundVertex> inherited;
		typedef typename inherited::CDataStorageAllocator inherited_allocator;
		typedef TCompoundVertex CGraphVertex;
		typedef typename CGraphVertex::_index_type _index_type;

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

// instantiated in CDijkstra
template <
	typename _algorithm, // CDataStorageBucketList|CDataStorageBinaryHeap
	typename _manager, // CVertexManagerFixed|CVertexManagerHashFixed
	typename _builder, // CEdgePath|CVertexPath
	typename _allocator, // CVertexAllocatorFixed
	typename TCompoundVertex // _Vertex -- dijkstra vertex
>
struct CDataStorageConstructor : // CDataStorageBucketList::CDataStorage<CManagerBuilderAllocatorConstructor<manager, path, allocator> >
    public _algorithm::template CDataStorage<
    CManagerBuilderAllocatorConstructor<_manager, _builder, _allocator>, TCompoundVertex>
{
    typedef typename _algorithm::template CDataStorage<
		CManagerBuilderAllocatorConstructor<_manager, _builder, _allocator>, TCompoundVertex> inherited;

	typedef TCompoundVertex CGraphVertex;
	typedef typename CGraphVertex::_index_type	_index_type;

	CDataStorageConstructor (const u32 vertex_count) :
	    inherited(vertex_count)
	{}
};
