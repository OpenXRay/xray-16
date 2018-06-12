////////////////////////////////////////////////////////////////////////////
//  Module      : data_storage_bucket_list.h
//  Created     : 21.03.2002
//  Modified    : 26.02.2004
//  Author      : Dmitriy Iassenev
//  Description : Bucket list data storage
////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename TPathId, typename TBucketId, u32 BucketCount, bool ClearBuckets>
struct CDataStorageBucketList
{
    template <typename TCompoundVertex>
    struct VertexData
    {
        TCompoundVertex* _next;
        TCompoundVertex* _prev;
        TPathId m_path_id;
        TBucketId m_bucket_id;
        TCompoundVertex*& next() { return _next; }
        TCompoundVertex*& prev() { return _prev; }
    };

    template <typename TManagerDataStorage>
    class CDataStorage : public TManagerDataStorage
    {
    protected:
        typename TManagerDataStorage::Vertex::Distance m_max_distance;
        typename TManagerDataStorage::Vertex m_list_data[2];
        typename TManagerDataStorage::Vertex* m_list_head;
        typename TManagerDataStorage::Vertex* m_list_tail;
        typename TManagerDataStorage::Vertex::Distance m_min_bucket_value;
        typename TManagerDataStorage::Vertex::Distance m_max_bucket_value;
        typename TManagerDataStorage::Vertex* m_buckets[BucketCount];
        u32 m_min_bucket_id;

    public:
        inline CDataStorage(const u32 vertex_count);
        virtual ~CDataStorage();
        inline void init();
        inline void add_best_closed();
        inline bool is_opened_empty();
        inline u32 compute_bucket_id(typename TManagerDataStorage::Vertex& vertex) const;
        inline void verify_buckets() const;
        inline void add_to_bucket(typename TManagerDataStorage::Vertex& vertex, u32 bucket_id);
        inline void add_opened(typename TManagerDataStorage::Vertex& vertex);
        inline void decrease_opened(typename TManagerDataStorage::Vertex& vertex, const typename TManagerDataStorage::Vertex::Distance value);
        inline void remove_best_opened();
        inline typename TManagerDataStorage::Vertex& get_best();
        inline void set_min_bucket_value(const typename TManagerDataStorage::Vertex::Distance min_bucket_value);
        inline void set_max_bucket_value(const typename TManagerDataStorage::Vertex::Distance max_bucket_value);
    };
};

#include "xrAICore/Navigation/data_storage_bucket_list_inline.h"
