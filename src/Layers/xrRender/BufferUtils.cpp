// dx11BufferUtils.cpp - NVRHI-based buffer utilities for D3D12
#include "stdafx.h"
#include "Layers/xrRender/BufferUtils.h"

#include <FlexibleVertexFormat.h>
#include <nvrhi/nvrhi.h>
#include "xrEngine/IRenderBackend.h"  // For DeviceState enum

namespace xray::render::fg
{
u32 GetFVFVertexSize(u32 FVF)
{
    return static_cast<u32>(::FVF::ComputeVertexSize(FVF));
}

u32 GetDeclVertexSize(const VertexElement* decl, u32 Stream)
{
    return static_cast<u32>(::FVF::ComputeVertexSize(reinterpret_cast<const D3DVERTEXELEMENT9*>(decl), Stream));
}

u32 GetDeclLength(const VertexElement* decl)
{
    return static_cast<u32>(::FVF::GetDeclLength(reinterpret_cast<const D3DVERTEXELEMENT9*>(decl)));
}

nvrhi::Format ToNvrhiFormat(u32 vertexFormat)
{
    switch (vertexFormat)
    {
    case VF_FLOAT1:    return nvrhi::Format::R32_FLOAT;
    case VF_FLOAT2:    return nvrhi::Format::RG32_FLOAT;
    case VF_FLOAT3:    return nvrhi::Format::RGB32_FLOAT;
    case VF_FLOAT4:    return nvrhi::Format::RGBA32_FLOAT;
    case VF_COLOR:     return nvrhi::Format::RGBA8_UNORM;
    case VF_UBYTE4:    return nvrhi::Format::RGBA8_UINT;
    case VF_SHORT2:    return nvrhi::Format::RG16_SINT;
    case VF_SHORT4:    return nvrhi::Format::RGBA16_SINT;
    case VF_UBYTE4N:   return nvrhi::Format::RGBA8_UNORM;
    case VF_SHORT2N:   return nvrhi::Format::RG16_SNORM;
    case VF_SHORT4N:   return nvrhi::Format::RGBA16_SNORM;
    case VF_USHORT2N:  return nvrhi::Format::RG16_UNORM;
    case VF_USHORT4N:  return nvrhi::Format::RGBA16_UNORM;
    case VF_FLOAT16_2: return nvrhi::Format::RG16_FLOAT;
    case VF_FLOAT16_4: return nvrhi::Format::RGBA16_FLOAT;
    default:           return nvrhi::Format::UNKNOWN;
    }
}

const char* ToSemanticName(u32 vertexSemantic)
{
    switch (vertexSemantic)
    {
    case VS_POSITION:     return "POSITION";
    case VS_BLENDWEIGHT:  return "BLENDWEIGHT";
    case VS_BLENDINDICES: return "BLENDINDICES";
    case VS_NORMAL:       return "NORMAL";
    case VS_PSIZE:        return "PSIZE";
    case VS_TEXCOORD:     return "TEXCOORD";
    case VS_TANGENT:      return "TANGENT";
    case VS_BINORMAL:     return "BINORMAL";
    case VS_TESSFACTOR:   return "TESSFACTOR";
    case VS_POSITIONT:    return "POSITIONT";
    case VS_COLOR:        return "COLOR";
    case VS_FOG:          return "FOG";
    case VS_DEPTH:        return "DEPTH";
    case VS_SAMPLE:       return "SAMPLE";
    default:              return "";
    }
}

// NVRHI-based buffer creation
static nvrhi::BufferHandle CreateNvrhiBuffer(nvrhi::IDevice* device, const void* pData, u32 dataSize,
    bool bDynamic, bool isVertexBuffer, bool isIndexBuffer, bool isConstantBuffer)
{
    nvrhi::BufferDesc desc;
    desc.byteSize = dataSize;
    desc.debugName = isVertexBuffer ? "VertexBuffer" : (isIndexBuffer ? "IndexBuffer" : "ConstantBuffer");

    if (isVertexBuffer)
        desc.isVertexBuffer = true;
    if (isIndexBuffer)
        desc.isIndexBuffer = true;
    if (isConstantBuffer)
        desc.isConstantBuffer = true;

    if (isVertexBuffer || isIndexBuffer)
    {
        desc.canHaveRawViews = true;
        desc.isAccelStructBuildInput = device->queryFeatureSupport(nvrhi::Feature::RayTracingAccelStruct);
    }

    if (bDynamic)
    {
        desc.cpuAccess = nvrhi::CpuAccessMode::Write;
    }

    // Tell NVRHI to track state from creation - required for D3D12
    desc.keepInitialState = true;
    if (isVertexBuffer)
        desc.initialState = nvrhi::ResourceStates::VertexBuffer;
    else if (isIndexBuffer)
        desc.initialState = nvrhi::ResourceStates::IndexBuffer;
    else if (isConstantBuffer)
        desc.initialState = nvrhi::ResourceStates::ConstantBuffer;
    else
        desc.initialState = nvrhi::ResourceStates::ShaderResource;

    nvrhi::BufferHandle buffer = device->createBuffer(desc);
    if (!buffer)
    {
        // Check if device was removed
        if (GEnv.Backend && GEnv.Backend->GetDeviceState() == DeviceState::Lost)
        {
            Msg("! [NVRHI] ERROR: Device Removed!");
        }
        Msg("! [NVRHI] Failed to create %s (size=%u bytes)", desc.debugName.c_str(), dataSize);
        return nullptr;
    }

    if (pData)
    {
        // Use backend's dedicated upload command list (reusable, prevents resource exhaustion)
        // Previously we created a new command list for EVERY buffer, which exhausted D3D12's
        // command allocator pool during model loading and caused DEVICE_REMOVED errors
        if (GEnv.Backend)
        {
            GEnv.Backend->UploadBufferData(buffer, pData, dataSize);
        }
        else
        {
            Msg("! [NVRHI] ERROR: Backend not available for buffer upload");
        }
    }

    return buffer;
}

static VertexBufferHandle CreateVertexBuffer(const void* pData, u32 dataSize, bool bDynamic)
{
    nvrhi::IDevice* device = GEnv.Backend->GetDevice();
    if (!device)
        return nullptr;

    return CreateNvrhiBuffer(device, pData, dataSize, bDynamic, true, false, false);
}

static IndexBufferHandle CreateIndexBuffer(const void* pData, u32 dataSize, bool bDynamic)
{
    nvrhi::IDevice* device = GEnv.Backend->GetDevice();
    if (!device)
        return nullptr;

    return CreateNvrhiBuffer(device, pData, dataSize, bDynamic, false, true, false);
}

static ConstantBufferHandle CreateConstantBufferInternal(u32 dataSize)
{
    nvrhi::IDevice* device = GEnv.Backend->GetDevice();
    if (!device)
        return nullptr;

    return CreateNvrhiBuffer(device, nullptr, dataSize, true, false, false, true);
}

namespace BufferUtils
{
ConstantBufferHandle CreateConstantBuffer(u32 DataSize)
{
    return fg::CreateConstantBufferInternal(DataSize);
}
};

static DXGI_FORMAT VertexFormatToDxgi(u32 vertexFormat)
{
    switch (vertexFormat)
    {
    case VF_FLOAT1:    return DXGI_FORMAT_R32_FLOAT;
    case VF_FLOAT2:    return DXGI_FORMAT_R32G32_FLOAT;
    case VF_FLOAT3:    return DXGI_FORMAT_R32G32B32_FLOAT;
    case VF_FLOAT4:    return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case VF_COLOR:     return DXGI_FORMAT_R8G8B8A8_UNORM;
    case VF_UBYTE4:    return DXGI_FORMAT_R8G8B8A8_UINT;
    case VF_SHORT2:    return DXGI_FORMAT_R16G16_SINT;
    case VF_SHORT4:    return DXGI_FORMAT_R16G16B16A16_SINT;
    case VF_UBYTE4N:   return DXGI_FORMAT_R8G8B8A8_UNORM;
    case VF_SHORT2N:   return DXGI_FORMAT_R16G16_SNORM;
    case VF_SHORT4N:   return DXGI_FORMAT_R16G16B16A16_SNORM;
    case VF_USHORT2N:  return DXGI_FORMAT_R16G16_UNORM;
    case VF_USHORT4N:  return DXGI_FORMAT_R16G16B16A16_UNORM;
    case VF_FLOAT16_2: return DXGI_FORMAT_R16G16_FLOAT;
    case VF_FLOAT16_4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    default:
        VERIFY(!"VertexFormatToDxgi: unsupported format");
        return DXGI_FORMAT_UNKNOWN;
    }
}

void ConvertVertexDeclaration(const xr_vector<VertexElement>& declIn, xr_vector<D3D_INPUT_ELEMENT_DESC>& declOut)
{
    s32 iDeclSize = declIn.size() - 1;
    declOut.resize(iDeclSize + 1);

    for (s32 i = 0; i < iDeclSize; ++i)
    {
        const VertexElement& descIn = declIn[i];
        D3D_INPUT_ELEMENT_DESC& descOut = declOut[i];

        descOut.SemanticName = ToSemanticName(descIn.Usage);
        descOut.SemanticIndex = descIn.UsageIndex;
        descOut.Format = VertexFormatToDxgi(descIn.Type);
        descOut.InputSlot = descIn.Stream;
        descOut.AlignedByteOffset = descIn.Offset;
        descOut.InputSlotClass = D3D_INPUT_PER_VERTEX_DATA;
        descOut.InstanceDataStepRate = 0;
    }

    if (iDeclSize >= 0)
        ZeroMemory(&declOut[iDeclSize], sizeof(declOut[iDeclSize]));
}

//-----------------------------------------------------------------------------
// VertexStagingBuffer - uses host memory + NVRHI buffer upload
//-----------------------------------------------------------------------------
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
    return m_DeviceBuffer != nullptr;
}

