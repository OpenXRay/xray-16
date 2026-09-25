#include "stdafx.h"
#include "TextureManager.h"
#include "DDSLoader.h"
#include "TextureStreaming.h"
#include "../RenderContext/RenderDevice.h"

// Modern Texture Manager Implementation
// Week 1 - Day 1-2: Tasks 1.4, 2.2
// Week 2 - Day 3: Task 3.3-3.4 (Streaming integration)

namespace xray::render::resources {

using namespace xray::render::fg;

// ═══════════════════════════════════════════════════
//  TEXTURE DESC - MEMORY CALCULATION
// ═══════════════════════════════════════════════════

u64 TextureDesc::CalculateMemorySize(u32 startMip, u32 mipCount) const {
    if (mipCount == 0) {
        mipCount = mipLevels;
    }

    u64 totalSize = 0;
    u32 mipWidth = width;
    u32 mipHeight = height;

    // Skip to start mip
    for (u32 i = 0; i < startMip && i < mipLevels; i++) {
        mipWidth = std::max(1u, mipWidth / 2);
        mipHeight = std::max(1u, mipHeight / 2);
    }

    // Calculate size for requested mip range
    for (u32 i = 0; i < mipCount && (startMip + i) < mipLevels; i++) {
        u32 w = std::max(1u, mipWidth);
        u32 h = std::max(1u, mipHeight);

        // Calculate size based on format
        u64 mipSize = 0;

        switch (format) {
            // Block-compressed formats
            case nvrhi::Format::BC1_UNORM:
            case nvrhi::Format::BC1_UNORM_SRGB:
                mipSize = ((w + 3) / 4) * ((h + 3) / 4) * 8;
                break;

            case nvrhi::Format::BC2_UNORM:
            case nvrhi::Format::BC2_UNORM_SRGB:
            case nvrhi::Format::BC3_UNORM:
            case nvrhi::Format::BC3_UNORM_SRGB:
            case nvrhi::Format::BC5_UNORM:
            case nvrhi::Format::BC5_SNORM:
                mipSize = ((w + 3) / 4) * ((h + 3) / 4) * 16;
                break;

            case nvrhi::Format::BC4_UNORM:
            case nvrhi::Format::BC4_SNORM:
                mipSize = ((w + 3) / 4) * ((h + 3) / 4) * 8;
                break;

            case nvrhi::Format::BC6H_UFLOAT:
            case nvrhi::Format::BC6H_SFLOAT:
            case nvrhi::Format::BC7_UNORM:
            case nvrhi::Format::BC7_UNORM_SRGB:
                mipSize = ((w + 3) / 4) * ((h + 3) / 4) * 16;
                break;

            // Uncompressed formats
            case nvrhi::Format::RGBA8_UNORM:
            case nvrhi::Format::RGBA8_SNORM:
            case nvrhi::Format::SRGBA8_UNORM:
            case nvrhi::Format::BGRA8_UNORM:
            case nvrhi::Format::SBGRA8_UNORM:
                mipSize = w * h * 4;
                break;

            case nvrhi::Format::RGBA16_FLOAT:
            case nvrhi::Format::RGBA16_UNORM:
            case nvrhi::Format::RGBA16_SNORM:
                mipSize = w * h * 8;
                break;

            case nvrhi::Format::RGBA32_FLOAT:
                mipSize = w * h * 16;
                break;

            case nvrhi::Format::R8_UNORM:
                mipSize = w * h;
                break;

            case nvrhi::Format::RG8_UNORM:
                mipSize = w * h * 2;
                break;

            case nvrhi::Format::R16_FLOAT:
            case nvrhi::Format::R16_UNORM:
                mipSize = w * h * 2;
                break;

            case nvrhi::Format::RG16_FLOAT:
                mipSize = w * h * 4;
                break;

            case nvrhi::Format::D24S8:
            case nvrhi::Format::D32:
                mipSize = w * h * 4;
                break;

            default:
                // Assume 32bpp for unknown formats
                mipSize = w * h * 4;
                break;
        }

        totalSize += mipSize;

        mipWidth = std::max(1u, mipWidth / 2);
        mipHeight = std::max(1u, mipHeight / 2);
    }

    // Multiply by array size for texture arrays/cubemaps
    totalSize *= arraySize;
    totalSize *= std::max(1u, sampleCount);

    return totalSize;
}

bool TextureKey::operator<(const TextureKey& other) const {
    if (colorSpace != other.colorSpace)
        return colorSpace < other.colorSpace;
    return path < other.path;
}

// ═══════════════════════════════════════════════════
//  CONSTRUCTION
// ═══════════════════════════════════════════════════

TextureManager::TextureManager(RenderDevice* device)
    : m_device(device)
{
    VERIFY(m_device);

    // Pre-allocate some capacity
    m_textures.reserve(1024);

    // Create streaming manager
    m_streamingManager = xr_make_unique<StreamingManager>(device, this);

    // Msg("! [TextureManager] Created with budget: %llu MB",
    //     m_memoryBudget / (1024 * 1024));
}

TextureManager::~TextureManager() {
    // Msg("! [TextureManager] Destroying...");

    // Check for leaks
    u32 leakCount = 0;
    for (const auto& tex : m_textures) {
        if (tex.isAlive) {
            Msg("! [TextureManager] ⚠️ Leak: %s (refCount=%u)",
                tex.filePath.c_str(), tex.refCount);
            leakCount++;
        }
    }

    if (leakCount > 0) {
        Msg("! [TextureManager] ❌ %u texture leaks detected!", leakCount);
    }

    PrintStatistics();
}

// ═══════════════════════════════════════════════════
//  HANDLE MANAGEMENT
// ═══════════════════════════════════════════════════

TextureHandle TextureManager::AllocateHandle() {
    u32 index;
    u32 generation;

    if (!m_freeSlots.empty()) {
        // Reuse freed slot
        index = m_freeSlots.back();
        m_freeSlots.pop_back();

        // Increment generation to invalidate old handles
        generation = m_textures[index].generation + 1;
        m_textures[index].generation = generation;
    } else {
        // Allocate new slot
        index = (u32)m_textures.size();
        generation = 0;

        m_textures.emplace_back();
        m_textures[index].generation = generation;
    }

    return TextureHandle(index, generation);
}

void TextureManager::FreeHandle(TextureHandle handle) {
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];
    meta.isAlive = false;

