#include "stdafx.h"
#include "FGResourcePool.h"

namespace xray::render::framegraph {

FGResourcePool::FGResourcePool(resources::ModernResourceManager* resourceManager)
    : m_resourceManager(resourceManager)
{
    VERIFY(m_resourceManager);
    Msg("! [FGResourcePool] Created");
}

FGResourcePool::~FGResourcePool() {
    Reset();
    PrintStatistics();
}

resources::TextureHandle FGResourcePool::AllocateTexture(const resources::TextureDesc& desc) {
    m_stats.texturesAllocated++;

    if (m_aliasingEnabled) {
        resources::TextureHandle pooled = FindCompatibleTexture(desc);

        if (pooled.IsValid()) {
            m_stats.texturesAliased++;
            m_stats.memorySaved += desc.CalculateMemorySize();

            Msg("! [FGResourcePool] ✅ Aliased texture: %s", desc.debugName.c_str());
            return pooled;
        }
    }

    resources::TextureHandle handle =
        m_resourceManager->GetTextureManager()->CreateTexture(desc);

    m_stats.texturesActive++;
    m_stats.memoryAllocated += desc.CalculateMemorySize();

    Msg("! [FGResourcePool] Allocated texture: %s", desc.debugName.c_str());

    return handle;
}

resources::TextureHandle FGResourcePool::AllocatePersistentTexture(const resources::TextureDesc& desc) {
    resources::TextureHandle handle =
        m_resourceManager->GetTextureManager()->CreateTexture(desc);

    m_stats.texturesAllocated++;
    m_stats.texturesActive++;
    m_stats.memoryAllocated += desc.CalculateMemorySize();

    Msg("! [FGResourcePool] Allocated persistent texture: %s", desc.debugName.c_str());

    return handle;
}

void FGResourcePool::FreeTexture(resources::TextureHandle handle) {
    if (!handle.IsValid()) return;

    const resources::TextureMetadata* meta =
        m_resourceManager->GetTextureManager()->GetMetadata(handle);

    if (!meta) return;

    if (m_aliasingEnabled) {
        PooledTexture pooled;
        pooled.handle = handle;
        pooled.desc = meta->desc;
        pooled.inUse = false;
        pooled.lastUsedFrame = m_currentFrame;

        m_texturePool.push_back(pooled);

        Msg("! [FGResourcePool] Freed texture to pool: %s", meta->desc.debugName.c_str());
    } else {
        m_resourceManager->GetTextureManager()->Release(handle);
        m_stats.texturesActive--;
    }
}

resources::BufferHandle FGResourcePool::AllocateBuffer(const resources::BufferDesc& desc) {
    resources::BufferHandle handle =
        m_resourceManager->GetBufferManager()->CreateBuffer(desc);

    Msg("! [FGResourcePool] Allocated buffer: %s", desc.debugName.c_str());

    return handle;
}

void FGResourcePool::FreeBuffer(resources::BufferHandle handle) {
    if (!handle.IsValid()) return;

    m_resourceManager->GetBufferManager()->Release(handle);

    Msg("! [FGResourcePool] Freed buffer");
}

resources::TextureHandle FGResourcePool::FindCompatibleTexture(const resources::TextureDesc& desc) {
    for (auto& pooled : m_texturePool) {
        if (!pooled.inUse && AreTexturesCompatible(pooled.desc, desc)) {
            pooled.inUse = true;
            pooled.lastUsedFrame = m_currentFrame;

            return pooled.handle;
        }
    }

    return resources::TextureHandle();
}

bool FGResourcePool::AreTexturesCompatible(
    const resources::TextureDesc& a,
    const resources::TextureDesc& b) const
{
    return a.width == b.width &&
           a.height == b.height &&
           a.format == b.format &&
           a.mipLevels == b.mipLevels &&
           a.arraySize == b.arraySize;
}

void FGResourcePool::Reset() {
    for (auto& pooled : m_texturePool) {
        m_resourceManager->GetTextureManager()->Release(pooled.handle);
    }

    m_texturePool.clear();
    m_stats.texturesActive = 0;

    m_currentFrame++;

    Msg("! [FGResourcePool] Reset (frame=%u)", m_currentFrame);
}

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

}
