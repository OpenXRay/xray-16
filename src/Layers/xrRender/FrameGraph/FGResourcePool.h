#pragma once

#include "../ResourceManager/ModernResourceManager.h"

namespace xray::render::framegraph {

class FGResourcePool {
public:
    FGResourcePool(resources::ModernResourceManager* resourceManager);
    ~FGResourcePool();

    resources::TextureHandle AllocateTexture(const resources::TextureDesc& desc);

    resources::TextureHandle AllocatePersistentTexture(const resources::TextureDesc& desc);

    void FreeTexture(resources::TextureHandle handle);

    resources::BufferHandle AllocateBuffer(const resources::BufferDesc& desc);
    void FreeBuffer(resources::BufferHandle handle);

    void SetAliasingEnabled(bool enabled) { m_aliasingEnabled = enabled; }
    bool IsAliasingEnabled() const { return m_aliasingEnabled; }

    void Reset();

    struct Statistics {
        u32 texturesAllocated = 0;
        u32 texturesAliased = 0;
        u32 texturesActive = 0;

        u64 memoryAllocated = 0;
        u64 memorySaved = 0;
    };

    Statistics GetStatistics() const { return m_stats; }
    void PrintStatistics() const;

private:
    resources::ModernResourceManager* m_resourceManager;

    struct PooledTexture {
        resources::TextureHandle handle;
        resources::TextureDesc desc;
        bool inUse = false;
        u32 lastUsedFrame = 0;
    };

    xr_vector<PooledTexture> m_texturePool;

    resources::TextureHandle FindCompatibleTexture(const resources::TextureDesc& desc);

    bool AreTexturesCompatible(const resources::TextureDesc& a, const resources::TextureDesc& b) const;

    bool m_aliasingEnabled = true;
    u32 m_currentFrame = 0;

    mutable Statistics m_stats;
};

}
