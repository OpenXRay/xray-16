#include "stdafx.h"
#include "dx11ConstantAllocator.h"

#if CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS 

#define r_constantbuffer_banksize 4
#define r_constantbuffer_watermark 64

static void ExtractBasePointer(ID3DBuffer* buffer, uint8_t*& base_ptr)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
#if USE_DX12
    HRESULT hr = HW.get_context(CHW::IMM_CTX_ID)->Map(buffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource);
#else
    HRESULT hr = HW.get_context(CHW::IMM_CTX_ID)->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
#endif
    if (hr != S_OK)
    {
        R_ASSERT2(0, "Could not create constant buffer resource!");
        return;
    }
    base_ptr = (uint8_t*)mappedResource.pData;
    HW.get_context(CHW::IMM_CTX_ID)->Unmap(buffer, 0);
}

/////////////////////////////////////////////////////////////////////////////////////

dx11ConstantBufferAllocator::dx11ConstantBufferAllocator()
    : m_frameid()
    , m_cb_bank_size(NextPower2(r_constantbuffer_banksize) << 20)
    , m_cb_threshold(NextPower2(r_constantbuffer_watermark) << 20)
    , m_pages()
{
    memset(m_fences, 0, sizeof(m_fences));
}

dx11ConstantBufferAllocator::~dx11ConstantBufferAllocator()
{
}

/////////////////////////////////////////////////////////////////////////////////////

void dx11ConstantBufferAllocator::ReleaseEmptyBanks()
{
    if (m_pages * m_cb_bank_size <= m_cb_threshold)
    {
        return;
    }

    for (size_t i = 0; i < 16; ++i)
    {
        for (PageBucketsT::iterator j = m_page_buckets[i].begin(), end = m_page_buckets[i].end(); j != end;)
        {
            if ((*j)->empty())
            {
                delete *j;
                --m_pages;
                j = m_page_buckets[i].erase(j);
                end = m_page_buckets[i].end();
            }
            else
            {
                ++j;
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////

bool dx11ConstantBufferAllocator::Initialize() 
{
    return true; 
}

bool dx11ConstantBufferAllocator::Shutdown()
{
    for (size_t i = 0; i < PoolConfig::POOL_FRAME_QUERY_COUNT; ++i)
    {
        for (size_t j = 0; j < m_retired_slots[i].size(); ++j)
        {
            const RetiredSlot& slot = m_retired_slots[i][j];
            slot.first->deallocate(slot.second);
        }
        m_retired_slots[i].clear();
    }

    for (size_t i = 0; i < 16; ++i)
    {
        for (size_t j = 0; j < m_page_buckets[i].size(); ++j)
        {
            delete m_page_buckets[i][j];
        }
        m_page_buckets[i].clear();
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////

bool dx11ConstantBufferAllocator::Allocate(dx11ConstantBuffer* cbuffer)
{
    const unsigned size = cbuffer->m_size;
    const unsigned nsize = NextPower2(size);
    const unsigned bucket = IntegerLog2(nsize) - 8;
    bool failed = false;
retry:
    for (size_t i = m_page_buckets[bucket].size(); i > 0; --i)
    {
        unsigned key = m_page_buckets[bucket][i - 1]->allocate();
        if (key != ~0u)
        {
            cbuffer->m_buffer = m_page_buckets[bucket][i - 1]->buffer();
            cbuffer->m_base_ptr = m_page_buckets[bucket][i - 1]->base_ptr();
            cbuffer->m_offset = key * nsize;
            cbuffer->m_allocator = reinterpret_cast<void*>(m_page_buckets[bucket][i - 1]);
            return true;
        }
    }
    if (!failed)
    {
        uint8_t* base_ptr = nullptr;
        ++m_pages;
        ID3DBuffer* buffer = NULL;
        if (BufferUtils::CreateConstantBuffer(&buffer, m_cb_bank_size) != S_OK)
        {
            Log("failed to create constant buffer pool");
            return false;
        }
        ExtractBasePointer(buffer, base_ptr);
        m_page_buckets[bucket].push_back(new PartitionAllocator(buffer, base_ptr, m_cb_bank_size, nsize));
        failed = true;
        goto retry;
    }
    return false;
}

void dx11ConstantBufferAllocator::Free(dx11ConstantBuffer* cbuffer)
{
    const unsigned size = cbuffer->m_size;
    const unsigned nsize = NextPower2(size);
    const unsigned bucket = IntegerLog2(nsize) - 8;
    PartitionAllocator* allocator = reinterpret_cast<PartitionAllocator*>(cbuffer->m_allocator);
    m_retired_slots[m_frameid].push_back(std::make_pair(allocator, (uint16_t)(cbuffer->m_offset >> (bucket + 8))));
}

void dx11ConstantBufferAllocator::Update(uint32_t frame_id, u64 fence)
{
    m_frameid = frame_id & PoolConfig::POOL_FRAME_QUERY_MASK;
    
    for (size_t i = m_frameid; i < m_frameid + PoolConfig::POOL_FRAME_QUERY_COUNT; ++i)
    {
         size_t idx = i & PoolConfig::POOL_FRAME_QUERY_MASK;
         if (m_fences[idx] && HW.SyncFence(m_fences[idx], true) == S_OK)
         {
             for (size_t j = 0, end = m_retired_slots[idx].size(); j < end; ++j)
             {
                 const RetiredSlot& slot = m_retired_slots[idx][j];
                 slot.first->deallocate(slot.second);
             }
             m_retired_slots[idx].clear();
         }
    }
     
    m_fences[m_frameid] = fence;
}

#endif
