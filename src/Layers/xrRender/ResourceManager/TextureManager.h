#pragma once

#include "ResourceHandle.h"
#include "xrCommon/xr_set.h"
#include "Layers/xrRender/ColorSpace.h"
#include <nvrhi/nvrhi.h>
#include <mutex>

// Modern Texture Manager
// Week 1 - Day 1: Tasks 1.2-1.3
// Week 3 - Day 5: Task 5.4 - Thread safety

namespace xray::render::fg {
    class RenderDevice;  // Forward declaration
}

namespace xray::render::resources {

struct DDSData;           // Forward declaration for video texture support

// ═══════════════════════════════════════════════════
//  TEXTURE STATE TRACKING
// ═══════════════════════════════════════════════════

enum class TextureState : u8 {
    Unloaded,       // Not in memory (on disk only)
    Loading,        // Async load in progress
    Resident,       // Fully loaded in VRAM
    Missing,
};

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

// ═══════════════════════════════════════════════════
//  TEXTURE DESCRIPTOR (Logical Description)
// ═══════════════════════════════════════════════════

struct TextureDesc {
    enum Type {
        Texture1D,
        Texture2D,
        Texture2DArray,
        Texture3D,
        TextureCube
    };

    Type type = Texture2D;

    u32 width = 0;
    u32 height = 0;
    u32 depth = 1;           // For 3D textures
    u32 arraySize = 1;       // For texture arrays/cubemaps
    u32 mipLevels = 1;
    u32 sampleCount = 1;

    nvrhi::Format format = nvrhi::Format::RGBA8_UNORM;

    // Usage flags
    bool isRenderTarget = false;
    bool isDepthStencil = false;
    bool isUAV = false;
    bool isSRGB = false;     // sRGB color space

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
    fg::TextureColorSpace colorSpace = fg::TextureColorSpace::Linear;

    // State
    TextureState state = TextureState::Unloaded;
    TexturePriority priority = TexturePriority::Medium;
    u32 generation = 0;          // For handle validation
    bool isAlive = true;         // Still valid?

    // Memory tracking
    u64 memoryUsed = 0;          // Bytes in VRAM

    // Usage tracking (for eviction)
    float lastAccessTime = 0.0f; // Time since last use
    u32 refCount = 0;            // Active references

    // Physical resource
    nvrhi::TextureHandle nvrhiTexture;  // May be null if unloaded
    u64 contentRevision = 0;

    // Video texture support (Week 6)
    xr_unique_ptr<DDSData> videoTextureData;  // Only set for video textures (.ogm/.avi)
    bool videoActiveThisFrame = false;        // Set when video is rendered, cleared each frame

    // Sequence texture support (animated .seq textures like cursor)
    xr_unique_ptr<DDSData> sequenceTextureData;  // Only set for sequence textures (.seq)

    // Async loading
    struct LoadRequest {
        bool inProgress = false;
        u32 targetMips = 0;
        // Add thread handle, etc. in Week 3
    } loadRequest;
};

class TextureKey
{
public:
    shared_str path;
    fg::TextureColorSpace colorSpace = fg::TextureColorSpace::Linear;

    bool operator<(const TextureKey& other) const;
};

// ═══════════════════════════════════════════════════
//  TEXTURE MANAGER (Main Interface)
// ═══════════════════════════════════════════════════

class TextureManager {
public:
    explicit TextureManager(xray::render::fg::RenderDevice* device);
    ~TextureManager();

    // ═══════════════════════════════════════════════════
    //  LOADING
    // ═══════════════════════════════════════════════════

    TextureHandle LoadTexture(
        const char* path,
        TexturePriority priority = TexturePriority::Medium
    );

    TextureHandle LoadTexture(
        const char* path,
        fg::TextureColorSpace colorSpace,
        TexturePriority priority = TexturePriority::Medium
    );

    // Create runtime texture (not from disk)
    TextureHandle CreateTexture(
        const TextureDesc& desc,
        const void* initialData = nullptr
    );

    // ═══════════════════════════════════════════════════
    //  ACCESS
    // ═══════════════════════════════════════════════════

    // Get NVRHI texture (may trigger load if unloaded)
    nvrhi::ITexture* GetNVRHITexture(TextureHandle handle);

    // Get metadata (for inspection)
    const TextureMetadata* GetMetadata(TextureHandle handle) const;
    void NotifyContentChanged(TextureMetadata& metadata);
    u64 GetContentRevision(const xr_set<nvrhi::ITexture*>& textures,
        nvrhi::ITexture* sky0, nvrhi::ITexture* sky1) const;

    // Find texture by path (returns invalid handle if not found)
    TextureHandle FindTexture(const char* path, fg::TextureColorSpace colorSpace = fg::TextureColorSpace::Linear) const;

    // ═══════════════════════════════════════════════════
    //  STREAMING CONTROL (Week 2)
    // ═══════════════════════════════════════════════════

    u64 GetMemoryBudget() const { return m_memoryBudget; }

    // ═══════════════════════════════════════════════════
    //  LIFECYCLE
    // ═══════════════════════════════════════════════════

    // Increment reference count
    void AddRef(TextureHandle handle);

    // Decrement reference count (evict if zero)
    void Release(TextureHandle handle);

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

        float memoryUsagePercent() const {
            if (memoryBudget == 0) return 0.0f;
            return (float)totalMemoryUsed / (float)memoryBudget * 100.0f;
        }
    };

    Statistics GetStatistics() const;
    void PrintStatistics() const;

private:
    xray::render::fg::RenderDevice* m_device;

    // ═══════════════════════════════════════════════════
    //  RESOURCE STORAGE (Sparse Array)
    // ═══════════════════════════════════════════════════

    xr_vector<TextureMetadata> m_textures;
    xr_vector<u32> m_freeSlots;  // Reusable indices
    u64 m_contentRevision = 0;

    // Name → Handle lookup (for deduplication)
    xr_map<TextureKey, TextureHandle> m_pathToHandle;

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

    // Video texture update (Week 6)
    void UpdateVideoTextures();

    // Sequence texture animation (.seq files like cursor)
    void UpdateSequenceTextures(float deltaTime);

    // ═══════════════════════════════════════════════════
    //  THREAD SAFETY (Week 3)
    // ═══════════════════════════════════════════════════

    mutable std::mutex m_texturesMutex;     // Protects m_textures

    // Statistics
    mutable Statistics m_stats;
};

TextureManager* GetActiveTextureManager();

class TextureRef
{
public:
    TextureRef() = default;
    TextureRef(const TextureRef& other);
    TextureRef(TextureRef&& other) noexcept;
    ~TextureRef();

    TextureRef& operator=(const TextureRef& other);
    TextureRef& operator=(TextureRef&& other) noexcept;

    nvrhi::ITexture* Load(const char* path, fg::TextureColorSpace colorSpace);
    nvrhi::ITexture* Get() const;
    void Reset();

private:
    TextureHandle m_handle;
};

} // namespace xray::render::resources
