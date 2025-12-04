#include "stdafx.h"
#include "dxRenderFactory.h"
#include "dxImGuiRender.h"
#include "ImGuiRendererNVRHI.h"
#include "RenderContext/RenderDevice.h"

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

    // Create NVRHI-based ImGui renderer for D3D12
    // Get RenderDevice from the renderer
    auto& render = RImplementation;
    if (render.m_renderDevice)
    {
        Msg("* ImGui: Creating NVRHI renderer");
        return xr_new<xray::render::ng::ImGuiRendererNVRHI>(render.m_renderDevice);
    }

    // Fallback to legacy DX11 implementation (should not happen with D3D12)
    Msg("! ImGui: No RenderDevice available, falling back to legacy DX11 renderer");
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