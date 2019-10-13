#include "stdafx.h"

u32 GetFVFVertexSize(u32 FVF)
{
    return D3DXGetFVFVertexSize(FVF);
}

u32 GetDeclVertexSize(const VertexElement* decl, DWORD Stream)
{
    return D3DXGetDeclVertexSize(decl, Stream);
}

u32 GetDeclLength(const VertexElement* decl)
{
    return D3DXGetDeclLength(decl);
}

//-----------------------------------------------------------------------------
VertexStagingBuffer::VertexStagingBuffer()
    : m_DeviceBuffer{ nullptr }
{
}

VertexStagingBuffer::~VertexStagingBuffer()
{
    Destroy();
}

void VertexStagingBuffer::Create(size_t size)
{
    m_Size = size;

    u32 dwUsage = D3DUSAGE_WRITEONLY;
    if (HW.Caps.geometry.bSoftware)
        dwUsage |= D3DUSAGE_SOFTWAREPROCESSING;
    R_CHK(HW.pDevice->CreateVertexBuffer(size, dwUsage, 0, D3DPOOL_MANAGED, &m_DeviceBuffer, nullptr));
    AddRef();
}

bool VertexStagingBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}

void* VertexStagingBuffer::GetHostPointer()
{
    VERIFY(IsValid());
    R_CHK(m_DeviceBuffer->Lock(0, 0, const_cast<void**>(&m_HostData), 0));
    return m_HostData;
}

void VertexStagingBuffer::Flush()
{
    VERIFY(IsValid());
    // Upload data to device
    R_CHK(m_DeviceBuffer->Unlock());
    HW.stats_manager.increment_stats_vb(m_DeviceBuffer);
    // Free host memory
    m_HostData = nullptr;
}

VertexBufferHandle VertexStagingBuffer::GetBufferHandle() const
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
IndexStagingBuffer::IndexStagingBuffer()
    : m_DeviceBuffer{ nullptr }
{
}

IndexStagingBuffer::~IndexStagingBuffer()
{
    Destroy();
}

void IndexStagingBuffer::Create(size_t size)
{
    m_Size = size;

    u32 dwUsage = D3DUSAGE_WRITEONLY;
    if (HW.Caps.geometry.bSoftware)
        dwUsage |= D3DUSAGE_SOFTWAREPROCESSING;
    R_CHK(HW.pDevice->CreateIndexBuffer(size, dwUsage, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_DeviceBuffer, NULL));
    AddRef();
}

bool IndexStagingBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}

void* IndexStagingBuffer::GetHostPointer()
{
    VERIFY(IsValid());
    R_CHK(m_DeviceBuffer->Lock(0, 0, const_cast<void**>(&m_HostData), 0));
    return m_HostData;
}

void IndexStagingBuffer::Flush()
{
    VERIFY(IsValid());
    // Upload data to device
    R_CHK(m_DeviceBuffer->Unlock());
    HW.stats_manager.increment_stats_ib(m_DeviceBuffer);
    // Free host memory
    m_HostData = nullptr;
}

IndexBufferHandle IndexStagingBuffer::GetBufferHandle() const
{
    return m_DeviceBuffer;
}

void IndexStagingBuffer::Destroy()
{
    HW.stats_manager.decrement_stats_ib(m_DeviceBuffer);
    _RELEASE(m_DeviceBuffer);
    m_DeviceBuffer = nullptr;
}
