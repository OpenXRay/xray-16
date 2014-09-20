////////////////////////////////////////////////////////////////////////////
//	Module 		: data_storage_binary_heap_inline.h
//	Created 	: 21.03.2002
//  Modified 	: 26.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Binary m_heap data storage inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template<\
		typename _data_storage,\
		template <typename _T> class _vertex\
	>

#define CBinaryHeap				CDataStorageBinaryHeap::CDataStorage<_data_storage,_vertex>

TEMPLATE_SPECIALIZATION
IC	CBinaryHeap::CDataStorage				(const u32 vertex_count) : 
		inherited(vertex_count)
{
	u32						memory_usage = 0;
	u32						byte_count;

	byte_count				= vertex_count*sizeof(CGraphVertex*);
	m_heap					= xr_alloc<CGraphVertex*>(vertex_count);
	ZeroMemory				(m_heap,byte_count);
	memory_usage			+= byte_count;
}

TEMPLATE_SPECIALIZATION
CBinaryHeap::~CDataStorage					()
{
	xr_free					(m_heap);
}

TEMPLATE_SPECIALIZATION
IC	void CBinaryHeap::init					()
{
	inherited::init			();
	m_heap_head				= m_heap_tail = m_heap;
}

TEMPLATE_SPECIALIZATION
IC	bool CBinaryHeap::is_opened_empty		() const
{
	VERIFY					(m_heap_head <= m_heap_tail);
	return					(m_heap_head == m_heap_tail);
}

TEMPLATE_SPECIALIZATION
IC	void CBinaryHeap::add_opened			(CGraphVertex &vertex)
{
	VERIFY					(m_heap_head <= m_heap_tail);
	inherited::add_opened	(vertex);
	if (!*m_heap_head || ((*m_heap_head)->f() < vertex.f())) {
		*m_heap_tail		= &vertex;
	}
	else {
		*m_heap_tail		= *m_heap_head;
		*m_heap_head		= &vertex;
	}
	std::push_heap			(m_heap_head,++m_heap_tail,CGraphNodePredicate());
}

TEMPLATE_SPECIALIZATION
IC	void CBinaryHeap::decrease_opened		(CGraphVertex &vertex, const _dist_type value)
{
	VERIFY					(!is_opened_empty());
	for (CGraphVertex **i = m_heap_head; *i != &vertex; ++i);
	std::push_heap			(m_heap_head,i + 1,CGraphNodePredicate());
}

TEMPLATE_SPECIALIZATION
IC	void CBinaryHeap::remove_best_opened	()
{
	VERIFY					(!is_opened_empty());
	std::pop_heap			(m_heap_head,m_heap_tail--,CGraphNodePredicate());
}

TEMPLATE_SPECIALIZATION
IC	void CBinaryHeap::add_best_closed		()
{
	VERIFY					(!is_opened_empty());
	inherited::add_closed	(**m_heap_head);
}

TEMPLATE_SPECIALIZATION
IC	typename CBinaryHeap::CGraphVertex &CBinaryHeap::get_best		() const
{
	VERIFY					(!is_opened_empty());
	return					(**m_heap_head);
}

#undef TEMPLATE_SPECIALIZATION
#undef CBinaryHeap