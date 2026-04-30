#include "stdafx.h"
#include "dxRenderFactory.h"
#include "Layers/xrRenderDX11/dxImGuiRender.h"
#include "ImGuiRendererNVRHI.h"
#include "RenderContext/RenderDevice.h"

// Forward declarations for integration
namespace xray::render {
    IImGuiRender* GetImGuiRenderer();
}

namespace xray::render::fg
{

IImGuiRender* dxRenderFactory::CreateImGuiRender()
{
    if (IImGuiRender* existingRenderer = xray::render::GetImGuiRenderer())
        return existingRenderer;

    auto* renderDevice = RImplementation.GetRenderDevice();
    R_ASSERT2(renderDevice, "ImGui: RenderDevice not initialized");
    return xr_new<xray::render::fg::ImGuiRendererNVRHI>(renderDevice);
}

void dxRenderFactory::DestroyImGuiRender(IImGuiRender* pObject)
{
    // Don't delete if it's the global renderer
    if (pObject != xray::render::GetImGuiRenderer())
    {
        xr_delete(pObject);
    }
}

} // namespace xray::render::fg