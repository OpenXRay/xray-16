#pragma once

#include "AsyncIO.h"
#include "TextureManager.h"
#include "BufferManager.h"
#include "SamplerCache.h"
#include "NativeRTFactory.h"

namespace xray::render::fg {
    class RenderDevice;
}

namespace xray::render::resources {

class FGResourceManager {
public:
    explicit FGResourceManager(xray::render::fg::RenderDevice* device);
    ~FGResourceManager();

    AsyncIOManager* GetIOService() { return m_ioService.get(); }
    TextureManager* GetTextureManager() { return m_textureManager.get(); }
    BufferManager* GetBufferManager() { return m_bufferManager.get(); }
    SamplerCache* GetSamplerCache() { return m_samplerCache.get(); }
    NativeRTFactory* GetRTFactory() { return m_rtFactory.get(); }

    void Update(float deltaTime);

    struct Statistics {
        TextureManager::Statistics textures;
        BufferManager::Statistics buffers;
        NativeRTFactory::Statistics renderTargets;
        AsyncIOManager::Statistics io;
        u32 samplersCached;

        u64 totalMemoryUsed() const {
            return textures.totalMemoryUsed + buffers.totalMemoryUsed + renderTargets.totalMemoryUsed;
        }
    };

    Statistics GetStatistics() const;
    void PrintStatistics() const;

private:
    xray::render::fg::RenderDevice* m_device;

    xr_unique_ptr<AsyncIOManager> m_ioService;
    xr_unique_ptr<TextureManager> m_textureManager;
    xr_unique_ptr<BufferManager> m_bufferManager;
    xr_unique_ptr<SamplerCache> m_samplerCache;
    xr_unique_ptr<NativeRTFactory> m_rtFactory;
};

}
