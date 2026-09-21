#pragma once

#include <nvrhi/nvrhi.h>
#include "Layers/xrRender/FrameGraph/FGTypes.h"

namespace xray::render {
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

inline constexpr u32 kEnvironmentSamplingLonCells = 32;
inline constexpr u32 kEnvironmentSamplingLatCells = 16;
inline constexpr u32 kEnvironmentSamplingCellCount = kEnvironmentSamplingLonCells * kEnvironmentSamplingLatCells;
inline constexpr u32 kEnvironmentSamplingEntryCount = kEnvironmentSamplingCellCount + 1;

class RTEnvironmentSamplingOutput
{
public:
    framegraph::VirtualResourceHandle distribution;
    bool active = false;
};

class EnvironmentSamplingCB
{
public:
    float skyBlend;
    float pad0;
    float pad1;
    float pad2;
};

static_assert(sizeof(EnvironmentSamplingCB) == 16);

class EnvironmentSamplingPassData
{
public:
    fg::RenderDevice* device = nullptr;
    framegraph::VirtualResourceHandle sky0;
    framegraph::VirtualResourceHandle sky1;
    framegraph::VirtualResourceHandle distribution;
    float blend = 0.0f;
};

RTEnvironmentSamplingOutput setupRTEnvironmentSamplingPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    nvrhi::ITexture* sky0,
    nvrhi::ITexture* sky1,
    float blend);

void ShutdownRTEnvironmentSampling();

}
