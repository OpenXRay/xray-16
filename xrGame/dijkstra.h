////////////////////////////////////////////////////////////////////////////
//	Module 		: dijkstra.h
//	Created 	: 21.03.2002
//  Modified 	: 02.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Implementation of the Dijkstra algorithm
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "vertex_path.h"
#include "data_storage_constructor.h"

template <
	typename _dist_type,
	typename _priority_queue, 
	typename _vertex_manager, 
	typename _vertex_allocator,
	bool	 euclidian_heuristics = true,
	typename _data_storage_base = CVertexPath<euclidian_heuristics>,
	template <typename _T> class _vertex = CEmptyClassTemplate,
	template <
		typename _1,
		typename _2
	>
	class	 _builder_allocator_constructor = CBuilderAllocatorConstructor,
	template <
		typename _1, 
		typename _2,
		typename _3,
		template <
			typename _1,
			typename _2
		>
		class	 _4
	>
	class	 _manager_builder_allocator_constructor = CManagerBuilderAllocatorConstructor,
	template <
		typename _algorithm, 
		typename _manager, 
		typename _builder, 
		typename _allocator,
		template <typename _T> class _vertex,
		template <
			typename _1,
			typename _2
		>
		class	 _builder_allocator_constructor = CBuilderAllocatorConstructor,
		template <
			typename _1, 
			typename _2,
			typename _3,
			template <
				typename _1,
				typename _2
			>
			class	 _4
		>
		class	 _manager_builder_allocator_constructor = CManagerBuilderAllocatorConstructor
	>
	class _data_storage_constructor = CDataStorageConstructor,
	typename _iteration_type = u32
> class CDijkstra
{
public:
	template <typename T1>
	struct _Vertex : public _vertex<T1> {
		typedef _dist_type _dist_type;

		_dist_type	_f;
		T1			*_back;

		IC	_dist_type &f()
		{
			return	(_f);
		}

		IC	const _dist_type &f() const
		{
			return	(_f);
		}

		IC	T1	*&back()
		{
			return	(_back);
		}
	};


	typedef _data_storage_constructor<
		_priority_queue,
		_vertex_manager,
		_data_storage_base,
		_vertex_allocator,
		_Vertex,
		_builder_allocator_constructor,
		_manager_builder_allocator_constructor
	>											CDataStorage;

protected:
	typedef typename CDataStorage::CGraphVertex CGraphVertex;
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

#include "dijkstra_inline.h"