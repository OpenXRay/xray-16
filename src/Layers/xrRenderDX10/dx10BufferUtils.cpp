#include "stdafx.h"
#include "Layers/xrRender/BufferUtils.h"

u32 GetFVFVertexSize(u32 FVF)
{
    return D3DXGetFVFVertexSize(FVF);
}

u32 GetDeclVertexSize(const VertexElement* decl, u32 Stream)
{
    return D3DXGetDeclVertexSize(decl, Stream);
}

u32 GetDeclLength(const VertexElement* decl)
{
    return D3DXGetDeclLength(decl);
}

static HRESULT CreateBuffer(ID3DBuffer** ppBuffer, const void* pData, u32 dataSize,
    bool bDynamic, D3D_BIND_FLAG bufferType)
{
    D3D_BUFFER_DESC desc;
    desc.ByteWidth      = dataSize;
    desc.Usage          = bDynamic ? D3D_USAGE_DYNAMIC : D3D_USAGE_DEFAULT;
    desc.BindFlags      = bufferType;
    desc.CPUAccessFlags = bDynamic ? D3D_CPU_ACCESS_WRITE : 0;
    desc.MiscFlags      = 0;

    D3D_SUBRESOURCE_DATA subData;
    subData.pSysMem = pData;

    HRESULT res = HW.pDevice->CreateBuffer(
        &desc,
        pData ? &subData : nullptr,
        ppBuffer);

    return res;
}

static inline HRESULT CreateVertexBuffer(VertexBufferHandle* ppBuffer, const void* pData, u32 dataSize, bool bDynamic)
{
    return CreateBuffer(ppBuffer, pData, dataSize, bDynamic, D3D_BIND_VERTEX_BUFFER);
}

static inline HRESULT CreateIndexBuffer(IndexBufferHandle* ppBuffer, const void* pData, u32 dataSize, bool bDynamic)
{
    return CreateBuffer(ppBuffer, pData, dataSize, bDynamic, D3D_BIND_INDEX_BUFFER);
}

static inline HRESULT CreateConstantBuffer(ConstantBufferHandle* ppBuffer, u32 dataSize)
{
    return CreateBuffer(ppBuffer, nullptr, dataSize, true, D3D_BIND_CONSTANT_BUFFER);
}

namespace BufferUtils
{
// TODO: replace by streaming buffer instance in `dx10ConstantBuffer`
HRESULT CreateConstantBuffer(ConstantBufferHandle* ppBuffer, u32 DataSize)
{
    return ::CreateConstantBuffer(ppBuffer, DataSize);
}
};

struct VertexFormatPairs
{
    D3DDECLTYPE m_dx9FMT;
    DXGI_FORMAT m_dx10FMT;
};

VertexFormatPairs VertexFormatList[] = {{D3DDECLTYPE_FLOAT1, DXGI_FORMAT_R32_FLOAT},
    {D3DDECLTYPE_FLOAT2, DXGI_FORMAT_R32G32_FLOAT}, {D3DDECLTYPE_FLOAT3, DXGI_FORMAT_R32G32B32_FLOAT},
    {D3DDECLTYPE_FLOAT4, DXGI_FORMAT_R32G32B32A32_FLOAT},
    {D3DDECLTYPE_D3DCOLOR,
        DXGI_FORMAT_R8G8B8A8_UNORM}, // Warning. Explicit RGB component swizzling is nesessary	//	Not available
    {D3DDECLTYPE_UBYTE4, DXGI_FORMAT_R8G8B8A8_UINT}, // Note: Shader gets UINT values, but if Direct3D 9 style integral
    // floats are needed (0.0f, 1.0f... 255.f), UINT can just be converted
    // to float32 in shader.
    {D3DDECLTYPE_SHORT2,
        DXGI_FORMAT_R16G16_SINT}, // Note: Shader gets SINT values, but if Direct3D 9 style integral floats
    // are needed, SINT can just be converted to float32 in shader.
    {D3DDECLTYPE_SHORT4,
        DXGI_FORMAT_R16G16B16A16_SINT}, // Note: Shader gets SINT values, but if Direct3D 9 style integral
    // floats are needed, SINT can just be converted to float32 in
    // shader.
    {D3DDECLTYPE_UBYTE4N, DXGI_FORMAT_R8G8B8A8_UNORM},
    {D3DDECLTYPE_SHORT2N, DXGI_FORMAT_R16G16_SNORM}, {D3DDECLTYPE_SHORT4N, DXGI_FORMAT_R16G16B16A16_SNORM},
    {D3DDECLTYPE_USHORT2N, DXGI_FORMAT_R16G16_UNORM}, {D3DDECLTYPE_USHORT4N, DXGI_FORMAT_R16G16B16A16_UNORM},
    // D3DDECLTYPE_UDEC3 Not available
    // D3DDECLTYPE_DEC3N Not available
    {D3DDECLTYPE_FLOAT16_2, DXGI_FORMAT_R16G16_FLOAT}, {D3DDECLTYPE_FLOAT16_4, DXGI_FORMAT_R16G16B16A16_FLOAT}};

