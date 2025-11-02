#pragma once

#include "ResourceHandle.h"
#include <nvrhi/nvrhi.h>

// Modern Texture Manager
// Week 1 - Day 1: Tasks 1.2-1.3

namespace xray::render::ng {
    class RenderDevice;  // Forward declaration
}

namespace xray::render::resources {

// ═══════════════════════════════════════════════════
//  TEXTURE STATE TRACKING
// ═══════════════════════════════════════════════════

enum class TextureState : u8 {
    Unloaded,       // Not in memory (on disk only)
    Loading,        // Async load in progress
    Resident,       // Fully loaded in VRAM
    Evicting,       // Marked for eviction
    Evicted,        // Was resident, now evicted (keep metadata)
};

const char* TextureStateToString(TextureState state);

// ═══════════════════════════════════════════════════
//  TEXTURE PRIORITY (For Streaming Decisions)
// ═══════════════════════════════════════════════════

enum class TexturePriority : u8 {
    Critical = 0,   // Never evict (UI, HUD, player weapon)
    High = 1,       // Visible this frame
    Medium = 2,     // Visible recently (within last 5 frames)
    Low = 3,        // Not visible, keep if memory available
    VeryLow = 4,    // Evict first
};

const char* TexturePriorityToString(TexturePriority priority);

// ═══════════════════════════════════════════════════
//  TEXTURE DESCRIPTOR (Logical Description)
// ═══════════════════════════════════════════════════

struct TextureDesc {
    enum Type {
        Texture1D,
        Texture2D,
        Texture3D,
        TextureCube
    };

    Type type = Texture2D;

    u32 width = 0;
    u32 height = 0;
    u32 depth = 1;           // For 3D textures
    u32 arraySize = 1;       // For texture arrays/cubemaps
    u32 mipLevels = 1;

    nvrhi::Format format = nvrhi::Format::RGBA8_UNORM;

    // Usage flags
    bool isRenderTarget = false;
    bool isDepthStencil = false;
    bool isUAV = false;
    bool isSRGB = false;     // sRGB color space

    // Streaming hints
    bool allowStreaming = true;   // Can stream mips?
    u32 minResidentMips = 3;      // Always keep this many mips

    shared_str debugName;

    // Calculate memory size for specific mip range
    u64 CalculateMemorySize(u32 startMip = 0, u32 mipCount = 0) const;
};

// ═══════════════════════════════════════════════════
//  TEXTURE METADATA (Runtime Tracking)
// ═══════════════════════════════════════════════════

struct TextureMetadata {
    // Identity
    shared_str filePath;         // "textures/concrete_diff.dds"
    TextureDesc desc;

    // State
    TextureState state = TextureState::Unloaded;
    TexturePriority priority = TexturePriority::Medium;
    u32 generation = 0;          // For handle validation
    bool isAlive = true;         // Still valid?

    // Streaming state
    u32 residentMips = 0;        // How many mips currently loaded
    u32 requestedMips = 0;       // How many mips needed
    u32 totalMips = 0;           // Total mips available on disk

    // Memory tracking
    u64 memoryUsed = 0;          // Bytes in VRAM

    // Usage tracking (for eviction)
    float lastAccessTime = 0.0f; // Time since last use
    u32 accessCount = 0;         // Total access count
    u32 refCount = 0;            // Active references

    // Physical resource
    nvrhi::TextureHandle nvrhiTexture;  // May be null if unloaded

    // Async loading
    struct LoadRequest {
        bool inProgress = false;
        u32 targetMips = 0;
        // Add thread handle, etc. in Week 3
    } loadRequest;

    // Helpers
    bool IsResident() const {
        return state == TextureState::Resident;
    }

    bool NeedsStreaming() const {
        return requestedMips > residentMips && state == TextureState::Resident;
    }

    bool CanEvict() const {
        return priority >= TexturePriority::Low &&
               refCount == 0 &&
               state == TextureState::Resident;
    }
};

// ═══════════════════════════════════════════════════
//  TEXTURE MANAGER (Main Interface)
// ═══════════════════════════════════════════════════

class TextureManager {
public:
    explicit TextureManager(xray::render::ng::RenderDevice* device);
    ~TextureManager();

    // ═══════════════════════════════════════════════════
    //  LOADING
    // ═══════════════════════════════════════════════════

