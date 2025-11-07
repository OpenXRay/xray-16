#include "stdafx.h"
#include "RenderDevice.h"
#include "RenderContext.h"
#include "../NVRHI/NVRHIDevice.h"
#include "../ResourceManager/ModernResourceManager.h"

namespace xray::render::ng {

// ═══════════════════════════════════════════════════
//  CONSTRUCTION / DESTRUCTION
// ═══════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════
//  INITIALIZATION
// ═══════════════════════════════════════════════════

bool RenderDevice::InitializeD3D11(ID3D11Device* device, ID3D11DeviceContext* context) {
    VERIFY(device);
    VERIFY(context);

    // Create NVRHI device wrapper
    m_nvrhiDevice = xr_make_unique<nvrhi_wrapper::NVRHIDevice>();
    if (!m_nvrhiDevice->Initialize(device, context)) {
        Msg("! [RenderDevice] Failed to initialize NVRHI device");
        return false;
    }

    // Create pipeline state cache
    m_pipelineCache = xr_make_unique<PipelineStateCache>(this);

    // Create modern resource manager (Week 2-3)
    m_modernResourceManager = xr_make_unique<xray::render::resources::ModernResourceManager>(this);
    Msg("* [RenderDevice] ModernResourceManager initialized");

    // Create persistent upload command list (reused for texture/buffer uploads)
    m_uploadCommandList = GetNativeDevice()->createCommandList();
    if (!m_uploadCommandList) {
        Msg("! [RenderDevice] Failed to create upload command list");
        return false;
    }
    Msg("* [RenderDevice] Created persistent upload command list");

    // Reserve initial capacity
    m_textures.reserve(256);
    m_buffers.reserve(512);
    m_samplers.reserve(64);
    m_shaders.reserve(128);

    m_initialized = true;

    return true;
}

void RenderDevice::Shutdown() {
    if (!m_initialized) return;

    // Print leak detection
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

    // Release upload command list
    m_uploadCommandList = nullptr;
    Msg("* [RenderDevice] Released upload command list");

    // Clear pipeline cache
    m_pipelineCache.reset();

    // Shutdown modern resource manager
    m_modernResourceManager.reset();

    // Clear all resources
    m_textures.clear();
    m_buffers.clear();
    m_samplers.clear();
    m_shaders.clear();

    // Shutdown NVRHI
    m_nvrhiDevice.reset();

    m_initialized = false;
}

// ═══════════════════════════════════════════════════
//  TEXTURE CREATION
// ═══════════════════════════════════════════════════

TextureHandle RenderDevice::CreateTexture(
    const TextureDesc& desc,
    const void* initialData) {

    VERIFY(m_initialized);
    VERIFY(desc.width > 0 && desc.height > 0);

    // Convert to NVRHI descriptor
    nvrhi::TextureDesc nvrhiDesc = ConvertTextureDesc(desc);

    // Create NVRHI texture
    nvrhi::TextureHandle nvrhiTexture = GetNativeDevice()->createTexture(nvrhiDesc);
    if (!nvrhiTexture) {
        Msg("! [RenderDevice] Failed to create texture: %s", desc.debugName.c_str());
        return TextureHandle{};
    }

    // Upload initial data if provided
    if (initialData) {
        nvrhi::ICommandList* cmdList = m_nvrhiDevice->GetCommandList();
        cmdList->open();
        cmdList->writeTexture(nvrhiTexture, 0, 0, initialData,
                              nvrhiDesc.width * nvrhi::getFormatInfo(nvrhiDesc.format).bytesPerBlock);
        cmdList->close();
        m_nvrhiDevice->ExecuteCommandList(cmdList);
    }

    // Allocate handle
    TextureHandle handle = AllocateTextureHandle();

    // Store texture info
    TextureInfo& info = m_textures[handle.index];
    info.nvrhiHandle = nvrhiTexture;
    info.desc = desc;

    // Calculate memory size manually
    u64 memorySize = desc.width * desc.height * desc.depth * desc.arraySize;
    memorySize *= nvrhi::getFormatInfo(desc.format).bytesPerBlock;
    memorySize = (memorySize * desc.mipLevels * 4) / 3;  // Approximate mip chain

    // Update statistics
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

    // Convert to NVRHI descriptor
    nvrhi::TextureDesc nvrhiDesc = ConvertTextureDesc(desc);

    // Wrap D3D11 resource
    nvrhi::TextureHandle nvrhiTexture = GetNativeDevice()->createHandleForNativeTexture(
        nvrhi::ObjectTypes::D3D11_Resource,
        nvrhi::Object(d3d11Texture),
        nvrhiDesc
    );

    if (!nvrhiTexture) {
        Msg("! [RenderDevice] Failed to wrap D3D11 texture: %s", desc.debugName.c_str());
        return TextureHandle{};
    }

    // Allocate handle
    TextureHandle handle = AllocateTextureHandle();

    // Store texture info
    TextureInfo& info = m_textures[handle.index];
    info.nvrhiHandle = nvrhiTexture;
    info.desc = desc;

    // Update statistics
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

    // Calculate memory size for statistics
    u64 memorySize = info.desc.width * info.desc.height * info.desc.depth * info.desc.arraySize;
    memorySize *= nvrhi::getFormatInfo(info.desc.format).bytesPerBlock;
    memorySize = (memorySize * info.desc.mipLevels * 4) / 3;

    // Update statistics
    m_stats.texturesAlive--;
    if (info.nvrhiHandle) {
        m_stats.textureMemory -= memorySize;
    }

    // Release NVRHI resource
    info.nvrhiHandle = nullptr;

    // Free handle
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
    nvrhi::ITexture* nvrhiTexture = info.nvrhiHandle;

    if (!nvrhiTexture) {
        Msg("! [RenderDevice] UploadTextureData: Texture has no NVRHI handle");
        return;
    }

    // Use persistent upload command list (reused, not recreated)
    VERIFY(m_uploadCommandList && "Upload command list not initialized");
    m_uploadCommandList->open();

    // Calculate row pitch based on format
    const nvrhi::FormatInfo& formatInfo = nvrhi::getFormatInfo(info.desc.format);
    u32 mipWidth = (info.desc.width >> mipLevel) > 0 ? (info.desc.width >> mipLevel) : 1;

    // For block-compressed formats (BC1-BC7), row pitch is in blocks, not pixels
    u32 rowPitch;
    if (formatInfo.blockSize > 1) {
        // Block-compressed format (4x4 blocks)
        rowPitch = ((mipWidth + 3) / 4) * formatInfo.bytesPerBlock;
    } else {
        // Uncompressed format
        rowPitch = mipWidth * formatInfo.bytesPerBlock;
    }

    // Write texture data
    m_uploadCommandList->writeTexture(nvrhiTexture, arraySlice, mipLevel, data, rowPitch);

    // Execute immediately
    m_uploadCommandList->close();
    GetNativeDevice()->executeCommandList(m_uploadCommandList);
    // Note: Command list is reused, not released - it's persistent!
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

    // Use persistent upload command list (reused, not recreated)
    VERIFY(m_uploadCommandList && "Upload command list not initialized");

    // Lock for thread safety - textures are loaded in parallel
    std::lock_guard<std::mutex> lock(m_uploadMutex);

    m_uploadCommandList->open();

    // Upload all slices
    const nvrhi::FormatInfo& formatInfo = nvrhi::getFormatInfo(info.desc.format);

    for (u32 i = 0; i < sliceCount; ++i) {
        const TextureSliceData& slice = slices[i];

        // Calculate row pitch for this mip level
        u32 mipWidth = (info.desc.width >> slice.mipLevel) > 0 ?
                       (info.desc.width >> slice.mipLevel) : 1;

        // For block-compressed formats (BC1-BC7), row pitch is in blocks, not pixels
        u32 rowPitch;
        if (formatInfo.blockSize > 1) {
            // Block-compressed format (4x4 blocks)
            rowPitch = ((mipWidth + 3) / 4) * formatInfo.bytesPerBlock;
        } else {
            // Uncompressed format
            rowPitch = mipWidth * formatInfo.bytesPerBlock;
        }

        // Write this slice
        m_uploadCommandList->writeTexture(nvrhiTexture, slice.arraySlice, slice.mipLevel,
                             slice.data, rowPitch);
    }

    // Execute immediately
    m_uploadCommandList->close();
    GetNativeDevice()->executeCommandList(m_uploadCommandList);
    // Note: Command list is reused, not released - it's persistent!
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

    // Lock for thread safety - textures are loaded in parallel
    std::lock_guard<std::mutex> lock(m_uploadMutex);

    m_uploadCommandList->open();

    // Get texture desc to calculate row pitch
    const auto& desc = texture->getDesc();
    u32 rowPitch = 0;

    // For uncompressed formats, calculate based on format size
    switch (desc.format) {
        case nvrhi::Format::RGBA8_UNORM:
        case nvrhi::Format::BGRA8_UNORM:
            rowPitch = desc.width * 4;
            break;
        case nvrhi::Format::R8_UNORM:
            rowPitch = desc.width;
            break;
        default:
            // For other formats, assume dataSize is correct for the entire texture
            rowPitch = dataSize / desc.height;
            break;
    }

    // Write texture data
    m_uploadCommandList->writeTexture(texture, arraySlice, mipLevel, data, rowPitch);

    // Execute immediately and wait for completion
    m_uploadCommandList->close();
    GetNVRHIDevice()->executeCommandList(m_uploadCommandList);

    // Wait for upload to complete to avoid D3D11 state corruption
    // (NVRHI command execution can unbind shaders/state)
    GetNVRHIDevice()->waitForIdle();
}

// ═══════════════════════════════════════════════════
//  BUFFER CREATION
// ═══════════════════════════════════════════════════

BufferHandle RenderDevice::CreateBuffer(
    const BufferDesc& desc,
    const void* initialData) {

    VERIFY(m_initialized);
    VERIFY(desc.byteSize > 0);

    // Convert to NVRHI descriptor
    nvrhi::BufferDesc nvrhiDesc = ConvertBufferDesc(desc);

    // Create NVRHI buffer
    nvrhi::BufferHandle nvrhiBuffer = GetNativeDevice()->createBuffer(nvrhiDesc);
    if (!nvrhiBuffer) {
        Msg("! [RenderDevice] Failed to create buffer: %s", desc.debugName.c_str());
        return BufferHandle{};
    }

    // Upload initial data if provided
    if (initialData) {
        nvrhi::ICommandList* cmdList = m_nvrhiDevice->GetCommandList();
        cmdList->open();
        cmdList->writeBuffer(nvrhiBuffer, initialData, desc.byteSize);
        cmdList->close();
        m_nvrhiDevice->ExecuteCommandList(cmdList);
    }

    // Allocate handle
    BufferHandle handle = AllocateBufferHandle();

    // Store buffer info
    BufferInfo& info = m_buffers[handle.index];
    info.nvrhiHandle = nvrhiBuffer;
    info.desc = desc;

    // Update statistics
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

    // Update statistics
    m_stats.buffersAlive--;
    m_stats.bufferMemory -= info.desc.byteSize;

    // Release NVRHI resource
    info.nvrhiHandle = nullptr;

    // Free handle
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

// ═══════════════════════════════════════════════════
//  SAMPLER CREATION
// ═══════════════════════════════════════════════════

SamplerHandle RenderDevice::CreateSampler(const SamplerDesc& desc) {
    VERIFY(m_initialized);

    // Convert to NVRHI descriptor
    nvrhi::SamplerDesc nvrhiDesc = ConvertSamplerDesc(desc);

    // Create NVRHI sampler
    nvrhi::SamplerHandle nvrhiSampler = GetNativeDevice()->createSampler(nvrhiDesc);
    if (!nvrhiSampler) {
        Msg("! [RenderDevice] Failed to create sampler: %s", desc.debugName.c_str());
        return SamplerHandle{};
    }

    // Allocate handle
    SamplerHandle handle = AllocateSamplerHandle();

    // Store sampler info
    SamplerInfo& info = m_samplers[handle.index];
    info.nvrhiHandle = nvrhiSampler;
    info.desc = desc;

    // Update statistics
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

    // Update statistics
    m_stats.samplersAlive--;

    // Release NVRHI resource
    info.nvrhiHandle = nullptr;

    // Free handle
    FreeSamplerHandle(handle);
}

// ═══════════════════════════════════════════════════════
//  BINDING LAYOUTS & SETS
// ═══════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════
//  SHADER CREATION
// ═══════════════════════════════════════════════════

ShaderHandle RenderDevice::CreateShader(
    ShaderStage stage,
    const void* bytecode,
    size_t bytecodeSize,
    const char* debugName) {

    VERIFY(m_initialized);
    VERIFY(bytecode && bytecodeSize > 0);

    // Convert to NVRHI shader descriptor
    nvrhi::ShaderDesc nvrhiDesc = ConvertShaderDesc(stage, debugName);

    // Create NVRHI shader
    nvrhi::ShaderHandle nvrhiShader = GetNativeDevice()->createShader(
        nvrhiDesc, bytecode, bytecodeSize);

    if (!nvrhiShader) {
        Msg("! [RenderDevice] Failed to create shader: %s", debugName);
        return ShaderHandle{};
    }

    // Allocate handle
    ShaderHandle handle = AllocateShaderHandle();

    // Create RCShader wrapper
    xr_unique_ptr<RCShader> shader = xr_make_unique<RCShader>(stage, nvrhiShader, debugName);

    // Store shader info
    ShaderInfo& info = m_shaders[handle.index];
    info.shader = std::move(shader);

    // Update statistics
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

    // Update statistics
    m_stats.shadersAlive--;

    // Release shader
    info.shader.reset();

    // Free handle
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

// ═══════════════════════════════════════════════════
//  CONTEXT MANAGEMENT
// ═══════════════════════════════════════════════════

RenderContext* RenderDevice::CreateContext() {
    VERIFY(m_initialized);

    nvrhi::CommandListHandle cmdList = GetNativeDevice()->createCommandList();
    if (!cmdList) {
        Msg("! [RenderDevice] Failed to create command list");
        return nullptr;
    }

    // Pass 'this' RenderDevice so RenderContext can resolve BufferHandles and other handles
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

// ═══════════════════════════════════════════════════
//  DEVICE ACCESS
// ═══════════════════════════════════════════════════

nvrhi::IDevice* RenderDevice::GetNativeDevice() const {
    VERIFY(m_nvrhiDevice);
    return m_nvrhiDevice->GetDevice();
}

// ═══════════════════════════════════════════════════
//  STATISTICS
// ═══════════════════════════════════════════════════

void RenderDevice::ResetStatistics() {
    // Don't reset alive counts, only creation counts
    m_stats.texturesCreated = 0;
    m_stats.buffersCreated = 0;
    m_stats.samplersCreated = 0;
    m_stats.shadersCreated = 0;
}

// ═══════════════════════════════════════════════════
//  HANDLE ALLOCATION (Generational System)
// ═══════════════════════════════════════════════════

TextureHandle RenderDevice::AllocateTextureHandle() {
    u32 index;
    u32 generation;

    if (!m_freeTextures.empty()) {
        // Reuse freed slot
        index = m_freeTextures.back();
        m_freeTextures.pop_back();

        TextureInfo& info = m_textures[index];
        info.generation++;  // Increment generation
        generation = info.generation;
        info.isAlive = true;
    } else {
        // Allocate new slot
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

        // Use emplace_back to construct in-place (avoids copy)
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

// ═══════════════════════════════════════════════════
//  HANDLE VALIDATION
// ═══════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════
//  CONVERSION HELPERS (X-Ray -> NVRHI)
// ═══════════════════════════════════════════════════

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

    // Dimension
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

    // Usage flags
    nvrhiDesc.isRenderTarget = desc.isRenderTarget;
    nvrhiDesc.isUAV = desc.isUAV;

    // Depth/stencil requires special format handling
    if (desc.isDepthStencil) {
        nvrhiDesc.isRenderTarget = true;  // Depth targets are render targets in NVRHI
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

    // Usage flags
    nvrhiDesc.isConstantBuffer = desc.isConstantBuffer;
    nvrhiDesc.isVertexBuffer = desc.isVertexBuffer;
    nvrhiDesc.isIndexBuffer = desc.isIndexBuffer;
    nvrhiDesc.isDrawIndirectArgs = desc.isDrawIndirectArgs;
    nvrhiDesc.canHaveUAVs = desc.isUAV;

    // CPU access
    nvrhiDesc.canHaveRawViews = desc.cpuRead || desc.cpuWrite;
    nvrhiDesc.isVolatile = desc.isVolatile;

    return nvrhiDesc;
}

nvrhi::SamplerDesc RenderDevice::ConvertSamplerDesc(const SamplerDesc& desc) {
    nvrhi::SamplerDesc nvrhiDesc;

    // Address modes
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

    // Filtering
    if (desc.anisotropyEnable) {
        nvrhiDesc.maxAnisotropy = static_cast<float>(desc.maxAnisotropy);
    }

    // Convert filter modes
    bool minLinear = (desc.minFilter == SamplerDesc::Filter::Linear);
    bool magLinear = (desc.magFilter == SamplerDesc::Filter::Linear);
    bool mipLinear = (desc.mipFilter == SamplerDesc::Filter::Linear);

    if (minLinear && magLinear && mipLinear) {
        nvrhiDesc.minFilter = nvrhiDesc.magFilter = nvrhiDesc.mipFilter = true;
    } else if (!minLinear && !magLinear && !mipLinear) {
        nvrhiDesc.minFilter = nvrhiDesc.magFilter = nvrhiDesc.mipFilter = false;
    }

    // LOD parameters
    nvrhiDesc.mipBias = desc.mipLodBias;

    // Comparison sampling
    if (desc.comparisonEnable) {
        nvrhiDesc.reductionType = nvrhi::SamplerReductionType::Comparison;
    }

    // Border color
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

} // namespace xray::render::ng
