// DetailPassSetup.h - Framegraph pass for detail objects (grass, etc.)
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include <nvrhi/nvrhi.h>

struct Fmatrix;

namespace xray::render::fg {
    class FGDetailManager;
}

namespace xray::render {
    namespace fg {
        class RenderDevice;
        class RenderContext;
    }
}

namespace xray::profiler {
    class GPUProfiler;
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

struct DetailPassState {
    bool detailDataUploaded = false;
    float lastBladeWidth = 0.0f;
};

struct DetailPassData {
    framegraph::VirtualResourceHandle inputColor;
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle outputColor;
    framegraph::VirtualResourceHandle outputNormal;
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle worldPos;
    framegraph::VirtualResourceHandle shadowMap;
    nvrhi::ITexture* shadowMapArray = nullptr;
    nvrhi::ITexture* shadowCascades[3] = {};
    nvrhi::ITexture* localShadowAtlas = nullptr;
    nvrhi::ITexture* localShadowESM = nullptr;
    nvrhi::ITexture* contactDepth = nullptr;
    nvrhi::ITexture* contactHistory = nullptr;
    fg::RenderDevice* device;
    fg::FGDetailManager* detailManager;
    framegraph::DefaultOutputLayout outputs;
    u32 width;
    u32 height;
    xray::profiler::GPUProfiler* gpuProfiler;
};

framegraph::DefaultOutputLayout setupDetailPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    fg::FGDetailManager* detailManager,
    const framegraph::DefaultOutputLayout& forwardInputs,
    u32 width,
    u32 height,
    xray::profiler::GPUProfiler* gpuProfiler = nullptr,
    nvrhi::ITexture* shadowMapArray = nullptr,
    framegraph::VirtualResourceHandle shadowMapHandle = {},
    nvrhi::ITexture* contactDepth = nullptr,
    nvrhi::ITexture* contactHistory = nullptr,
    framegraph::VirtualResourceHandle perlinReady = {},
    nvrhi::ITexture* const* shadowCascades = nullptr,
    nvrhi::ITexture* localShadowAtlas = nullptr,
    nvrhi::ITexture* localShadowESM = nullptr
);

} // namespace xray::render::fg::passes
