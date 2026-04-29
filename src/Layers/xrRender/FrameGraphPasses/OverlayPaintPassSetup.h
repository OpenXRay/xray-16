#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
namespace fg { class RenderDevice; }
}

namespace xray::render::framegraph { class FrameGraph; }

namespace xray::render::fg {
namespace decals { class OverlayManager; }
}

namespace xray::render::fg::passes {

struct OverlayPaintPassState {
    nvrhi::ComputePipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    bool initialized = false;
    bool enabled = false;
};

void InitializeOverlayPaintResources(fg::RenderDevice* device, OverlayPaintPassState& state);

void setupOverlayPaintPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    decals::OverlayManager* overlayMgr,
    OverlayPaintPassState& state
);

} // namespace xray::render::fg::passes
