////////////////////////////////////////////////////////////////////////////
//	Module 		: data_storage_bucket_list.h
//	Created 	: 21.03.2002
//  Modified 	: 26.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Bucket list data storage
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "data_storage_double_linked_list.h"

template <
	typename	_path_id_type,
	typename	_bucket_id_type,
	u32			bucket_count,
	bool		clear_buckets
>
struct CDataStorageBucketList {

	template <template <typename _T> class T1>
	struct BucketList {
		template<typename T2>
		struct _vertex : public T1<T2> {
			_path_id_type	m_path_id;
			_bucket_id_type	m_bucket_id;
		};
	};

	template <
		typename _data_storage,
		template <typename _T> class _vertex = CEmptyClassTemplate
	>
	class CDataStorage : public CDataStorageDoubleLinkedList<false>::CDataStorage<_data_storage,BucketList<_vertex>::_vertex> {
	public:
		typedef typename CDataStorageDoubleLinkedList<false>::CDataStorage<
			_data_storage,
			BucketList<_vertex>::_vertex
		>											inherited;
		typedef typename inherited::inherited_base	inherited_base;
		typedef typename inherited::CGraphVertex	CGraphVertex;
		typedef typename CGraphVertex::_dist_type	_dist_type;
		typedef typename CGraphVertex::_index_type	_index_type;

	protected:
		_dist_type				m_min_bucket_value;
		_dist_type				m_max_bucket_value;
		CGraphVertex			*m_buckets[bucket_count];
		u32						m_min_bucket_id;

	public:
		IC						CDataStorage		(const u32 vertex_count);
		virtual					~CDataStorage		();
		IC		void			init				();
		IC		void			add_best_closed		();
		IC		bool			is_opened_empty		();
		IC		u32				compute_bucket_id	(CGraphVertex &vertex) const;
		IC		void			verify_buckets		() const;
		IC		void			add_to_bucket		(CGraphVertex &vertex, u32 bucket_id);
		IC		void			add_opened			(CGraphVertex &vertex);
		IC		void			decrease_opened		(CGraphVertex &vertex, const _dist_type value);
		IC		void			remove_best_opened	();
		IC		CGraphVertex	&get_best			();
		IC		void			set_min_bucket_value(const _dist_type min_bucket_value);
		IC		void			set_max_bucket_value(const _dist_type max_bucket_value);
	};
};

#include "data_storage_bucket_list_inline.h"