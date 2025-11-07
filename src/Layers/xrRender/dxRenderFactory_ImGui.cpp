#include "stdafx.h"
#include "dxRenderFactory.h"
#include "dxImGuiRender.h"
#include "ImGuiRendererNVRHI.h"

// Forward declarations for integration
namespace xray::render {
    IImGuiRender* GetImGuiRenderer();
}

namespace xray::render::RENDER_NAMESPACE
{

// Always use our new integration approach
IImGuiRender* dxRenderFactory::CreateImGuiRender()
{
    // Check if we already have an ImGui renderer initialized
    IImGuiRender* existingRenderer = xray::render::GetImGuiRenderer();
    if (existingRenderer)
    {
        Msg("* ImGui: Using existing renderer");
        return existingRenderer;
    }

    // Fallback to legacy DX11 implementation
    Msg("* ImGui: Creating legacy DX11 renderer");
    return xr_new<dxImGuiRender>();
}

void dxRenderFactory::DestroyImGuiRender(IImGuiRender* pObject)
{
    // Don't delete if it's the global renderer
    if (pObject != xray::render::GetImGuiRenderer())
    {
        xr_delete(pObject);
    }
}

} // namespace xray::render::RENDER_NAMESPACE