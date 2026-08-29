#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
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
};

struct SunShadowCullOutput {
    framegraph::VirtualResourceHandle opaqueArgs;
    framegraph::VirtualResourceHandle terrainArgs;
    framegraph::VirtualResourceHandle atArgs;
    bool active = false;
};

void ComputeSunFarVP(Fmatrix& outVP, float& outTexel, const Fvector& sunDir, float boxSize, u32 mapSize);

SunShadowCullOutput setupSunShadowCullPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle orderAfter,
    nvrhi::IBuffer* entryBuffer,
    u32 entryCount,
    SunShadowState* state);

} // namespace xray::render::fg::passes
