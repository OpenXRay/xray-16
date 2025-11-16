// xrRender/FrameGraphPasses/ImGuiPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render::ng {
    class ImGuiRendererNVRHI;
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::passes {

// Lambda-based ImGui pass setup
// Renders ImGui interface on top of final output
framegraph::VirtualResourceHandle setupImGuiPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle inputTarget,
    ng::ImGuiRendererNVRHI* imguiRenderer,
    u32 width,
    u32 height
);

} // namespace xray::render::passes