DXGI_FORMAT ConvertVertexFormat(D3DDECLTYPE dx9FMT)
{
    size_t arrayLength = sizeof(VertexFormatList) / sizeof(VertexFormatList[0]);
    for (size_t i = 0; i < arrayLength; ++i)
    {
        if (VertexFormatList[i].m_dx9FMT == dx9FMT)
            return VertexFormatList[i].m_dx10FMT;
    }

    VERIFY(!"ConvertVertexFormat didn't find appropriate dx10 vertex format!");
    return DXGI_FORMAT_UNKNOWN;
}

struct VertexSemanticPairs
{
    D3DDECLUSAGE m_dx9Semantic;
    LPCSTR m_dx10Semantic;
};

VertexSemanticPairs VertexSemanticList[] = {
    {D3DDECLUSAGE_POSITION, "POSITION"}, //	0
    {D3DDECLUSAGE_BLENDWEIGHT, "BLENDWEIGHT"}, // 1
    {D3DDECLUSAGE_BLENDINDICES, "BLENDINDICES"}, // 2
    {D3DDECLUSAGE_NORMAL, "NORMAL"}, // 3
    {D3DDECLUSAGE_PSIZE, "PSIZE"}, // 4
    {D3DDECLUSAGE_TEXCOORD, "TEXCOORD"}, // 5
    {D3DDECLUSAGE_TANGENT, "TANGENT"}, // 6
    {D3DDECLUSAGE_BINORMAL, "BINORMAL"}, // 7
    // D3DDECLUSAGE_TESSFACTOR,    // 8
    {D3DDECLUSAGE_POSITIONT, "POSITIONT"}, // 9
    {D3DDECLUSAGE_COLOR, "COLOR"}, // 10
    // D3DDECLUSAGE_FOG,           // 11
    // D3DDECLUSAGE_DEPTH,         // 12
    // D3DDECLUSAGE_SAMPLE,        // 13
};

LPCSTR ConvertSemantic(D3DDECLUSAGE Semantic)
{
    size_t arrayLength = sizeof(VertexSemanticList) / sizeof(VertexSemanticList[0]);
    for (size_t i = 0; i < arrayLength; ++i)
    {
        if (VertexSemanticList[i].m_dx9Semantic == Semantic)
            return VertexSemanticList[i].m_dx10Semantic;
    }

    VERIFY(!"ConvertSemantic didn't find appropriate dx10 input semantic!");
    return 0;
}

void ConvertVertexDeclaration(const xr_vector<D3DVERTEXELEMENT9>& declIn, xr_vector<D3D_INPUT_ELEMENT_DESC>& declOut)
{
    s32 iDeclSize = declIn.size() - 1;
    declOut.resize(iDeclSize + 1);

    for (s32 i = 0; i < iDeclSize; ++i)
    {
        const D3DVERTEXELEMENT9& descIn = declIn[i];
        D3D_INPUT_ELEMENT_DESC& descOut = declOut[i];

        descOut.SemanticName = ConvertSemantic((D3DDECLUSAGE)descIn.Usage);
        descOut.SemanticIndex = descIn.UsageIndex;
        descOut.Format = ConvertVertexFormat((D3DDECLTYPE)descIn.Type);
        descOut.InputSlot = descIn.Stream;
        descOut.AlignedByteOffset = descIn.Offset;
        descOut.InputSlotClass = D3D_INPUT_PER_VERTEX_DATA;
        descOut.InstanceDataStepRate = 0;
    }

    if (iDeclSize >= 0)
        ZeroMemory(&declOut[iDeclSize], sizeof(declOut[iDeclSize]));
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

    m_HostBuffer = xr_alloc<u8>(size);
    AddRef();
}

bool VertexStagingBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}

void* VertexStagingBuffer::Map(
    size_t offset /*= 0*/,
    size_t size /*= 0*/,
    bool read /*= false*/)
{
    VERIFY2(m_HostBuffer, "Buffer wasn't created or already discarded");
    VERIFY2(!read || m_AllowReadBack, "Can't read from write only buffer");
    VERIFY2((size + offset) <= m_Size, "Map region is too large");

    return static_cast<u8*>(m_HostBuffer) + offset;
}

void VertexStagingBuffer::Unmap(bool doFlush /*= false*/)
{
    if (!doFlush)
    {
        /* Do nothing*/
        return;
    }

    VERIFY2(!m_DeviceBuffer, "Attempting to upload buffer twice");
    VERIFY(m_HostBuffer && m_Size);

    // Upload data to device
    R_CHK(CreateVertexBuffer(&m_DeviceBuffer, m_HostBuffer, m_Size, false));
    VERIFY(m_DeviceBuffer);
    HW.stats_manager.increment_stats_vb(m_DeviceBuffer);

    if (!m_AllowReadBack)
    {
        // Cache buffer isn't required anymore. Free host memory
        DiscardHostBuffer();
    }
}

VertexBufferHandle VertexStagingBuffer::GetBufferHandle() const
{
    return m_DeviceBuffer;
}

