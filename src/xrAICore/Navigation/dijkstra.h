////////////////////////////////////////////////////////////////////////////
//	Module 		: dijkstra.h
//	Created 	: 21.03.2002
//  Modified 	: 02.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Implementation of the Dijkstra algorithm
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Navigation/vertex_path.h"
#include "xrAICore/Navigation/data_storage_constructor.h"

template<typename _dist_type, typename TVertexData>
struct DijkstraVertexData
{
    template<typename TCompoundVertex>
	struct VertexData : TVertexData::template VertexData<TCompoundVertex>
    {
		typedef _dist_type _dist_type;

		_dist_type _f;
        TCompoundVertex *_back;

		_dist_type &f() { return _f; }
		const _dist_type &f() const { return _f; }
        TCompoundVertex *&back() { return _back; }
	};
};

template <
	typename _dist_type,
	typename _priority_queue, 
	typename _vertex_manager, 
	typename _vertex_allocator,
	bool	 euclidian_heuristics = true,
	typename _data_storage_base = CVertexPath<euclidian_heuristics>,
	typename _iteration_type = u32,
    typename TVertexData = EmptyVertexData
> class CDijkstra
{
public:
    using CompoundVertex = CompoundVertex<DijkstraVertexData<_dist_type, TVertexData>,
        _priority_queue, _vertex_manager, _vertex_allocator, _data_storage_base>;
	typedef CDataStorageConstructor<
		_priority_queue, // algorithm
		_vertex_manager, // manager
		_data_storage_base, // builder
		_vertex_allocator, // allocator
        CompoundVertex
	> CDataStorage;

protected:
	typedef CompoundVertex CGraphVertex;
	typedef typename CGraphVertex::_dist_type	_dist_type;
	typedef typename CGraphVertex::_index_type	_index_type;

protected:
	bool				m_search_started;
	CDataStorage		*m_data_storage;

protected:
	template <typename _PathManager>
	IC		void				initialize		(_PathManager &path_manager);
	template <typename _PathManager>
	IC		bool				step			(_PathManager &path_manager);
	template <typename _PathManager>
	IC		void				finalize		(_PathManager &path_manager);

public:
	IC							CDijkstra		(const u32 max_vertex_count);
	virtual						~CDijkstra		();
	template <typename _PathManager>
	IC		bool				find			(_PathManager &path_manager);
	IC		CDataStorage		&data_storage	();
	IC		const CDataStorage	&data_storage	() const;
};

#include "xrAICore/Navigation/dijkstra_inline.h"