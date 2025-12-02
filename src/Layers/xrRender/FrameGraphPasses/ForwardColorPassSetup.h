#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    class GeometryCollector;
    class MaterialCache;
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
    struct DefaultOutputLayout;
}

namespace xray::render::RENDER_NAMESPACE::passes {

struct BindlessForwardConfig {
    nvrhi::IBuffer* compactDrawArgsBuffer = nullptr;

    nvrhi::IBuffer* compactMaterialIDBuffer = nullptr;

    nvrhi::IBuffer* compactBatchIndicesBuffer = nullptr;

    nvrhi::IBuffer* instanceBuffer = nullptr;

    u32 totalObjectCount = 0;

    bool enabled = false;

    nvrhi::IBuffer* megaVertexBuffer = nullptr;
    nvrhi::IBuffer* megaIndexBuffer = nullptr;
    bool megaBuffersReady = false;

    bool IsValid() const {
        return enabled && compactDrawArgsBuffer && compactMaterialIDBuffer && totalObjectCount > 0;
    }

    bool UseGPUCulling() const {
        return IsValid() && compactBatchIndicesBuffer && instanceBuffer;
    }

    bool UseMegaBuffers() const {
        return megaBuffersReady && megaVertexBuffer && megaIndexBuffer;
    }
};

framegraph::DefaultOutputLayout setupForwardColorPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    framegraph::VirtualResourceHandle colorInput,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    framegraph::VirtualResourceHandle drawArgsBuffer = framegraph::VirtualResourceHandle(),
    const BindlessForwardConfig& bindlessConfig = BindlessForwardConfig()
);

}