    if (!meta.filePath.empty()) {
        const auto mapped = m_pathToHandle.find(TextureKey{ meta.filePath, meta.colorSpace });
        if (mapped != m_pathToHandle.end() && mapped->second == handle)
            m_pathToHandle.erase(mapped);
    }

    meta.nvrhiTexture = nullptr;
    meta.videoTextureData.reset();
    meta.sequenceTextureData.reset();
    meta.filePath = shared_str();
    meta.colorSpace = fg::TextureColorSpace::Linear;
    meta.state = TextureState::Unloaded;
    meta.refCount = 0;
    meta.memoryUsed = 0;
    meta.residentMips = 0;
    meta.requestedMips = 0;
    meta.totalMips = 0;
    meta.lastAccessTime = 0.0f;
    meta.accessCount = 0;
    meta.videoActiveThisFrame = false;

    m_freeSlots.push_back(handle.index);
}

bool TextureManager::ValidateHandle(TextureHandle handle) const {
    if (!handle.IsValid()) return false;
    if (handle.index >= m_textures.size()) return false;

    const TextureMetadata& meta = m_textures[handle.index];
    if (!meta.isAlive) return false;
    if (meta.generation != handle.generation) return false;  // Stale!

    return true;
}

// ═══════════════════════════════════════════════════
//  LOADING (Stub - Will Implement Fully in Day 2)
// ═══════════════════════════════════════════════════

TextureHandle TextureManager::LoadTexture(
    const char* path,
    TexturePriority priority)
{
    return LoadTexture(path, fg::TextureColorSpace::Linear, priority);
}

