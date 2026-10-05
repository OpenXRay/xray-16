#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
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
}

namespace xray::render::fg::passes
{
framegraph::VirtualResourceHandle setupSkyVisibilityBakePass(framegraph::FrameGraph& fg, RenderDevice* device,
    RTAccelStructManager* accelMgr, SkyVisibilityGrid& grid);
void RequestSkyVisibilityRebake();
bool ConsumeSkyVisibilityRebake();
void ShutdownSkyVisibility();
}
