#include "stdafx.h"
#include "TextureManager.h"
#include "DDSLoader.h"
#include "TextureStreaming.h"
#include "../RenderContext/RenderDevice.h"
#include "xrEngine/IFrameGraphRender.h"

// Modern Texture Manager Implementation
// Week 1 - Day 1-2: Tasks 1.4, 2.2
// Week 2 - Day 3: Task 3.3-3.4 (Streaming integration)

namespace xray::render::resources {

using namespace xray::render::ng;

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

    return totalSize;
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

    // Remove from path lookup
    if (!meta.filePath.empty()) {
        m_pathToHandle.erase(meta.filePath);
    }

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
    shared_str pathStr = path;

    // Check if already loaded (deduplication)
    auto it = m_pathToHandle.find(pathStr);
    if (it != m_pathToHandle.end()) {
        TextureHandle existing = it->second;
        if (ValidateHandle(existing)) {
            AddRef(existing);
            return existing;
        }
    }

    // Allocate handle
    TextureHandle handle = AllocateHandle();
    TextureMetadata& meta = m_textures[handle.index];

    // Setup metadata
    meta.filePath = pathStr;
    meta.state = TextureState::Unloaded;
    meta.priority = priority;
    meta.isAlive = true;
    meta.refCount = 1;  // Start with 1 reference

    // Register path
    m_pathToHandle[pathStr] = handle;

    m_stats.texturesTotal++;

    // Load synchronously (Week 1)
    // Week 3 will add async loading
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
    nvrhiDesc.debugName = desc.debugName.c_str();
    nvrhiDesc.initialState = nvrhi::ResourceStates::Common;
    nvrhiDesc.keepInitialState = true;  // D3D12 requires state tracking