TextureHandle TextureManager::LoadTexture(
    const char* path,
    fg::TextureColorSpace colorSpace,
    TexturePriority priority)
{
    const TextureKey key{ shared_str(path), colorSpace };

    auto it = m_pathToHandle.find(key);
    if (it != m_pathToHandle.end()) {
        TextureHandle existing = it->second;
        if (ValidateHandle(existing)) {
            AddRef(existing);
            return existing;
        }
    }

    TextureHandle handle = AllocateHandle();
    TextureMetadata& meta = m_textures[handle.index];

    meta.filePath = key.path;
    meta.colorSpace = colorSpace;
    meta.state = TextureState::Unloaded;
    meta.priority = priority;
    meta.isAlive = true;
    meta.refCount = 1;

    m_pathToHandle[key] = handle;

    m_stats.texturesTotal++;

    LoadTextureSync(handle);

    return handle;
}

TextureHandle TextureManager::CreateTexture(
    const TextureDesc& desc,
    const void* initialData)
{
    // Allocate handle
    TextureHandle handle = AllocateHandle();
    TextureMetadata& meta = m_textures[handle.index];

    // Setup metadata
    meta.desc = desc;
    meta.state = TextureState::Unloaded;
    meta.priority = TexturePriority::High;  // Runtime textures are important
    meta.isAlive = true;
    meta.refCount = 1;

    // Create NVRHI texture immediately for runtime textures
    nvrhi::TextureDesc nvrhiDesc;
    nvrhiDesc.width = desc.width;
    nvrhiDesc.height = desc.height;
    nvrhiDesc.depth = desc.depth;
    nvrhiDesc.arraySize = desc.arraySize;
    nvrhiDesc.mipLevels = desc.mipLevels;
    nvrhiDesc.format = desc.format;
    nvrhiDesc.sampleCount = std::max(1u, desc.sampleCount);
    nvrhiDesc.debugName = desc.debugName.c_str();
    nvrhiDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    nvrhiDesc.keepInitialState = true;  // D3D12 requires state tracking

    // Set dimension
    switch (desc.type) {
        case TextureDesc::Texture1D:
            nvrhiDesc.dimension = nvrhi::TextureDimension::Texture1D;
            break;
        case TextureDesc::Texture2D:
            nvrhiDesc.dimension = nvrhiDesc.sampleCount > 1
                ? nvrhi::TextureDimension::Texture2DMS
                : nvrhi::TextureDimension::Texture2D;
            break;
        case TextureDesc::Texture2DArray:
            nvrhiDesc.dimension = nvrhiDesc.sampleCount > 1
                ? nvrhi::TextureDimension::Texture2DMSArray
                : nvrhi::TextureDimension::Texture2DArray;
            break;
        case TextureDesc::Texture3D:
            nvrhiDesc.dimension = nvrhi::TextureDimension::Texture3D;
            break;
        case TextureDesc::TextureCube:
            nvrhiDesc.dimension = nvrhi::TextureDimension::TextureCube;
            break;
        default:
            nvrhiDesc.dimension = nvrhi::TextureDimension::Texture2D;
            break;
    }

    // Set usage flags
    nvrhiDesc.isRenderTarget = desc.isRenderTarget;
    nvrhiDesc.isUAV = desc.isUAV;
    nvrhiDesc.isShaderResource = true;

    // Set optimized clear value for render targets (D3D12 performance optimization)
    if (desc.isRenderTarget && !desc.isDepthStencil) {
        nvrhiDesc.useClearValue = true;
        nvrhiDesc.clearValue = nvrhi::Color(0.0f);
    }

    // Depth/stencil handling
    if (desc.isDepthStencil) {
        nvrhiDesc.isRenderTarget = true;
        nvrhiDesc.isTypeless = true;
        nvrhiDesc.useClearValue = true;
        nvrhiDesc.clearValue = nvrhi::Color(0.0f);
    }

    // Create texture
    meta.nvrhiTexture = m_device->GetNVRHIDevice()->createTexture(nvrhiDesc);

    if (meta.nvrhiTexture) {
        meta.state = TextureState::Resident;
        meta.residentMips = desc.mipLevels;
        meta.requestedMips = desc.mipLevels;

        // Calculate memory
        meta.memoryUsed = desc.CalculateMemorySize();
        m_stats.texturesResident++;
        m_memoryUsed += meta.memoryUsed;

        // Msg("~ [TextureManager] Created runtime texture '%s': %ux%ux%u, %.2f MB",
        //     desc.debugName.c_str(),
        //     desc.width, desc.height, desc.depth,
        //     meta.memoryUsed / (1024.0f * 1024.0f));
    } else {
        Msg("! [TextureManager] Failed to create NVRHI texture '%s'", desc.debugName.c_str());
    }

    m_stats.texturesTotal++;

    return handle;
}

