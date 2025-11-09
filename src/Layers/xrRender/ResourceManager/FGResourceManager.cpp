#include "stdafx.h"
#include "FGResourceManager.h"

// FrameGraph Resource Manager Implementation
// Manages all resources for the FrameGraph renderer

namespace xray::render::resources {

FGResourceManager::FGResourceManager(xray::render::ng::RenderDevice* device)
    : m_device(device)
{
    VERIFY(m_device);

    Msg("! [FGResourceManager] Creating...");

    m_textureManager = xr_make_unique<TextureManager>(device);
    m_bufferManager = xr_make_unique<BufferManager>(device);
    m_samplerCache = xr_make_unique<SamplerCache>(device);
    m_rtFactory = xr_make_unique<NativeRTFactory>(device, m_textureManager.get());

    Msg("! [FGResourceManager] ✅ Created with Native RT Factory");
}

FGResourceManager::~FGResourceManager() {
    Msg("! [FGResourceManager] Destroying...");
    PrintStatistics();
}

// ═══════════════════════════════════════════════════
//  FRAME MANAGEMENT
// ═══════════════════════════════════════════════════

void FGResourceManager::BeginFrame() {
    m_bufferManager->BeginFrame();
}

void FGResourceManager::EndFrame() {
    m_bufferManager->EndFrame();
}

void FGResourceManager::Update(float deltaTime) {
    m_textureManager->Update(deltaTime);
}

// ═══════════════════════════════════════════════════
//  STATISTICS
// ═══════════════════════════════════════════════════

FGResourceManager::Statistics FGResourceManager::GetStatistics() const {
    Statistics stats;
    stats.textures = m_textureManager->GetStatistics();
    stats.buffers = m_bufferManager->GetStatistics();
    stats.renderTargets = m_rtFactory->GetStatistics();
    stats.samplersCached = m_samplerCache->GetCacheSize();
    return stats;
}

void FGResourceManager::PrintStatistics() const {
    Msg("! [FGResourceManager] === Statistics ===");
    m_textureManager->PrintStatistics();
    m_bufferManager->PrintStatistics();
    m_rtFactory->PrintStatistics();
    Msg("!   Samplers cached: %u", m_samplerCache->GetCacheSize());
    Msg("! [FGResourceManager] Total Memory: %llu MB",
        GetStatistics().totalMemoryUsed() / (1024 * 1024));
}

} // namespace xray::render::resources
