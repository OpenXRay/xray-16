////////////////////////////////////////////////////////////////////////////
//	Module 		: data_storage_bucket_list_רעהרעף.h
//	Created 	: 21.03.2002
//  Modified 	: 26.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Bucket list data storage inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template <\
		typename	_path_id_type,\
		typename	_bucket_id_type,\
		u32			bucket_count,\
		bool		clear_buckets\
	>\
	template <\
		typename _data_storage,\
		template <typename _T> class _vertex\
	>

#define CBucketList CDataStorageBucketList<_path_id_type,_bucket_id_type,bucket_count,clear_buckets>::CDataStorage<_data_storage,_vertex>

TEMPLATE_SPECIALIZATION
IC	CBucketList::CDataStorage			(const u32 vertex_count) :
		inherited(vertex_count)
{
	m_min_bucket_value		= _dist_type(0);
	m_max_bucket_value		= _dist_type(1000);
	ZeroMemory				(m_buckets,bucket_count*sizeof(CGraphVertex*));
}

TEMPLATE_SPECIALIZATION
CBucketList::~CDataStorage				()
{
}

TEMPLATE_SPECIALIZATION
IC	void CBucketList::init				()
{
	inherited::init			();
	m_min_bucket_id			= bucket_count;
	if (clear_buckets)
		ZeroMemory			(m_buckets,bucket_count*sizeof(CGraphVertex*));
}

TEMPLATE_SPECIALIZATION
IC	void CBucketList::add_best_closed	()
{
	VERIFY					(!is_opened_empty());
	inherited_base::add_closed	(*m_buckets[m_min_bucket_id]);
}

TEMPLATE_SPECIALIZATION
IC	bool CBucketList::is_opened_empty	()
{
	if (m_min_bucket_id == bucket_count)
		return				(true);
	if (!m_buckets[m_min_bucket_id]) {
		if (!clear_buckets)
			for (++m_min_bucket_id; (m_min_bucket_id < bucket_count) && (!m_buckets[m_min_bucket_id] || (m_buckets[m_min_bucket_id]->m_path_id != current_path_id()) || (m_buckets[m_min_bucket_id]->m_bucket_id != m_min_bucket_id)); ++m_min_bucket_id);
		else
			for (++m_min_bucket_id; (m_min_bucket_id < bucket_count) && !m_buckets[m_min_bucket_id]; ++m_min_bucket_id);
		return				(m_min_bucket_id >= bucket_count);
	}
	return					(false);
}

TEMPLATE_SPECIALIZATION
IC	u32	 CBucketList::compute_bucket_id	(CGraphVertex &vertex) const
{
	if (vertex.f() >= m_max_bucket_value)
		return			(bucket_count - 1);
	if (vertex.f() <= m_min_bucket_value)
		return			(0);
	return				(u32(bucket_count*(vertex.f() - m_min_bucket_value)/(m_max_bucket_value - m_min_bucket_value)));
}

TEMPLATE_SPECIALIZATION
IC	void CBucketList::verify_buckets	() const
{
//		for (u32 i=0; i<bucket_count; ++i) {
//			CGraphVertex	*j = m_buckets[i], *k;
//			if (!j || (indexes[j->index()].m_path_id != current_path_id()) || (indexes[j->index()].vertex != j))
//				continue;
//			u32			count = 0, count1 = 0;
//			for ( ; j; k=j,j=j->next(), ++count) {
//				VERIFY	(indexes[j->index()].m_path_id == current_path_id());
//				VERIFY	(compute_bucket_id(*j) == i);
//				VERIFY	(!j->prev() || (j == j->prev()->next()));
//				VERIFY	(!j->next() || (j == j->next()->prev()));
//				VERIFY	(!j->next() || (j != j->next()));
//				VERIFY	(!j->prev() || (j != j->prev()));
//			}
//			for ( ; k; k=k->prev(), ++count1) {
//				VERIFY	(indexes[k->index()].m_path_id == current_path_id());
//				VERIFY	(compute_bucket_id(*k) == i);
//				VERIFY	(!k->prev() || (k == k->prev()->next()));
//				VERIFY	(!k->next() || (k == k->next()->prev()));
//				VERIFY	(!k->next() || (k != k->next()));
//				VERIFY	(!k->prev() || (k != k->prev()));
//			}
//			VERIFY		(count == count1);
//		}
}

