#include "stdafx.h"

u32 GetFVFVertexSize(u32 FVF)
{
    return D3DXGetFVFVertexSize(FVF);
}

u32 GetDeclVertexSize(const D3DVERTEXELEMENT9* decl, DWORD Stream)
{
    return D3DXGetDeclVertexSize(decl, Stream);
}

u32 GetDeclLength(const D3DVERTEXELEMENT9* decl)
{
    return D3DXGetDeclLength(decl);
}

//-----------------------------------------------------------------------------
void VertexStagingBuffer::Create(size_t size)
{
    m_Size = size;

    u32 dwUsage = D3DUSAGE_WRITEONLY;
    if (HW.Caps.geometry.bSoftware)
        dwUsage |= D3DUSAGE_SOFTWAREPROCESSING;
    R_CHK(HW.pDevice->CreateVertexBuffer(size, dwUsage, 0, D3DPOOL_MANAGED, &m_DeviceBuffer, nullptr));
}

bool VertexStagingBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}

void* VertexStagingBuffer::GetHostPointer() const
{
    VERIFY(m_DeviceBuffer);
    R_CHK(m_DeviceBuffer->Lock(0, 0, const_cast<void**>(&m_HostData), 0));
    return m_HostData;
}

void VertexStagingBuffer::Flush()
{
    // Upload data to device
    R_CHK(m_DeviceBuffer->Unlock());
    HW.stats_manager.increment_stats_vb(m_DeviceBuffer);
    // Free host memory
    m_HostData = nullptr;
}

ID3DVertexBuffer* VertexStagingBuffer::GetBufferHandle() const
{
    return m_DeviceBuffer;
}

void VertexStagingBuffer::Destroy()
{
    HW.stats_manager.decrement_stats_vb(m_DeviceBuffer);
    _RELEASE(m_DeviceBuffer);
    m_DeviceBuffer = nullptr;
}

//-----------------------------------------------------------------------------
void IndexStagingBuffer::Create(size_t size)
{
    m_Size = size;

    u32 dwUsage = D3DUSAGE_WRITEONLY;
    if (HW.Caps.geometry.bSoftware)
        dwUsage |= D3DUSAGE_SOFTWAREPROCESSING;
    R_CHK(HW.pDevice->CreateIndexBuffer(size, dwUsage, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_DeviceBuffer, NULL));
}

bool IndexStagingBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
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
    m_HostData = nullptr;
}

ID3DIndexBuffer* IndexStagingBuffer::GetBufferHandle() const
{
    return m_DeviceBuffer;
}

void IndexStagingBuffer::Destroy()
{
    HW.stats_manager.decrement_stats_ib(m_DeviceBuffer);
    _RELEASE(m_DeviceBuffer);
    m_DeviceBuffer = nullptr;
}
