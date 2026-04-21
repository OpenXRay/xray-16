#pragma once

#include "ResourceHandle.h"
#include <nvrhi/nvrhi.h>

// Native Render Target Factory
// Creates render targets as native NVRHI resources from the start
// No wrapping of legacy DX11 resources!

namespace xray::render::fg {
    class RenderDevice;  // Forward declaration
}

namespace xray::render::resources {

class TextureManager;  // Forward declaration

// ═══════════════════════════════════════════════════
//  NATIVE RENDER TARGET FACTORY
// ═══════════════════════════════════════════════════
// Creates render targets as native NVRHI textures
// Part of Phase 1: Resource Management Migration

class NativeRTFactory {
public:
    explicit NativeRTFactory(xray::render::fg::RenderDevice* device, TextureManager* textureManager);
    ~NativeRTFactory();

    // ═══════════════════════════════════════════════════
    //  G-BUFFER RENDER TARGETS
    // ═══════════════════════════════════════════════════

    // Position buffer (world space) - RGBA16F
    TextureHandle CreatePositionBuffer(u32 width, u32 height, const char* name = "rt_Position");

    // Normal buffer (view space) - RGBA16F or RGB10A2
    TextureHandle CreateNormalBuffer(u32 width, u32 height, const char* name = "rt_Normal");

    // Albedo/Color buffer - RGBA8_UNORM or RGBA8_SRGB
    TextureHandle CreateAlbedoBuffer(u32 width, u32 height, bool sRGB = false, const char* name = "rt_Albedo");

    // Depth/Stencil buffer - D24S8 or D32F
    TextureHandle CreateDepthStencil(u32 width, u32 height, bool use32bit = false, const char* name = "rt_Depth");

    // Generic G-Buffer target with custom format
    TextureHandle CreateGBufferRT(u32 width, u32 height, nvrhi::Format format, const char* name);

    // ═══════════════════════════════════════════════════
    //  POST-PROCESS RENDER TARGETS
    // ═══════════════════════════════════════════════════

    // HDR accumulation buffer - RGBA16F or RGB11B10F
    TextureHandle CreateHDRTarget(u32 width, u32 height, const char* name = "rt_HDR");

    // Bloom chain (multiple mips) - RGBA16F
    TextureHandle CreateBloomTarget(u32 width, u32 height, u32 mipLevels, const char* name = "rt_Bloom");

    // LDR output - RGBA8_UNORM
    TextureHandle CreateLDRTarget(u32 width, u32 height, const char* name = "rt_LDR");

    // ═══════════════════════════════════════════════════
    //  TEMPORAL RENDER TARGETS
    // ═══════════════════════════════════════════════════

    // History buffer for TAA - Same format as main RT
    TextureHandle CreateHistoryBuffer(u32 width, u32 height, nvrhi::Format format, const char* name = "rt_History");

    // Velocity/Motion vectors - RG16F or RG16_SNORM
    TextureHandle CreateVelocityBuffer(u32 width, u32 height, const char* name = "rt_Velocity");

    // ═══════════════════════════════════════════════════
    //  SHADOW MAP TARGETS
    // ═══════════════════════════════════════════════════

    // Single shadow map - D16 or D32F
    TextureHandle CreateShadowMap(u32 size, bool use32bit = false, const char* name = "rt_Shadow");

    // Cascaded shadow map array
    TextureHandle CreateCascadedShadowMap(u32 size, u32 cascadeCount, bool use32bit = false, const char* name = "rt_CascadedShadow");

    // ═══════════════════════════════════════════════════
    //  COMPUTE TARGETS
    // ═══════════════════════════════════════════════════

    // UAV-enabled texture for compute shaders
    TextureHandle CreateComputeTarget(u32 width, u32 height, nvrhi::Format format, const char* name = "rt_Compute");

    // 3D texture for volumetrics
    TextureHandle CreateVolumeTarget(u32 width, u32 height, u32 depth, nvrhi::Format format, const char* name = "rt_Volume");

    // ═══════════════════════════════════════════════════
    //  BATCH CREATION
    // ═══════════════════════════════════════════════════

    struct RTSetDesc {
        u32 width;
        u32 height;
        bool includePosition;
        bool includeNormal;
        bool includeAlbedo;
        bool includeDepth;
        bool includeVelocity;
        bool sRGB;  // For albedo
    };

    struct RTSet {
        TextureHandle position;
        TextureHandle normal;
        TextureHandle albedo;
        TextureHandle depth;
        TextureHandle velocity;
    };

    // Create complete G-Buffer set
    RTSet CreateGBufferSet(const RTSetDesc& desc);

    // ═══════════════════════════════════════════════════
    //  RESOURCE MANAGEMENT
    // ═══════════════════════════════════════════════════

    // Resize all managed RTs (on window resize)
    void ResizeRenderTargets(u32 newWidth, u32 newHeight);

    // Release a specific RT
    void ReleaseRenderTarget(TextureHandle& handle);

    // Get statistics
    struct Statistics {
        u32 numRenderTargets;
        u64 totalMemoryUsed;
        u32 numResizes;
    };

    Statistics GetStatistics() const;
    void PrintStatistics() const;

private:
    // Internal helper to create RT with specific parameters
    TextureHandle CreateRenderTargetInternal(
        u32 width,
        u32 height,
        u32 depth,
        u32 arraySize,
        u32 mipLevels,
        nvrhi::Format format,
        bool isDepthStencil,
        bool isUAV,
        const char* debugName
    );

    // Track all created RTs for resize operations
    struct ManagedRT {
        TextureHandle handle;
        u32 width;
        u32 height;
        nvrhi::Format format;
        bool isDepthStencil;
        bool isUAV;
        shared_str name;
    };

    xray::render::fg::RenderDevice* m_device;
    TextureManager* m_textureManager;
    xr_vector<ManagedRT> m_managedRTs;

    // Statistics
    mutable Statistics m_stats;
};

} // namespace xray::render::resources