#include "stdafx.h"
#include "r2.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"

namespace xray::render::fg
{

void CRender::RenderMenu()
{
    if (m_framegraphRenderer)
        m_framegraphRenderer->RenderMenu();
}

void CRender::Render()
{
    if (m_framegraphRenderer)
        m_framegraphRenderer->Render();
}

void CRender::render_forward()
{
}

void CRender::BeforeWorldRender() {}
void CRender::AfterWorldRender() {}

} // namespace xray::render::fg