TextureHandle TextureManager::ImportTexture(
    nvrhi::TextureHandle nvrhiTexture,
    const TextureDesc& desc,
    const char* debugName)
{
    // Allocate handle
    TextureHandle handle = AllocateHandle();
    TextureMetadata& meta = m_textures[handle.index];

    // Setup metadata
    meta.desc = desc;
    meta.filePath = debugName;
    meta.state = TextureState::Resident;
    meta.priority = TexturePriority::Critical;  // Imported textures (like backbuffer) never evict
    meta.isAlive = true;
    meta.refCount = 1;
    meta.nvrhiTexture = nvrhiTexture;
    meta.residentMips = desc.mipLevels;
    meta.totalMips = desc.mipLevels;
    meta.memoryUsed = desc.CalculateMemorySize();

    m_memoryUsed += meta.memoryUsed;

    m_stats.texturesTotal++;
    m_stats.texturesResident++;

    return handle;
}

// ═══════════════════════════════════════════════════
//  ACCESS
// ═══════════════════════════════════════════════════

nvrhi::ITexture* TextureManager::GetNVRHITexture(TextureHandle handle) {
    if (!ValidateHandle(handle)) return nullptr;

    TextureMetadata& meta = m_textures[handle.index];

    // ═══════════════════════════════════════════════════
    //  AUTO-TOUCH FOR LRU
    // ═══════════════════════════════════════════════════

    meta.lastAccessTime = 0.0f;  // Reset LRU timer
    meta.accessCount++;

    // Mark video as active for this frame (enables decoding)
    if (meta.videoTextureData) {
        meta.videoActiveThisFrame = true;
    }

    if (meta.state == TextureState::Unloaded)
        LoadTextureSync(handle);

    return meta.nvrhiTexture.Get();
}

const TextureMetadata* TextureManager::GetMetadata(TextureHandle handle) const {
    if (!ValidateHandle(handle)) return nullptr;
    return &m_textures[handle.index];
}

void TextureManager::NotifyContentChanged(TextureMetadata& metadata)
{
    std::lock_guard<std::mutex> lock(m_texturesMutex);
    metadata.contentRevision = ++m_contentRevision;
}

u64 TextureManager::GetContentRevision(const xr_set<nvrhi::ITexture*>& textures,
    nvrhi::ITexture* sky0, nvrhi::ITexture* sky1) const
{
    std::lock_guard<std::mutex> lock(m_texturesMutex);
    u64 revision = 0;
    for (const auto& metadata : m_textures)
    {
        auto* texture = metadata.nvrhiTexture.Get();
        if (metadata.isAlive && texture && (texture == sky0 || texture == sky1 || textures.find(texture) != textures.end()))
            revision = std::max(revision, metadata.contentRevision);
    }
    return revision;
}

TextureHandle TextureManager::FindTexture(const char* path, fg::TextureColorSpace colorSpace) const {
    auto it = m_pathToHandle.find(TextureKey{ shared_str(path), colorSpace });
    if (it != m_pathToHandle.end()) {
        TextureHandle handle = it->second;
        if (ValidateHandle(handle))
            return handle;
    }
    return TextureHandle();  // Invalid handle
}

bool TextureManager::IsResident(TextureHandle handle) const {
    if (!ValidateHandle(handle)) return false;
    return m_textures[handle.index].IsResident();
}

// ═══════════════════════════════════════════════════
//  STREAMING CONTROL (Stubs for Week 2)
// ═══════════════════════════════════════════════════

void TextureManager::RequestMips(TextureHandle handle, u32 mipCount) {
    if (!ValidateHandle(handle)) return;

    const TextureMetadata& meta = m_textures[handle.index];
    u32 targetMips = std::min(mipCount, meta.totalMips);

    // Update requested count
    m_textures[handle.index].requestedMips = targetMips;

    // Forward to streaming manager
    m_streamingManager->RequestMips(handle, targetMips, meta.priority);
}

