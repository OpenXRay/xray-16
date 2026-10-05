#pragma once

#include "xrCore/xrCore.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg
{
class RenderDevice;

enum class SkyComputeStage : u8
{
    Source,
    Downsample,
    Irradiance,
    Specular,
    DFG,
    Count
};

class SkyComputePipeline
{
public:
    const char* shader = nullptr;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::ComputePipelineHandle pipeline;
};

class SkyEnvironmentParams
{
public:
    bool Matches(const SkyEnvironmentParams& other) const;

    nvrhi::ITexture* sky0 = nullptr;
    nvrhi::ITexture* sky1 = nullptr;
    Fvector tint = { 1.0f, 1.0f, 1.0f };
    float blend = 0.0f;
    float rotation = 0.0f;
    float energy = 1.0f;
    float groundAlbedo = 0.25f;
};

class SkyEnvironmentFrame
{
public:
    bool Valid() const;

    framegraph::VirtualResourceHandle cube;
    framegraph::VirtualResourceHandle irradiance;
    framegraph::VirtualResourceHandle specular;
    framegraph::VirtualResourceHandle dfg;
    framegraph::VirtualResourceHandle probes;
    nvrhi::ITexture* texture = nullptr;
    u64 revision = 0;
    bool sourceReady = false;
    bool lightingReady = false;
};

class SkyEnvironment
{
public:
    static constexpr u32 kCubeSize = 512;
    static constexpr u32 kSpecularSize = 256;
    static constexpr u32 kSpecularLevels = 7;
    static constexpr u32 kIrradianceCoefficients = 9;
    static constexpr u32 kIrradianceFaceSize = 32;
    static constexpr u32 kDFGSize = 128;

    void Initialize(RenderDevice* device);
    void Shutdown();
    void Invalidate();
    void InvalidatePipelines();
    bool Prepare(SkyEnvironmentParams& params);
    bool NeedsSource(const SkyEnvironmentParams& params) const;
    bool NeedsDFG() const;
    void CommitSource(const SkyEnvironmentParams& params);
    void DiscardSource();
    bool IsSourceValid() const;
    void CompleteLighting(bool ready);
    void CompleteDFG(bool ready);
    SkyEnvironmentFrame Use(framegraph::FrameGraph& graph, bool sourceReady) const;
    RenderDevice* GetDevice() const;
    const SkyComputePipeline& GetPipeline(SkyComputeStage stage) const;

private:
    bool EnsureResources();
    bool EnsurePipelines();

    RenderDevice* m_device = nullptr;
    nvrhi::TextureHandle m_cube;
    nvrhi::TextureHandle m_specular;
    nvrhi::TextureHandle m_dfg;
    nvrhi::BufferHandle m_irradiance;
    SkyComputePipeline m_pipelines[u32(SkyComputeStage::Count)];
    SkyEnvironmentParams m_source;
    u64 m_revision = 0;
    bool m_sourceValid = false;
    bool m_lightingReady = false;
    bool m_dfgReady = false;
    bool m_pipelineFailed = false;
};
}
