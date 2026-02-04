#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"

namespace xray::render {
    struct GeometryBatch;
    class MaterialCache;
    class GeometryCollector;
    class dxRender_Visual;
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

void InitializeSkinningPipelines(ng::RenderDevice* device);

void ShutdownSkinningPipelines();

using SkinnedStatsCallback = void(*)(u32 rendered, u32 culled, void* userData);

using VisibilityByVisualCallback = u32(*)(const dxRender_Visual* visual, void* userData);

struct SkinnedVisibilityData {
    bool enabled = false;

    VisibilityByVisualCallback visibilityByVisualCallback = nullptr;
    void* visibilityByVisualUserData = nullptr;

    SkinnedStatsCallback statsCallback = nullptr;
    void* statsUserData = nullptr;
};

framegraph::DefaultOutputLayout setupSkinningPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const GeometryCollector* geometry,
    const xr_vector<GeometryBatch>* hudBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    const SkinnedVisibilityData& visibilityData = {}
);

}
