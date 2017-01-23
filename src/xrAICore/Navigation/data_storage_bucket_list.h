////////////////////////////////////////////////////////////////////////////
//  Module      : data_storage_bucket_list.h
//  Created     : 21.03.2002
//  Modified    : 26.02.2004
//  Author      : Dmitriy Iassenev
//  Description : Bucket list data storage
////////////////////////////////////////////////////////////////////////////

#pragma once

template <
    typename TPathId,
    typename TBucketId,
    u32 BucketCount,
    bool ClearBuckets
>
struct CDataStorageBucketList
{
    template <typename TCompoundVertex>
    struct VertexData
    {
        TCompoundVertex *_next;
        TCompoundVertex *_prev;
        TPathId m_path_id;
        TBucketId m_bucket_id;
        TCompoundVertex *&next() { return _next; }
        TCompoundVertex *&prev() { return _prev; }
    };

    template <typename TManagerDataStorage>
    class CDataStorage : public TManagerDataStorage
    {
    public:
        using Inherited = TManagerDataStorage;
        using Vertex = typename TManagerDataStorage::Vertex;
        using Distance = typename Vertex::Distance;
        using Index = typename Vertex::Index;

    protected:
        Distance m_max_distance;
        Vertex m_list_data[2];
        Vertex *m_list_head;
        Vertex *m_list_tail;
        Distance m_min_bucket_value;
        Distance m_max_bucket_value;
        Vertex *m_buckets[BucketCount];
        u32 m_min_bucket_id;

    public:
        inline CDataStorage(const u32 vertex_count);
        virtual ~CDataStorage();
        inline void init();
        inline void add_best_closed();
        inline bool is_opened_empty();
        inline u32 compute_bucket_id(Vertex &vertex) const;
        inline void verify_buckets() const;
        inline void add_to_bucket(Vertex &vertex, u32 bucket_id);
        inline void add_opened(Vertex &vertex);
        inline void decrease_opened(Vertex &vertex, const Distance value);
        inline void remove_best_opened();
        inline Vertex &get_best();
        inline void set_min_bucket_value(const Distance min_bucket_value);
        inline void set_max_bucket_value(const Distance max_bucket_value);
    };
};

#include "xrAICore/Navigation/data_storage_bucket_list_inline.h"
