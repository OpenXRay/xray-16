// r2_R_render.cpp
// Stub implementations - FrameGraph handles all rendering
#include "stdafx.h"
#include "r2.h"

namespace xray::render::RENDER_NAMESPACE
{

void CRender::RenderMenu()
{
    // FrameGraph handles menu rendering via UIPass
}

void CRender::Render()
{
    // FrameGraph handles all rendering via FrameGraphRenderer::Render()
}

void CRender::render_forward()
{
    // FrameGraph handles forward rendering
}

void CRender::BeforeWorldRender() {}
void CRender::AfterWorldRender() {}

} // namespace xray::render::RENDER_NAMESPACE
