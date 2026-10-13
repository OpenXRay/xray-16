#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/RayTracing/SkyVisibilityDebug.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg
{
class RenderDevice;
class RTAccelStructManager;
class SkyVisibilityGrid;
class LightingFrameState;
}

namespace xray::render::fg::passes
{
class DeferredLightPassState;

class SkyVisibilityInspectCB
{
public:
    u32 capture[4] = {};
    Fvector4 settings = {};
    Fvector4 lighting = {};
    u32 scene[4] = {};
};
static_assert(sizeof(SkyVisibilityInspectCB) == 64);

class SkyVisibilityInspectPassData
{
public:
    RenderDevice* device = nullptr;
    SkyVisibilityGrid* grid = nullptr;
    const DeferredLightPassState* deferred = nullptr;
    const LightingFrameState* lighting = nullptr;
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle material;
    framegraph::VirtualResourceHandle probes;
    framegraph::VirtualResourceHandle tlas;
    framegraph::VirtualResourceHandle output;
    SkyVisibilityInspectCB constants;
    nvrhi::IBuffer* constantBuffer = nullptr;
    u32 variant = 0;
    mutable bool recorded = false;
};

class SkyVisibilityInspectCopyPassData
{
public:
    SkyVisibilityGrid* grid = nullptr;
    const SkyVisibilityInspectPassData* capture = nullptr;
    framegraph::VirtualResourceHandle source;
    framegraph::VirtualResourceHandle destination;
};

class SkyVisibilityInspection
{
public:
    framegraph::VirtualResourceHandle buffer;
    const SkyVisibilityInspectPassData* capture = nullptr;
};

framegraph::VirtualResourceHandle setupSkyVisibilityBakePass(framegraph::FrameGraph& fg, RenderDevice* device,
    RTAccelStructManager* accelMgr, SkyVisibilityGrid& grid);
SkyVisibilityInspection setupSkyVisibilityInspectPass(framegraph::FrameGraph& fg, RenderDevice* device,
    RTAccelStructManager* accelMgr, SkyVisibilityGrid& grid, framegraph::VirtualResourceHandle probes,
    framegraph::VirtualResourceHandle depth, framegraph::VirtualResourceHandle normal,
    framegraph::VirtualResourceHandle material, framegraph::VirtualResourceHandle lightingResult,
    const DeferredLightPassState* deferred, const LightingFrameState* lighting);
framegraph::VirtualResourceHandle setupSkyVisibilityDebugPass(framegraph::FrameGraph& fg, RenderDevice* device,
    SkyVisibilityGrid& grid, framegraph::VirtualResourceHandle probes, framegraph::VirtualResourceHandle target,
    framegraph::VirtualResourceHandle depth, const SkyVisibilityInspection& inspection, u32 width, u32 height);
void RequestSkyVisibilityRebake();
bool ConsumeSkyVisibilityRebake();
void ShutdownSkyVisibility();
}
