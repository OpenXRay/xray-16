#include "stdafx.h"

enum
{
    LOCKFLAGS_FLUSH  = D3DLOCK_DISCARD,
    LOCKFLAGS_APPEND = D3DLOCK_NOOVERWRITE,
};

u32 GetFVFVertexSize(u32 FVF)
{
    u32 offset = 0;

    // Position attribute
    if (FVF & D3DFVF_XYZRHW)
        offset += sizeof(Fvector4);
    else if (FVF & D3DFVF_XYZ)
        offset += sizeof(Fvector);

    // Diffuse color attribute
    if (FVF & D3DFVF_DIFFUSE)
        offset += sizeof(u32);

    // Specular color attribute
    if (FVF & D3DFVF_SPECULAR)
        offset += sizeof(u32);

    // Texture coordinates
    for (u32 i = 0; i < (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT; i++)
    {
        u32 size = 2;
        if (FVF & D3DFVF_TEXCOORDSIZE1(i))
            size = 1;
        if (FVF & D3DFVF_TEXCOORDSIZE3(i))
            size = 3;
        if (FVF & D3DFVF_TEXCOORDSIZE4(i))
            size = 4;

        offset += size * sizeof(float);
    }

    return offset;
}

const u32 VertexSizeList[] =
{
    1, // D3DDECLTYPE_FLOAT1
    2, // D3DDECLTYPE_FLOAT2
    3, // D3DDECLTYPE_FLOAT3
    4, // D3DDECLTYPE_FLOAT4
    4, // D3DDECLTYPE_D3DCOLOR
    4, // D3DDECLTYPE_UBYTE4
    2, // D3DDECLTYPE_SHORT2
    4, // D3DDECLTYPE_SHORT4
    4, // D3DDECLTYPE_UBYTE4N
    2, // D3DDECLTYPE_SHORT2N
    4, // D3DDECLTYPE_SHORT4N
    2, // D3DDECLTYPE_USHORT2N
    4, // D3DDECLTYPE_USHORT4N
    1, // D3DDECLTYPE_UDEC3
    1, // D3DDECLTYPE_DEC3N
    2, // D3DDECLTYPE_FLOAT16_2
    4 // D3DDECLTYPE_FLOAT16_4
};

const u32 VertexTypeSizeList[] =
{
    sizeof(FLOAT), // D3DDECLTYPE_FLOAT1
    sizeof(FLOAT), // D3DDECLTYPE_FLOAT2
    sizeof(FLOAT), // D3DDECLTYPE_FLOAT3
    sizeof(FLOAT), // D3DDECLTYPE_FLOAT4
    sizeof(BYTE), // D3DDECLTYPE_D3DCOLOR
    sizeof(BYTE), // D3DDECLTYPE_UBYTE4
    sizeof(SHORT), // D3DDECLTYPE_SHORT2
    sizeof(SHORT), // D3DDECLTYPE_SHORT4
    sizeof(BYTE), // D3DDECLTYPE_UBYTE4N
    sizeof(SHORT), // D3DDECLTYPE_SHORT2N
    sizeof(SHORT), // D3DDECLTYPE_SHORT4N
    sizeof(USHORT), // D3DDECLTYPE_USHORT2N
    sizeof(USHORT), // D3DDECLTYPE_USHORT4N
    sizeof(UINT), // D3DDECLTYPE_UDEC3
    sizeof(INT), // D3DDECLTYPE_DEC3N
    sizeof(DirectX::PackedVector::HALF), // D3DDECLTYPE_FLOAT16_2
    sizeof(DirectX::PackedVector::HALF) // D3DDECLTYPE_FLOAT16_4
};

u32 GetDeclVertexSize(const VertexElement* decl, u32 Stream)
{
    u32 size = 0;
    for (int i = 0; i < MAXD3DDECLLENGTH; ++i)
    {
        const D3DVERTEXELEMENT9& desc = decl[i];

        if (desc.Stream == 0xFF)
            break;

        size += VertexSizeList[desc.Type] * VertexTypeSizeList[desc.Type];
    }
    return size;
}

u32 GetDeclLength(const VertexElement* decl)
{
    const D3DVERTEXELEMENT9* element;

    for (element = decl; element->Stream != 0xff; ++element);

    return element - decl;
}

static void AppendElement(VertexElement* declaration, UINT* idx, UINT* offset,
    D3DDECLTYPE type, D3DDECLUSAGE usage, UINT usage_idx)
{
    declaration[*idx].Stream = 0;
    declaration[*idx].Offset = *offset;
    declaration[*idx].Type = type;
    declaration[*idx].Method = D3DDECLMETHOD_DEFAULT;
    declaration[*idx].Usage = usage;
    declaration[*idx].UsageIndex = usage_idx;

    *offset += VertexSizeList[type] * VertexTypeSizeList[type];
    ++(*idx);
}


bool DeclaratorFromFVF(u32 fvf, VertexElement* decl)
{
    static const VertexElement end_element = D3DDECL_END();
    DWORD tex_count = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    unsigned int offset = 0;
    unsigned int idx = 0;
    unsigned int i;

    if (fvf & (D3DFVF_RESERVED0 | D3DFVF_RESERVED2)) return false;

    if (fvf & D3DFVF_POSITION_MASK)
    {
        BOOL has_blend = (fvf & D3DFVF_XYZB5) >= D3DFVF_XYZB1;
        DWORD blend_count = 1 + (((fvf & D3DFVF_XYZB5) - D3DFVF_XYZB1) >> 1);
        BOOL has_blend_idx = (fvf & D3DFVF_LASTBETA_D3DCOLOR) || (fvf & D3DFVF_LASTBETA_UBYTE4);

        if (has_blend_idx) --blend_count;

        if ((fvf & D3DFVF_POSITION_MASK) == D3DFVF_XYZW
            || (has_blend && blend_count > 4))
            return false;

        if ((fvf & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)
            AppendElement(decl, &idx, &offset, D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_POSITIONT, 0);
        else
            AppendElement(decl, &idx, &offset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0);

        if (has_blend)
        {
            switch (blend_count)
            {
            case 0:
                break;
            case 1:
                AppendElement(decl, &idx, &offset, D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_BLENDWEIGHT, 0);
                break;
            case 2:
                AppendElement(decl, &idx, &offset, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_BLENDWEIGHT, 0);
                break;
            case 3:
                AppendElement(decl, &idx, &offset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_BLENDWEIGHT, 0);
                break;
            case 4:
                AppendElement(decl, &idx, &offset, D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_BLENDWEIGHT, 0);
                break;
            default:
                return false;
                break;
            }

            if (has_blend_idx)
            {
                if (fvf & D3DFVF_LASTBETA_UBYTE4)
                    AppendElement(decl, &idx, &offset, D3DDECLTYPE_UBYTE4, D3DDECLUSAGE_BLENDINDICES, 0);
                else if (fvf & D3DFVF_LASTBETA_D3DCOLOR)
                    AppendElement(decl, &idx, &offset, D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_BLENDINDICES, 0);
            }
        }
    }

    if (fvf & D3DFVF_NORMAL)
        AppendElement(decl, &idx, &offset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_NORMAL, 0);
    if (fvf & D3DFVF_PSIZE)
        AppendElement(decl, &idx, &offset, D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_PSIZE, 0);
    if (fvf & D3DFVF_DIFFUSE)
        AppendElement(decl, &idx, &offset, D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 0);
    if (fvf & D3DFVF_SPECULAR)
        AppendElement(decl, &idx, &offset, D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 1);

    for (i = 0; i < tex_count; ++i)
    {
        switch ((fvf >> (16 + 2 * i)) & 0x03)
        {
        case D3DFVF_TEXTUREFORMAT1:
            AppendElement(decl, &idx, &offset, D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_TEXCOORD, i);
            break;
        case D3DFVF_TEXTUREFORMAT2:
            AppendElement(decl, &idx, &offset, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, i);
            break;
        case D3DFVF_TEXTUREFORMAT3:
            AppendElement(decl, &idx, &offset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_TEXCOORD, i);
            break;
        case D3DFVF_TEXTUREFORMAT4:
            AppendElement(decl, &idx, &offset, D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_TEXCOORD, i);
            break;
        }
    }

    decl[idx] = end_element;

    return true;
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

    u32 mapMode = read ? D3DLOCK_READONLY : 0;
    R_CHK(m_DeviceBuffer->Lock(offset, size, const_cast<void**>(&m_HostBuffer), mapMode));
    return m_HostBuffer;
}

void VertexStagingBuffer::Unmap(bool /*doFlush = false*/)
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

void IndexStagingBuffer::Create(size_t size, bool allowReadBack /*= false*/, bool managed /*= true*/)
{
    m_Size = size;
    m_AllowReadBack = allowReadBack;

    u32 dwUsage = m_AllowReadBack ? 0 : D3DUSAGE_WRITEONLY;
    if (HW.Caps.geometry.bSoftware)
        dwUsage |= D3DUSAGE_SOFTWAREPROCESSING;
    R_CHK(HW.pDevice->CreateIndexBuffer(size, dwUsage, D3DFMT_INDEX16, managed ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT, &m_DeviceBuffer, NULL));

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

    u32 mapMode = read ? D3DLOCK_READONLY : 0;
    R_CHK(m_DeviceBuffer->Lock(offset, size, const_cast<void**>(&m_HostBuffer), mapMode));
    return m_HostBuffer;
}

void IndexStagingBuffer::Unmap(bool /*doFlush = false*/)
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

//-----------------------------------------------------------------------------
VertexStreamBuffer::VertexStreamBuffer()
    : m_DeviceBuffer(nullptr)
{
}

VertexStreamBuffer::~VertexStreamBuffer()
{
    Destroy();
}

void VertexStreamBuffer::Create(size_t size)
{
    R_CHK(HW.pDevice->CreateVertexBuffer(
        size,
        D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
        0,
        D3DPOOL_DEFAULT,
        &m_DeviceBuffer,
        NULL));
    VERIFY(m_DeviceBuffer);
    AddRef();
    HW.stats_manager.increment_stats_vb(m_DeviceBuffer);
}

void VertexStreamBuffer::Destroy()
{
    if (m_DeviceBuffer == nullptr)
        return;

    HW.stats_manager.decrement_stats_vb(m_DeviceBuffer);
    _RELEASE(m_DeviceBuffer);
}

void* VertexStreamBuffer::Map(size_t offset, size_t size, bool flush /*= false*/)
{
    VERIFY(m_DeviceBuffer);

    void *pData = nullptr;
    const auto flags = flush ? LOCKFLAGS_FLUSH : LOCKFLAGS_APPEND;
    R_CHK(m_DeviceBuffer->Lock(offset, size, &pData, flags));
    return pData;
}

void VertexStreamBuffer::Unmap()
{
    VERIFY(m_DeviceBuffer);
    m_DeviceBuffer->Unlock();
}

bool VertexStreamBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}

//-----------------------------------------------------------------------------
IndexStreamBuffer::IndexStreamBuffer()
    : m_DeviceBuffer(nullptr)
{
}

IndexStreamBuffer::~IndexStreamBuffer()
{
    Destroy();
}

void IndexStreamBuffer::Create(size_t size)
{
    R_CHK(HW.pDevice->CreateIndexBuffer(
        size,
        D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &m_DeviceBuffer,
        NULL));
    VERIFY(m_DeviceBuffer);
    AddRef();
    HW.stats_manager.increment_stats_ib(m_DeviceBuffer);
}

void IndexStreamBuffer::Destroy()
{
    if (m_DeviceBuffer == nullptr)
        return;

    HW.stats_manager.decrement_stats_ib(m_DeviceBuffer);
    _RELEASE(m_DeviceBuffer);
}

void* IndexStreamBuffer::Map(size_t offset, size_t size, bool flush /*= false*/)
{
    VERIFY(m_DeviceBuffer);

    void *pData = nullptr;
    const auto flags = flush ? LOCKFLAGS_FLUSH : LOCKFLAGS_APPEND;
    m_DeviceBuffer->Lock(offset, size, &pData, flags);
    return pData;
}

void IndexStreamBuffer::Unmap()
{
    VERIFY(m_DeviceBuffer);
    m_DeviceBuffer->Unlock();
}

bool IndexStreamBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}
