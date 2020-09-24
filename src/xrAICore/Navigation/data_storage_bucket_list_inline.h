////////////////////////////////////////////////////////////////////////////
//  Module      : data_storage_bucket_list_inline.h
//  Created     : 21.03.2002
//  Modified    : 26.02.2004
//  Author      : Dmitriy Iassenev
//  Description : Bucket list data storage inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION                                                         \
    template <typename TPathId, typename TBucketId, u32 BucketCount, bool ClearBuckets> \
    template <typename TManagerDataStorage>

#define CBucketList \
    CDataStorageBucketList<TPathId, TBucketId, BucketCount, ClearBuckets>::CDataStorage<TManagerDataStorage>

TEMPLATE_SPECIALIZATION
inline CBucketList::CDataStorage(const u32 vertex_count) : TManagerDataStorage(vertex_count)
{
    m_max_distance = typename TManagerDataStorage::Vertex::Distance(-1);
    m_min_bucket_value = typename TManagerDataStorage::Vertex::Distance(0);
    m_max_bucket_value = typename TManagerDataStorage::Vertex::Distance(1000);
    ZeroMemory(m_buckets, BucketCount * sizeof(typename TManagerDataStorage::Vertex*));
    m_min_bucket_id = 0;
}

TEMPLATE_SPECIALIZATION
CBucketList::~CDataStorage() {}
TEMPLATE_SPECIALIZATION
inline void CBucketList::init()
{
    TManagerDataStorage::init();
    ZeroMemory(m_list_data, 2 * sizeof(typename TManagerDataStorage::Vertex));
    m_list_head = m_list_data;
    m_list_tail = m_list_data + 1;
    m_list_head->next() = m_list_tail;
    m_list_tail->f() = m_max_distance;
    m_list_tail->prev() = m_list_head;
    m_min_bucket_id = BucketCount;
    if (ClearBuckets)
        ZeroMemory(m_buckets, BucketCount * sizeof(typename TManagerDataStorage::Vertex*));
}

TEMPLATE_SPECIALIZATION
inline void CBucketList::add_best_closed()
{
    VERIFY(!is_opened_empty());
    TManagerDataStorage::add_closed(*m_buckets[m_min_bucket_id]);
}

TEMPLATE_SPECIALIZATION
inline bool CBucketList::is_opened_empty()
{
    if (m_min_bucket_id == BucketCount)
        return true;
    if (!m_buckets[m_min_bucket_id])
    {
        m_min_bucket_id++;
        if (!ClearBuckets)
        {
            while (m_min_bucket_id < BucketCount)
            {
                auto bucket = m_buckets[m_min_bucket_id];
                if (!bucket || bucket->m_path_id != this->current_path_id() || bucket->m_bucket_id != m_min_bucket_id)
                {
                    m_min_bucket_id++;
                    continue;
                }
                break;
            }
        }
        else
        {
            while (m_min_bucket_id < BucketCount && !m_buckets[m_min_bucket_id])
                m_min_bucket_id++;
        }
        return m_min_bucket_id >= BucketCount;
    }
    return false;
}

TEMPLATE_SPECIALIZATION
inline u32 CBucketList::compute_bucket_id(typename TManagerDataStorage::Vertex& vertex) const
{
    typename TManagerDataStorage::Vertex::Distance dist = vertex.f();
    if (dist >= m_max_bucket_value)
        return BucketCount - 1;
    if (dist <= m_min_bucket_value)
        return 0;
    return u32(BucketCount * (dist - m_min_bucket_value) / (m_max_bucket_value - m_min_bucket_value));
}

TEMPLATE_SPECIALIZATION
inline void CBucketList::verify_buckets() const
{
#if 0
    for (u32 i = 0; i<BucketCount; i++)
    {
        Vertex *j = m_buckets[i], *k;
        if (!j)
            continue;
        auto index = indexes[j->index()];
        if (index.m_path_id!= this->current_path_id() || index.vertex!=j)
            continue;
        u32 count1 = 0, count2 = 0;
        for (; j; k = j, j = j->next(), count1++)
        {
            VERIFY(indexes[j->index()].m_path_id== this->current_path_id());
            VERIFY(compute_bucket_id(*j)==i);
            VERIFY(!j->prev() || j==j->prev()->next());
            VERIFY(!j->next() || j==j->next()->prev());
            VERIFY(!j->next() || j!=j->next());
            VERIFY(!j->prev() || j!=j->prev());
        }
        for (; k; k = k->prev(), count2++)
        {
            VERIFY(indexes[k->index()].m_path_id== this->current_path_id());
            VERIFY(compute_bucket_id(*k)==i);
            VERIFY(!k->prev() || k==k->prev()->next());
            VERIFY(!k->next() || k==k->next()->prev());
            VERIFY(!k->next() || k!=k->next());
            VERIFY(!k->prev() || k!=k->prev());
        }
        VERIFY(count1==count2);
    }
#endif
}

