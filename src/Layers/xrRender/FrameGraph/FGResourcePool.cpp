#include "stdafx.h"
#include "FGResourcePool.h"

// FrameGraph Resource Pool Implementation
// Week 4: FrameGraph Integration with ResourceManager

namespace xray::render::framegraph {

FGResourcePool::FGResourcePool(resources::FGResourceManager* resourceManager)
    : m_resourceManager(resourceManager)
{
    VERIFY(m_resourceManager);
}

FGResourcePool::~FGResourcePool() {
    Reset();
    PrintStatistics();
}

// ═══════════════════════════════════════════════════
//  TEXTURE ALLOCATION
// ═══════════════════════════════════════════════════

resources::TextureHandle FGResourcePool::AllocateTexture(const resources::TextureDesc& desc) {
    m_stats.texturesAllocated++;

    // Try to find compatible texture in pool (if aliasing enabled)
    if (m_aliasingEnabled) {
        resources::TextureHandle pooled = FindCompatibleTexture(desc);

        if (pooled.IsValid()) {
            m_stats.texturesAliased++;
            m_stats.memorySaved += desc.CalculateMemorySize();
            return pooled;
        }
    }

    // No compatible texture found - allocate new
    resources::TextureHandle handle =
        m_resourceManager->GetTextureManager()->CreateTexture(desc);

    m_stats.texturesActive++;
    m_stats.memoryAllocated += desc.CalculateMemorySize();

    return handle;
}

resources::TextureHandle FGResourcePool::AllocatePersistentTexture(const resources::TextureDesc& desc) {
    // Persistent textures are never aliased
    resources::TextureHandle handle =
        m_resourceManager->GetTextureManager()->CreateTexture(desc);

    m_stats.texturesAllocated++;
    m_stats.texturesActive++;
    m_stats.memoryAllocated += desc.CalculateMemorySize();


    return handle;
}

void FGResourcePool::FreeTexture(resources::TextureHandle handle) {
    if (!handle.IsValid()) {
        Msg("! [FGResourcePool] FreeTexture called with invalid handle");
        return;
    }

    const resources::TextureMetadata* meta =
        m_resourceManager->GetTextureManager()->GetMetadata(handle);

    if (!meta) {
        Msg("! [FGResourcePool] FreeTexture: No metadata for handle");
        return;
    }

    // Add to pool for potential reuse
    if (m_aliasingEnabled) {
        // Check if this texture is already in the pool
        bool foundInPool = false;
        for (auto& pooled : m_texturePool) {
            if (pooled.handle.index == handle.index && pooled.handle.generation == handle.generation) {
                // Already in pool - just mark as available
                pooled.inUse = false;
                pooled.lastUsedFrame = m_currentFrame;
                foundInPool = true;
                Msg("~ [FGResourcePool] FreeTexture: Marked as available in pool");
                break;
            }
        }

        // If not in pool, add it
        if (!foundInPool) {
            PooledTexture pooled;
            pooled.handle = handle;
            pooled.desc = meta->desc;
            pooled.inUse = false;
            pooled.lastUsedFrame = m_currentFrame;

            m_texturePool.push_back(pooled);
            Msg("~ [FGResourcePool] FreeTexture: Added NEW to pool (pool size=%u)", (u32)m_texturePool.size());
        }

    } else {
        // Aliasing disabled - actually destroy
        m_resourceManager->GetTextureManager()->Release(handle);
        m_stats.texturesActive--;
        Msg("~ [FGResourcePool] FreeTexture: Released (aliasing disabled)");
    }
}

// ═══════════════════════════════════════════════════
//  BUFFER ALLOCATION
// ═══════════════════════════════════════════════════

resources::BufferHandle FGResourcePool::AllocateBuffer(const resources::BufferDesc& desc) {
    // For now, always allocate new buffers (no aliasing)
    // TODO: Implement buffer pooling similar to textures
    resources::BufferHandle handle =
        m_resourceManager->GetBufferManager()->CreateBuffer(desc);


    return handle;
}

void FGResourcePool::FreeBuffer(resources::BufferHandle handle) {
    if (!handle.IsValid()) return;

    // For now, immediately release
    // TODO: Pool buffers for reuse
    m_resourceManager->GetBufferManager()->Release(handle);

}

// ═══════════════════════════════════════════════════
//  INTERNAL HELPERS
// ═══════════════════════════════════════════════════

resources::TextureHandle FGResourcePool::FindCompatibleTexture(const resources::TextureDesc& desc) {
    for (auto& pooled : m_texturePool) {
        if (!pooled.inUse && AreTexturesCompatible(pooled.desc, desc)) {
            pooled.inUse = true;
            pooled.lastUsedFrame = m_currentFrame;

            return pooled.handle;
        }
    }

    return resources::TextureHandle();  // Not found
}

bool FGResourcePool::AreTexturesCompatible(
    const resources::TextureDesc& a,
    const resources::TextureDesc& b) const
{
    // Must match exactly for now
    // Could be more flexible (allow larger texture for smaller request, etc.)
    return a.width == b.width &&
           a.height == b.height &&
           a.format == b.format &&
           a.mipLevels == b.mipLevels &&
           a.arraySize == b.arraySize;
}

// ═══════════════════════════════════════════════════
//  POOL MANAGEMENT
// ═══════════════════════════════════════════════════

void FGResourcePool::Reset() {
    // Release all pooled textures
    for (auto& pooled : m_texturePool) {
        m_resourceManager->GetTextureManager()->Release(pooled.handle);
    }

    m_texturePool.clear();

    // Reset ALL statistics (not just texturesActive!)
    // The old code only reset texturesActive, causing accumulated stats to leak
    m_stats.texturesAllocated = 0;
    m_stats.texturesAliased = 0;
    m_stats.texturesActive = 0;
    m_stats.memoryAllocated = 0;
    m_stats.memorySaved = 0;

    m_currentFrame++;

}

// ═══════════════════════════════════════════════════
//  STATISTICS
// ═══════════════════════════════════════════════════

void FGResourcePool::PrintStatistics() const {
    Msg("! [FGResourcePool] Statistics:");
    Msg("!   Allocated: %u, Aliased: %u, Active: %u",
        m_stats.texturesAllocated,
        m_stats.texturesAliased,
        m_stats.texturesActive);
    Msg("!   Memory: %llu MB allocated, %llu MB saved via aliasing",
        m_stats.memoryAllocated / (1024 * 1024),
        m_stats.memorySaved / (1024 * 1024));
}

} // namespace xray::render::framegraph
