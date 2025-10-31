#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render {
    struct GeometryBatch;
}

namespace xray::render::passes {

struct GBufferPassConfig {
    u32 width = 1920;
    u32 height = 1080;

    nvrhi::Format albedoFormat = nvrhi::Format::RGBA8_UNORM;
    nvrhi::Format normalFormat = nvrhi::Format::RGBA16_FLOAT;
    nvrhi::Format materialFormat = nvrhi::Format::R32_FLOAT;
    nvrhi::Format depthFormat = nvrhi::Format::D32;

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float clearDepth = 1.0f;
    u8 clearStencil = 0;

    bool enableMSAA = false;
    u32 msaaSampleCount = 1;

    bool visualizeNormals = false;
    bool visualizeDepth = false;
};

struct GBufferOutputs {
    framegraph::VirtualResourceHandle albedo;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle material;
    framegraph::VirtualResourceHandle depth;
};

class GBufferPass {
public:
    GBufferPass(const GBufferPassConfig& config = GBufferPassConfig());
    ~GBufferPass();

    GBufferOutputs Setup(framegraph::FrameGraph& fg);

    const GBufferPassConfig& GetConfig() const { return m_config; }

    struct Stats {
        u32 numDrawCalls = 0;
        u32 numTriangles = 0;
        u32 numObjects = 0;
        float cpuTimeMs = 0.0f;
        float gpuTimeMs = 0.0f;
    };

    const Stats& GetStats() const { return m_stats; }

private:
    GBufferPassConfig m_config;
    Stats m_stats;

    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg,
                const GBufferOutputs& outputs);

    void UpdatePerObjectConstants(ng::RenderContext& ctx, const GeometryBatch& batch);
};

}
