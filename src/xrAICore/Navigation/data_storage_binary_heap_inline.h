////////////////////////////////////////////////////////////////////////////
//  Module      : data_storage_binary_heap_inline.h
//  Created     : 21.03.2002
//  Modified    : 26.02.2004
//  Author      : Dmitriy Iassenev
//  Description : Binary m_heap data storage inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION template <typename TManagerDataStorage>

#define CBinaryHeap CDataStorageBinaryHeap::CDataStorage<TManagerDataStorage>

TEMPLATE_SPECIALIZATION
inline CBinaryHeap::CDataStorage(const u32 vertex_count) : Inherited(vertex_count)
{
    m_heap = xr_alloc<Vertex*>(vertex_count);
    ZeroMemory(m_heap, vertex_count * sizeof(Vertex*));
}

TEMPLATE_SPECIALIZATION
CBinaryHeap::~CDataStorage() { xr_free(m_heap); }
TEMPLATE_SPECIALIZATION
inline void CBinaryHeap::init()
{
    Inherited::init();
    m_heap_head = m_heap_tail = m_heap;
}

TEMPLATE_SPECIALIZATION
inline bool CBinaryHeap::is_opened_empty() const
{
    VERIFY(m_heap_head <= m_heap_tail);
    return m_heap_head == m_heap_tail;
}

TEMPLATE_SPECIALIZATION
inline void CBinaryHeap::add_opened(Vertex& vertex)
{
    VERIFY(m_heap_head <= m_heap_tail);
    Inherited::add_opened(vertex);
    if (!*m_heap_head || (*m_heap_head)->f() < vertex.f())
    {
        *m_heap_tail = &vertex;
    }
    else
    {
        *m_heap_tail = *m_heap_head;
        *m_heap_head = &vertex;
    }
    std::push_heap(m_heap_head, ++m_heap_tail, VertexPredicate());
}

TEMPLATE_SPECIALIZATION
inline void CBinaryHeap::decrease_opened(Vertex& vertex, const Distance value)
{
    VERIFY(!is_opened_empty());
    Vertex** i = m_heap_head;
    while (*i != &vertex)
        ++i;
    std::push_heap(m_heap_head, i + 1, VertexPredicate());
}

TEMPLATE_SPECIALIZATION
inline void CBinaryHeap::remove_best_opened()
{
    VERIFY(!is_opened_empty());
    std::pop_heap(m_heap_head, m_heap_tail--, VertexPredicate());
}

TEMPLATE_SPECIALIZATION
inline void CBinaryHeap::add_best_closed()
{
    VERIFY(!is_opened_empty());
    Inherited::add_closed(**m_heap_head);
}

TEMPLATE_SPECIALIZATION
inline typename CBinaryHeap::Vertex& CBinaryHeap::get_best() const
{
    VERIFY(!is_opened_empty());
    return **m_heap_head;
}

#undef TEMPLATE_SPECIALIZATION
#undef CBinaryHeap
