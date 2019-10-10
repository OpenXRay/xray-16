#include "stdafx.h"

void IndexStagingBuffer::Create(size_t size)
{
    m_HostData = xr_alloc<u8>(size);
    m_Size = size;

    u32 dwUsage = D3DUSAGE_WRITEONLY;
    if (HW.Caps.geometry.bSoftware)
        dwUsage |= D3DUSAGE_SOFTWAREPROCESSING;
    R_CHK(HW.pDevice->CreateIndexBuffer(size, dwUsage, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_DeviceBuffer, NULL));
}

void* IndexStagingBuffer::GetHostPointer() const
{
    VERIFY(m_DeviceBuffer);
    R_CHK(m_DeviceBuffer->Lock(0, 0, const_cast<void**>(&m_HostData), 0));
    return m_HostData;
}

void IndexStagingBuffer::Flush()
{
    // Upload data to device
    R_CHK(m_DeviceBuffer->Unlock());
    HW.stats_manager.increment_stats_ib(m_DeviceBuffer);
    // Free host memory
    xr_delete(m_HostData);
    m_HostData = nullptr;
}

ID3DIndexBuffer* IndexStagingBuffer::GetBufferHandle() const
{
    return m_DeviceBuffer;
}

void IndexStagingBuffer::Destroy()
{
    if (m_HostData)
        xr_delete(m_HostData);
    HW.stats_manager.decrement_stats_ib(m_DeviceBuffer);
    _RELEASE(m_DeviceBuffer);
}
