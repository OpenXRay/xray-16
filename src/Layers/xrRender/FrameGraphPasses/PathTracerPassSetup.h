#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/LightingMode.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "ClusterLightPassSetup.h"

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg::passes
{
class PathTracerConfig
{
public:
    u32 maxBounces = 8;
    u32 diffuseMode = 0;
};

class PathTracerOutput
{
public:
    framegraph::VirtualResourceHandle composited;
};

class PathTracerCB
{
public:
    Fmatrix invViewProj;
    Fvector4 cameraPos_pad;
    Fvector4 sunDir_intensity;
    Fvector4 sunColor_skyWeight;
    float screenWidth;
    float screenHeight;
    u32 sampleIndex;
    u32 maxBounces;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 transparentBatchCount;
    u32 skinnedBatchStart;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    u32 diffuseMode;
    u32 lightCount;
};

static_assert(sizeof(PathTracerCB) == 160);

class PathTracerHistory
{
public:
    PathTracerCB parameters = {};
    nvrhi::TextureHandle sky0;
    nvrhi::TextureHandle sky1;
    u64 sceneRevision = 0;
    u64 textureRevision = 0;
    u64 lightingSignature = 0;
    Fvector4 foliageSSS = {};
    Fvector4 foliageParams = {};
    Fvector4 foliageParams2 = {};
    u32 samples = 0;
    bool valid = false;
};

class PathTracerPassState
{
public:
    nvrhi::TextureHandle accumulation;
    u32 width = 0;
    u32 height = 0;
    PathTracerHistory history;
    PathTracerHistory pending;
};

class PathTracerData
{
public:
    RenderDevice* device = nullptr;
    LightingFrameState* lighting = nullptr;
    PathTracerPassState* state = nullptr;
    RTFrameResources scene;
    framegraph::VirtualResourceHandle outputTex;
    framegraph::VirtualResourceHandle accumulation;
    framegraph::VirtualResourceHandle lightData;
    PathTracerCB cbData;
    u32 width = 0;
    u32 height = 0;
    nvrhi::TextureHandle sky0;
    nvrhi::TextureHandle sky1;
};

LightingFallback EnsurePathTracerResources(RenderDevice* device, u32 width, u32 height, PathTracerPassState& state);
PathTracerOutput setupPathTracerPass(framegraph::FrameGraph& fg, RenderDevice* device, RTAccelStructManager* accelMgr,
    framegraph::VirtualResourceHandle sceneColorIn, const ClusterLightOutput& clusterLights,
    LightingFrameState& lighting, const PathTracerConfig& config, const Fmatrix& invViewProj,
    const Fvector& cameraPos, u32 width, u32 height, PathTracerPassState& state);

void ShutdownPathTracer();
}
