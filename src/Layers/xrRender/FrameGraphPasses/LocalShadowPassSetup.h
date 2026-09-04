#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    class MaterialCache;
    namespace fg {
        class RenderDevice;
        class GPUCullingManager;
        class light;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::profiler {
    class GPUProfiler;
}

namespace xray::render::fg::passes {

constexpr u32 kLocalShadowAtlas = 4096;
constexpr u32 kLocalSpotTile = 1024;
constexpr u32 kLocalSpotSlots = 8;
constexpr u32 kLocalPointFace = 512;
constexpr u32 kLocalPointLights = 4;
constexpr u32 kLocalPointSlots = kLocalPointLights * 6;
constexpr u32 kLocalTileCount = kLocalSpotSlots + kLocalPointSlots;
constexpr u32 kLocalPullVertices = 384;
constexpr u32 kLocalStatWords = 32;
constexpr u32 kLocalStreamCount = 6;
constexpr u32 kLocalPairCapOpaque = 1u << 18;
constexpr u32 kLocalPairCapTerrain = 1u << 17;
constexpr u32 kLocalPairCapAT = 1u << 17;
constexpr u32 kLocalPairCapDynOpaque = 1u << 16;
constexpr u32 kLocalPairCapDynAT = 1u << 15;
constexpr u32 kLocalPairCapSkinned = 1u << 16;

struct LocalShadowTileGPU {
    Fmatrix viewProj;
    Fvector4 rect;
    Fvector4 zparams;
    Fvector4 lightPos;
    Fvector4 planes[6];
};
static_assert(sizeof(LocalShadowTileGPU) == 208, "LocalShadowTileGPU is shader-visible");

struct LocalTile {
    const light* owner = nullptr;
    Fvector pos = {};
    Fvector dir = {};
    float range = 0.0f;
    float cone = 0.0f;
    u32 lastSeen = 0;
    bool staticValid = false;
    bool inView = false;
};

struct LocalShadowState {
    LocalTile spots[kLocalSpotSlots];
    LocalTile points[kLocalPointLights];
    LocalShadowTileGPU records[kLocalTileCount] = {};
    u32 refreshStatic[kLocalTileCount] = {};
    u32 refreshDyn[kLocalTileCount] = {};
    u32 refreshStaticCount = 0;
    u32 refreshDynCount = 0;
    xr_vector<u32> slotOfLight;
    u32 pooledSpots = 0;
    u32 pooledPoints = 0;
    u32 frame = 0;

    nvrhi::BufferHandle tiles;
    nvrhi::BufferHandle refreshStaticBuffer;
    nvrhi::BufferHandle refreshDynBuffer;
    nvrhi::BufferHandle stats;
    nvrhi::BufferHandle pairs[kLocalStreamCount];
    nvrhi::BufferHandle args;
    nvrhi::BufferHandle clearArgs;
    nvrhi::TextureHandle staticAtlas;
    nvrhi::TextureHandle dynAtlas;
    bool staticAtlasFirst = true;
    bool dynAtlasFirst = true;

    nvrhi::ComputePipelineHandle binPipeline;
    nvrhi::BindingLayoutHandle binLayout;
    nvrhi::ComputePipelineHandle argsPipeline;
    nvrhi::BindingLayoutHandle argsLayout;
    nvrhi::GraphicsPipelineHandle clearPipeline;
    nvrhi::BindingLayoutHandle clearLayout;
    nvrhi::GraphicsPipelineHandle pagePipeline;
    nvrhi::BindingLayoutHandle pageLayout;
    nvrhi::GraphicsPipelineHandle pageATPipeline;
    nvrhi::BindingLayoutHandle pageATLayout;
    nvrhi::GraphicsPipelineHandle skinPagePipeline;
    nvrhi::BindingLayoutHandle skinPageLayout;
    nvrhi::ShaderHandle pageVS;
    nvrhi::ShaderHandle skinPageVS;
    bool pipelinesFailed = false;
    bool resourcesFailed = false;
};

struct LocalShadowConfig {
    GPUCullingManager* gpuCulling = nullptr;
    nvrhi::IBuffer* entryBuffer = nullptr;
    nvrhi::IBuffer* staticInstanceBuffer = nullptr;
    nvrhi::IBuffer* terrainInstanceBuffer = nullptr;
    nvrhi::IBuffer* dynamicInstanceBuffer = nullptr;
    nvrhi::IBuffer* megaVertexBuffer = nullptr;
    nvrhi::IBuffer* megaIndexBuffer = nullptr;
    MaterialCache* materialCache = nullptr;
};

struct LocalShadowOutput {
    framegraph::VirtualResourceHandle tiles;
    framegraph::VirtualResourceHandle staticAtlas;
    framegraph::VirtualResourceHandle dynAtlas;
    LocalShadowState* state = nullptr;
    bool active = false;
};

void ResetLocalShadowPool(LocalShadowState& state);

void SelectLocalShadowLights(
    LocalShadowState& state,
    const xr_vector<const light*>& lights,
    const Fvector& camPos,
    const Fmatrix& camViewProj);

LocalShadowOutput setupLocalShadowPasses(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle orderAfter,
    const LocalShadowConfig& config,
    LocalShadowState* state,
    xray::profiler::GPUProfiler* gpuProfiler);

void ResolveLocalShadowBindings(
    const framegraph::FrameGraph& fg,
    const LocalShadowOutput& out,
    nvrhi::IDevice* device,
    nvrhi::IBuffer*& tiles,
    nvrhi::ITexture*& staticAtlas,
    nvrhi::ITexture*& dynAtlas);

}
