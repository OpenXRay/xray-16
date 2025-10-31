// xrRender/FrameGraphPasses/GBufferPass.h
#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render {
    struct GeometryBatch;  // Forward declaration
}

namespace xray::render::passes {

// ══════════════════════════════════════════════════════════
//  G-BUFFER PASS CONFIGURATION
// ══════════════════════════════════════════════════════════

struct GBufferPassConfig {
    // Output resolution
    u32 width = 1920;
    u32 height = 1080;

    // G-Buffer formats
    nvrhi::Format albedoFormat = nvrhi::Format::RGBA8_UNORM;      // Albedo + metallic
    nvrhi::Format normalFormat = nvrhi::Format::RGBA16_FLOAT;     // Normal + roughness
    nvrhi::Format materialFormat = nvrhi::Format::R32_FLOAT;      // Material ID
    nvrhi::Format depthFormat = nvrhi::Format::D32;               // Depth (NVRHI handles typeless with proper views)

    // Clear values
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float clearDepth = 1.0f;
    u8 clearStencil = 0;

    // Rendering options
    bool enableMSAA = false;
    u32 msaaSampleCount = 1;

    // Debug
    bool visualizeNormals = false;
    bool visualizeDepth = false;
};

// ══════════════════════════════════════════════════════════
//  G-BUFFER PASS OUTPUTS
// ══════════════════════════════════════════════════════════

struct GBufferOutputs {
    framegraph::VirtualResourceHandle albedo;    // RT0: Albedo.rgb + Metallic.a
    framegraph::VirtualResourceHandle normal;    // RT1: Normal.xyz + Roughness.a
    framegraph::VirtualResourceHandle material;  // RT2: Material ID
    framegraph::VirtualResourceHandle depth;     // Depth/Stencil
};

// ══════════════════════════════════════════════════════════
//  G-BUFFER PASS BUILDER
// ══════════════════════════════════════════════════════════

class GBufferPass {
public:
    GBufferPass(ng::RenderDevice* device, const GBufferPassConfig& config = GBufferPassConfig());
    ~GBufferPass();

    // Setup pass in FrameGraph
    GBufferOutputs Setup(framegraph::FrameGraph& fg);

    // Get configuration
    const GBufferPassConfig& GetConfig() const { return m_config; }

    // Statistics
    struct Stats {
        u32 numDrawCalls = 0;
        u32 numTriangles = 0;
        u32 numObjects = 0;
        float cpuTimeMs = 0.0f;
        float gpuTimeMs = 0.0f;
    };

    const Stats& GetStats() const { return m_stats; }

    // Get the native pipeline for geometry submission
    nvrhi::IGraphicsPipeline* GetPipeline() const {
        return m_pipeline ? m_pipeline->GetNativePipeline() : nullptr;
    }

private:
    ng::RenderDevice* m_device;
    GBufferPassConfig m_config;
    Stats m_stats;

    // Shaders
    nvrhi::ShaderHandle m_vertexShaderNative;
    nvrhi::ShaderHandle m_pixelShaderNative;
    xr_unique_ptr<ng::RCShader> m_vertexShader;
    xr_unique_ptr<ng::RCShader> m_pixelShader;
    ng::PipelineState* m_pipeline = nullptr;

    // Per-object constant buffer (using our abstraction layer)
    ng::BufferHandle m_perObjectCB;

    // Load shaders
    bool LoadShaders();

    // Create pipeline state object
    bool CreatePipeline(const GBufferOutputs& outputs, const framegraph::FrameGraph& fg);

    // Execution callback
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg,
                const GBufferOutputs& outputs);

    // Update per-object constant buffer
    void UpdatePerObjectConstants(ng::RenderContext& ctx, const GeometryBatch& batch);
};

} // namespace xray::render::passes
