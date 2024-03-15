#ifndef __CONSTANT_ALLOCATOR_H__
#define __CONSTANT_ALLOCATOR_H__

#include <numeric>

//////////////////////////////////////////////////////////////////////////
// Partition based allocator for constant buffers of roughly the same size
struct PartitionAllocator
{
    ID3DBuffer* m_buffer;
    void* m_base_ptr;
    uint32_t m_page_size;
    uint32_t m_bucket_size;
    uint32_t m_partition;
    uint32_t m_capacity;

    std::vector<uint32_t> m_table;
    std::vector<uint32_t> m_remap;

    PartitionAllocator(ID3DBuffer* buffer, void* base_ptr, size_t page_size, size_t bucket_size)
        : m_buffer(buffer), m_base_ptr(base_ptr), m_page_size((uint32_t)page_size),
          m_bucket_size((uint32_t)bucket_size), m_partition(0), m_capacity(page_size / bucket_size), m_table(),
          m_remap()
    {
        m_table.resize(page_size / bucket_size);
        m_remap.resize(page_size / bucket_size);
        std::iota(m_table.begin(), m_table.end(), 0);
    }
   
    ~PartitionAllocator()
    {
        //R_ASSERT(m_partition == 0);
        SAFE_RELEASE(m_buffer); 
    }

    ID3DBuffer* buffer() const { return m_buffer; }
    void* base_ptr() const { return m_base_ptr; }
    bool empty() const { return m_partition == 0; }

    uint32_t allocate()
    {
        size_t key = ~0u;
        if (m_partition + 1 >= m_capacity)
        {
            return ~0u;
        }
        uint32_t storage_index = m_table[key = m_partition++];
        m_remap[storage_index] = key;
        return storage_index;
    }

    void deallocate(size_t key)
    {
        R_ASSERT(m_partition && key < m_remap.size());
        uint32_t roster_index = m_remap[key];
        std::swap(m_table[roster_index], m_table[--m_partition]);
        std::swap(m_remap[key], m_remap[m_table[roster_index]]);
    }
};

#if USE_DX12
enum PoolConfig
{
    POOL_STAGING_COUNT = 1,
    POOL_ALIGNMENT = 128,
    POOL_FRAME_QUERY_COUNT = 4,
    POOL_MAX_ALLOCATION_SIZE = 64 << 20,
    POOL_FRAME_QUERY_MASK = POOL_FRAME_QUERY_COUNT - 1
};

class dx11ConstantBufferAllocator
{
    const u32 m_cb_bank_size = 0;
    const u32 m_cb_threshold = 0;

    // The page buckets
    typedef std::vector<PartitionAllocator*> PageBucketsT;
    PageBucketsT m_page_buckets[18];

    // The retired allocations
    typedef std::pair<PartitionAllocator*, uint16_t> RetiredSlot;
    std::vector<RetiredSlot> m_retired_slots[PoolConfig::POOL_FRAME_QUERY_COUNT];

    // Device fences issues at the end of a frame
    u64 m_fences[PoolConfig::POOL_FRAME_QUERY_COUNT];

    // Current frameid
    uint32_t m_frameid;

    // The number of allocate pages
    uint32_t m_pages;

public:

    dx11ConstantBufferAllocator();    
    ~dx11ConstantBufferAllocator();

    void ReleaseEmptyBanks();

    bool Initialize();
    bool Shutdown();

    bool Allocate(class dx11ConstantBuffer* cbuffer);
    void Free(class dx11ConstantBuffer* cbuffer);
    void Update(uint32_t frame_id, u64 fence);
};

#endif
#endif