void* VertexStagingBuffer::Map(size_t offset /*= 0*/, size_t size /*= 0*/, bool read /*= false*/)
{
    VERIFY2(m_HostBuffer, "Buffer wasn't created or already discarded");
    VERIFY2(!read || m_AllowReadBack, "Can't read from write only buffer");
    VERIFY2((size + offset) <= m_Size, "Map region is too large");

    return static_cast<u8*>(m_HostBuffer) + offset;
}

void VertexStagingBuffer::Unmap(bool doFlush /*= false*/)
{
    if (!doFlush)
        return;

    VERIFY2(!m_DeviceBuffer, "Attempting to upload buffer twice");
    VERIFY(m_HostBuffer && m_Size);

    // Upload data to device using NVRHI
    m_DeviceBuffer = CreateVertexBuffer(m_HostBuffer, m_Size, false);
    if (!m_DeviceBuffer)
    {
        return;
    }

    if (!m_AllowReadBack)
    {
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
    m_DeviceBuffer = nullptr;  // NVRHI handles release via RefCountPtr
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
        return m_DeviceBuffer->getDesc().byteSize;
    }
    return 0;
}

//-----------------------------------------------------------------------------
// IndexStagingBuffer - uses host memory + NVRHI buffer upload
//-----------------------------------------------------------------------------
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
    return m_DeviceBuffer != nullptr;
}

