#pragma once

#include "UIGeometryBatch.h"
#include "dxFontRender.h"

namespace xray::render::ui
{
using namespace xray::render::RENDER_NAMESPACE;

class FontRenderCollector
{
public:
    FontRenderCollector() = default;
    ~FontRenderCollector() = default;

    void BeginFont(dxFontRender* fontRender);

    void AddQuad(const UIVertex& v0, const UIVertex& v1, const UIVertex& v2, const UIVertex& v3);

    void EndFont();

    const xr_vector<UIGeometryBatch>& GetBatches() const { return m_batches; }

    void Clear();

private:
    xr_vector<UIGeometryBatch> m_batches;
    UIGeometryBatch* m_currentBatch{nullptr};
    dxFontRender* m_currentFont{nullptr};
};

}