    // Set dimension
    switch (desc.type) {
        case TextureDesc::Texture1D:
            nvrhiDesc.dimension = nvrhi::TextureDimension::Texture1D;
            break;
        case TextureDesc::Texture2D:
            nvrhiDesc.dimension = nvrhi::TextureDimension::Texture2D;
            break;
        case TextureDesc::Texture2DArray:
            nvrhiDesc.dimension = nvrhi::TextureDimension::Texture2DArray;
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

    // Depth/stencil handling
    if (desc.isDepthStencil) {
        nvrhiDesc.isRenderTarget = true;
        nvrhiDesc.isTypeless = true;
        nvrhiDesc.useClearValue = true;
        nvrhiDesc.clearValue = nvrhi::Color(1.0f);
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
        m_stats.totalMemoryUsed += meta.memoryUsed;

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

    // ═══════════════════════════════════════════════════
    //  AUTO-RELOAD IF EVICTED
    // ═══════════════════════════════════════════════════

    if (meta.state == TextureState::Unloaded || meta.state == TextureState::Evicted) {
        // Msg("! [TextureManager] ⚠️ Accessing evicted texture: %s - reloading...",
        //     meta.filePath.c_str());
        LoadTextureSync(handle);
    }

    return meta.nvrhiTexture.Get();
}

const TextureMetadata* TextureManager::GetMetadata(TextureHandle handle) const {
    if (!ValidateHandle(handle)) return nullptr;
    return &m_textures[handle.index];
}

TextureHandle TextureManager::FindTexture(const char* path) const {
    auto it = m_pathToHandle.find(shared_str(path));
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

void TextureManager::SetMemoryBudget(u64 bytes) {
    m_memoryBudget = bytes;
    // Msg("! [TextureManager] Memory budget set to: %llu MB",
    //     m_memoryBudget / (1024 * 1024));
}

void TextureManager::RequestMips(TextureHandle handle, u32 mipCount) {
    if (!ValidateHandle(handle)) return;

    const TextureMetadata& meta = m_textures[handle.index];
    u32 targetMips = std::min(mipCount, meta.totalMips);

    // Update requested count
    m_textures[handle.index].requestedMips = targetMips;

    // Forward to streaming manager
    m_streamingManager->RequestMips(handle, targetMips, meta.priority);
}

void TextureManager::SetPriority(TextureHandle handle, TexturePriority priority) {
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];
    meta.priority = priority;
}

void TextureManager::Touch(TextureHandle handle) {
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];
    meta.lastAccessTime = 0.0f;
    meta.accessCount++;
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

void TextureManager::Evict(TextureHandle handle) {
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];

    if (meta.state == TextureState::Resident) {
        // Msg("! [TextureManager] Evicting: %s", meta.filePath.c_str());
        EvictTextureInternal(handle);
    }
}

// ═══════════════════════════════════════════════════
//  MEMORY BUDGET ENFORCEMENT (Week 2 - Day 3)
// ═══════════════════════════════════════════════════

bool TextureManager::CheckMemoryBudget(u64 requiredBytes) const {
    return (m_memoryUsed + requiredBytes) <= m_memoryBudget;
}

bool TextureManager::EnforceMemoryBudget(u64 requiredBytes) {
    if (CheckMemoryBudget(requiredBytes)) {
        return true;  // Within budget
    }

    u64 bytesNeeded = (m_memoryUsed + requiredBytes) - m_memoryBudget;

    // Msg("! [TextureManager] Over budget! Need to free %llu MB",
    //     bytesNeeded / (1024 * 1024));

    // Evict textures to make room
    return EvictTextures(bytesNeeded);
}

bool TextureManager::EvictTextures(u64 bytesNeeded) {
    u64 bytesFreed = 0;

    // ═══════════════════════════════════════════════════
    //  BUILD EVICTION CANDIDATES (LRU + Priority)
    // ═══════════════════════════════════════════════════

    struct EvictionCandidate {
        TextureHandle handle;
        float score;  // Higher = evict first
        u64 memoryUsed;

        bool operator<(const EvictionCandidate& other) const {
            return score > other.score;  // Descending order
        }
    };

    xr_vector<EvictionCandidate> candidates;

    for (u32 i = 0; i < m_textures.size(); i++) {
        const TextureMetadata& meta = m_textures[i];

        if (!meta.isAlive) continue;
        if (!meta.CanEvict()) continue;  // Check priority, refCount

        // Calculate eviction score
        // Higher score = more likely to evict
        float score = 0.0f;

        // Factor 1: Time since last access (LRU)
        score += meta.lastAccessTime * 10.0f;

        // Factor 2: Priority (low priority = evict first)
        score += (float)meta.priority * 100.0f;

        // Factor 3: Access count (less used = evict first)
        score -= (float)meta.accessCount * 0.1f;

        // Factor 4: Memory used (larger = prefer to evict for space)
        score += (float)meta.memoryUsed / (1024.0f * 1024.0f);

        EvictionCandidate candidate;
        candidate.handle = TextureHandle(i, meta.generation);
        candidate.score = score;
        candidate.memoryUsed = meta.memoryUsed;

        candidates.push_back(candidate);
    }

    // Sort by score
    std::sort(candidates.begin(), candidates.end());

    // Msg("! [TextureManager] Found %u eviction candidates", candidates.size());

    // ═══════════════════════════════════════════════════
    //  EVICT UNTIL WE HAVE ENOUGH SPACE
    // ═══════════════════════════════════════════════════

    for (const auto& candidate : candidates) {
        if (bytesFreed >= bytesNeeded) {
            break;  // Freed enough
        }

        const TextureMetadata* meta = GetMetadata(candidate.handle);

        // Msg("!   Evicting: %s (score=%.2f, %llu KB)",
        //     meta->filePath.c_str(),
        //     candidate.score,
        //     candidate.memoryUsed / 1024);

        EvictTextureInternal(candidate.handle);

        bytesFreed += candidate.memoryUsed;
    }

    // Msg("! [TextureManager] Freed %llu MB (needed %llu MB)",
    //     bytesFreed / (1024 * 1024),
    //     bytesNeeded / (1024 * 1024));

    return bytesFreed >= bytesNeeded;
}

void TextureManager::EvictTextureInternal(TextureHandle handle) {
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];

    if (meta.state != TextureState::Resident) {
        return;  // Already evicted
    }

