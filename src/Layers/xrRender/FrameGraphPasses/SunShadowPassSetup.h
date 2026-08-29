#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    class MaterialCache;
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

struct SunShadowState {
    static constexpr u32 kReadbackSlots = 6;

    nvrhi::ComputePipelineHandle cullPipeline;
    nvrhi::BindingLayoutHandle cullLayout;
    nvrhi::ComputePipelineHandle argsPipeline;
    nvrhi::BindingLayoutHandle argsLayout;
    bool pipelinesFailed = false;

    nvrhi::BufferHandle countBuffer;
    nvrhi::BufferHandle opaqueStream;
    nvrhi::BufferHandle terrainStream;
    nvrhi::BufferHandle atStream;
    nvrhi::BufferHandle opaqueArgs;
    nvrhi::BufferHandle terrainArgs;
    nvrhi::BufferHandle atArgs;
    u32 streamCapacity = 0;

    nvrhi::BufferHandle readback[kReadbackSlots];
    u32 readbackWrite = 0;
    u32 readbackScheduled = 0;

    u32 candidates = 0;
    u32 castersOpaque = 0;
    u32 castersTerrain = 0;
    u32 castersAT = 0;

    Fmatrix farVP;
    float farTexel = 0.0f;
    bool farValid = false;

    nvrhi::TextureHandle farMap;
    u32 farMapSize = 0;
    nvrhi::GraphicsPipelineHandle depthOpaquePipeline;
    nvrhi::GraphicsPipelineHandle depthATPipeline;
    nvrhi::BindingLayoutHandle depthOpaqueLayout;
    nvrhi::BindingLayoutHandle depthATLayout;
    nvrhi::ShaderHandle clusterVS;
    nvrhi::ShaderHandle depthOpaquePS;
    nvrhi::ShaderHandle depthATPS;
    bool depthPipelinesFailed = false;
};

struct SunShadowCullOutput {
    framegraph::VirtualResourceHandle opaqueArgs;
    framegraph::VirtualResourceHandle terrainArgs;
    framegraph::VirtualResourceHandle atArgs;
    bool active = false;
};

struct SunShadowDrawConfig {
    nvrhi::IBuffer* entryBuffer = nullptr;
    nvrhi::IBuffer* staticInstanceBuffer = nullptr;
    nvrhi::IBuffer* terrainInstanceBuffer = nullptr;
    nvrhi::IBuffer* megaVertexBuffer = nullptr;
    nvrhi::IBuffer* megaIndexBuffer = nullptr;
    MaterialCache* materialCache = nullptr;
};

void ComputeSunFarVP(Fmatrix& outVP, float& outTexel, const Fvector& sunDir, float boxSize, u32 mapSize);

SunShadowCullOutput setupSunShadowCullPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle orderAfter,
    nvrhi::IBuffer* entryBuffer,
    u32 entryCount,
    SunShadowState* state);

framegraph::VirtualResourceHandle setupSunShadowFarPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const SunShadowCullOutput& cull,
    const SunShadowDrawConfig& config,
    SunShadowState* state);

} // namespace xray::render::fg::passes
