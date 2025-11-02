#include "stdafx.h"
#include "TextureManager.h"
#include "../RenderContext/RenderDevice.h"

// Modern Texture Manager Implementation
// Week 1 - Day 1: Task 1.4

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

    Msg("! [TextureManager] Created with budget: %llu MB",
        m_memoryBudget / (1024 * 1024));
}

TextureManager::~TextureManager() {
    Msg("! [TextureManager] Destroying...");

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
            Msg("! [TextureManager] Texture already loaded: %s", path);
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

    Msg("! [TextureManager] Registered texture: %s (handle=%u.%u)",
        path, handle.index, handle.generation);

    // TODO: Kick off async load (Week 3)
    // For Week 1, we'll implement LoadTextureSync in Day 2

    m_stats.texturesTotal++;

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

    // TODO: Create NVRHI texture (Day 2)

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

    // Touch for LRU
    meta.lastAccessTime = 0.0f;  // TODO: Use actual time in Week 2
    meta.accessCount++;

    // TODO: If unloaded, trigger load (Day 2)
    if (meta.state == TextureState::Unloaded) {
        Msg("! [TextureManager] ⚠️ Accessing unloaded texture: %s",
            meta.filePath.c_str());
        // LoadTextureSync(handle);  // Will implement in Day 2
    }

    return meta.nvrhiTexture.Get();
}

const TextureMetadata* TextureManager::GetMetadata(TextureHandle handle) const {
    if (!ValidateHandle(handle)) return nullptr;
    return &m_textures[handle.index];
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
    Msg("! [TextureManager] Memory budget set to: %llu MB",
        m_memoryBudget / (1024 * 1024));
}

void TextureManager::RequestMips(TextureHandle handle, u32 mipCount) {
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];
    meta.requestedMips = std::min(mipCount, meta.totalMips);

    // TODO: Forward to streaming manager (Week 2)
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

    // If no more references and not high priority, consider evicting
    if (meta.refCount == 0 && meta.priority >= TexturePriority::Low) {
        // Mark as evictable (will be handled by Update() in Week 2)
    }
}

void TextureManager::Evict(TextureHandle handle) {
    // TODO: Implement in Week 2
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];

    if (meta.state == TextureState::Resident) {
        Msg("! [TextureManager] Evicting: %s", meta.filePath.c_str());
        EvictTextureInternal(handle);
    }
}

// ═══════════════════════════════════════════════════
//  UPDATE (Stub)
// ═══════════════════════════════════════════════════

void TextureManager::Update(float deltaTime) {
    // TODO: Implement streaming and eviction in Week 2
    // For now, just update timers

    for (auto& meta : m_textures) {
        if (!meta.isAlive) continue;
        meta.lastAccessTime += deltaTime;
    }
}

// ═══════════════════════════════════════════════════
//  INTERNAL METHODS (Stubs for Day 2 / Week 2)
// ═══════════════════════════════════════════════════

void TextureManager::LoadTextureSync(TextureHandle handle) {
    // TODO: Implement in Day 2
}

void TextureManager::LoadTextureAsync(TextureHandle handle) {
    // TODO: Implement in Week 3
}

void TextureManager::StreamMips(TextureHandle handle, u32 targetMips) {
    // TODO: Implement in Week 2
}

void TextureManager::EvictTextures(u64 bytesNeeded) {
    // TODO: Implement in Week 2
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

    Msg("! [TextureManager] Evicted: %s", meta.filePath.c_str());
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
//  HELPERS
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

} // namespace xray::render::resources
