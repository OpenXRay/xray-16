// xrRender/FrameGraphPasses/ImGuiPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render::fg {
    class ImGuiRendererNVRHI;
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

struct ImGuiPassData {
    framegraph::VirtualResourceHandle input;
    framegraph::VirtualResourceHandle output;
    fg::ImGuiRendererNVRHI* renderer;
    u32 width;
    u32 height;
};

// Lambda-based ImGui pass setup
// Renders ImGui interface on top of final output
framegraph::VirtualResourceHandle setupImGuiPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle inputTarget,
    fg::ImGuiRendererNVRHI* imguiRenderer,
    u32 width,
    u32 height
);

} // namespace xray::render::RENDER_NAMESPACE::passes