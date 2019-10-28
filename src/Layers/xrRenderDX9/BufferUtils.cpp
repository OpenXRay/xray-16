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
    , m_HostBuffer{ nullptr }
{
}

VertexStagingBuffer::~VertexStagingBuffer()
{
    Destroy();
}

void VertexStagingBuffer::Create(size_t size, bool allowReadBack /*= false*/)
{
    m_Size = size;
    m_AllowReadBack = allowReadBack;

    u32 dwUsage = allowReadBack ? 0 : D3DUSAGE_WRITEONLY;
    if (HW.Caps.geometry.bSoftware)
        dwUsage |= D3DUSAGE_SOFTWAREPROCESSING;
    R_CHK(HW.pDevice->CreateVertexBuffer(size, dwUsage, 0, D3DPOOL_MANAGED, &m_DeviceBuffer, nullptr));
    VERIFY(m_DeviceBuffer);

    HW.stats_manager.increment_stats_vb(m_DeviceBuffer);
    AddRef();
}

bool VertexStagingBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}

void* VertexStagingBuffer::Map(
    size_t offset /*= 0 */,
    size_t size /*= 0 */,
    bool read /*= false*/)
{
    VERIFY(IsValid());
    VERIFY2(!read || m_AllowReadBack, "Can't read from write only buffer");
    VERIFY(size <= m_Size);

    DWORD mapMode = read ? D3DLOCK_READONLY : 0;
    R_CHK(m_DeviceBuffer->Lock(offset, size, const_cast<void**>(&m_HostBuffer), mapMode));
    return m_HostBuffer;
}

void VertexStagingBuffer::Unmap(bool doFlush /*= false*/)
{
    VERIFY(IsValid());
    R_CHK(m_DeviceBuffer->Unlock());
}

void VertexStagingBuffer::DiscardHostBuffer()
{
    /* Do nothing */
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

size_t VertexStagingBuffer::GetSystemMemoryUsage() const
{
    if (IsValid())
    {
        D3DVERTEXBUFFER_DESC desc;
        m_DeviceBuffer->GetDesc(&desc);

        if (desc.Pool == D3DPOOL_MANAGED || desc.Pool == D3DPOOL_SCRATCH)
            return desc.Size;
    }

    return 0;
}

size_t VertexStagingBuffer::GetVideoMemoryUsage() const
{
    if (IsValid())
    {
        D3DVERTEXBUFFER_DESC desc;
        m_DeviceBuffer->GetDesc(&desc);

        if (desc.Pool == D3DPOOL_DEFAULT || desc.Pool == D3DPOOL_MANAGED)
            return desc.Size;
    }

    return 0;
}

//-----------------------------------------------------------------------------
IndexStagingBuffer::IndexStagingBuffer()
    : m_DeviceBuffer{ nullptr }
    , m_HostBuffer{ nullptr }
{
}

IndexStagingBuffer::~IndexStagingBuffer()
{
    Destroy();
}

void IndexStagingBuffer::Create(size_t size, bool allowReadBack /*= false*/)
{
    m_Size = size;
    m_AllowReadBack = allowReadBack;

    u32 dwUsage = m_AllowReadBack ? 0 : D3DUSAGE_WRITEONLY;
    if (HW.Caps.geometry.bSoftware)
        dwUsage |= D3DUSAGE_SOFTWAREPROCESSING;
    R_CHK(HW.pDevice->CreateIndexBuffer(size, dwUsage, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_DeviceBuffer, NULL));

    HW.stats_manager.increment_stats_ib(m_DeviceBuffer);
    AddRef();
}

bool IndexStagingBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}

void* IndexStagingBuffer::Map(
    size_t offset /*= 0 */,
    size_t size /*= 0 */,
    bool read /*= false*/)
{
    VERIFY(IsValid());
    VERIFY2(!read || m_AllowReadBack, "Can't read from write only buffer");
    VERIFY(size <= m_Size);

    DWORD mapMode = read ? D3DLOCK_READONLY : 0;
    R_CHK(m_DeviceBuffer->Lock(offset, size, const_cast<void**>(&m_HostBuffer), mapMode));
    return m_HostBuffer;
}

void IndexStagingBuffer::Unmap(bool doFlush /*= false*/)
{
    VERIFY(IsValid());
    R_CHK(m_DeviceBuffer->Unlock());
}

void IndexStagingBuffer::DiscardHostBuffer()
{
    /* Do nothing */
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

size_t IndexStagingBuffer::GetSystemMemoryUsage() const
{
    if (IsValid())
    {
        D3DINDEXBUFFER_DESC desc;
        m_DeviceBuffer->GetDesc(&desc);

        if (desc.Pool == D3DPOOL_MANAGED || desc.Pool == D3DPOOL_SCRATCH)
            return desc.Size;
    }

    return 0;
}

size_t IndexStagingBuffer::GetVideoMemoryUsage() const
{
    if (IsValid())
    {
        D3DINDEXBUFFER_DESC desc;
        m_DeviceBuffer->GetDesc(&desc);

        if (desc.Pool == D3DPOOL_DEFAULT || desc.Pool == D3DPOOL_MANAGED)
            return desc.Size;
    }

    return 0;
}
