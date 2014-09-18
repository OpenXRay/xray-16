////////////////////////////////////////////////////////////////////////////
//	Module 		: vertex_builder_allocator_constructor.h
//	Created 	: 21.03.2002
//  Modified 	: 28.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Manager builder allocator constructor
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "builder_allocator_constructor.h"

template <
	typename _manager, 
	typename _builder,
	typename _allocator,
	template <
		typename _builder,
		typename _allocator
	>
	class	 _builder_allocator_constructor = CBuilderAllocatorConstructor
>
struct CManagerBuilderAllocatorConstructor {
	template <
		template <typename T> class _vertex = CEmptyClassTemplate,
		template <typename T1, typename T2> class _index_vertex = CEmptyClassTemplate2
	>
	class CDataStorage : 
		public _manager::CDataStorage<
			_vertex,
			_index_vertex,
			_builder_allocator_constructor<
				_builder,
				_allocator
			>
		>
	{
	public:
		typedef typename _manager::CDataStorage<
			_vertex,
			_index_vertex,
			_builder_allocator_constructor<
			_builder,
			_allocator
			>
		> inherited;
		typedef typename inherited::inherited		inherited_allocator;
		typedef typename inherited::CGraphVertex	CGraphVertex;
		typedef typename CGraphVertex::_index_type	_index_type;

	public:
		IC							CDataStorage	(const u32 vertex_count);
		virtual						~CDataStorage	();
		IC		void				init			();
		IC		CGraphVertex		&create_vertex	(const _index_type &vertex_id);
	};
};

#include "manager_builder_allocator_constructor_inline.h"