    // Release NVRHI texture
    if (meta.nvrhiTexture) {
        // NVRHI will destroy when ref count reaches zero
        meta.nvrhiTexture = nullptr;
    }

    // Update state
    meta.state = TextureState::Evicted;
    meta.residentMips = 0;

    // Update memory tracking
    m_memoryUsed -= meta.memoryUsed;
    meta.memoryUsed = 0;

    // Msg("! [TextureManager] Evicted: %s", meta.filePath.c_str());
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

    // Check if over budget (trigger eviction)
    if (m_memoryUsed > m_memoryBudget) {
        u64 excess = m_memoryUsed - m_memoryBudget;
        EvictTextures(excess);
    }
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
        meta.state = TextureState::Unloaded;
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

    // ═══════════════════════════════════════════════════
    //  CHECK MEMORY BUDGET BEFORE ALLOCATING TEXTURE
    // ═══════════════════════════════════════════════════

    u64 requiredMemory = ddsData.totalDataSize;

    if (!EnforceMemoryBudget(requiredMemory)) {
        Msg("! [TextureManager] ❌ Cannot load %s - out of memory! (need %llu MB, have %llu / %llu MB used)",
            meta.filePath.c_str(),
            requiredMemory / (1024 * 1024),
            m_memoryUsed / (1024 * 1024),
            m_memoryBudget / (1024 * 1024));
        meta.state = TextureState::Unloaded;
        return;
    }

    // Create NVRHI texture (without initial data)
    ng::RenderDevice::TextureDesc deviceDesc;
    deviceDesc.width = ddsData.desc.width;
    deviceDesc.height = ddsData.desc.height;
    deviceDesc.depth = ddsData.desc.depth;
    deviceDesc.arraySize = ddsData.desc.arraySize;
    deviceDesc.mipLevels = ddsData.desc.mipLevels;
    deviceDesc.format = ddsData.desc.format;
    deviceDesc.debugName = ddsData.filePath.c_str();

    // Determine dimension
    switch (ddsData.desc.type) {
        case TextureDesc::Texture1D:
            deviceDesc.dimension = ng::RenderDevice::TextureDesc::Texture1D;
            break;
        case TextureDesc::Texture2D:
            deviceDesc.dimension = ng::RenderDevice::TextureDesc::Texture2D;
            break;
        case TextureDesc::Texture3D:
            deviceDesc.dimension = ng::RenderDevice::TextureDesc::Texture3D;
            break;
        case TextureDesc::TextureCube:
            deviceDesc.dimension = ng::RenderDevice::TextureDesc::TextureCube;
            break;
    }

    // Create texture (no initial data - we'll upload separately)
    ng::TextureHandle deviceHandle = m_device->CreateTexture(deviceDesc, nullptr);
    if (!deviceHandle.IsValid()) {
        Msg("! [TextureManager] Failed to create NVRHI texture: %s", meta.filePath.c_str());
        meta.state = TextureState::Unloaded;
        return;
    }

    // Upload all mip levels using RenderDevice's upload API
    xr_vector<ng::RenderDevice::TextureSliceData> slices;
    slices.reserve(ddsData.mipLevels.size());

    for (const DDSMipLevel& mip : ddsData.mipLevels) {
        ng::RenderDevice::TextureSliceData slice;

        // Calculate which array slice and mip this belongs to
        // DDS stores: [slice0_mip0, slice0_mip1, ..., slice1_mip0, slice1_mip1, ...]
        u32 totalMips = ddsData.desc.mipLevels;
        u32 mipIndex = (u32)(&mip - &ddsData.mipLevels[0]);

        slice.arraySlice = mipIndex / totalMips;
        slice.mipLevel = mipIndex % totalMips;
        slice.data = mip.data;
        slice.dataSize = mip.size;
        slice.rowPitch = mip.rowPitch;      // Use pitch calculated by DDSLoader
        slice.slicePitch = mip.slicePitch;  // Use pitch calculated by DDSLoader

        slices.push_back(slice);
    }

    // Upload all slices in one command list
    m_device->UploadTextureData(deviceHandle, slices.data(), (u32)slices.size());

