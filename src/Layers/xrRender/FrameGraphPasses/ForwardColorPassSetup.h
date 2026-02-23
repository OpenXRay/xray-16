// xrRender/FrameGraphPasses/ForwardColorPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/ShaderVariant/VariantPartitionConfig.h"
#include <nvrhi/nvrhi.h>

// Forward declarations
namespace xray::render {
    class GeometryCollector;
    class MaterialCache;
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

// ═══════════════════════════════════════════════════════
//  FORWARD COLOR PASS (Phase 1: Single-RT Forward Rendering)
// ═══════════════════════════════════════════════════════
//
// CHANGES FROM GBUFFER PASS:
// - Single HDR color buffer (RGBA16_FLOAT) instead of 3 RTs
// - 60% bandwidth reduction (128 bits/pixel → 64 bits/pixel)
// - Simpler, cleaner architecture
// - Foundation for Forward+ lighting (Phase 3+)
//
// GPU CULLING INTEGRATION (Phase 3.5):
// - When drawArgsBuffer is valid, uses DrawIndexedIndirect for GPU-driven rendering
// - Draw args buffer is passed through FrameGraph for proper dependency tracking
// - This ensures correct state transition (UAV -> IndirectArgument)
// - Falls back to standard DrawIndexed if drawArgsBuffer is invalid
//
// ═══════════════════════════════════════════════════════
//  BINDLESS FORWARD CONFIG
// ═══════════════════════════════════════════════════════
// Configuration for bindless GPU-driven rendering mode

struct BindlessDrawSet {
    nvrhi::IBuffer* compactDrawArgsBuffer = nullptr;       // Compact draw args buffer (visible batches)
    nvrhi::IBuffer* compactMaterialIDBuffer = nullptr;     // Material IDs parallel to compact draw args
    nvrhi::IBuffer* compactBatchIndicesBuffer = nullptr;   // Draw index → batch index map
    nvrhi::IBuffer* instanceBuffer = nullptr;              // Instance data buffer
    nvrhi::IBuffer* compactCountBuffer = nullptr;          // Visible draw count
    u32 totalObjectCount = 0;                               // Max draw count

    bool IsValid() const {
        return compactDrawArgsBuffer && compactMaterialIDBuffer &&
            compactBatchIndicesBuffer && instanceBuffer && compactCountBuffer &&
            totalObjectCount > 0;
    }
};

struct BindlessForwardConfig {
    BindlessDrawSet staticSet;
    BindlessDrawSet dynamicSet;

    // Enable bindless rendering mode
    bool enabled = false;

    // ═══════════════════════════════════════════════════════
    //  MEGA-BUFFER SYSTEM (GPU-Driven Rendering)
    // ═══════════════════════════════════════════════════════
    // Unified vertex/index buffers containing all level geometry
    // When available, render path uses these instead of per-batch buffers

    nvrhi::IBuffer* megaVertexBuffer = nullptr;   // UnifiedVertex format (48-byte)
    nvrhi::IBuffer* megaIndexBuffer = nullptr;    // 32-bit indices
    bool megaBuffersReady = false;

    // ═══════════════════════════════════════════════════════
    //  TERRAIN RENDERING (4-layer detail blending)
    // ═══════════════════════════════════════════════════════
    // Terrain uses separate TerrainMaterialBuffer (register t9)
    // Rendered after regular geometry with terrain shader

    nvrhi::IBuffer* terrainDrawArgsBuffer = nullptr;     // Terrain indirect draw args
    nvrhi::IBuffer* terrainMaterialIDBuffer = nullptr;   // Terrain material IDs (TerrainMaterialBuffer)
    nvrhi::IBuffer* terrainInstanceBuffer = nullptr;     // Terrain world transforms
    nvrhi::IBuffer* terrainBatchIndicesBuffer = nullptr; // Identity mapping (0,1,2,3...) for direct indexing
    nvrhi::IBuffer* terrainCompactDrawArgsBuffer = nullptr;    // Terrain compact draw args
    nvrhi::IBuffer* terrainCompactBatchIndicesBuffer = nullptr; // Terrain compact batch indices
    nvrhi::IBuffer* terrainCompactMaterialIDBuffer = nullptr;   // Terrain compact material IDs
    nvrhi::IBuffer* terrainCompactCountBuffer = nullptr;        // Terrain compact visible count
    u32 terrainObjectCount = 0;                          // Number of terrain objects

    bool UseGPUCulling() const {
        return enabled && (staticSet.IsValid() || dynamicSet.IsValid());
    }

    bool UseMegaBuffers() const {
        return megaBuffersReady && megaVertexBuffer && megaIndexBuffer;
    }

    bool UseTerrainCompaction() const {
        return terrainCompactDrawArgsBuffer && terrainCompactBatchIndicesBuffer &&
            terrainCompactMaterialIDBuffer && terrainCompactCountBuffer;
    }

    bool HasTerrain() const {
        return terrainObjectCount > 0 && (terrainDrawArgsBuffer || terrainCompactDrawArgsBuffer);
    }

    VariantPartitionConfig variantPartition;
};

struct ForwardColorPassState {
    nvrhi::GraphicsPipelineHandle bindlessPipeline;
    nvrhi::BindingLayoutHandle bindlessLayout;
    nvrhi::InputLayoutHandle bindlessInputLayout;
    nvrhi::SamplerHandle linearSampler;
    nvrhi::ShaderHandle bindlessVS;
    nvrhi::ShaderHandle bindlessPS;
    bool bindlessInitialized = false;
    nvrhi::GraphicsPipelineHandle terrainPipeline;
    nvrhi::BindingLayoutHandle terrainLayout;
    nvrhi::ShaderHandle terrainPS;
    bool terrainInitialized = false;
};

struct ForwardColorPassData {
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle color;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle worldPos;
    framegraph::VirtualResourceHandle drawArgsBuffer;
    ng::RenderDevice* device;
    const GeometryCollector* geometry;
    MaterialCache* materialCache;
    ForwardColorPassState* passState;
    framegraph::DefaultOutputLayout outputs;
    u32 width;
    u32 height;
    BindlessForwardConfig bindlessConfig;
};

void InitializeForwardResources(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer, ForwardColorPassState& state);

// Lambda-based ForwardColorPass setup function (Frostbite pattern)
// Replaces the wasteful 3-RT G-buffer with single color output
// NOTE: Does NOT clear color buffer - sky pass renders background first
framegraph::DefaultOutputLayout setupForwardColorPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    framegraph::VirtualResourceHandle colorInput,
    framegraph::VirtualResourceHandle normalInput,
    framegraph::VirtualResourceHandle baseColorInput,
    framegraph::VirtualResourceHandle worldPosInput,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    framegraph::VirtualResourceHandle drawArgsBuffer = framegraph::VirtualResourceHandle(),
    const BindlessForwardConfig& bindlessConfig = BindlessForwardConfig(),
    ForwardColorPassState* state = nullptr
);

} // namespace xray::render::RENDER_NAMESPACE::passes