// ═══════════════════════════════════════════════════
//  LIFECYCLE
// ═══════════════════════════════════════════════════

void TextureManager::AddRef(TextureHandle handle) {
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];
    meta.refCount++;
}

void TextureManager::Release(TextureHandle handle) {
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];

    if (meta.refCount > 0) {
        meta.refCount--;
    }

    // If no more references, free the handle immediately
    if (meta.refCount == 0) {
        // Release GPU resources
        if (meta.nvrhiTexture) {
            m_memoryUsed -= meta.memoryUsed;
            meta.nvrhiTexture = nullptr;
            meta.memoryUsed = 0;
        }

        // Free the handle slot for reuse
        FreeHandle(handle);
    }
}

// ═══════════════════════════════════════════════════
//  UPDATE (Week 2 - Integrated Streaming)
// ═══════════════════════════════════════════════════

void TextureManager::Update(float deltaTime) {
    // Update timers
    for (auto& meta : m_textures) {
        if (!meta.isAlive) continue;
        meta.lastAccessTime += deltaTime;
    }

    // Update video textures (Week 6)
    UpdateVideoTextures();

    // Update sequence textures (animated .seq)
    UpdateSequenceTextures(deltaTime);

    // Update streaming
    m_streamingManager->Update(deltaTime);
}

// ═══════════════════════════════════════════════════
//  INTERNAL METHODS (Stubs for Day 2 / Week 2)
// ═══════════════════════════════════════════════════


void TextureManager::LoadTextureSync(TextureHandle handle) {
    if (!ValidateHandle(handle)) {
        Msg("! [TextureManager] LoadTextureSync: Invalid handle");
        return;
    }

    TextureMetadata& meta = m_textures[handle.index];

    // Already loaded?
    if (meta.state == TextureState::Resident) {
        return;
    }

    // Mark as loading
    meta.state = TextureState::Loading;

    // ═══════════════════════════════════════════════════
    //  LOAD TEXTURE FILE (AUTO-DETECTS .DDS OR .OGM)
    // ═══════════════════════════════════════════════════
    // DDSLoader now automatically detects file type:
    // - .dds → Static DDS texture
    // - .ogm → Theora video texture (dynamic, per-frame decode)

    DDSData ddsData;
    if (!DDSLoader::LoadFromFile(meta.filePath.c_str(), ddsData)) {
        Msg("! [TextureManager] Failed to load texture: %s", meta.filePath.c_str());
        meta.state = TextureState::Missing;
        return;
    }

    // Check texture type
    bool isVideoTexture = (ddsData.type == DDSData::TextureType::Video);
    bool isSequenceTexture = (ddsData.type == DDSData::TextureType::Sequence);

    // if (isVideoTexture) {
    //     Msg("* [TextureManager] Video texture detected: %s", meta.filePath.c_str());
    // }
    // if (isSequenceTexture) {
    //     Msg("* [TextureManager] Sequence texture detected: %s (%u frames)",
    //         meta.filePath.c_str(),
    //         (u32)ddsData.sequenceState->frameData.size());
    // }

    const TextureDesc sourceDesc = ddsData.desc;
    const u64 sourceDataSize = ddsData.totalDataSize;

    nvrhi::TextureDesc textureDesc;
    textureDesc.width = sourceDesc.width;
    textureDesc.height = sourceDesc.height;
    textureDesc.depth = sourceDesc.depth;
    textureDesc.arraySize = sourceDesc.arraySize;
    textureDesc.mipLevels = sourceDesc.mipLevels;
    textureDesc.format = fg::FormatForColorSpace(sourceDesc.format, meta.colorSpace);
    textureDesc.debugName = ddsData.filePath.c_str();
    textureDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    textureDesc.keepInitialState = true;
    switch (sourceDesc.type) {
        case TextureDesc::Texture1D:
            textureDesc.dimension = nvrhi::TextureDimension::Texture1D;
            break;
        case TextureDesc::Texture2DArray:
            textureDesc.dimension = nvrhi::TextureDimension::Texture2DArray;
            break;
        case TextureDesc::Texture3D:
            textureDesc.dimension = nvrhi::TextureDimension::Texture3D;
            break;
        case TextureDesc::TextureCube:
            textureDesc.dimension = nvrhi::TextureDimension::TextureCube;
            break;
        default:
            textureDesc.dimension = nvrhi::TextureDimension::Texture2D;
            break;
    }

    nvrhi::TextureHandle texture = m_device->GetNVRHIDevice()->createTexture(textureDesc);
    if (!texture) {
        Msg("! [TextureManager] Failed to create NVRHI texture: %s", meta.filePath.c_str());
        meta.state = TextureState::Missing;
        return;
    }

    const u32 mipsPerSlice = std::max(1u, sourceDesc.mipLevels);
    xr_vector<fg::RenderDevice::TextureSliceData> slices;
    slices.reserve(ddsData.mipLevels.size());
    for (u32 index = 0; index < ddsData.mipLevels.size(); ++index) {
        const DDSMipLevel& mip = ddsData.mipLevels[index];
        fg::RenderDevice::TextureSliceData slice;
        slice.arraySlice = index / mipsPerSlice;
        slice.mipLevel = index % mipsPerSlice;
        slice.data = mip.data;
        slice.dataSize = mip.size;
        slice.rowPitch = mip.rowPitch;
        slice.slicePitch = mip.slicePitch;
        slices.push_back(slice);
    }
    m_device->UploadTextureSlices(texture, slices.data(), (u32)slices.size());

    meta.nvrhiTexture = texture;
    meta.desc = sourceDesc;
    meta.desc.format = textureDesc.format;
    meta.desc.isSRGB = nvrhi::getFormatInfo(textureDesc.format).isSRGB;

    if (isVideoTexture) {
        meta.videoTextureData = xr_make_unique<DDSData>();
        *meta.videoTextureData = std::move(ddsData);
    }

    if (isSequenceTexture) {
        meta.sequenceTextureData = xr_make_unique<DDSData>();
        *meta.sequenceTextureData = std::move(ddsData);
        meta.sequenceTextureData->sequenceState->currentFrame = 0;
    }

    const bool singleFrame = isVideoTexture || isSequenceTexture;
    meta.state = TextureState::Resident;
    meta.residentMips = singleFrame ? 1 : sourceDesc.mipLevels;
    meta.totalMips = singleFrame ? 1 : sourceDesc.mipLevels;
    meta.requestedMips = singleFrame ? 1 : sourceDesc.mipLevels;
    meta.memoryUsed = sourceDataSize;

    m_memoryUsed += meta.memoryUsed;
}