TEMPLATE_SPECIALIZATION
inline void CBucketList::add_to_bucket(typename TManagerDataStorage::Vertex& vertex, u32 m_bucket_id)
{
    if (m_bucket_id < m_min_bucket_id)
        m_min_bucket_id = m_bucket_id;
    typename TManagerDataStorage::Vertex* i = m_buckets[m_bucket_id];
    if (!i || (!ClearBuckets && (i->m_path_id != this->current_path_id())) || (i->m_bucket_id != m_bucket_id))
    {
        vertex.m_bucket_id = m_bucket_id;
        vertex.m_path_id = this->current_path_id();
        m_buckets[m_bucket_id] = &vertex;
        vertex.next() = vertex.prev() = 0;
        verify_buckets();
        return;
    }
    vertex.m_bucket_id = m_bucket_id;
    vertex.m_path_id = this->current_path_id();
    if (i->f() >= vertex.f())
    {
        m_buckets[m_bucket_id] = &vertex;
        vertex.next() = i;
        vertex.prev() = 0;
        i->prev() = &vertex;
        verify_buckets();
        return;
    }
    if (!i->next())
    {
        vertex.prev() = i;
        vertex.next() = 0;
        i->next() = &vertex;
        verify_buckets();
        return;
    }
    for (i = i->next(); i->next(); i = i->next())
    {
        if (i->f() >= vertex.f())
        {
            vertex.next() = i;
            vertex.prev() = i->prev();
            i->prev()->next() = &vertex;
            i->prev() = &vertex;
            verify_buckets();
            return;
        }
    }
    if (i->f() >= vertex.f())
    {
        vertex.next() = i;
        vertex.prev() = i->prev();
        i->prev()->next() = &vertex;
        i->prev() = &vertex;
        verify_buckets();
        return;
    }
    else
    {
        vertex.next() = 0;
        vertex.prev() = i;
        i->next() = &vertex;
        verify_buckets();
        return;
    }
}

TEMPLATE_SPECIALIZATION
inline void CBucketList::add_opened(typename TManagerDataStorage::Vertex& vertex)
{
    TManagerDataStorage::add_opened(vertex);
    add_to_bucket(vertex, compute_bucket_id(vertex));
    verify_buckets();
}

TEMPLATE_SPECIALIZATION
inline void CBucketList::decrease_opened(typename TManagerDataStorage::Vertex& vertex, const typename TManagerDataStorage::Vertex::Distance /*value*/)
{
    VERIFY(!is_opened_empty());
    u32 node_bucket_id = compute_bucket_id(vertex);
    if (vertex.prev())
        vertex.prev()->next() = vertex.next();
    else
    {
        VERIFY(m_buckets[vertex.m_bucket_id] == &vertex);
        m_buckets[vertex.m_bucket_id] = vertex.next();
    }
    if (vertex.next())
        vertex.next()->prev() = vertex.prev();
    verify_buckets();
    add_to_bucket(vertex, node_bucket_id);
    verify_buckets();
}

TEMPLATE_SPECIALIZATION
inline void CBucketList::remove_best_opened()
{
    VERIFY(!is_opened_empty());
    verify_buckets();
    VERIFY(m_buckets[m_min_bucket_id] && this->is_visited(m_buckets[m_min_bucket_id]->index()));
    m_buckets[m_min_bucket_id] = m_buckets[m_min_bucket_id]->next();
    if (m_buckets[m_min_bucket_id])
        m_buckets[m_min_bucket_id]->prev() = 0;
    verify_buckets();
}

TEMPLATE_SPECIALIZATION
inline typename TManagerDataStorage::Vertex& CBucketList::get_best()
{
    VERIFY(!is_opened_empty());
    return (*m_buckets[m_min_bucket_id]);
}

TEMPLATE_SPECIALIZATION
inline void CBucketList::set_min_bucket_value(const typename TManagerDataStorage::Vertex::Distance min_bucket_value)
{
    m_min_bucket_value = min_bucket_value;
}

TEMPLATE_SPECIALIZATION
inline void CBucketList::set_max_bucket_value(const typename TManagerDataStorage::Vertex::Distance max_bucket_value)
{
    m_max_bucket_value = max_bucket_value;
}

#undef TEMPLATE_SPECIALIZATION
#undef CBucketList
