#pragma once

#include "../ResourceManager/FGResourceManager.h"

namespace xray::render::framegraph {

class FGResourcePool {
public:
    explicit FGResourcePool(resources::FGResourceManager* resourceManager);
    ~FGResourcePool();

    resources::TextureHandle AllocateTexture(const resources::TextureDesc& desc);
    void FreeTexture(resources::TextureHandle handle);

    resources::BufferHandle AllocateBuffer(const resources::BufferDesc& desc);
    void FreeBuffer(resources::BufferHandle handle);

    void Reset();
    void Tick();

private:
    struct PooledTexture {
        resources::TextureHandle handle;
        resources::TextureDesc desc;
        bool inUse = false;
        u32 lastUsedFrame = 0;
    };

    struct PooledBuffer {
        resources::BufferHandle handle;
        resources::BufferDesc desc;
        bool inUse = false;
        u32 lastUsedFrame = 0;
    };

    static bool AreTexturesCompatible(const resources::TextureDesc& a, const resources::TextureDesc& b);
    static bool AreBuffersCompatible(const resources::BufferDesc& a, const resources::BufferDesc& b);

    resources::FGResourceManager* m_resourceManager;
    xr_vector<PooledTexture> m_texturePool;
    xr_vector<PooledBuffer> m_bufferPool;
    u32 m_currentFrame = 0;

    static constexpr u32 kEvictAfterFrames = 4;
};

}
