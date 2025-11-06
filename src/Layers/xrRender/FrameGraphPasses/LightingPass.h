// xrRender/FrameGraphPasses/LightingPass.h
#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "GBufferPass.h"

namespace xray::render::passes {

// ══════════════════════════════════════════════════════════
//  LIGHTING PASS CONFIGURATION
// ══════════════════════════════════════════════════════════

struct LightingPassConfig {
    // Output resolution
    u32 width = 1920;
    u32 height = 1080;

    // HDR format
    nvrhi::Format hdrFormat = nvrhi::Format::RGBA16_FLOAT;

    // Lighting options
    bool enableAmbient = true;
    bool enableDirectional = true;
    float ambientIntensity = 0.1f;

    // Debug
    bool visualizeLighting = false;
};

// ══════════════════════════════════════════════════════════
//  LIGHTING PASS OUTPUT
// ══════════════════════════════════════════════════════════

struct LightingPassOutput {
    framegraph::VirtualResourceHandle hdrColor;  // Lit scene
};

// ══════════════════════════════════════════════════════════
//  LIGHTING PASS
// ══════════════════════════════════════════════════════════

class LightingPass {
public:
    LightingPass(ng::RenderDevice* device, const LightingPassConfig& config = LightingPassConfig());
    ~LightingPass();

    // Setup pass in FrameGraph
    LightingPassOutput Setup(
        framegraph::FrameGraph& fg,
        const framegraph::DefaultOutputLayout& gbuffer
    );

    // Get configuration
    const LightingPassConfig& GetConfig() const { return m_config; }

    // Statistics
    struct Stats {
        float cpuTimeMs = 0.0f;
        float gpuTimeMs = 0.0f;
        u32 numLights = 0;
    };

    const Stats& GetStats() const { return m_stats; }

private:
    ng::RenderDevice* m_device;
    LightingPassConfig m_config;
    Stats m_stats;

    // Shaders
    nvrhi::ShaderHandle m_vertexShaderNative;
    nvrhi::ShaderHandle m_pixelShaderNative;
    xr_unique_ptr<ng::RCShader> m_vertexShader;
    xr_unique_ptr<ng::RCShader> m_pixelShader;
    ng::PipelineState* m_pipeline = nullptr;

    // Load shaders
    bool LoadShaders();

    // Create pipeline state object
    bool CreatePipeline(nvrhi::ITexture* hdrTexture);

    // Execution
    void Execute(
        ng::RenderContext& ctx,
        const framegraph::FrameGraph& fg,
        const framegraph::DefaultOutputLayout& gbuffer,
        const LightingPassOutput& output
    );
};

} // namespace xray::render::passes
