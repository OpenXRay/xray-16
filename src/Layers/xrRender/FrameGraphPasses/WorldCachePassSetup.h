#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/LightingMode.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/RayTracing/WorldRadianceCache.h"
#include "ReSTIRGIPassSetup.h"

namespace xray::render::framegraph
{
class FrameGraph;
class BindingSetBuilder;
struct ExtractedReflection;
}

namespace xray::render::fg::passes
{
class WorldCachePassInputs
{
public:
    RTAccelStructManager* accelMgr = nullptr;
    WorldRadianceCache* cache = nullptr;
    LightingFrameState* lighting = nullptr;
    framegraph::VirtualResourceHandle lightData;
    framegraph::VirtualResourceHandle clusterGrid;
    framegraph::VirtualResourceHandle lightIndexList;
    framegraph::VirtualResourceHandle environmentDistribution;
    framegraph::VirtualResourceHandle sky;
    RTGIRawCB sceneConstants = {};
    Fvector cameraPos = {};
    u32 frame = 0;
};

class WorldCachePassOutput
{
public:
    WorldRadianceCacheCB constants = {};
    nvrhi::IBuffer* constantBuffer = nullptr;
    bool scheduled = false;
    LightingFallback fallback = LightingFallback::None;
};

class WorldCacheDecayPassData
{
public:
    RenderDevice* device = nullptr;
    WorldRadianceCache* cache = nullptr;
    LightingFrameState* lighting = nullptr;
    WorldRadianceCacheResources resources;
    WorldRadianceCacheCB constants = {};
    nvrhi::IBuffer* constantBuffer = nullptr;
    u32 capacity = 0;
};

class WorldCacheSelectPassData
{
public:
    RenderDevice* device = nullptr;
    WorldRadianceCache* cache = nullptr;
    LightingFrameState* lighting = nullptr;
    WorldRadianceCacheResources resources;
    WorldRadianceCacheCB constants = {};
    nvrhi::IBuffer* constantBuffer = nullptr;
    u32 capacity = 0;
};

class WorldCacheUpdatePassData
{
public:
    RenderDevice* device = nullptr;
    WorldRadianceCache* cache = nullptr;
    LightingFrameState* lighting = nullptr;
    RTFrameResources scene;
    WorldRadianceCacheResources resources;
    framegraph::VirtualResourceHandle lightData;
    framegraph::VirtualResourceHandle clusterGrid;
    framegraph::VirtualResourceHandle lightIndexList;
    framegraph::VirtualResourceHandle environmentDistribution;
    framegraph::VirtualResourceHandle sky;
    RTGIRawCB sceneConstants = {};
    WorldRadianceCacheCB constants = {};
    nvrhi::IBuffer* constantBuffer = nullptr;
    nvrhi::IBuffer* sceneConstantBuffer = nullptr;
};

WorldRadianceCacheConfig BuildWorldCacheConfig();
WorldCachePassOutput setupWorldCachePass(framegraph::FrameGraph& fg, RenderDevice* device,
    const WorldCachePassInputs& inputs);
void BindWorldCacheResources(framegraph::BindingSetBuilder& bsb, const framegraph::ExtractedReflection& reflection,
    const WorldRadianceCacheBuffers& buffers, nvrhi::IBuffer* constants);
void ShutdownWorldCache();
}