void TextureManager::StreamMips(TextureHandle handle, u32 targetMips) {
    // Forward to streaming manager
    m_streamingManager->RequestMips(handle, targetMips, TexturePriority::Medium);
}

// ═══════════════════════════════════════════════════
//  STATISTICS
// ═══════════════════════════════════════════════════

TextureManager::Statistics TextureManager::GetStatistics() const {
    m_stats.totalMemoryUsed = m_memoryUsed;
    m_stats.memoryBudget = m_memoryBudget;
    m_stats.texturesTotal = 0;
    m_stats.texturesResident = 0;
    m_stats.texturesLoading = 0;

    for (const auto& meta : m_textures) {
        if (!meta.isAlive) continue;

        m_stats.texturesTotal++;

        switch (meta.state) {
            case TextureState::Resident:
                m_stats.texturesResident++;
                break;
            case TextureState::Loading:
                m_stats.texturesLoading++;
                break;
            default:
                break;
        }
    }

    return m_stats;
}

void TextureManager::PrintStatistics() const {
    auto stats = GetStatistics();

    Msg("! [TextureManager] Statistics:");
    Msg("!   Memory: %llu / %llu MB (%.1f%%)",
        stats.totalMemoryUsed / (1024 * 1024),
        stats.memoryBudget / (1024 * 1024),
        stats.memoryUsagePercent());
    Msg("!   Textures: %u total, %u resident, %u loading",
        stats.texturesTotal,
        stats.texturesResident,
        stats.texturesLoading);
}

