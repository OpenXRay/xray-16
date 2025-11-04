// xrRender/FrameGraphPasses/GBufferPass.h
#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render {
    struct GeometryBatch;  // Forward declaration
    class MaterialCache;   // Forward declaration
}

namespace xray::render::passes {

// ══════════════════════════════════════════════════════════
//  G-BUFFER PASS CONFIGURATION
// ══════════════════════════════════════════════════════════

struct GBufferPassConfig {
    // Output resolution - MUST be set to Device.dwWidth/dwHeight (render target size)
    // DO NOT use screen resolution! Game may render at lower res than display.
    u32 width = 0;   // Default 0 = INVALID, must be explicitly set!
    u32 height = 0;

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
//  G-BUFFER PASS BUILDER (Week 16: Inherits from IPass)
// ══════════════════════════════════════════════════════════

class GBufferPass : public framegraph::IPass {
public:
    GBufferPass(ng::RenderDevice* device, const GBufferPassConfig& config = GBufferPassConfig());
    ~GBufferPass() override;

    // ═══════════════════════════════════════════════════
    //  IPASS INTERFACE (Week 16)
    // ═══════════════════════════════════════════════════

    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;

    framegraph::RenderPhase GetPhase() const override {
        return framegraph::RenderPhase::Geometry;
    }

    // Get configuration
    const GBufferPassConfig& GetConfig() const { return m_config; }

    // Statistics (GBufferPass-specific, different from IPass::PassStats)
    struct Stats {
        u32 numDrawCalls = 0;
        u32 numTriangles = 0;
        u32 numObjects = 0;
        float cpuTimeMs = 0.0f;
        float gpuTimeMs = 0.0f;
    };

    // Non-virtual getter for GBufferPass-specific stats
    const Stats& GetGBufferStats() const { return m_gbufferStats; }

    // Access GBuffer outputs (for RT registry)
    const GBufferOutputs& GetOutputs() const { return m_outputs; }

    // Access material cache (for phase detection)
    MaterialCache* GetMaterialCache() const { return m_materialCache.get(); }

    // Get the native pipeline for geometry submission
    nvrhi::IGraphicsPipeline* GetPipeline() const {
        return m_pipeline ? m_pipeline->GetNativePipeline() : nullptr;
    }

private:
    ng::RenderDevice* m_device;
    GBufferPassConfig m_config;
    Stats m_gbufferStats;  // GBufferPass-specific stats

    // G-Buffer outputs (stored for RT registry access)
    GBufferOutputs m_outputs;

    // Material system
    xr_unique_ptr<MaterialCache> m_materialCache;

    // Shaders (legacy - will be replaced by MaterialCache)
    nvrhi::ShaderHandle m_vertexShaderNative;
    nvrhi::ShaderHandle m_pixelShaderNative;
    xr_unique_ptr<ng::RCShader> m_vertexShader;
    xr_unique_ptr<ng::RCShader> m_pixelShader;
    ng::PipelineState* m_pipeline = nullptr;

    // Shader bytecode (for reflection and analysis)
    ID3DBlob* m_vertexShaderBytecode = nullptr;
    ID3DBlob* m_pixelShaderBytecode = nullptr;

    // Volatile constant buffer pool (dynamically creates VCBs on-demand with deduplication)
    xr_unique_ptr<framegraph::VolatileConstantBufferPool> m_vcbPool;

    // CB layouts discovered from shader reflection
    framegraph::VolatileConstantBufferPool::CBLayout m_dynamicTransformsLayout;  // For trees, static meshes
    framegraph::VolatileConstantBufferPool::CBLayout m_materialLayout;           // For pixel shader materials

    // Legacy: Per-object constant buffer (will be replaced by VCB pool)
    // ng::BufferHandle m_perObjectCB;

    // Load shaders
    bool LoadShaders();

    // Create pipeline state object
    bool CreatePipeline(const GBufferOutputs& outputs, const framegraph::FrameGraph& fg);
};

} // namespace xray::render::passes
