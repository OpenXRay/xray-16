#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    class MaterialCache;
    class GeometryCollector;
    struct GeometryBatch;
    namespace fg {
        class RenderDevice;
        class GPUCullingManager;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
    class RenderPassBuilder;
    class BindingSetBuilder;
}

namespace xray::profiler {
    class GPUProfiler;
}

namespace xray::render::fg::passes {

constexpr u32 kSunTargetFar = 0;
constexpr u32 kSunTargetCasc0 = 1;
constexpr u32 kSunTargetCasc1 = 2;
constexpr u32 kSunTargetCount = 3;
constexpr u32 kSunMapSlots = kSunTargetCount + 1;

struct SunShadowTarget {
    static constexpr u32 kReadbackSlots = 6;

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

    u32 castersOpaque = 0;
    u32 castersTerrain = 0;
    u32 castersAT = 0;

    nvrhi::TextureHandle map;
    u32 mapSize = 0;
    Fmatrix vp;
    float texel = 0.0f;
    bool valid = false;
    bool redraw = false;
};

struct SunShadowState {
    bool receiverActive = false;

    nvrhi::ComputePipelineHandle cullPipeline;
    nvrhi::BindingLayoutHandle cullLayout;
    nvrhi::ComputePipelineHandle argsPipeline;
    nvrhi::BindingLayoutHandle argsLayout;
    bool pipelinesFailed = false;

    nvrhi::GraphicsPipelineHandle depthOpaquePipeline;
    nvrhi::GraphicsPipelineHandle depthATPipeline;
    nvrhi::BindingLayoutHandle depthOpaqueLayout;
    nvrhi::BindingLayoutHandle depthATLayout;
    nvrhi::ShaderHandle clusterVS;
    nvrhi::ShaderHandle depthOpaquePS;
    nvrhi::ShaderHandle depthATPS;
    nvrhi::GraphicsPipelineHandle depthDynamicPipeline;
    nvrhi::BindingLayoutHandle depthDynamicLayout;
    nvrhi::InputLayoutHandle depthDynamicInputLayout;
    nvrhi::ShaderHandle forwardVS;
    nvrhi::ShaderHandle depthDynamicPS;
    bool depthPipelinesFailed = false;

    nvrhi::GraphicsPipelineHandle skinnedMDIPipeline;
    nvrhi::BindingLayoutHandle skinnedMDILayout;
    nvrhi::ShaderHandle skinnedDepthMDIPS;
    bool skinnedPipelinesReady = false;
    bool skinnedPipelinesFailed = false;
    int rasterBias = -1;
    float rasterSlope = -1.0f;

    SunShadowTarget targets[kSunTargetCount];
    u32 candidates = 0;

    Fvector farCamPos;
    Fvector farSunDir;
    Fvector cascSunDir;
    bool cascSunInit = false;
    u32 farEntryCount = 0;
    float farBox = 0.0f;
    float farLod = 0.0f;
    int farAT = 0;
    u32 farRedraws = 0;
};

struct SunShadowCullOutput {
    struct Target {
        framegraph::VirtualResourceHandle opaqueArgs;
        framegraph::VirtualResourceHandle terrainArgs;
        framegraph::VirtualResourceHandle atArgs;
    };
    Target targets[kSunTargetCount];
    bool active = false;
};

struct SunShadowDrawConfig {
    nvrhi::IBuffer* entryBuffer = nullptr;
    nvrhi::IBuffer* staticInstanceBuffer = nullptr;
    nvrhi::IBuffer* terrainInstanceBuffer = nullptr;
    nvrhi::IBuffer* megaVertexBuffer = nullptr;
    nvrhi::IBuffer* megaIndexBuffer = nullptr;
    MaterialCache* materialCache = nullptr;
    nvrhi::IBuffer* dynamicCompactDrawArgs = nullptr;
    nvrhi::IBuffer* dynamicCompactMaterialIDs = nullptr;
    nvrhi::IBuffer* dynamicCompactBatchIndices = nullptr;
    nvrhi::IBuffer* dynamicCompactCount = nullptr;
    nvrhi::IBuffer* dynamicInstanceBuffer = nullptr;
    nvrhi::IBuffer* dynamicFadeBuffer = nullptr;
    u32 dynamicObjectCount = 0;
    framegraph::VirtualResourceHandle dynamicArgs;
    const GeometryCollector* geometry = nullptr;
    GPUCullingManager* gpuCulling = nullptr;
    framegraph::VirtualResourceHandle skinnedArgs;
};

struct SunShadowMaps {
    framegraph::VirtualResourceHandle maps[kSunTargetCount];
    framegraph::VirtualResourceHandle mask;
};

void ReadSunShadowMaps(framegraph::RenderPassBuilder& builder, const SunShadowMaps& in, SunShadowMaps& out);
void ResolveSunShadowMaps(const framegraph::FrameGraph& fg, const SunShadowMaps& maps, nvrhi::IDevice* device, nvrhi::ITexture** out);
void BindSunShadowMaps(framegraph::BindingSetBuilder& bsb, nvrhi::ITexture* const* maps);

void InvalidateSunShadowCache(SunShadowState& state);

void ComputeSunFarVP(Fmatrix& outVP, float& outTexel, const Fvector& sunDir, float boxSize, u32 mapSize);
void ComputeSunCascadeVP(Fmatrix& outVP, float& outTexel, const Fvector& sunDir, float boxSize, u32 mapSize);

SunShadowCullOutput setupSunShadowCullPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle orderAfter,
    nvrhi::IBuffer* entryBuffer,
    u32 entryCapacity,
    u32 entryCount,
    SunShadowState* state,
    xray::profiler::GPUProfiler* gpuProfiler = nullptr);

SunShadowMaps setupSunShadowMapPasses(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const SunShadowCullOutput& cull,
    const SunShadowDrawConfig& config,
    SunShadowState* state,
    xray::profiler::GPUProfiler* gpuProfiler = nullptr);

} // namespace xray::render::fg::passes
