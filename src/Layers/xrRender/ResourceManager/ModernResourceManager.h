#pragma once

#include "TextureManager.h"
#include "BufferManager.h"
#include "SamplerCache.h"

namespace xray::render::ng {
    class RenderDevice;
}

namespace xray::render::resources {

class ModernResourceManager {
public:
    explicit ModernResourceManager(xray::render::ng::RenderDevice* device);
    ~ModernResourceManager();

    TextureManager* GetTextureManager() { return m_textureManager.get(); }
    BufferManager* GetBufferManager() { return m_bufferManager.get(); }
    SamplerCache* GetSamplerCache() { return m_samplerCache.get(); }

    void BeginFrame();
    void EndFrame();
    void Update(float deltaTime);

    struct Statistics {
        TextureManager::Statistics textures;
        BufferManager::Statistics buffers;
        u32 samplersCached;

        u64 totalMemoryUsed() const {
            return textures.totalMemoryUsed + buffers.totalMemoryUsed;
        }
    };

    Statistics GetStatistics() const;
    void PrintStatistics() const;

private:
    xray::render::ng::RenderDevice* m_device;

    xr_unique_ptr<TextureManager> m_textureManager;
    xr_unique_ptr<BufferManager> m_bufferManager;
    xr_unique_ptr<SamplerCache> m_samplerCache;
};

}
