#include "stdafx.h"
#include "dxRenderFactory.h"
#include "Layers/xrRenderDX11/dxImGuiRender.h"
#include "ImGuiRendererNVRHI.h"
#include "RenderContext/RenderDevice.h"

namespace xray::render {
    IImGuiRender* GetImGuiRenderer();
}

namespace xray::render::RENDER_NAMESPACE
{

IImGuiRender* dxRenderFactory::CreateImGuiRender()
{
    if (IImGuiRender* existingRenderer = xray::render::GetImGuiRenderer())
        return existingRenderer;

    auto& render = RImplementation;
    R_ASSERT2(render.m_renderDevice, "ImGui: RenderDevice not initialized");
    return xr_new<xray::render::fg::ImGuiRendererNVRHI>(render.m_renderDevice);
}

void dxRenderFactory::DestroyImGuiRender(IImGuiRender* pObject)
{
    if (pObject != xray::render::GetImGuiRenderer())
    {
        xr_delete(pObject);
    }
}

}