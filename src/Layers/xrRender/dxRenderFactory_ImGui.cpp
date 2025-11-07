#include "stdafx.h"
#include "dxRenderFactory.h"
#include "dxImGuiRender.h"
#include "ImGuiRendererNVRHI.h"

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

    Msg("* ImGui: Creating legacy DX11 renderer");
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