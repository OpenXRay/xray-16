#include "stdafx.h"
#include "NativeRTFactory.h"
#include "TextureManager.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

// Native Render Target Factory Implementation
// Creates render targets as native NVRHI resources from the start
// Part of Phase 1: Resource Management Migration

namespace xray::render::resources {

NativeRTFactory::NativeRTFactory(xray::render::fg::RenderDevice* device, TextureManager* textureManager)
    : m_device(device)
    , m_textureManager(textureManager)
    , m_stats{}
{
    VERIFY(m_device);
    VERIFY(m_textureManager);

    Msg("* [NativeRTFactory] Created - Native NVRHI render target creation enabled");
}

NativeRTFactory::~NativeRTFactory() {
    PrintStatistics();

    // Release all managed RTs
    for (auto& rt : m_managedRTs) {
        if (rt.handle.IsValid()) {
            m_textureManager->Release(rt.handle);
        }
    }

    Msg("* [NativeRTFactory] Destroyed");
}

// ═══════════════════════════════════════════════════
//  G-BUFFER RENDER TARGETS
// ═══════════════════════════════════════════════════

TextureHandle NativeRTFactory::CreatePositionBuffer(u32 width, u32 height, const char* name) {
    Msg("  [NativeRTFactory] Creating Position Buffer: %s (%ux%u)", name, width, height);

    // Use RGBA16F for world space position (high precision needed)
    return CreateGBufferRT(width, height, nvrhi::Format::RGBA16_FLOAT, name);
}

TextureHandle NativeRTFactory::CreateNormalBuffer(u32 width, u32 height, const char* name) {
    Msg("  [NativeRTFactory] Creating Normal Buffer: %s (%ux%u)", name, width, height);

    // Use RGBA16F for view space normals (matches legacy format)
    // Could optimize to RGB10A2 later
    return CreateGBufferRT(width, height, nvrhi::Format::RGBA16_FLOAT, name);
}

TextureHandle NativeRTFactory::CreateAlbedoBuffer(u32 width, u32 height, bool sRGB, const char* name) {
    Msg("  [NativeRTFactory] Creating Albedo Buffer: %s (%ux%u, sRGB=%s)",
        name, width, height, sRGB ? "true" : "false");

    // Use RGBA8 for albedo/diffuse color
    nvrhi::Format format = sRGB ? nvrhi::Format::SRGBA8_UNORM : nvrhi::Format::RGBA8_UNORM;
    return CreateGBufferRT(width, height, format, name);
}

TextureHandle NativeRTFactory::CreateDepthStencil(u32 width, u32 height, bool use32bit, const char* name) {
    Msg("  [NativeRTFactory] Creating Depth/Stencil: %s (%ux%u, 32bit=%s)",
        name, width, height, use32bit ? "true" : "false");

    nvrhi::Format format = use32bit ? nvrhi::Format::D32 : nvrhi::Format::D24S8;

    return CreateRenderTargetInternal(
        width, height,
        1,      // depth
        1,      // arraySize
        1,      // mipLevels
        format,
        true,   // isDepthStencil
        false,  // isUAV
        name
    );
}

TextureHandle NativeRTFactory::CreateGBufferRT(u32 width, u32 height, nvrhi::Format format, const char* name) {
    return CreateRenderTargetInternal(
        width, height,
        1,      // depth
        1,      // arraySize
        1,      // mipLevels
        format,
        false,  // isDepthStencil
        false,  // isUAV
        name
    );
}

// ═══════════════════════════════════════════════════
//  POST-PROCESS RENDER TARGETS
// ═══════════════════════════════════════════════════

TextureHandle NativeRTFactory::CreateHDRTarget(u32 width, u32 height, const char* name) {
    Msg("  [NativeRTFactory] Creating HDR Target: %s (%ux%u)", name, width, height);

    // Use RGBA16F for HDR accumulation
    return CreateGBufferRT(width, height, nvrhi::Format::RGBA16_FLOAT, name);
}

TextureHandle NativeRTFactory::CreateBloomTarget(u32 width, u32 height, u32 mipLevels, const char* name) {
    Msg("  [NativeRTFactory] Creating Bloom Target: %s (%ux%u, %u mips)", name, width, height, mipLevels);

    return CreateRenderTargetInternal(
        width, height,
        1,          // depth
        1,          // arraySize
        mipLevels,  // multiple mips for bloom chain
        nvrhi::Format::RGBA16_FLOAT,
        false,      // isDepthStencil
        false,      // isUAV
        name
    );
}

TextureHandle NativeRTFactory::CreateLDRTarget(u32 width, u32 height, const char* name) {
    Msg("  [NativeRTFactory] Creating LDR Target: %s (%ux%u)", name, width, height);

    // Use RGBA8 for final LDR output
    return CreateGBufferRT(width, height, nvrhi::Format::RGBA8_UNORM, name);
}

// ═══════════════════════════════════════════════════
//  TEMPORAL RENDER TARGETS
// ═══════════════════════════════════════════════════

TextureHandle NativeRTFactory::CreateHistoryBuffer(u32 width, u32 height, nvrhi::Format format, const char* name) {
    Msg("  [NativeRTFactory] Creating History Buffer: %s (%ux%u)", name, width, height);

    return CreateGBufferRT(width, height, format, name);
}

TextureHandle NativeRTFactory::CreateVelocityBuffer(u32 width, u32 height, const char* name) {
    Msg("  [NativeRTFactory] Creating Velocity Buffer: %s (%ux%u)", name, width, height);

    // Use RG16F for motion vectors (2 components)
    return CreateGBufferRT(width, height, nvrhi::Format::RG16_FLOAT, name);
}

// ═══════════════════════════════════════════════════
//  SHADOW MAP TARGETS
// ═══════════════════════════════════════════════════

TextureHandle NativeRTFactory::CreateShadowMap(u32 size, bool use32bit, const char* name) {
    Msg("  [NativeRTFactory] Creating Shadow Map: %s (%ux%u, 32bit=%s)",
        name, size, size, use32bit ? "true" : "false");

    nvrhi::Format format = use32bit ? nvrhi::Format::D32 : nvrhi::Format::D16;

    return CreateRenderTargetInternal(
        size, size,
        1,      // depth
        1,      // arraySize
        1,      // mipLevels
        format,
        true,   // isDepthStencil
        false,  // isUAV
        name
    );
}

TextureHandle NativeRTFactory::CreateCascadedShadowMap(u32 size, u32 cascadeCount, bool use32bit, const char* name) {
    Msg("  [NativeRTFactory] Creating Cascaded Shadow Map: %s (%ux%u, %u cascades, 32bit=%s)",
        name, size, size, cascadeCount, use32bit ? "true" : "false");

    nvrhi::Format format = use32bit ? nvrhi::Format::D32 : nvrhi::Format::D16;

    return CreateRenderTargetInternal(
        size, size,
        1,              // depth
        cascadeCount,   // arraySize for cascades
        1,              // mipLevels
        format,
        true,           // isDepthStencil
        false,          // isUAV
        name
    );
}

// ═══════════════════════════════════════════════════
//  COMPUTE TARGETS
// ═══════════════════════════════════════════════════

TextureHandle NativeRTFactory::CreateComputeTarget(u32 width, u32 height, nvrhi::Format format, const char* name) {
    Msg("  [NativeRTFactory] Creating Compute Target: %s (%ux%u)", name, width, height);

    return CreateRenderTargetInternal(
        width, height,
        1,      // depth
        1,      // arraySize
        1,      // mipLevels
        format,
        false,  // isDepthStencil
        true,   // isUAV - enable compute write
        name
    );
}

TextureHandle NativeRTFactory::CreateVolumeTarget(u32 width, u32 height, u32 depth, nvrhi::Format format, const char* name) {
    Msg("  [NativeRTFactory] Creating Volume Target: %s (%ux%ux%u)", name, width, height, depth);

    return CreateRenderTargetInternal(
        width, height,
        depth,  // 3D texture
        1,      // arraySize
        1,      // mipLevels
        format,
        false,  // isDepthStencil
        true,   // isUAV - enable compute write
        name
    );
}

// ═══════════════════════════════════════════════════
//  BATCH CREATION
// ═══════════════════════════════════════════════════

NativeRTFactory::RTSet NativeRTFactory::CreateGBufferSet(const RTSetDesc& desc) {
    Msg("* [NativeRTFactory] Creating G-Buffer set (%ux%u)", desc.width, desc.height);

    RTSet set;

    if (desc.includePosition) {
        set.position = CreatePositionBuffer(desc.width, desc.height);
    }

    if (desc.includeNormal) {
        set.normal = CreateNormalBuffer(desc.width, desc.height);
    }

    if (desc.includeAlbedo) {
        set.albedo = CreateAlbedoBuffer(desc.width, desc.height, desc.sRGB);
    }

    if (desc.includeDepth) {
        set.depth = CreateDepthStencil(desc.width, desc.height);
    }

    if (desc.includeVelocity) {
        set.velocity = CreateVelocityBuffer(desc.width, desc.height);
    }

    Msg("  ✓ G-Buffer set created");
    return set;
}

// ═══════════════════════════════════════════════════
//  RESOURCE MANAGEMENT
// ═══════════════════════════════════════════════════

void NativeRTFactory::ResizeRenderTargets(u32 newWidth, u32 newHeight) {
    Msg("* [NativeRTFactory] Resizing all render targets to %ux%u", newWidth, newHeight);

    for (auto& rt : m_managedRTs) {
        if (rt.handle.IsValid()) {
            // Release old RT
            m_textureManager->Release(rt.handle);

            // Recreate with new size
            rt.handle = CreateRenderTargetInternal(
                newWidth, newHeight,
                1,  // depth (TODO: store original depth)
                1,  // arraySize (TODO: store original)
                1,  // mipLevels (TODO: store original)
                rt.format,
                rt.isDepthStencil,
                rt.isUAV,
                rt.name.c_str()
            );

            rt.width = newWidth;
            rt.height = newHeight;
        }
    }

    m_stats.numResizes++;
    Msg("  ✓ Resized %u render targets", m_managedRTs.size());
}

void NativeRTFactory::ReleaseRenderTarget(TextureHandle& handle) {
    if (!handle.IsValid()) return;

    // Remove from managed list
    auto it = std::find_if(m_managedRTs.begin(), m_managedRTs.end(),
        [&handle](const ManagedRT& rt) { return rt.handle == handle; });

    if (it != m_managedRTs.end()) {
        m_managedRTs.erase(it);
    }

    // Release through texture manager
    m_textureManager->Release(handle);
    handle = TextureHandle();
}

// ═══════════════════════════════════════════════════
//  INTERNAL IMPLEMENTATION
// ═══════════════════════════════════════════════════

TextureHandle NativeRTFactory::CreateRenderTargetInternal(
    u32 width, u32 height, u32 depth, u32 arraySize, u32 mipLevels,
    nvrhi::Format format, bool isDepthStencil, bool isUAV, const char* debugName)
{
    // Build TextureDesc for TextureManager
    TextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.depth = depth;
    desc.arraySize = arraySize;
    desc.mipLevels = mipLevels;
    desc.format = format;
    desc.isRenderTarget = !isDepthStencil;
    desc.isDepthStencil = isDepthStencil;
    desc.isUAV = isUAV;
    desc.allowStreaming = false;  // Never stream render targets
    desc.debugName = debugName;

    // Determine texture type
    if (depth > 1) {
        desc.type = TextureDesc::Texture3D;
    } else if (arraySize == 6) {
        desc.type = TextureDesc::TextureCube;
    } else if (arraySize > 1) {
        desc.type = TextureDesc::Texture2DArray;
    } else {
        desc.type = TextureDesc::Texture2D;
    }

    // Create through TextureManager (it handles NVRHI texture creation)
    TextureHandle handle = m_textureManager->CreateTexture(desc);

    if (handle.IsValid()) {
        // Track for management
        ManagedRT managedRT;
        managedRT.handle = handle;
        managedRT.width = width;
        managedRT.height = height;
        managedRT.format = format;
        managedRT.isDepthStencil = isDepthStencil;
        managedRT.isUAV = isUAV;
        managedRT.name = debugName;
        m_managedRTs.push_back(managedRT);

        // Update stats
        m_stats.numRenderTargets++;
        m_stats.totalMemoryUsed += desc.CalculateMemorySize();

        Msg("    ✓ Created native NVRHI RT: %s (format=%d, size=%ux%u)",
            debugName, (int)format, width, height);
    } else {
        Msg("!   Failed to create RT: %s", debugName);
    }

    return handle;
}

// ═══════════════════════════════════════════════════
//  STATISTICS
// ═══════════════════════════════════════════════════

NativeRTFactory::Statistics NativeRTFactory::GetStatistics() const {
    return m_stats;
}

void NativeRTFactory::PrintStatistics() const {
    Msg("* [NativeRTFactory] Statistics:");
    Msg("  - Render Targets: %u", m_stats.numRenderTargets);
    Msg("  - Memory Used: %.2f MB", m_stats.totalMemoryUsed / (1024.0f * 1024.0f));
    Msg("  - Resizes: %u", m_stats.numResizes);
}

} // namespace xray::render::resources
