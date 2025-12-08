// xrRender/FrameGraphPasses/SkinningPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"

namespace xray::render {
    struct GeometryBatch;
    class MaterialCache;
    class GeometryCollector;
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

// ═══════════════════════════════════════════════════════════════════════════
//  SKINNING PASS - Consolidated skinned mesh rendering
// ═══════════════════════════════════════════════════════════════════════════
// Renders all skinned meshes in two phases:
//   1. World Phase - NPCs, monsters, etc. with normal depth [0.0, 1.0]
//   2. HUD Phase - First-person weapons/hands with compressed depth [0.0, 0.1]
//
// This pass consolidates all skinned mesh rendering that was previously split
// between ForwardColorPass (world skinned) and HUDPass (HUD skinned).

// Initialize skinned pipelines eagerly at device creation
// Must be called before any skinning pass execution
void InitializeSkinningPipelines(ng::RenderDevice* device);

// Release all skinned pipeline resources
void ShutdownSkinningPipelines();

// Main skinning pass setup function
// Renders world skinned meshes followed by HUD skinned meshes
framegraph::DefaultOutputLayout setupSkinningPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const GeometryCollector* geometry,       // Contains world skinned batches
    const xr_vector<GeometryBatch>* hudBatches,  // HUD skinned batches (weapons, hands)
    MaterialCache* materialCache,
    u32 width,
    u32 height
);

} // namespace xray::render::RENDER_NAMESPACE::passes
