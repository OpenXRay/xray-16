////////////////////////////////////////////////////////////////////////////
//	Module 		: data_storage_binary_heap.h
//	Created 	: 21.03.2002
//  Modified 	: 26.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Binary heap data storage
////////////////////////////////////////////////////////////////////////////

#pragma once

struct CDataStorageBinaryHeap {

	template <
		typename _data_storage,
		template <typename _T> class _vertex = CEmptyClassTemplate
	>
	class CDataStorage : public _data_storage::CDataStorage<_vertex> {
	public:
		typedef typename _data_storage::CDataStorage<_vertex>	inherited;
		typedef typename inherited::CGraphVertex				CGraphVertex;
		typedef typename CGraphVertex::_dist_type				_dist_type;
		typedef typename CGraphVertex::_index_type				_index_type;

		struct CGraphNodePredicate {
			IC			bool	operator()(CGraphVertex *node1, CGraphVertex *node2)
			{
				return				(node1->f() > node2->f());
			};
		};

	protected:
		CGraphVertex			**m_heap;
		CGraphVertex			**m_heap_head;
		CGraphVertex			**m_heap_tail;

	public:
		IC						CDataStorage		(const u32 vertex_count);
		virtual					~CDataStorage		();
		IC		void			init				();
		IC		bool			is_opened_empty		() const;
		IC		void			add_opened			(CGraphVertex &vertex);
		IC		void			decrease_opened		(CGraphVertex &vertex, const _dist_type value);
		IC		void			remove_best_opened	();
		IC		void			add_best_closed		();
		IC		CGraphVertex	&get_best			() const;
	};
};

#include "data_storage_binary_heap_inline.h"