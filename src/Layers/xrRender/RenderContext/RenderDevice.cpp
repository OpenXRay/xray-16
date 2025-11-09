#include "stdafx.h"
#include "RenderDevice.h"
#include "RenderContext.h"
#include "../NVRHI/NVRHIDevice.h"
#include "../ResourceManager/FGResourceManager.h"

namespace xray::render::ng {

RenderDevice::RenderDevice()
    : m_initialized(false)
{
}

RenderDevice::~RenderDevice() {
    Shutdown();
}

nvrhi::IDevice* RenderDevice::GetNVRHIDevice() const {
    return m_nvrhiDevice ? m_nvrhiDevice->GetDevice() : nullptr;
}

nvrhi::ICommandList* RenderDevice::GetImmediateCommandList() const {
    return m_nvrhiDevice ? m_nvrhiDevice->GetCommandList() : nullptr;
}

bool RenderDevice::InitializeD3D11(ID3D11Device* device, ID3D11DeviceContext* context) {
    VERIFY(device);
    VERIFY(context);

    m_nvrhiDevice = xr_make_unique<nvrhi_wrapper::NVRHIDevice>();
    if (!m_nvrhiDevice->Initialize(device, context)) {
        Msg("! [RenderDevice] Failed to initialize NVRHI device");
        return false;
    }

    m_pipelineCache = xr_make_unique<PipelineStateCache>(this);

    m_modernResourceManager = xr_make_unique<xray::render::resources::FGResourceManager>(this);
    Msg("* [RenderDevice] FGResourceManager initialized");

    m_uploadCommandList = GetNativeDevice()->createCommandList();
    if (!m_uploadCommandList) {
        Msg("! [RenderDevice] Failed to create upload command list");
        return false;
    }
    Msg("* [RenderDevice] Created persistent upload command list");

    m_textures.reserve(256);
    m_buffers.reserve(512);
    m_samplers.reserve(64);
    m_shaders.reserve(128);

    m_initialized = true;

    return true;
}

void RenderDevice::Shutdown() {
    if (!m_initialized) return;

    if (m_stats.texturesAlive > 0) {
        Msg("! [RenderDevice] WARNING: %u textures leaked", m_stats.texturesAlive);
    }
    if (m_stats.buffersAlive > 0) {
        Msg("! [RenderDevice] WARNING: %u buffers leaked", m_stats.buffersAlive);
    }
    if (m_stats.samplersAlive > 0) {
        Msg("! [RenderDevice] WARNING: %u samplers leaked", m_stats.samplersAlive);
    }
    if (m_stats.shadersAlive > 0) {
        Msg("! [RenderDevice] WARNING: %u shaders leaked", m_stats.shadersAlive);
    }

    m_uploadCommandList = nullptr;
    Msg("* [RenderDevice] Released upload command list");

    m_pipelineCache.reset();

    m_modernResourceManager.reset();

    m_textures.clear();
    m_buffers.clear();
    m_samplers.clear();
    m_shaders.clear();

    m_nvrhiDevice.reset();

    m_initialized = false;
}

TextureHandle RenderDevice::CreateTexture(
    const TextureDesc& desc,
    const void* initialData) {

    VERIFY(m_initialized);
    VERIFY(desc.width > 0 && desc.height > 0);

    nvrhi::TextureDesc nvrhiDesc = ConvertTextureDesc(desc);

    nvrhi::TextureHandle nvrhiTexture = GetNativeDevice()->createTexture(nvrhiDesc);
    if (!nvrhiTexture) {
        Msg("! [RenderDevice] Failed to create texture: %s", desc.debugName.c_str());
        return TextureHandle{};
    }

    if (initialData) {
        nvrhi::ICommandList* cmdList = m_nvrhiDevice->GetCommandList();
        cmdList->open();
        cmdList->writeTexture(nvrhiTexture, 0, 0, initialData,
                              nvrhiDesc.width * nvrhi::getFormatInfo(nvrhiDesc.format).bytesPerBlock);
        cmdList->close();
        m_nvrhiDevice->ExecuteCommandList(cmdList);
    }

    TextureHandle handle = AllocateTextureHandle();

    TextureInfo& info = m_textures[handle.index];
    info.nvrhiHandle = nvrhiTexture;
    info.desc = desc;

    u64 memorySize = desc.width * desc.height * desc.depth * desc.arraySize;
    memorySize *= nvrhi::getFormatInfo(desc.format).bytesPerBlock;
    memorySize = (memorySize * desc.mipLevels * 4) / 3;

    m_stats.texturesAlive++;
    m_stats.texturesCreated++;
    m_stats.textureMemory += memorySize;

    return handle;
}

TextureHandle RenderDevice::CreateTextureFromD3D11(
    ID3D11Resource* d3d11Texture,
    const TextureDesc& desc) {

    VERIFY(m_initialized);
    VERIFY(d3d11Texture);

    nvrhi::TextureDesc nvrhiDesc = ConvertTextureDesc(desc);

    nvrhi::TextureHandle nvrhiTexture = GetNativeDevice()->createHandleForNativeTexture(
        nvrhi::ObjectTypes::D3D11_Resource,
        nvrhi::Object(d3d11Texture),
        nvrhiDesc
    );

    if (!nvrhiTexture) {
        Msg("! [RenderDevice] Failed to wrap D3D11 texture: %s", desc.debugName.c_str());
        return TextureHandle{};
    }

    TextureHandle handle = AllocateTextureHandle();

    TextureInfo& info = m_textures[handle.index];
    info.nvrhiHandle = nvrhiTexture;
    info.desc = desc;

    m_stats.texturesAlive++;
    m_stats.texturesCreated++;

    return handle;
}

void RenderDevice::DestroyTexture(TextureHandle handle) {
    if (!ValidateTextureHandle(handle)) {
        Msg("! [RenderDevice] DestroyTexture: Invalid handle");
        return;
    }

    TextureInfo& info = m_textures[handle.index];

    u64 memorySize = info.desc.width * info.desc.height * info.desc.depth * info.desc.arraySize;
    memorySize *= nvrhi::getFormatInfo(info.desc.format).bytesPerBlock;
    memorySize = (memorySize * info.desc.mipLevels * 4) / 3;

    m_stats.texturesAlive--;
    if (info.nvrhiHandle) {
        m_stats.textureMemory -= memorySize;
    }

    info.nvrhiHandle = nullptr;

    FreeTextureHandle(handle);
}

nvrhi::ITexture* RenderDevice::GetNativeTexture(TextureHandle handle) {
    if (!ValidateTextureHandle(handle))
        return nullptr;
    return m_textures[handle.index].nvrhiHandle.Get();
}

const RenderDevice::TextureDesc* RenderDevice::GetTextureDesc(TextureHandle handle) {
    if (!ValidateTextureHandle(handle))
        return nullptr;
    return &m_textures[handle.index].desc;
}

bool RenderDevice::IsTextureValid(TextureHandle handle) const {
    return ValidateTextureHandle(handle);
}

void RenderDevice::UploadTextureData(
    TextureHandle handle,
    u32 arraySlice,
    u32 mipLevel,
    const void* data,
    size_t dataSize
) {
    if (!ValidateTextureHandle(handle)) {
        Msg("! [RenderDevice] UploadTextureData: Invalid texture handle");
        return;
    }

    TextureInfo& info = m_textures[handle.index];

    const nvrhi::FormatInfo& formatInfo = nvrhi::getFormatInfo(info.desc.format);
    u32 mipWidth = (info.desc.width >> mipLevel) > 0 ? (info.desc.width >> mipLevel) : 1;
    u32 mipHeight = (info.desc.height >> mipLevel) > 0 ? (info.desc.height >> mipLevel) : 1;

    u32 rowPitch, slicePitch;
    if (formatInfo.blockSize > 1) {
        u32 blockWidth = (mipWidth + 3) / 4;
        u32 blockHeight = (mipHeight + 3) / 4;
        rowPitch = blockWidth * formatInfo.bytesPerBlock;
        slicePitch = rowPitch * blockHeight;
    } else {
        rowPitch = mipWidth * formatInfo.bytesPerBlock;
        slicePitch = rowPitch * mipHeight;
    }

    TextureSliceData slice;
    slice.arraySlice = arraySlice;
    slice.mipLevel = mipLevel;
    slice.data = data;
    slice.dataSize = dataSize;
    slice.rowPitch = rowPitch;
    slice.slicePitch = slicePitch;

    UploadTextureData(handle, &slice, 1);
}

void RenderDevice::UploadTextureData(
    TextureHandle handle,
    const TextureSliceData* slices,
    u32 sliceCount
) {
    if (!ValidateTextureHandle(handle)) {
        Msg("! [RenderDevice] UploadTextureData: Invalid texture handle");
        return;
    }

    if (!slices || sliceCount == 0) {
        return;
    }

    TextureInfo& info = m_textures[handle.index];
    nvrhi::ITexture* nvrhiTexture = info.nvrhiHandle;

    if (!nvrhiTexture) {
        Msg("! [RenderDevice] UploadTextureData: Texture has no NVRHI handle");
        return;
    }

    VERIFY(m_uploadCommandList && "Upload command list not initialized");

    std::lock_guard<std::mutex> lock(m_uploadMutex);

    m_uploadCommandList->open();

    for (u32 i = 0; i < sliceCount; ++i) {
        const TextureSliceData& slice = slices[i];

        m_uploadCommandList->writeTexture(nvrhiTexture, slice.arraySlice, slice.mipLevel,
                             slice.data, slice.rowPitch);
    }

    m_uploadCommandList->close();
    GetNativeDevice()->executeCommandList(m_uploadCommandList);
}

void RenderDevice::UploadTextureDataToNVRHI(
    nvrhi::ITexture* texture,
    u32 arraySlice,
    u32 mipLevel,
    const void* data,
    size_t dataSize)
{
    VERIFY(m_initialized);
    VERIFY(texture != nullptr);
    VERIFY(data != nullptr);
    VERIFY(dataSize > 0);
    VERIFY(m_uploadCommandList && "Upload command list not initialized");

    std::lock_guard<std::mutex> lock(m_uploadMutex);

    m_uploadCommandList->open();

    const auto& desc = texture->getDesc();
    u32 rowPitch = 0;

    switch (desc.format) {
        case nvrhi::Format::RGBA8_UNORM:
        case nvrhi::Format::BGRA8_UNORM:
            rowPitch = desc.width * 4;
            break;
        case nvrhi::Format::R8_UNORM:
            rowPitch = desc.width;
            break;
        default:
            rowPitch = dataSize / desc.height;
            break;
    }

    m_uploadCommandList->writeTexture(texture, arraySlice, mipLevel, data, rowPitch);

    m_uploadCommandList->close();
    GetNVRHIDevice()->executeCommandList(m_uploadCommandList);

    GetNVRHIDevice()->waitForIdle();
}

BufferHandle RenderDevice::CreateBuffer(
    const BufferDesc& desc,
    const void* initialData) {

    VERIFY(m_initialized);
    VERIFY(desc.byteSize > 0);

    nvrhi::BufferDesc nvrhiDesc = ConvertBufferDesc(desc);

    nvrhi::BufferHandle nvrhiBuffer = GetNativeDevice()->createBuffer(nvrhiDesc);
    if (!nvrhiBuffer) {
        Msg("! [RenderDevice] Failed to create buffer: %s", desc.debugName.c_str());
        return BufferHandle{};
    }

    if (initialData) {
        nvrhi::ICommandList* cmdList = m_nvrhiDevice->GetCommandList();
        cmdList->open();
        cmdList->writeBuffer(nvrhiBuffer, initialData, desc.byteSize);
        cmdList->close();
        m_nvrhiDevice->ExecuteCommandList(cmdList);
    }

    BufferHandle handle = AllocateBufferHandle();

    BufferInfo& info = m_buffers[handle.index];
    info.nvrhiHandle = nvrhiBuffer;
    info.desc = desc;

    m_stats.buffersAlive++;
    m_stats.buffersCreated++;
    m_stats.bufferMemory += desc.byteSize;

    return handle;
}

void RenderDevice::DestroyBuffer(BufferHandle handle) {
    if (!ValidateBufferHandle(handle)) {
        Msg("! [RenderDevice] DestroyBuffer: Invalid handle");
        return;
    }

    BufferInfo& info = m_buffers[handle.index];

    m_stats.buffersAlive--;
    m_stats.bufferMemory -= info.desc.byteSize;

    info.nvrhiHandle = nullptr;

    FreeBufferHandle(handle);
}

void RenderDevice::UpdateBuffer(
    BufferHandle handle,
    const void* data,
    u64 size,
    u64 offset) {

    if (!ValidateBufferHandle(handle)) {
        Msg("! [RenderDevice] UpdateBuffer: Invalid handle");
        return;
    }

    BufferInfo& info = m_buffers[handle.index];
    VERIFY(offset + size <= info.desc.byteSize);

    nvrhi::ICommandList* cmdList = m_nvrhiDevice->GetCommandList();
    cmdList->open();
    cmdList->writeBuffer(info.nvrhiHandle, data, size, offset);
    cmdList->close();
    m_nvrhiDevice->ExecuteCommandList(cmdList);
}

nvrhi::IBuffer* RenderDevice::GetNativeBuffer(BufferHandle handle) {
    if (!ValidateBufferHandle(handle))
        return nullptr;
    return m_buffers[handle.index].nvrhiHandle.Get();
}

const RenderDevice::BufferDesc* RenderDevice::GetBufferDesc(BufferHandle handle) {
    if (!ValidateBufferHandle(handle))
        return nullptr;
    return &m_buffers[handle.index].desc;
}

bool RenderDevice::IsBufferValid(BufferHandle handle) const {
    return ValidateBufferHandle(handle);
}

SamplerHandle RenderDevice::CreateSampler(const SamplerDesc& desc) {
    VERIFY(m_initialized);

    nvrhi::SamplerDesc nvrhiDesc = ConvertSamplerDesc(desc);

    nvrhi::SamplerHandle nvrhiSampler = GetNativeDevice()->createSampler(nvrhiDesc);
    if (!nvrhiSampler) {
        Msg("! [RenderDevice] Failed to create sampler: %s", desc.debugName.c_str());
        return SamplerHandle{};
    }

    SamplerHandle handle = AllocateSamplerHandle();

    SamplerInfo& info = m_samplers[handle.index];
    info.nvrhiHandle = nvrhiSampler;
    info.desc = desc;

    m_stats.samplersAlive++;
    m_stats.samplersCreated++;

    return handle;
}

void RenderDevice::DestroySampler(SamplerHandle handle) {
    if (!ValidateSamplerHandle(handle)) {
        Msg("! [RenderDevice] DestroySampler: Invalid handle");
        return;
    }

    SamplerInfo& info = m_samplers[handle.index];

    m_stats.samplersAlive--;

    info.nvrhiHandle = nullptr;

    FreeSamplerHandle(handle);
}

nvrhi::BindingLayoutHandle RenderDevice::CreateBindingLayout(const nvrhi::BindingLayoutDesc& desc) {
    VERIFY(m_initialized);
    return GetNativeDevice()->createBindingLayout(desc);
}

nvrhi::BindingSetHandle RenderDevice::CreateBindingSet(const nvrhi::BindingSetDesc& desc, nvrhi::IBindingLayout* layout) {
    VERIFY(m_initialized);
    return GetNativeDevice()->createBindingSet(desc, layout);
}

nvrhi::FramebufferHandle RenderDevice::CreateFramebuffer(const nvrhi::FramebufferDesc& desc) {
    VERIFY(m_initialized);
    return GetNativeDevice()->createFramebuffer(desc);
}

nvrhi::ISampler* RenderDevice::GetNativeSampler(SamplerHandle handle) {
    if (!ValidateSamplerHandle(handle))
        return nullptr;
    return m_samplers[handle.index].nvrhiHandle.Get();
}

const RenderDevice::SamplerDesc* RenderDevice::GetSamplerDesc(SamplerHandle handle) {
    if (!ValidateSamplerHandle(handle))
        return nullptr;
    return &m_samplers[handle.index].desc;
}

bool RenderDevice::IsSamplerValid(SamplerHandle handle) const {
    return ValidateSamplerHandle(handle);
}

ShaderHandle RenderDevice::CreateShader(
    ShaderStage stage,
    const void* bytecode,
    size_t bytecodeSize,
    const char* debugName) {

    VERIFY(m_initialized);
    VERIFY(bytecode && bytecodeSize > 0);

    nvrhi::ShaderDesc nvrhiDesc = ConvertShaderDesc(stage, debugName);

    nvrhi::ShaderHandle nvrhiShader = GetNativeDevice()->createShader(
        nvrhiDesc, bytecode, bytecodeSize);

    if (!nvrhiShader) {
        Msg("! [RenderDevice] Failed to create shader: %s", debugName);
        return ShaderHandle{};
    }

    ShaderHandle handle = AllocateShaderHandle();

    xr_unique_ptr<RCShader> shader = xr_make_unique<RCShader>(stage, nvrhiShader, debugName);

    ShaderInfo& info = m_shaders[handle.index];
    info.shader = std::move(shader);

    m_stats.shadersAlive++;
    m_stats.shadersCreated++;

    return handle;
}

void RenderDevice::DestroyShader(ShaderHandle handle) {
    if (!ValidateShaderHandle(handle)) {
        Msg("! [RenderDevice] DestroyShader: Invalid handle");
        return;
    }

    ShaderInfo& info = m_shaders[handle.index];

    m_stats.shadersAlive--;

    info.shader.reset();

    FreeShaderHandle(handle);
}

RCShader* RenderDevice::GetShader(ShaderHandle handle) {
    if (!ValidateShaderHandle(handle))
        return nullptr;
    return m_shaders[handle.index].shader.get();
}

bool RenderDevice::IsShaderValid(ShaderHandle handle) const {
    return ValidateShaderHandle(handle);
}

RenderContext* RenderDevice::CreateContext() {
    VERIFY(m_initialized);

    nvrhi::CommandListHandle cmdList = GetNativeDevice()->createCommandList();
    if (!cmdList) {
        Msg("! [RenderDevice] Failed to create command list");
        return nullptr;
    }

    RenderContext* context = new RenderContext(this, cmdList);
    m_stats.contextsAlive++;

    return context;
}

void RenderDevice::DestroyContext(RenderContext* context) {
    if (!context) return;

    delete context;
    m_stats.contextsAlive--;
}

void RenderDevice::ExecuteContext(RenderContext* context) {
    VERIFY(context);
    m_nvrhiDevice->ExecuteCommandList(context->GetCommandList());
}

nvrhi::IDevice* RenderDevice::GetNativeDevice() const {
    VERIFY(m_nvrhiDevice);
    return m_nvrhiDevice->GetDevice();
}

void RenderDevice::ResetStatistics() {
    m_stats.texturesCreated = 0;
    m_stats.buffersCreated = 0;
    m_stats.samplersCreated = 0;
    m_stats.shadersCreated = 0;
}

TextureHandle RenderDevice::AllocateTextureHandle() {
    u32 index;
    u32 generation;

    if (!m_freeTextures.empty()) {
        index = m_freeTextures.back();
        m_freeTextures.pop_back();

        TextureInfo& info = m_textures[index];
        info.generation++;
        generation = info.generation;
        info.isAlive = true;
    } else {
        index = static_cast<u32>(m_textures.size());
        generation = 0;

        TextureInfo info;
        info.generation = generation;
        info.isAlive = true;
        m_textures.push_back(info);
    }

    return TextureHandle(index, generation);
}

BufferHandle RenderDevice::AllocateBufferHandle() {
    u32 index;
    u32 generation;

    if (!m_freeBuffers.empty()) {
        index = m_freeBuffers.back();
        m_freeBuffers.pop_back();

        BufferInfo& info = m_buffers[index];
        info.generation++;
        generation = info.generation;
        info.isAlive = true;
    } else {
        index = static_cast<u32>(m_buffers.size());
        generation = 0;

        BufferInfo info;
        info.generation = generation;
        info.isAlive = true;
        m_buffers.push_back(info);
    }

    return BufferHandle(index, generation);
}

SamplerHandle RenderDevice::AllocateSamplerHandle() {
    u32 index;
    u32 generation;

    if (!m_freeSamplers.empty()) {
        index = m_freeSamplers.back();
        m_freeSamplers.pop_back();

        SamplerInfo& info = m_samplers[index];
        info.generation++;
        generation = info.generation;
        info.isAlive = true;
    } else {
        index = static_cast<u32>(m_samplers.size());
        generation = 0;

        SamplerInfo info;
        info.generation = generation;
        info.isAlive = true;
        m_samplers.push_back(info);
    }

    return SamplerHandle(index, generation);
}

ShaderHandle RenderDevice::AllocateShaderHandle() {
    u32 index;
    u32 generation;

    if (!m_freeShaders.empty()) {
        index = m_freeShaders.back();
        m_freeShaders.pop_back();

        ShaderInfo& info = m_shaders[index];
        info.generation++;
        generation = info.generation;
        info.isAlive = true;
    } else {
        index = static_cast<u32>(m_shaders.size());
        generation = 0;

        m_shaders.emplace_back();
        ShaderInfo& info = m_shaders.back();
        info.generation = generation;
        info.isAlive = true;
    }

    return ShaderHandle(index, generation);
}

void RenderDevice::FreeTextureHandle(TextureHandle handle) {
    VERIFY(handle.index < m_textures.size());
    m_textures[handle.index].isAlive = false;
    m_freeTextures.push_back(handle.index);
}

void RenderDevice::FreeBufferHandle(BufferHandle handle) {
    VERIFY(handle.index < m_buffers.size());
    m_buffers[handle.index].isAlive = false;
    m_freeBuffers.push_back(handle.index);
}

void RenderDevice::FreeSamplerHandle(SamplerHandle handle) {
    VERIFY(handle.index < m_samplers.size());
    m_samplers[handle.index].isAlive = false;
    m_freeSamplers.push_back(handle.index);
}

void RenderDevice::FreeShaderHandle(ShaderHandle handle) {
    VERIFY(handle.index < m_shaders.size());
    m_shaders[handle.index].isAlive = false;
    m_freeShaders.push_back(handle.index);
}

bool RenderDevice::ValidateTextureHandle(TextureHandle handle) const {
    if (handle.index >= m_textures.size())
        return false;

    const TextureInfo& info = m_textures[handle.index];
    return info.isAlive && info.generation == handle.generation;
}

bool RenderDevice::ValidateBufferHandle(BufferHandle handle) const {
    if (handle.index >= m_buffers.size())
        return false;

    const BufferInfo& info = m_buffers[handle.index];
    return info.isAlive && info.generation == handle.generation;
}

bool RenderDevice::ValidateSamplerHandle(SamplerHandle handle) const {
    if (handle.index >= m_samplers.size())
        return false;

    const SamplerInfo& info = m_samplers[handle.index];
    return info.isAlive && info.generation == handle.generation;
}

bool RenderDevice::ValidateShaderHandle(ShaderHandle handle) const {
    if (handle.index >= m_shaders.size())
        return false;

    const ShaderInfo& info = m_shaders[handle.index];
    return info.isAlive && info.generation == handle.generation;
}

nvrhi::TextureDesc RenderDevice::ConvertTextureDesc(const TextureDesc& desc) {
    nvrhi::TextureDesc nvrhiDesc;

    nvrhiDesc.width = desc.width;
    nvrhiDesc.height = desc.height;
    nvrhiDesc.depth = desc.depth;
    nvrhiDesc.arraySize = desc.arraySize;
    nvrhiDesc.mipLevels = desc.mipLevels;
    nvrhiDesc.sampleCount = 1;
    nvrhiDesc.sampleQuality = 0;
    nvrhiDesc.format = desc.format;
    nvrhiDesc.debugName = desc.debugName.c_str();
    nvrhiDesc.initialState = desc.initialState;
    nvrhiDesc.keepInitialState = true;

    switch (desc.dimension) {
        case TextureDesc::Texture1D:
            nvrhiDesc.dimension = nvrhi::TextureDimension::Texture1D;
            break;
        case TextureDesc::Texture2D:
            nvrhiDesc.dimension = nvrhi::TextureDimension::Texture2D;
            break;
        case TextureDesc::Texture3D:
            nvrhiDesc.dimension = nvrhi::TextureDimension::Texture3D;
            break;
        case TextureDesc::TextureCube:
            nvrhiDesc.dimension = nvrhi::TextureDimension::TextureCube;
            break;
    }

    nvrhiDesc.isRenderTarget = desc.isRenderTarget;
    nvrhiDesc.isUAV = desc.isUAV;

    if (desc.isDepthStencil) {
        nvrhiDesc.isRenderTarget = true;
    }

    return nvrhiDesc;
}

nvrhi::BufferDesc RenderDevice::ConvertBufferDesc(const BufferDesc& desc) {
    nvrhi::BufferDesc nvrhiDesc;

    nvrhiDesc.byteSize = desc.byteSize;
    nvrhiDesc.structStride = desc.structStride;
    nvrhiDesc.debugName = desc.debugName.c_str();
    nvrhiDesc.initialState = nvrhi::ResourceStates::Common;
    nvrhiDesc.keepInitialState = true;

    nvrhiDesc.isConstantBuffer = desc.isConstantBuffer;
    nvrhiDesc.isVertexBuffer = desc.isVertexBuffer;
    nvrhiDesc.isIndexBuffer = desc.isIndexBuffer;
    nvrhiDesc.isDrawIndirectArgs = desc.isDrawIndirectArgs;
    nvrhiDesc.canHaveUAVs = desc.isUAV;

    nvrhiDesc.canHaveRawViews = desc.cpuRead || desc.cpuWrite;
    nvrhiDesc.isVolatile = desc.isVolatile;

    return nvrhiDesc;
}

nvrhi::SamplerDesc RenderDevice::ConvertSamplerDesc(const SamplerDesc& desc) {
    nvrhi::SamplerDesc nvrhiDesc;

    auto convertAddressMode = [](SamplerDesc::AddressMode mode) -> nvrhi::SamplerAddressMode {
        switch (mode) {
            case SamplerDesc::AddressMode::Wrap:       return nvrhi::SamplerAddressMode::Wrap;
            case SamplerDesc::AddressMode::Mirror:     return nvrhi::SamplerAddressMode::Mirror;
            case SamplerDesc::AddressMode::Clamp:      return nvrhi::SamplerAddressMode::Clamp;
            case SamplerDesc::AddressMode::Border:     return nvrhi::SamplerAddressMode::Border;
            case SamplerDesc::AddressMode::MirrorOnce: return nvrhi::SamplerAddressMode::MirrorOnce;
            default: return nvrhi::SamplerAddressMode::Wrap;
        }
    };

    nvrhiDesc.addressU = convertAddressMode(desc.addressU);
    nvrhiDesc.addressV = convertAddressMode(desc.addressV);
    nvrhiDesc.addressW = convertAddressMode(desc.addressW);

    if (desc.anisotropyEnable) {
        nvrhiDesc.maxAnisotropy = static_cast<float>(desc.maxAnisotropy);
    }

    bool minLinear = (desc.minFilter == SamplerDesc::Filter::Linear);
    bool magLinear = (desc.magFilter == SamplerDesc::Filter::Linear);
    bool mipLinear = (desc.mipFilter == SamplerDesc::Filter::Linear);

    if (minLinear && magLinear && mipLinear) {
        nvrhiDesc.minFilter = nvrhiDesc.magFilter = nvrhiDesc.mipFilter = true;
    } else if (!minLinear && !magLinear && !mipLinear) {
        nvrhiDesc.minFilter = nvrhiDesc.magFilter = nvrhiDesc.mipFilter = false;
    }

    nvrhiDesc.mipBias = desc.mipLodBias;

    if (desc.comparisonEnable) {
        nvrhiDesc.reductionType = nvrhi::SamplerReductionType::Comparison;
    }

    nvrhiDesc.borderColor.r = desc.borderColor[0];
    nvrhiDesc.borderColor.g = desc.borderColor[1];
    nvrhiDesc.borderColor.b = desc.borderColor[2];
    nvrhiDesc.borderColor.a = desc.borderColor[3];

    return nvrhiDesc;
}

nvrhi::ShaderDesc RenderDevice::ConvertShaderDesc(ShaderStage stage, const char* debugName) {
    nvrhi::ShaderDesc nvrhiDesc;

    nvrhiDesc.debugName = debugName;

    switch (stage) {
        case ShaderStage::Vertex:   nvrhiDesc.shaderType = nvrhi::ShaderType::Vertex; break;
        case ShaderStage::Pixel:    nvrhiDesc.shaderType = nvrhi::ShaderType::Pixel; break;
        case ShaderStage::Geometry: nvrhiDesc.shaderType = nvrhi::ShaderType::Geometry; break;
        case ShaderStage::Hull:     nvrhiDesc.shaderType = nvrhi::ShaderType::Hull; break;
        case ShaderStage::Domain:   nvrhiDesc.shaderType = nvrhi::ShaderType::Domain; break;
        case ShaderStage::Compute:  nvrhiDesc.shaderType = nvrhi::ShaderType::Compute; break;
    }

    return nvrhiDesc;
}

}