void* IndexStagingBuffer::Map(size_t offset /*= 0*/, size_t size /*= 0*/, bool read /*= false*/)
{
    VERIFY2(m_HostBuffer, "Buffer wasn't created or already discarded");
    VERIFY2(!read || m_AllowReadBack, "Can't read from write only buffer");
    VERIFY2((size + offset) <= m_Size, "Map region is too large");

    return static_cast<u8*>(m_HostBuffer) + offset;
}

void IndexStagingBuffer::Unmap(bool doFlush /*= false*/)
{
    if (!doFlush)
        return;

    VERIFY2(!m_DeviceBuffer, "Attempting to upload buffer twice");
    VERIFY(m_HostBuffer && m_Size);

    // Upload data to device using NVRHI
    m_DeviceBuffer = CreateIndexBuffer(m_HostBuffer, m_Size, false);
    if (!m_DeviceBuffer)
    {
        return;
    }

    if (!m_AllowReadBack)
    {
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
    m_DeviceBuffer = nullptr;  // NVRHI handles release via RefCountPtr
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
        return m_DeviceBuffer->getDesc().byteSize;
    }
    return 0;
}

//-----------------------------------------------------------------------------
// VertexStreamBuffer - dynamic vertex buffer with NVRHI
//-----------------------------------------------------------------------------
VertexStreamBuffer::~VertexStreamBuffer()
{
    Destroy();
}

