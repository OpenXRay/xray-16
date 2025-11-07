#include "stdafx.h"
#include "dxRenderFactory.h"
#include "dxImGuiRender.h"
#include "ImGuiRendererNVRHI.h"

// Forward declarations for integration
namespace xray::render::ng {
    extern RenderDevice* g_ModernRenderDevice;
}

namespace xray::render {
    IImGuiRender* CreateModernImGuiRenderer();
}

namespace xray::render::RENDER_NAMESPACE
{

// Override the macro-generated CreateImGuiRender with custom implementation
#ifdef USE_NVRHI_IMGUI

IImGuiRender* dxRenderFactory::CreateImGuiRender()
{
    // First check if we have a modern NVRHI render device available
    if (ng::g_ModernRenderDevice && ng::g_ModernRenderDevice->IsInitialized())
    {
        // Create NVRHI-based ImGui renderer
        auto modernRenderer = ng::ImGuiRendererFactory::Create(ng::g_ModernRenderDevice);
        if (modernRenderer)
        {
            Msg("* ImGui: Created NVRHI-based renderer");
            return modernRenderer.release();
        }
    }

    // Fallback to legacy DX11 implementation
    Msg("* ImGui: Using legacy DX11 renderer");
    return xr_new<dxImGuiRender>();
}

void dxRenderFactory::DestroyImGuiRender(IImGuiRender* pObject)
{
    xr_delete(pObject);
}

#else // USE_NVRHI_IMGUI

// Use standard macro implementation if NVRHI ImGui is not enabled
// This will use the macro from dxRenderFactory.cpp

#endif // USE_NVRHI_IMGUI

} // namespace xray::render::RENDER_NAMESPACE