// xrRender/FrameGraphPasses/UIPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::passes {

// Lambda-based UI pass setup
// Renders UI sprites/widgets to separate render target
framegraph::VirtualResourceHandle setupUIPass(
    framegraph::FrameGraph& fg,
    u32 width,
    u32 height
);

// Text pass - renders text on top of UI
framegraph::VirtualResourceHandle setupTextPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle uiTarget,
    u32 width,
    u32 height
);

// Cursor pass - renders cursor on top of everything
framegraph::VirtualResourceHandle setupCursorPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle uiTarget,
    u32 width,
    u32 height
);

// Composite pass - combines scene and UI layers
framegraph::VirtualResourceHandle setupCompositePass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle sceneInput,
    framegraph::VirtualResourceHandle uiInput,
    u32 width,
    u32 height
);

} // namespace xray::render::passes