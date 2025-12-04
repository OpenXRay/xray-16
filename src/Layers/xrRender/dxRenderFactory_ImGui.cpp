#include "stdafx.h"
#include "dxRenderFactory.h"
#include "dxImGuiRender.h"
#include "ImGuiRendererNVRHI.h"
#include "RenderContext/RenderDevice.h"

namespace xray::render {
    IImGuiRender* GetImGuiRenderer();
}

namespace xray::render::RENDER_NAMESPACE
{

IImGuiRender* dxRenderFactory::CreateImGuiRender()
{
    IImGuiRender* existingRenderer = xray::render::GetImGuiRenderer();
    if (existingRenderer)
    {
        Msg("* ImGui: Using existing renderer");
        return existingRenderer;
    }

    auto& render = RImplementation;
    if (render.m_renderDevice)
    {
        Msg("* ImGui: Creating NVRHI renderer");
        return xr_new<xray::render::ng::ImGuiRendererNVRHI>(render.m_renderDevice);
    }

    Msg("! ImGui: No RenderDevice available, falling back to legacy DX11 renderer");
    return xr_new<dxImGuiRender>();
}

void dxRenderFactory::DestroyImGuiRender(IImGuiRender* pObject)
{
    if (pObject != xray::render::GetImGuiRenderer())
    {
        xr_delete(pObject);
    }
}

}