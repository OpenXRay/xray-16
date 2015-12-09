////////////////////////////////////////////////////////////////////////////
//	Module 		: data_storage_double_linked_list.h
//	Created 	: 21.03.2002
//  Modified 	: 26.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Double linked list data storage
////////////////////////////////////////////////////////////////////////////

#pragma once

template <bool sorted = false>
struct CDataStorageDoubleLinkedList
{
	template <template <typename _T> class T1>
	struct DoubleLinkedList
    {
		template<typename T2>
		struct _vertex : public T1<T2>
        {
            T2 *_next;
			T2 *_prev;
            T2 *&next() { return _next; }
			T2 *&prev() { return _prev; }
		};
	};

	template <
		typename _data_storage,
		template <typename _T> class _vertex = CEmptyClassTemplate
	>
	class CDataStorage : public _data_storage::template CDataStorage<DoubleLinkedList<_vertex>::_vertex>
    {
	public:
        typedef typename _data_storage::template CDataStorage<DoubleLinkedList<_vertex>::_vertex> inherited;
		typedef typename inherited::CGraphVertex	CGraphVertex;
		typedef typename CGraphVertex::_dist_type	_dist_type;
		typedef typename CGraphVertex::_index_type	_index_type;

	protected:
        _dist_type m_max_distance;
		CGraphVertex m_list_data[2];
		CGraphVertex *m_list_head;
		CGraphVertex *m_list_tail;
		_dist_type m_switch_factor;

	public:
		IC						CDataStorage		(const u32 vertex_count, const _dist_type _max_distance = _dist_type(u32(-1)));
		virtual					~CDataStorage		();
		IC		void			init				();
		IC		bool			is_opened_empty		() const;
		IC		void			add_best_closed		();
		IC		void			set_switch_factor	(const _dist_type _switch_factor);
		IC		void			add_opened			(CGraphVertex &vertex);
		IC		void			decrease_opened		(CGraphVertex &vertex, const _dist_type value);
		IC		void			remove_best_opened	();
		IC		CGraphVertex	&get_best			() const;
	};
};

#include "xrAICore/Navigation/data_storage_double_linked_list_inline.h"