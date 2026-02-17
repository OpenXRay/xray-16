// xrRender/FrameGraphPasses/UIPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "PassVertexFormats.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {
    class FrameGraph;
}

class CGameFont;

namespace xray::render::RENDER_NAMESPACE::passes {

struct UIPassData {
    framegraph::VirtualResourceHandle sceneInput;
    framegraph::VirtualResourceHandle sceneOutput;
    u32 width;
    u32 height;
};

struct UITextPassState {
    nvrhi::BufferHandle vertexBuffer;
    nvrhi::BufferHandle indexBuffer;
    bool initialized = false;
};

struct TextPassData {
    framegraph::VirtualResourceHandle uiTarget;
    u32 width;
    u32 height;
    UITextPassState* passState;
};

struct FontBatch {
    CGameFont* font;
    xr_vector<TextVertex> vertices;
    u32 numStrings;
};

struct CursorPassData {
    framegraph::VirtualResourceHandle uiTarget;
    u32 width;
    u32 height;
};

// Lambda-based UI pass setup
// Renders UI sprites/widgets directly to scene HDR target with alpha blending
framegraph::VirtualResourceHandle setupUIPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle sceneTarget,
    u32 width,
    u32 height
);

framegraph::VirtualResourceHandle setupTextPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle uiTarget,
    u32 width,
    u32 height,
    UITextPassState& state
);

// Cursor pass - renders cursor on top of everything
framegraph::VirtualResourceHandle setupCursorPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle uiTarget,
    u32 width,
    u32 height
);

} // namespace xray::render::RENDER_NAMESPACE::passes