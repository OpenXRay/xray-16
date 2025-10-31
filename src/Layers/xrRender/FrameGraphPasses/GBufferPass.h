#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render {
    struct GeometryBatch;
    class MaterialCache;
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
    GBufferPass(ng::RenderDevice* device, const GBufferPassConfig& config = GBufferPassConfig());
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

    nvrhi::IGraphicsPipeline* GetPipeline() const {
        return m_pipeline ? m_pipeline->GetNativePipeline() : nullptr;
    }

private:
    ng::RenderDevice* m_device;
    GBufferPassConfig m_config;
    Stats m_stats;

    xr_unique_ptr<MaterialCache> m_materialCache;

    nvrhi::ShaderHandle m_vertexShaderNative;
    nvrhi::ShaderHandle m_pixelShaderNative;
    xr_unique_ptr<ng::RCShader> m_vertexShader;
    xr_unique_ptr<ng::RCShader> m_pixelShader;
    ng::PipelineState* m_pipeline = nullptr;

    ng::BufferHandle m_perObjectCB;

    bool LoadShaders();

    bool CreatePipeline(const GBufferOutputs& outputs, const framegraph::FrameGraph& fg);

    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg,
                const GBufferOutputs& outputs);

    void UpdatePerObjectConstants(ng::RenderContext& ctx, const GeometryBatch& batch);
};

}
