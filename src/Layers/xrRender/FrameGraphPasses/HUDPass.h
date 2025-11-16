#pragma once

#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"

namespace xray::render {
    class MaterialCache;
}

namespace xray::render::passes {

struct HUDPassConfig {
    u32 width = 0;
    u32 height = 0;
};

class HUDPass : public framegraph::IPass {
public:
    HUDPass(const HUDPassConfig& config);
    ~HUDPass() override;

    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;
    framegraph::RenderPhase GetPhase() const override { return framegraph::RenderPhase::Geometry; }

    void SetHUDBatches(const xr_vector<GeometryBatch>* batches) { m_hudBatches = batches; }

    void SetMaterialCache(MaterialCache* cache) { m_materialCache = cache; }

    void SetOutputs(const framegraph::DefaultOutputLayout& outputs) { m_outputs = outputs; }

private:
    HUDPassConfig m_config;

    const xr_vector<GeometryBatch>* m_hudBatches = nullptr;

    MaterialCache* m_materialCache = nullptr;

    framegraph::DefaultOutputLayout m_outputs;
};

}
