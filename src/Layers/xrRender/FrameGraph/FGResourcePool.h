#pragma once

#include "../ResourceManager/FGResourceManager.h"

// FrameGraph Resource Pool
// Week 4: FrameGraph Integration with ResourceManager

namespace xray::render::framegraph {

// ═══════════════════════════════════════════════════
//  FRAMEGRAPH RESOURCE POOL
//  (Uses ResourceManager for Physical Allocation)
// ═══════════════════════════════════════════════════

class FGResourcePool {
public:
    FGResourcePool(resources::FGResourceManager* resourceManager);
    ~FGResourcePool();

    // ═══════════════════════════════════════════════════
    //  TEXTURE ALLOCATION
    // ═══════════════════════════════════════════════════

    // Allocate transient texture (short-lived, can be aliased)
    resources::TextureHandle AllocateTexture(const resources::TextureDesc& desc);

    // Allocate persistent texture (long-lived, imported)
    resources::TextureHandle AllocatePersistentTexture(const resources::TextureDesc& desc);

    // Free texture (return to pool for aliasing)
    void FreeTexture(resources::TextureHandle handle);

    // ═══════════════════════════════════════════════════
    //  BUFFER ALLOCATION
    // ═══════════════════════════════════════════════════

    resources::BufferHandle AllocateBuffer(const resources::BufferDesc& desc);
    void FreeBuffer(resources::BufferHandle handle);

    // ═══════════════════════════════════════════════════
    //  ALIASING CONTROL
    // ═══════════════════════════════════════════════════

    // Enable/disable memory aliasing (reusing freed resources)
    void SetAliasingEnabled(bool enabled) { m_aliasingEnabled = enabled; }
    bool IsAliasingEnabled() const { return m_aliasingEnabled; }

    // Reset pool (free all transient resources)
    void Reset();

    // ═══════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════

    struct Statistics {
        u32 texturesAllocated = 0;
        u32 texturesAliased = 0;      // Reused from pool
        u32 texturesActive = 0;

        u64 memoryAllocated = 0;
        u64 memorySaved = 0;          // Saved via aliasing
    };

    Statistics GetStatistics() const { return m_stats; }
    void PrintStatistics() const;

private:
    resources::FGResourceManager* m_resourceManager;

    // ═══════════════════════════════════════════════════
    //  TEXTURE POOL (For Aliasing)
    // ═══════════════════════════════════════════════════

    struct PooledTexture {
        resources::TextureHandle handle;
        resources::TextureDesc desc;
        bool inUse = false;
        u32 lastUsedFrame = 0;
    };

    xr_vector<PooledTexture> m_texturePool;

    // Try to find compatible texture in pool
    resources::TextureHandle FindCompatibleTexture(const resources::TextureDesc& desc);

    // Check if two texture descs are compatible for aliasing
    bool AreTexturesCompatible(const resources::TextureDesc& a, const resources::TextureDesc& b) const;

    // ═══════════════════════════════════════════════════
    //  CONFIGURATION
    // ═══════════════════════════════════════════════════

    bool m_aliasingEnabled = true;
    u32 m_currentFrame = 0;

    // Statistics
    mutable Statistics m_stats;
};

} // namespace xray::render::framegraph