// ═══════════════════════════════════════════════════
//  THREAD-SAFE OPERATIONS (Week 3)
// ═══════════════════════════════════════════════════

TextureHandle TextureManager::LoadTextureThreadSafe(
    const char* path,
    TexturePriority priority)
{
    const TextureKey key{ shared_str(path), fg::TextureColorSpace::Linear };

    // Check if already loaded (thread-safe)
    {
        std::lock_guard<std::mutex> lock(m_pathLookupMutex);

        auto it = m_pathToHandle.find(key);
        if (it != m_pathToHandle.end()) {
            TextureHandle existing = it->second;

            if (ValidateHandleThreadSafe(existing)) {
                AddRef(existing);
                return existing;
            }
        }
    }

    // Allocate handle (thread-safe)
    TextureHandle handle = AllocateHandleThreadSafe();

    // Setup metadata
    {
        std::lock_guard<std::mutex> lock(m_texturesMutex);

        TextureMetadata& meta = m_textures[handle.index];
        meta.filePath = key.path;
        meta.colorSpace = key.colorSpace;
        meta.state = TextureState::Unloaded;
        meta.priority = priority;
        meta.isAlive = true;
        meta.refCount = 1;  // Initial reference
    }

    // Register path
    {
        std::lock_guard<std::mutex> lock(m_pathLookupMutex);
        m_pathToHandle[key] = handle;
    }

    // Msg("! [TextureManager] LoadTextureThreadSafe: %s", path);

    // Load synchronously (thread-safe mutex protection above ensures correctness)
    LoadTextureSync(handle);

    return handle;
}

TextureHandle TextureManager::AllocateHandleThreadSafe() {
    std::lock_guard<std::mutex> lock(m_texturesMutex);
    return AllocateHandle();  // Use existing non-thread-safe version under lock
}

bool TextureManager::ValidateHandleThreadSafe(TextureHandle handle) const {
    std::lock_guard<std::mutex> lock(m_texturesMutex);
    return ValidateHandle(handle);  // Use existing non-thread-safe version under lock
}

// ═══════════════════════════════════════════════════

const char* TextureStateToString(TextureState state) {
    switch (state) {
        case TextureState::Unloaded: return "Unloaded";
        case TextureState::Loading: return "Loading";
        case TextureState::Resident: return "Resident";
        case TextureState::Missing: return "Missing";
        default: return "Unknown";
    }
}

const char* TexturePriorityToString(TexturePriority priority) {
    switch (priority) {
        case TexturePriority::Critical: return "Critical";
        case TexturePriority::High: return "High";
        case TexturePriority::Medium: return "Medium";
        case TexturePriority::Low: return "Low";
        case TexturePriority::VeryLow: return "VeryLow";
        default: return "Unknown";
    }
}

// ═══════════════════════════════════════════════════
//  VIDEO TEXTURE UPDATE (Week 6)
// ═══════════════════════════════════════════════════

