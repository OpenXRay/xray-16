#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/SkyEnvironment.h"

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg::passes
{
class SkySourceConstants
{
public:
    float blend = 0.0f;
    float rotation = 0.0f;
    float energy = 1.0f;
    float groundAlbedo = 0.0f;
    u32 faceSize = 0;
    u32 pad0 = 0;
    u32 pad1 = 0;
    u32 pad2 = 0;
};

class SkySourcePassData
{
public:
    framegraph::VirtualResourceHandle cube;
    SkyEnvironment* environment = nullptr;
    SkyEnvironmentParams params;
};

SkyEnvironmentFrame setupSkyEnvironmentPass(framegraph::FrameGraph& fg, SkyEnvironment& environment);
}
