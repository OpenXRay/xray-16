// xrRender/FontRenderCollector.h
#pragma once

#include "UIGeometryBatch.h"
#include "dxFontRender.h"

namespace xray::render::ui
{
using namespace xray::render::fg;  // For dxFontRender

// Collector for font geometry
// Fonts use the same vertex format as UI (FVF::TL = UIVertex)
// So we can reuse UIGeometryBatch for font rendering
class FontRenderCollector
{
public:
    FontRenderCollector() = default;
    ~FontRenderCollector() = default;

    // Begin collecting geometry for a font
    void BeginFont(dxFontRender* fontRender);

    // Add a font quad (4 vertices forming 2 triangles)
    void AddQuad(const UIVertex& v0, const UIVertex& v1, const UIVertex& v2, const UIVertex& v3);

    // End collecting for current font
    void EndFont();

    // Get collected batches
    const xr_vector<UIGeometryBatch>& GetBatches() const { return m_batches; }

    // Clear all collected geometry
    void Clear();

private:
    xr_vector<UIGeometryBatch> m_batches;
    UIGeometryBatch* m_currentBatch{nullptr};
    dxFontRender* m_currentFont{nullptr};
};

} // namespace xray::render::ui