TEMPLATE_SPECIALIZATION
IC	void CBucketList::add_to_bucket		(CGraphVertex &vertex, u32 m_bucket_id)
{
	if (m_bucket_id < m_min_bucket_id)
		m_min_bucket_id		= m_bucket_id;

	CGraphVertex				*i = m_buckets[m_bucket_id];
	if (!i || (!clear_buckets && ((i->m_path_id != current_path_id()) || (i->m_bucket_id != m_bucket_id)))) {
		vertex.m_bucket_id		= m_bucket_id;
		vertex.m_path_id		= current_path_id();
		m_buckets[m_bucket_id]	= &vertex;
		vertex.next()				= vertex.prev() = 0;
		verify_buckets			();
		return;
	}

	vertex.m_bucket_id			= m_bucket_id;
	vertex.m_path_id			= current_path_id();

	if (i->f() >= vertex.f()) {
		m_buckets[m_bucket_id]	= &vertex;
		vertex.next()			= i;
		vertex.prev()			= 0;
		i->prev()				= &vertex;
		verify_buckets		();
		return;
	}
	
	if (!i->next()) {
		vertex.prev()			= i;
		vertex.next()			= 0;
		i->next()				= &vertex;
		verify_buckets		();
		return;
	}
	
	for (i = i->next(); i->next(); i = i->next()) {
		if (i->f() >= vertex.f()) {
			vertex.next()		= i;
			vertex.prev()		= i->prev();
			i->prev()->next()	= &vertex;
			i->prev()			= &vertex;
			verify_buckets	();
			return;
		}
	}
	
	if (i->f() >= vertex.f()) {
		vertex.next()		= i;
		vertex.prev()		= i->prev();
		i->prev()->next()	= &vertex;
		i->prev()			= &vertex;
		verify_buckets	();
		return;
	}
	else {
		vertex.next()		= 0;
		vertex.prev()		= i;
		i->next()			= &vertex;
		verify_buckets	();
		return;
	}
	
//		verify_buckets	();
}

TEMPLATE_SPECIALIZATION
IC	void CBucketList::add_opened		(CGraphVertex &vertex)
{
//	ai().m_visited_nodes.push_back	(vertex.index());
	inherited_base::add_opened	(vertex);
	add_to_bucket				(vertex,compute_bucket_id(vertex));
	verify_buckets				();
}

TEMPLATE_SPECIALIZATION
IC	void CBucketList::decrease_opened	(CGraphVertex &vertex, const _dist_type value)
{
	VERIFY					(!is_opened_empty());
	u32						node_bucket_id = compute_bucket_id(vertex);
	if (vertex.prev())
		vertex.prev()->next()	= vertex.next();
	else {
		VERIFY				(m_buckets[vertex.m_bucket_id] == &vertex);
		m_buckets[vertex.m_bucket_id] = vertex.next();
	}
	if (vertex.next())
		vertex.next()->prev()		= vertex.prev();

	verify_buckets			();
	add_to_bucket			(vertex,node_bucket_id);
	verify_buckets			();
}

TEMPLATE_SPECIALIZATION
IC	void CBucketList::remove_best_opened()
{
	VERIFY					(!is_opened_empty());
	verify_buckets			();
	VERIFY					(m_buckets[m_min_bucket_id] && is_visited(m_buckets[m_min_bucket_id]->index()));
	m_buckets[m_min_bucket_id]	= m_buckets[m_min_bucket_id]->next();
	if (m_buckets[m_min_bucket_id])
		m_buckets[m_min_bucket_id]->prev() = 0;
	verify_buckets			();
}

TEMPLATE_SPECIALIZATION
IC	typename CBucketList::CGraphVertex &CBucketList::get_best	()
{
	VERIFY					(!is_opened_empty());
	return					(*m_buckets[m_min_bucket_id]);
}

TEMPLATE_SPECIALIZATION
IC	void CBucketList::set_min_bucket_value	(const _dist_type min_bucket_value)
{
	m_min_bucket_value		= min_bucket_value;
}

TEMPLATE_SPECIALIZATION
IC	void CBucketList::set_max_bucket_value	(const _dist_type max_bucket_value)
{
	m_max_bucket_value		= max_bucket_value;
}

#undef TEMPLATE_SPECIALIZATION
#undef CBucketList