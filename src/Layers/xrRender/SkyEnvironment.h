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

class SkyEnvironmentParams
{
public:
    bool Matches(const SkyEnvironmentParams& other) const;

    nvrhi::ITexture* sky0 = nullptr;
    nvrhi::ITexture* sky1 = nullptr;
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
    nvrhi::ITexture* texture = nullptr;
    u64 revision = 0;
};

class SkyEnvironment
{
public:
    static constexpr u32 kCubeSize = 512;

    void Initialize(RenderDevice* device);
    void Shutdown();
    void Invalidate();
    void InvalidatePipelines();
    bool Prepare(SkyEnvironmentParams& params);
    bool NeedsSource(const SkyEnvironmentParams& params) const;
    void CommitSource(const SkyEnvironmentParams& params);
    void DiscardSource();
    SkyEnvironmentFrame Use(framegraph::FrameGraph& graph) const;
    RenderDevice* GetDevice() const;
    nvrhi::IBindingLayout* GetSourceLayout() const;
    nvrhi::IComputePipeline* GetSourcePipeline() const;

private:
    bool EnsureCube();
    bool EnsureSourcePipeline();

    RenderDevice* m_device = nullptr;
    nvrhi::TextureHandle m_cube;
    nvrhi::BindingLayoutHandle m_sourceLayout;
    nvrhi::ComputePipelineHandle m_sourcePipeline;
    SkyEnvironmentParams m_source;
    u64 m_revision = 0;
    bool m_sourceValid = false;
    bool m_pipelineFailed = false;
};
}