void TextureManager::UpdateVideoTextures() {
    static int s_frameCount = 0;
    s_frameCount++;

    // Iterate through all textures and update video textures
    int videoTextureCount = 0;
    int updatedCount = 0;
    int skippedInactive = 0;

    for (auto& meta : m_textures) {
        if (!meta.isAlive || !meta.videoTextureData) {
            continue;  // Not a video texture
        }

        videoTextureCount++;

        // OPTIMIZATION: Only update videos that were rendered recently
        // Videos that aren't visible on screen don't need CPU decoding
        if (!meta.videoActiveThisFrame && meta.lastAccessTime > 0.5f) {
            skippedInactive++;
            continue;  // Skip inactive videos (saves ~3ms per video)
        }

        // Reset active flag for next frame
        meta.videoActiveThisFrame = false;

        // Decode next frame
        bool frameChanged = DDSLoader::UpdateVideoFrame(*meta.videoTextureData, Device.dwTimeContinual);

        if (!frameChanged) {
            continue;  // Same frame, no update needed
        }

        updatedCount++;

        auto* videoState = meta.videoTextureData->videoState;
        if (!videoState || !videoState->needsUpdate) {
            Msg("! [TextureManager] Frame changed but needsUpdate=false for: %s", meta.filePath.c_str());
            continue;
        }

        // ═══════════════════════════════════════════════════
        //  UPLOAD NEW FRAME TO GPU (Use RenderDevice's upload path)
        // ═══════════════════════════════════════════════════
        // This uses the persistent upload command list with proper mutex locking

        nvrhi::ITexture* nvrhiTex = meta.nvrhiTexture;
        if (!nvrhiTex) {
            Msg("! [TextureManager] Video texture has no NVRHI texture: %s", meta.filePath.c_str());
            continue;
        }

        // Calculate data size (RGBA8 format)
        u32 texWidth = videoState->textureWidth;
        u32 texHeight = videoState->textureHeight;
        size_t dataSize = texWidth * texHeight * 4;  // RGBA8 = 4 bytes per pixel

        // Upload using RenderDevice's persistent upload command list
        // This handles command list management, mutex locking, and GPU sync
        m_device->UploadTextureDataToNVRHI(
            nvrhiTex,
            0,  // arraySlice
            0,  // mipLevel
            videoState->frameBuffer.data(),
            dataSize
        );
        NotifyContentChanged(meta);

        // Clear the update flag
        videoState->needsUpdate = false;

        // if (s_frameCount % 60 == 0) {  // Log every 60 frames
        //     Msg("* [TextureManager] Updated video texture: %s (%ux%u)",
        //         meta.filePath.c_str(), texWidth, texHeight);
        // }
    }

    // if (s_frameCount % 120 == 0 && videoTextureCount > 0) {  // Log every 120 frames
    //     Msg("* [TextureManager] UpdateVideoTextures: %d video textures, %d updated this frame",
    //         videoTextureCount, updatedCount);
    // }
}

void TextureManager::UpdateSequenceTextures(float deltaTime) {
    // Animate .seq texture sequences (cursor, animated UI elements, etc.)
    // Uses absolute time like OGM videos, so it works in paused menus

    // Get absolute time (milliseconds since engine start, unaffected by pause)
    u32 currentTime = Device.dwTimeContinual;

    for (auto& meta : m_textures) {
        if (!meta.isAlive || !meta.sequenceTextureData) {
            continue;  // Not a sequence texture
        }

        auto* seqState = meta.sequenceTextureData->sequenceState;
        if (!seqState || seqState->frameData.empty()) {
            continue;
        }

        // Initialize lastUpdateTime on first update
        if (seqState->lastUpdateTime == 0) {
            seqState->lastUpdateTime = currentTime;
            continue;
        }

        // Calculate elapsed time since last update
        u32 elapsedMs = currentTime - seqState->lastUpdateTime;

        // Check if we should advance frame
        if (elapsedMs >= seqState->msPerFrame) {
            // Update last update time
            seqState->lastUpdateTime = currentTime;

            // Advance frame
            u32 nextFrame = seqState->currentFrame + 1;

            // Handle cycling
            if (nextFrame >= seqState->frameData.size()) {
                if (seqState->cycled) {
                    nextFrame = 0;  // Loop back to start
                } else {
                    nextFrame = seqState->currentFrame;  // Stay on last frame
                    continue;
                }
            }

            // Update frame buffer with next frame's pixels (OGM-style pattern)
            if (DDSLoader::UpdateSequenceFrame(*meta.sequenceTextureData, nextFrame)) {
                // Upload to GPU (reuse existing texture, just update data)
                nvrhi::ITexture* nvrhiTex = meta.nvrhiTexture;
                if (!nvrhiTex) {
                    continue;
                }

                // Get the actual frame data (has correct rowPitch/slicePitch for compressed formats)
                const DDSMipLevel& frameData = seqState->frameData[nextFrame];

                // Upload frame buffer with correct pitch for compressed formats
                m_device->UploadTextureDataToNVRHI(
                    nvrhiTex,
                    0,  // arraySlice
                    0,  // mipLevel
                    seqState->frameBuffer.data(),
                    frameData.size,
                    frameData.rowPitch,
                    frameData.slicePitch
                );
                NotifyContentChanged(meta);

                seqState->currentFrame = nextFrame;
                seqState->needsUpdate = false;  // Mark as uploaded
            }
        }
    }
}

} // namespace xray::render::resources
