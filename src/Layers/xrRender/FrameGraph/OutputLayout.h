#pragma once

#include "FrameGraph.h"

namespace xray::render::framegraph {

struct DefaultOutputLayout {
    VirtualResourceHandle albedo;
    VirtualResourceHandle normal;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle material;
    VirtualResourceHandle depth;
    VirtualResourceHandle distortion;
};

}