void VertexStagingBuffer::Destroy()
{
    DiscardHostBuffer();
    m_Size = 0;

    HW.stats_manager.decrement_stats_vb(m_DeviceBuffer);
    _RELEASE(m_DeviceBuffer);
}

void VertexStagingBuffer::DiscardHostBuffer()
{
    if (m_HostBuffer)
        xr_free(m_HostBuffer);
}

size_t VertexStagingBuffer::GetSystemMemoryUsage() const
{
    return m_HostBuffer ? m_Size : 0;
}

size_t VertexStagingBuffer::GetVideoMemoryUsage() const
{
    if (m_DeviceBuffer)
    {
        D3D_BUFFER_DESC desc;
        m_DeviceBuffer->GetDesc(&desc);
        return desc.ByteWidth;
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

void IndexStagingBuffer::Create(size_t size, bool allowReadBack /*= false*/, bool /*managed = true*/)
{
    m_Size = size;
    m_AllowReadBack = allowReadBack;

    m_HostBuffer = xr_alloc<u8>(size);
    AddRef();
}

bool IndexStagingBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}

void* IndexStagingBuffer::Map(
    size_t offset /*= 0*/,
    size_t size /*= 0*/,
    bool read /*= false*/)
{
    VERIFY2(m_HostBuffer, "Buffer wasn't created or already discarded");
    VERIFY2(!read || m_AllowReadBack, "Can't read from write only buffer");
    VERIFY2((size + offset) <= m_Size, "Map region is too large");

    return static_cast<u8*>(m_HostBuffer) + offset;
}

void IndexStagingBuffer::Unmap(bool doFlush /*= false*/)
{
    if (!doFlush)
    {
        /* Do nothing*/
        return;
    }

    VERIFY2(!m_DeviceBuffer, "Attempting to upload buffer twice");
    VERIFY(m_HostBuffer && m_Size);

    // Upload data to device
    R_CHK(CreateIndexBuffer(&m_DeviceBuffer, m_HostBuffer, m_Size, false));
    VERIFY(m_DeviceBuffer);
    HW.stats_manager.increment_stats_ib(m_DeviceBuffer);

    if (!m_AllowReadBack)
    {
        // Cache buffer isn't required anymore. Free host memory
        DiscardHostBuffer();
    }
}

IndexBufferHandle IndexStagingBuffer::GetBufferHandle() const
{
    return m_DeviceBuffer;
}

void IndexStagingBuffer::Destroy()
{
    DiscardHostBuffer();
    m_Size = 0;

    HW.stats_manager.decrement_stats_ib(m_DeviceBuffer);
    _RELEASE(m_DeviceBuffer);
}

void IndexStagingBuffer::DiscardHostBuffer()
{
    if (m_HostBuffer)
        xr_free(m_HostBuffer);
}

size_t IndexStagingBuffer::GetSystemMemoryUsage() const
{
    return m_HostBuffer ? m_Size : 0;
}

size_t IndexStagingBuffer::GetVideoMemoryUsage() const
{
    if (m_DeviceBuffer)
    {
        D3D_BUFFER_DESC desc;
        m_DeviceBuffer->GetDesc(&desc);
        return desc.ByteWidth;
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
    R_CHK(CreateVertexBuffer(&m_DeviceBuffer, nullptr, size, true));
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

void* VertexStreamBuffer::Map(size_t offset, size_t /*size*/, bool flush /*= false*/)
{
    VERIFY(m_DeviceBuffer);

    const auto flag = flush ? D3D_MAP_WRITE_DISCARD : D3D_MAP_WRITE_NO_OVERWRITE;

    D3D11_MAPPED_SUBRESOURCE MappedSubRes;
    HW.pContext->Map(m_DeviceBuffer, 0, flag, 0, &MappedSubRes);

    u8* pData = static_cast<u8*>(MappedSubRes.pData);
    pData += offset;

    return static_cast<void*>(pData);
}

void VertexStreamBuffer::Unmap()
{
    VERIFY(m_DeviceBuffer);
    HW.pContext->Unmap(m_DeviceBuffer, 0);
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
    R_CHK(CreateIndexBuffer(&m_DeviceBuffer, nullptr, size, true));
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

void* IndexStreamBuffer::Map(size_t offset, size_t /*size*/, bool flush /*= false*/)
{
    VERIFY(m_DeviceBuffer);

    const auto flag = flush ? D3D_MAP_WRITE_DISCARD : D3D_MAP_WRITE_NO_OVERWRITE;

    D3D11_MAPPED_SUBRESOURCE MappedSubRes;
    HW.pContext->Map(m_DeviceBuffer, 0, flag, 0, &MappedSubRes);

    u8* pData = static_cast<u8*>(MappedSubRes.pData);
    pData += offset;

    return static_cast<void*>(pData);
}

void IndexStreamBuffer::Unmap()
{
    VERIFY(m_DeviceBuffer);
    HW.pContext->Unmap(m_DeviceBuffer, 0);
}

bool IndexStreamBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}
