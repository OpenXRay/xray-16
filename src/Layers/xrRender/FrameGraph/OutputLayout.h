#pragma once

#include "FrameGraph.h"

namespace xray::render::framegraph {

struct DefaultOutputLayout {
    VirtualResourceHandle albedo;      // RT0: Lit HDR color (RGBA16_FLOAT)
    VirtualResourceHandle normal;      // RT1: World normal.xyz + Roughness.a (RGBA16_FLOAT)
    VirtualResourceHandle baseColor;   // RT2: Unlit diffuse albedo.rgb + Metallic.a (RGBA8_UNORM)
    VirtualResourceHandle depth;       // Depth/Stencil (D32)
    VirtualResourceHandle distortion;  // Distortion buffer (RG = UV offset, A = intensity)
};

}
