#include "stdafx.h"
#include "dxRenderFactory.h"
#include "dxImGuiRender.h"
#include "ImGuiRendererNVRHI.h"

namespace xray::render::ng {
    extern RenderDevice* g_ModernRenderDevice;
}

namespace xray::render {
    IImGuiRender* CreateModernImGuiRenderer();
}

namespace xray::render::RENDER_NAMESPACE
{

#ifdef USE_NVRHI_IMGUI

IImGuiRender* dxRenderFactory::CreateImGuiRender()
{
    if (ng::g_ModernRenderDevice && ng::g_ModernRenderDevice->IsInitialized())
    {
        auto modernRenderer = ng::ImGuiRendererFactory::Create(ng::g_ModernRenderDevice);
        if (modernRenderer)
        {
            Msg("* ImGui: Created NVRHI-based renderer");
            return modernRenderer.release();
        }
    }

    Msg("* ImGui: Using legacy DX11 renderer");
    return xr_new<dxImGuiRender>();
}

void dxRenderFactory::DestroyImGuiRender(IImGuiRender* pObject)
{
    xr_delete(pObject);
}

#else

#endif

}