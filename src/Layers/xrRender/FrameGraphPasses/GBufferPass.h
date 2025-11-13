#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render {
    struct GeometryBatch;
    class MaterialCache;
}

namespace xray::render::passes {

struct GBufferPassConfig {
    u32 width = 0;
    u32 height = 0;

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

class GBufferPass : public framegraph::IPass {
public:
    GBufferPass(ng::RenderDevice* device, const GBufferPassConfig& config = GBufferPassConfig());
    ~GBufferPass() override;

    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;

    framegraph::RenderPhase GetPhase() const override {
        return framegraph::RenderPhase::Geometry;
    }

    const GBufferPassConfig& GetConfig() const { return m_config; }

    struct Stats {
        u32 numDrawCalls = 0;
        u32 numTriangles = 0;
        u32 numObjects = 0;
        float cpuTimeMs = 0.0f;
        float gpuTimeMs = 0.0f;
    };

    const Stats& GetGBufferStats() const { return m_gbufferStats; }

    const framegraph::DefaultOutputLayout& GetOutputs() const { return m_outputs; }

    MaterialCache* GetMaterialCache() const { return m_materialCache.get(); }

    nvrhi::IGraphicsPipeline* GetPipeline() const {
        return m_pipeline ? m_pipeline->GetNativePipeline() : nullptr;
    }

private:
    ng::RenderDevice* m_device;
    GBufferPassConfig m_config;
    Stats m_gbufferStats;

    framegraph::DefaultOutputLayout m_outputs;

    xr_unique_ptr<MaterialCache> m_materialCache;

    nvrhi::ShaderHandle m_vertexShaderNative;
    nvrhi::ShaderHandle m_pixelShaderNative;
    xr_unique_ptr<ng::RCShader> m_vertexShader;
    xr_unique_ptr<ng::RCShader> m_pixelShader;
    ng::PipelineState* m_pipeline = nullptr;

    xr_vector<u8> m_vertexShaderBytecode;
    xr_vector<u8> m_pixelShaderBytecode;

    xr_unique_ptr<framegraph::VolatileConstantBufferPool> m_vcbPool;

    framegraph::VolatileConstantBufferPool::CBLayout m_dynamicTransformsLayout;
    framegraph::VolatileConstantBufferPool::CBLayout m_materialLayout;

    bool LoadShaders();

    bool CreatePipeline(const framegraph::DefaultOutputLayout& outputs, const framegraph::FrameGraph& fg);
};

}
