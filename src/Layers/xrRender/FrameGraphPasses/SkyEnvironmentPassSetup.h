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
    Fvector tint = { 1.0f, 1.0f, 1.0f };
    u32 faceSize = 0;
    float blend = 0.0f;
    float rotation = 0.0f;
    float energy = 1.0f;
    float groundAlbedo = 0.0f;
};

class SkyDownsampleConstants
{
public:
    u32 targetSize = 0;
    u32 pad0 = 0;
    u32 pad1 = 0;
    u32 pad2 = 0;
};

class SkyIrradianceConstants
{
public:
    u32 faceSize = 0;
    float sourceLod = 0.0f;
    u32 pad0 = 0;
    u32 pad1 = 0;
};

class SkySpecularConstants
{
public:
    float roughness = 0.0f;
    u32 faceSize = 0;
    float mirrorLod = 0.0f;
    float sourceTexelSolidAngle = 0.0f;
    u32 sampleCount = 0;
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

class SkyLightingPassData
{
public:
    framegraph::VirtualResourceHandle cube;
    framegraph::VirtualResourceHandle irradiance;
    framegraph::VirtualResourceHandle specular;
    SkyEnvironment* environment = nullptr;
};

class SkyDFGPassData
{
public:
    framegraph::VirtualResourceHandle dfg;
    SkyEnvironment* environment = nullptr;
};

SkyEnvironmentFrame setupSkyEnvironmentPass(framegraph::FrameGraph& fg, SkyEnvironment& environment);
}
