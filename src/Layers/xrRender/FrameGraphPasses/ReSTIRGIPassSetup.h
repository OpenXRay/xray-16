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
    nvrhi::ComputePipelineHandle initialPipeline;
    nvrhi::BindingLayoutHandle initialLayout;
    nvrhi::ComputePipelineHandle temporalPipeline;
    nvrhi::BindingLayoutHandle temporalLayout;
    nvrhi::ComputePipelineHandle compositePipeline;
    nvrhi::BindingLayoutHandle compositeLayout;
    nvrhi::IBuffer* cb = nullptr;
    nvrhi::SamplerHandle sampler;
    nvrhi::TextureHandle reservoirA[2];
    nvrhi::TextureHandle reservoirB[2];
    nvrhi::TextureHandle directLighting;
    nvrhi::TextureHandle indirectLighting;
    u32 currTemporalIdx = 0;
    u32 texWidth = 0;
    u32 texHeight = 0;
    LightingFallback readiness = LightingFallback::ResourcesUnavailable;
    LightingFallback temporalReadiness = LightingFallback::ResourcesUnavailable;
    bool initialized = false;
    bool historyValid = false;
    bool initialRecorded = false;
};

class ReSTIRGIOutput
{
public:
    framegraph::VirtualResourceHandle sceneColor;
};

class ReSTIRGICB
{
public:
    Fmatrix invViewProj;
    Fmatrix prevViewProj;
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
    u32 pad;
};

static_assert(sizeof(ReSTIRGICB) == 224);

class TemporalCB
{
public:
    Fmatrix invViewProj;
    Fmatrix prevInvViewProj;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    u32 frameIndex;
    u32 diffuseMode;
    u32 pad[2];
};

static_assert(sizeof(TemporalCB) == 176);

class CompositeCB
{
public:
    Fmatrix invViewProj;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float giIntensity;
    u32 diffuseMode;
};

static_assert(sizeof(CompositeCB) == 96);

class InitialPassData
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
    framegraph::VirtualResourceHandle lightData;
    framegraph::VirtualResourceHandle directLighting;
    framegraph::VirtualResourceHandle indirectLighting;
    framegraph::VirtualResourceHandle reservoirA;
    framegraph::VirtualResourceHandle reservoirB;
    ReSTIRGICB cbData;
    u32 width = 0;
    u32 height = 0;
    nvrhi::TextureHandle sky0;
    nvrhi::TextureHandle sky1;
};

class TemporalPassData
{
public:
    RenderDevice* device = nullptr;
    ReSTIRGIPassState* state = nullptr;
    LightingFrameState* lighting = nullptr;
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle prevNormals;
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle material;
    framegraph::VirtualResourceHandle prevDepth;
    framegraph::VirtualResourceHandle motionVectors;
    framegraph::VirtualResourceHandle reservoirA;
    framegraph::VirtualResourceHandle reservoirB;
    framegraph::VirtualResourceHandle previousA;
    framegraph::VirtualResourceHandle previousB;
    TemporalCB cbData;
    u32 width = 0;
    u32 height = 0;
};

class CompositePassData
{
public:
    RenderDevice* device = nullptr;
    ReSTIRGIPassState* state = nullptr;
    LightingFrameState* lighting = nullptr;
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle material;
    framegraph::VirtualResourceHandle sceneColor;
    framegraph::VirtualResourceHandle directLighting;
    framegraph::VirtualResourceHandle indirectLighting;
    framegraph::VirtualResourceHandle reservoirA;
    framegraph::VirtualResourceHandle reservoirB;
    CompositeCB cbData;
    u32 width = 0;
    u32 height = 0;
    u32 reservoirIdx = 0;
};

LightingFallback EnsureReSTIRGIResources(RenderDevice* device, ReSTIRGIPassState& state, u32 width, u32 height, bool reuseReservoirs);
ReSTIRGIOutput setupReSTIRGIPass(framegraph::FrameGraph& fg, RenderDevice* device, RTAccelStructManager* accelMgr, const framegraph::DefaultOutputLayout& inputs,
    const ClusterLightOutput& clusterLights, framegraph::VirtualResourceHandle prevNormals,
    framegraph::VirtualResourceHandle prevDepth, framegraph::VirtualResourceHandle motionVectors, const Fmatrix& invViewProj, const Fmatrix& prevViewProj,
    const Fvector& cameraPos, float giIntensity, u32 width, u32 height, ReSTIRGIPassState& state, bool hasPrevFrameData, LightingFrameState& lighting);

void ShutdownReSTIRGI(ReSTIRGIPassState& state);
}
