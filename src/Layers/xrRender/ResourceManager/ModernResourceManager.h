#pragma once

#include "TextureManager.h"
#include "BufferManager.h"
#include "SamplerCache.h"
#include "NativeRTFactory.h"

// Unified Modern Resource Manager
// Week 2 - Day 4: Task 4.4
// Updated: Phase 1 - Native Resource Management

namespace xray::render::ng {
    class RenderDevice;  // Forward declaration
}

namespace xray::render::resources {

// ═══════════════════════════════════════════════════
//  UNIFIED RESOURCE MANAGER
// ═══════════════════════════════════════════════════

class ModernResourceManager {
public:
    explicit ModernResourceManager(xray::render::ng::RenderDevice* device);
    ~ModernResourceManager();

    // ═══════════════════════════════════════════════════
    //  SUB-MANAGERS
    // ═══════════════════════════════════════════════════

    TextureManager* GetTextureManager() { return m_textureManager.get(); }
    BufferManager* GetBufferManager() { return m_bufferManager.get(); }
    SamplerCache* GetSamplerCache() { return m_samplerCache.get(); }
    NativeRTFactory* GetRTFactory() { return m_rtFactory.get(); }  // NEW: Native RT creation

    // ═══════════════════════════════════════════════════
    //  FRAME MANAGEMENT
    // ═══════════════════════════════════════════════════

    void BeginFrame();
    void EndFrame();
    void Update(float deltaTime);

    // ═══════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════

    struct Statistics {
        TextureManager::Statistics textures;
        BufferManager::Statistics buffers;
        NativeRTFactory::Statistics renderTargets;
        u32 samplersCached;

        u64 totalMemoryUsed() const {
            return textures.totalMemoryUsed + buffers.totalMemoryUsed + renderTargets.totalMemoryUsed;
        }
    };

    Statistics GetStatistics() const;
    void PrintStatistics() const;

private:
    xray::render::ng::RenderDevice* m_device;

    xr_unique_ptr<TextureManager> m_textureManager;
    xr_unique_ptr<BufferManager> m_bufferManager;
    xr_unique_ptr<SamplerCache> m_samplerCache;
    xr_unique_ptr<NativeRTFactory> m_rtFactory;
};

} // namespace xray::render::resources