    // Store NVRHI handle
    meta.nvrhiTexture = m_device->GetNativeTexture(deviceHandle);

    // ═══════════════════════════════════════════════════
    //  STORE VIDEO TEXTURE STATE (IF APPLICABLE)
    // ═══════════════════════════════════════════════════

    if (isVideoTexture) {
        // Move DDSData into metadata so we can update it each frame
        meta.videoTextureData = xr_make_unique<DDSData>();
        *meta.videoTextureData = std::move(ddsData);  // Transfer ownership

        // Msg("* [TextureManager] Video texture state stored for: %s", meta.filePath.c_str());
    }

    if (isSequenceTexture) {
        // Move DDSData into metadata so we can animate frames
        meta.sequenceTextureData = xr_make_unique<DDSData>();
        *meta.sequenceTextureData = std::move(ddsData);  // Transfer ownership

        // Initialize animation state
        meta.sequenceTextureData->sequenceState->currentFrame = 0;
        meta.sequenceTextureData->sequenceState->elapsedTime = 0.0f;

        // Msg("* [TextureManager] Sequence texture state stored for: %s", meta.filePath.c_str());
    }

    // Update metadata
    meta.state = TextureState::Resident;
    meta.residentMips = (isVideoTexture || isSequenceTexture) ? 1 : ddsData.desc.mipLevels;  // Video/sequence textures have 1 mip
    meta.totalMips = (isVideoTexture || isSequenceTexture) ? 1 : ddsData.desc.mipLevels;
    meta.requestedMips = (isVideoTexture || isSequenceTexture) ? 1 : ddsData.desc.mipLevels;
    meta.memoryUsed = ddsData.totalDataSize;

    // Update memory tracking
    m_memoryUsed += meta.memoryUsed;
}

void TextureManager::LoadTextureAsync(TextureHandle handle) {
    // TODO: Implement in Week 3
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
    m_stats.texturesEvicted = 0;

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
            case TextureState::Evicted:
                m_stats.texturesEvicted++;
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
    Msg("!   Textures: %u total, %u resident, %u loading, %u evicted",
        stats.texturesTotal,
        stats.texturesResident,
        stats.texturesLoading,
        stats.texturesEvicted);
}

// ═══════════════════════════════════════════════════
//  THREAD-SAFE OPERATIONS (Week 3)
// ═══════════════════════════════════════════════════

TextureHandle TextureManager::LoadTextureThreadSafe(
    const char* path,
    TexturePriority priority)
{
    shared_str pathStr = path;

    // Check if already loaded (thread-safe)
    {
        std::lock_guard<std::mutex> lock(m_pathLookupMutex);

        auto it = m_pathToHandle.find(pathStr);
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
        meta.filePath = pathStr;
        meta.state = TextureState::Unloaded;
        meta.priority = priority;
        meta.isAlive = true;
        meta.refCount = 1;  // Initial reference
    }

    // Register path
    {
        std::lock_guard<std::mutex> lock(m_pathLookupMutex);
        m_pathToHandle[pathStr] = handle;
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
        case TextureState::Evicting: return "Evicting";
        case TextureState::Evicted: return "Evicted";
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

    for (auto& meta : m_textures) {
        if (!meta.isAlive || !meta.videoTextureData) {
            continue;  // Not a video texture
        }

        videoTextureCount++;

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
    // Follows the same pattern as UpdateVideoTextures (OGM-style)

    for (auto& meta : m_textures) {
        if (!meta.isAlive || !meta.sequenceTextureData) {
            continue;  // Not a sequence texture
        }

        auto* seqState = meta.sequenceTextureData->sequenceState;
        if (!seqState || seqState->frameData.empty()) {
            continue;
        }

        // Update animation timer
        seqState->elapsedTime += deltaTime * 1000.0f;  // Convert to milliseconds

        // Check if we should advance frame
        if (seqState->elapsedTime >= seqState->msPerFrame) {
            seqState->elapsedTime -= seqState->msPerFrame;

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

                seqState->currentFrame = nextFrame;
                seqState->needsUpdate = false;  // Mark as uploaded
            }
        }
    }
}

} // namespace xray::render::resources
