// xrRender/FrameGraphPasses/HUDPass.h
#pragma once

#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"  // For PassOutputs
#include "Layers/xrRender/Geometry/GeometryBatch.h"

// Forward declarations
namespace xray::render {
    class MaterialCache;
}

namespace xray::render::passes {

// ══════════════════════════════════════════════════════════
//  HUD PASS - World-Space HUD Rendering
// ══════════════════════════════════════════════════════════
//
// Renders HUD items (weapons, hands) in world space with:
// - Depth range [0.0, 0.1] to ensure HUD always on top
// - Same G-Buffer targets as world geometry
// - Full deferred lighting (PBR materials, reflections, etc.)
//
// Why world-space HUD?
// - Proper lighting on weapons (looks better than unlit)
// - PBR materials work correctly (metallic, roughness)
// - Easier to implement effects (emissive, wetness)
// - Modern approach (Unreal, Unity do this)

struct HUDPassConfig {
    u32 width = 0;
    u32 height = 0;
};

class HUDPass : public framegraph::IPass {
public:
    HUDPass(const HUDPassConfig& config);
    ~HUDPass() override;

    // IPass interface
    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;
    framegraph::RenderPhase GetPhase() const override { return framegraph::RenderPhase::Geometry; }

    // Set HUD batches to render
    void SetHUDBatches(const xr_vector<GeometryBatch>* batches) { m_hudBatches = batches; }

    // Share MaterialCache with GBufferPass
    void SetMaterialCache(MaterialCache* cache) { m_materialCache = cache; }

    // Set render target outputs (from GBufferPass)
    void SetOutputs(const framegraph::DefaultOutputLayout& outputs) { m_outputs = outputs; }

private:
    HUDPassConfig m_config;

    // HUD batches (owned by FrameGraphRenderer)
    const xr_vector<GeometryBatch>* m_hudBatches = nullptr;

    // Material cache (shared with GBufferPass)
    MaterialCache* m_materialCache = nullptr;

    // Render target outputs (uses DefaultOutputLayout from IPass.h)
    framegraph::DefaultOutputLayout m_outputs;
};

} // namespace xray::render::passes
