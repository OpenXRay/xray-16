#pragma once

#include "FrameGraph.h"

namespace xray::render::framegraph {

struct DefaultOutputLayout {
    VirtualResourceHandle albedo;
    VirtualResourceHandle normal;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle depth;
    VirtualResourceHandle distortion;
};

}
