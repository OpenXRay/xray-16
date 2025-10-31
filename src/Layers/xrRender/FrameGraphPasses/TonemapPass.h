// xrRender/FrameGraphPasses/TonemapPass.h
#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "LightingPass.h"

namespace xray::render::passes {

// ══════════════════════════════════════════════════════════
//  TONEMAP PASS CONFIGURATION
// ══════════════════════════════════════════════════════════

struct TonemapPassConfig {
    float exposure = 1.0f;
    float gamma = 2.2f;
};

// ══════════════════════════════════════════════════════════
//  TONEMAP PASS
// ══════════════════════════════════════════════════════════

class TonemapPass {
public:
    TonemapPass(ng::RenderDevice* device, const TonemapPassConfig& config = TonemapPassConfig());
    ~TonemapPass();

    // Setup pass
    void Setup(
        framegraph::FrameGraph& fg,
        framegraph::VirtualResourceHandle hdrInput,
        framegraph::VirtualResourceHandle backbuffer
    );

    // Settings
    void SetExposure(float exposure) { m_config.exposure = exposure; }
    void SetGamma(float gamma) { m_config.gamma = gamma; }

    const TonemapPassConfig& GetConfig() const { return m_config; }

    // Statistics
    struct Stats {
        float cpuTimeMs = 0.0f;
        float gpuTimeMs = 0.0f;
    };

    const Stats& GetStats() const { return m_stats; }

private:
    ng::RenderDevice* m_device;
    TonemapPassConfig m_config;
    Stats m_stats;

    nvrhi::GraphicsPipelineHandle m_pipeline;
    nvrhi::IShader* m_vertexShader = nullptr;
    nvrhi::IShader* m_pixelShader = nullptr;

    // Load shaders
    bool LoadShaders();

    void Execute(
        ng::RenderContext& ctx,
        const framegraph::FrameGraph& fg,
        framegraph::VirtualResourceHandle hdrInput,
        framegraph::VirtualResourceHandle backbuffer
    );
};

} // namespace xray::render::passes