    // Load texture from disk (Week 1: synchronous, Week 3: async)
    TextureHandle LoadTexture(
        const char* path,
        TexturePriority priority = TexturePriority::Medium
    );

    // Create runtime texture (not from disk)
    TextureHandle CreateTexture(
        const TextureDesc& desc,
        const void* initialData = nullptr
    );

    // Create from existing NVRHI texture (e.g. backbuffer)
    TextureHandle ImportTexture(
        nvrhi::TextureHandle nvrhiTexture,
        const TextureDesc& desc,
        const char* debugName
    );

    // ═══════════════════════════════════════════════════
    //  ACCESS
    // ═══════════════════════════════════════════════════

    // Get NVRHI texture (may trigger load if unloaded)
    nvrhi::ITexture* GetNVRHITexture(TextureHandle handle);

    // Get metadata (for inspection)
    const TextureMetadata* GetMetadata(TextureHandle handle) const;

    // Check if texture is resident
    bool IsResident(TextureHandle handle) const;

    // ═══════════════════════════════════════════════════
    //  STREAMING CONTROL (Week 2)
    // ═══════════════════════════════════════════════════

    // Set memory budget (total VRAM for textures)
    void SetMemoryBudget(u64 bytes);
    u64 GetMemoryBudget() const { return m_memoryBudget; }

    // Request specific number of mips (for LOD)
    void RequestMips(TextureHandle handle, u32 mipCount);

    // Change priority (affects eviction order)
    void SetPriority(TextureHandle handle, TexturePriority priority);

    // Mark as accessed (updates LRU)
    void Touch(TextureHandle handle);

    // ═══════════════════════════════════════════════════
    //  LIFECYCLE
    // ═══════════════════════════════════════════════════

    // Increment reference count
    void AddRef(TextureHandle handle);

    // Decrement reference count (evict if zero)
    void Release(TextureHandle handle);

    // Force eviction
    void Evict(TextureHandle handle);

    // ═══════════════════════════════════════════════════
    //  UPDATE (Per Frame)
    // ═══════════════════════════════════════════════════

    void Update(float deltaTime);

    // ═══════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════

    struct Statistics {
        u64 totalMemoryUsed = 0;
        u64 memoryBudget = 0;

        u32 texturesTotal = 0;
        u32 texturesResident = 0;
        u32 texturesLoading = 0;
        u32 texturesEvicted = 0;

        u32 streamingRequestsPending = 0;
        u32 evictionsPending = 0;

        float memoryUsagePercent() const {
            if (memoryBudget == 0) return 0.0f;
            return (float)totalMemoryUsed / (float)memoryBudget * 100.0f;
        }
    };

    Statistics GetStatistics() const;
    void PrintStatistics() const;

private:
    xray::render::ng::RenderDevice* m_device;

    // ═══════════════════════════════════════════════════
    //  RESOURCE STORAGE (Sparse Array)
    // ═══════════════════════════════════════════════════

    xr_vector<TextureMetadata> m_textures;
    xr_vector<u32> m_freeSlots;  // Reusable indices

    // Name → Handle lookup (for deduplication)
    xr_map<shared_str, TextureHandle> m_pathToHandle;

    // ═══════════════════════════════════════════════════
    //  MEMORY MANAGEMENT
    // ═══════════════════════════════════════════════════

    u64 m_memoryBudget = 2ULL * 1024 * 1024 * 1024;  // 2GB default
    u64 m_memoryUsed = 0;

    // ═══════════════════════════════════════════════════
    //  INTERNAL METHODS
    // ═══════════════════════════════════════════════════

    // Handle allocation
    TextureHandle AllocateHandle();
    void FreeHandle(TextureHandle handle);
    bool ValidateHandle(TextureHandle handle) const;

    // Loading (Week 1: sync, Week 2: async)
    void LoadTextureSync(TextureHandle handle);
    void LoadTextureAsync(TextureHandle handle);  // Week 3
    void StreamMips(TextureHandle handle, u32 targetMips);  // Week 2

    // Eviction (Week 2)
    void EvictTextures(u64 bytesNeeded);
    void EvictTextureInternal(TextureHandle handle);

    // Statistics
    mutable Statistics m_stats;
};

} // namespace xray::render::resources
