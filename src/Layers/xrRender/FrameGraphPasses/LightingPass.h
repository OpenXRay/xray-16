#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "GBufferPass.h"

namespace xray::render::passes {

struct LightingPassConfig {
    u32 width = 1920;
    u32 height = 1080;

    nvrhi::Format hdrFormat = nvrhi::Format::RGBA16_FLOAT;

    bool enableAmbient = true;
    bool enableDirectional = true;
    float ambientIntensity = 0.1f;

    bool visualizeLighting = false;
};

struct LightingPassOutput {
    framegraph::VirtualResourceHandle hdrColor;
};

class LightingPass {
public:
    LightingPass(ng::RenderDevice* device, const LightingPassConfig& config = LightingPassConfig());
    ~LightingPass();

    LightingPassOutput Setup(
        framegraph::FrameGraph& fg,
        const GBufferOutputs& gbuffer
    );

    const LightingPassConfig& GetConfig() const { return m_config; }

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

    nvrhi::IShader* m_vertexShaderNative = nullptr;
    nvrhi::IShader* m_pixelShaderNative = nullptr;
    xr_unique_ptr<ng::RCShader> m_vertexShader;
    xr_unique_ptr<ng::RCShader> m_pixelShader;
    ng::PipelineState* m_pipeline = nullptr;

    bool LoadShaders();

    bool CreatePipeline(nvrhi::ITexture* hdrTexture);

    void Execute(
        ng::RenderContext& ctx,
        const framegraph::FrameGraph& fg,
        const GBufferOutputs& gbuffer,
        const LightingPassOutput& output
    );
};

}
