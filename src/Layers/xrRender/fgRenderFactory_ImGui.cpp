#include "stdafx.h"
#include "fgRenderFactory.h"
#include "ImGuiRendererNVRHI.h"
#include "RenderContext/RenderDevice.h"

namespace xray::render {
    IImGuiRender* GetImGuiRenderer();
}

namespace xray::render::fg
{

IImGuiRender* fgRenderFactory::CreateImGuiRender()
{
    if (IImGuiRender* existingRenderer = xray::render::GetImGuiRenderer())
        return existingRenderer;

    auto* renderDevice = RImplementation.GetRenderDevice();
    R_ASSERT2(renderDevice, "ImGui: RenderDevice not initialized");
    return xr_new<xray::render::fg::ImGuiRendererNVRHI>(renderDevice);
}

void fgRenderFactory::DestroyImGuiRender(IImGuiRender* pObject)
{
    if (pObject != xray::render::GetImGuiRenderer())
        xr_delete(pObject);
}

} // namespace xray::render::fg