#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/LightingMode.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/FrameGraph/OutputLayout.h"
#include "ClusterLightPassSetup.h"

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg::passes
{
class ReSTIRGIPassState
{
public:
    nvrhi::ComputePipelineHandle tracePipeline;
    nvrhi::BindingLayoutHandle traceLayout;
    nvrhi::ComputePipelineHandle compositePipeline;
    nvrhi::BindingLayoutHandle compositeLayout;
    nvrhi::IBuffer* cb = nullptr;
    nvrhi::TextureHandle rawDiffuse;
    nvrhi::TextureHandle rawSpecular;
    nvrhi::TextureHandle emission;
    nvrhi::TextureHandle normalRoughness;
    nvrhi::TextureHandle albedoMetallic;
    nvrhi::TextureHandle pathData;
    nvrhi::TextureHandle surfaceData;
    nvrhi::TextureHandle motion;
    u32 texWidth = 0;
    u32 texHeight = 0;
    LightingFallback readiness = LightingFallback::ResourcesUnavailable;
    bool initialized = false;
    bool historyValid = false;
    bool initialRecorded = false;
};

class ReSTIRGIOutput
{
public:
    framegraph::VirtualResourceHandle sceneColor;
    framegraph::VirtualResourceHandle rawDiffuse;
    framegraph::VirtualResourceHandle rawSpecular;
    framegraph::VirtualResourceHandle emission;
    framegraph::VirtualResourceHandle normalRoughness;
    framegraph::VirtualResourceHandle albedoMetallic;
    framegraph::VirtualResourceHandle pathData;
    framegraph::VirtualResourceHandle surfaceData;
    framegraph::VirtualResourceHandle motionVectors;
};

class RTGIRawCB
{
public:
    Fmatrix invViewProj;
    Fvector4 cameraPos;
    Fvector4 sunDir_intensity;
    Fvector4 sunColor_skyWeight;
    float screenWidth;
    float screenHeight;
    float giIntensity;
    u32 frameIndex;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 skinnedBatchStart;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    u32 diffuseMode;
    u32 lightCount;
    u32 emissiveCount;
    u32 maxNullEvents;
    u32 maxBounces;
    u32 samplesPerPixel;
    float rayDistance;
    float environmentRotation;
    float sunAngularRadius;
    float cameraConeSpread;
    u32 pad;
};

static_assert(sizeof(RTGIRawCB) == 192);

class RTGICompositeParams
{
public:
    u32 width;
    u32 height;
    u32 pad0;
    u32 pad1;
};

static_assert(sizeof(RTGICompositeParams) == 16);

class RTGITracePassData
{
public:
    RenderDevice* device = nullptr;
    RTFrameResources scene;
    ReSTIRGIPassState* state = nullptr;
    LightingFrameState* lighting = nullptr;
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle material;
    framegraph::VirtualResourceHandle sourceColor;
    framegraph::VirtualResourceHandle motionVectors;
    framegraph::VirtualResourceHandle lightData;
    framegraph::VirtualResourceHandle environmentDistribution;
    framegraph::VirtualResourceHandle rawDiffuse;
    framegraph::VirtualResourceHandle rawSpecular;
    framegraph::VirtualResourceHandle emission;
    framegraph::VirtualResourceHandle normalRoughness;
    framegraph::VirtualResourceHandle albedoMetallic;
    framegraph::VirtualResourceHandle pathData;
    framegraph::VirtualResourceHandle surfaceData;
    framegraph::VirtualResourceHandle outMotion;
    RTGIRawCB cbData;
    u32 width = 0;
    u32 height = 0;
    nvrhi::TextureHandle sky0;
    nvrhi::TextureHandle sky1;
};

class RTGICompositePassData
{
public:
    RenderDevice* device = nullptr;
    ReSTIRGIPassState* state = nullptr;
    LightingFrameState* lighting = nullptr;
    framegraph::VirtualResourceHandle rawDiffuse;
    framegraph::VirtualResourceHandle rawSpecular;
    framegraph::VirtualResourceHandle emission;
    framegraph::VirtualResourceHandle sceneColor;
    RTGICompositeParams cbData;
    u32 bounces = 0;
    u32 samples = 0;
    float rayDistance = 0.0f;
    u32 width = 0;
    u32 height = 0;
};

LightingFallback EnsureReSTIRGIResources(RenderDevice* device, ReSTIRGIPassState& state, u32 width, u32 height, bool reuseRequested);
ReSTIRGIOutput setupReSTIRGIPass(framegraph::FrameGraph& fg, RenderDevice* device, RTAccelStructManager* accelMgr, const framegraph::DefaultOutputLayout& inputs,
    const ClusterLightOutput& clusterLights, framegraph::VirtualResourceHandle prevNormals,
    framegraph::VirtualResourceHandle prevDepth, framegraph::VirtualResourceHandle motionVectors, const Fmatrix& invViewProj, const Fmatrix& prevViewProj,
    const Fvector& cameraPos, float giIntensity, u32 width, u32 height, ReSTIRGIPassState& state, bool hasPrevFrameData, LightingFrameState& lighting);

void ShutdownReSTIRGI(ReSTIRGIPassState& state);
}