void VertexStreamBuffer::Create(size_t size)
{
    m_DeviceBuffer = CreateVertexBuffer(nullptr, size, true);
    if (!m_DeviceBuffer)
    {
        VERIFY(!"Failed to create vertex stream buffer");
        return;
    }
    AddRef();
}

void VertexStreamBuffer::Destroy()
{
    m_DeviceBuffer = nullptr;  // NVRHI handles release via RefCountPtr
}

void* VertexStreamBuffer::Map(size_t offset, size_t size, bool flush /*= false*/)
{
    // For NVRHI, we use a staging approach - allocate temp memory
    // and use writeBuffer on unmap. This is a simplified implementation.
    // A proper implementation would use a ring buffer or upload heap.

    if (!m_DeviceBuffer)
        return nullptr;

    // For dynamic buffers in NVRHI/D3D12, we need to use a different approach
    // since Map/Unmap is not directly supported like D3D11
    // We'll use a host-side buffer and write on unmap

    // Store the mapping info for later
    m_MappedOffset = offset;
    m_MappedSize = size;
    m_MappedFlush = flush;

    // Allocate temp buffer if needed
    if (!m_MappedData)
    {
        size_t bufSize = m_DeviceBuffer->getDesc().byteSize;
        m_MappedData = xr_alloc<u8>(bufSize);
    }

    return static_cast<u8*>(m_MappedData) + offset;
}

void VertexStreamBuffer::Unmap()
{
    if (!m_DeviceBuffer || !m_MappedData)
        return;

    // Write the data to the GPU buffer
    nvrhi::ICommandList* cmdList = GEnv.Backend->GetCommandList();
    if (cmdList)
    {
        size_t bufSize = m_DeviceBuffer->getDesc().byteSize;
        cmdList->writeBuffer(m_DeviceBuffer, m_MappedData, bufSize);
    }
}

bool VertexStreamBuffer::IsValid() const
{
    return m_DeviceBuffer != nullptr;
}

//-----------------------------------------------------------------------------
// IndexStreamBuffer - dynamic index buffer with NVRHI
//-----------------------------------------------------------------------------
IndexStreamBuffer::~IndexStreamBuffer()
{
    Destroy();
}

void IndexStreamBuffer::Create(size_t size)
{
    m_DeviceBuffer = CreateIndexBuffer(nullptr, size, true);
    if (!m_DeviceBuffer)
    {
        VERIFY(!"Failed to create index stream buffer");
        return;
    }
    AddRef();
}

void IndexStreamBuffer::Destroy()
{
    m_DeviceBuffer = nullptr;  // NVRHI handles release via RefCountPtr
}

void* IndexStreamBuffer::Map(size_t offset, size_t size, bool flush /*= false*/)
{
    if (!m_DeviceBuffer)
        return nullptr;

    m_MappedOffset = offset;
    m_MappedSize = size;
    m_MappedFlush = flush;

    if (!m_MappedData)
    {
        size_t bufSize = m_DeviceBuffer->getDesc().byteSize;
        m_MappedData = xr_alloc<u8>(bufSize);
    }

    return static_cast<u8*>(m_MappedData) + offset;
}

void IndexStreamBuffer::Unmap()
{
    if (!m_DeviceBuffer || !m_MappedData)
        return;

    nvrhi::ICommandList* cmdList = GEnv.Backend->GetCommandList();
    if (cmdList)
    {
        size_t bufSize = m_DeviceBuffer->getDesc().byteSize;
        cmdList->writeBuffer(m_DeviceBuffer, m_MappedData, bufSize);
    }
}

bool IndexStreamBuffer::IsValid() const
{
    return m_DeviceBuffer != nullptr;
}

} // namespace xray::render::fg
