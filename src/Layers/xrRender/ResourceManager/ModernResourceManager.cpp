#include "stdafx.h"
#include "ModernResourceManager.h"

// Unified Modern Resource Manager Implementation
// Week 2 - Day 4: Task 4.4
// Updated: Phase 1 - Native Resource Management

namespace xray::render::resources {

ModernResourceManager::ModernResourceManager(xray::render::ng::RenderDevice* device)
    : m_device(device)
{
    VERIFY(m_device);

    Msg("! [ModernResourceManager] Creating...");

    m_textureManager = xr_make_unique<TextureManager>(device);
    m_bufferManager = xr_make_unique<BufferManager>(device);
    m_samplerCache = xr_make_unique<SamplerCache>(device);
    m_rtFactory = xr_make_unique<NativeRTFactory>(device, m_textureManager.get());

    Msg("! [ModernResourceManager] ✅ Created with Native RT Factory");
}

ModernResourceManager::~ModernResourceManager() {
    Msg("! [ModernResourceManager] Destroying...");
    PrintStatistics();
}

// ═══════════════════════════════════════════════════
//  FRAME MANAGEMENT
// ═══════════════════════════════════════════════════

void ModernResourceManager::BeginFrame() {
    m_bufferManager->BeginFrame();
}

void ModernResourceManager::EndFrame() {
    m_bufferManager->EndFrame();
}

void ModernResourceManager::Update(float deltaTime) {
    m_textureManager->Update(deltaTime);
}

// ═══════════════════════════════════════════════════
//  STATISTICS
// ═══════════════════════════════════════════════════

ModernResourceManager::Statistics ModernResourceManager::GetStatistics() const {
    Statistics stats;
    stats.textures = m_textureManager->GetStatistics();
    stats.buffers = m_bufferManager->GetStatistics();
    stats.renderTargets = m_rtFactory->GetStatistics();
    stats.samplersCached = m_samplerCache->GetCacheSize();
    return stats;
}

void ModernResourceManager::PrintStatistics() const {
    Msg("! [ModernResourceManager] === Statistics ===");
    m_textureManager->PrintStatistics();
    m_bufferManager->PrintStatistics();
    m_rtFactory->PrintStatistics();
    Msg("!   Samplers cached: %u", m_samplerCache->GetCacheSize());
    Msg("! [ModernResourceManager] Total Memory: %llu MB",
        GetStatistics().totalMemoryUsed() / (1024 * 1024));
}

} // namespace xray::render::resources
