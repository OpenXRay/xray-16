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

class EnvironmentSamplingPassData
{
public:
    fg::RenderDevice* device = nullptr;
    framegraph::VirtualResourceHandle sky;
    framegraph::VirtualResourceHandle distribution;
};

RTEnvironmentSamplingOutput setupRTEnvironmentSamplingPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sky);

void ShutdownRTEnvironmentSampling();

}
