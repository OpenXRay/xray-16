#include "stdafx.h"
#include "ImGuiRendererNVRHI.h"
#include "Layers/xrRenderDX11/dxImGuiRender.h"
#include "RenderContext/RenderDevice.h"
#include "Layers/xrRender_R2/r2.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"

namespace xray::render {

// Global pointer to current ImGui renderer
static xr_unique_ptr<IImGuiRender> g_ImGuiRenderer;

// Global pointer to the NVRHI render device (set during initialization)
static fg::RenderDevice* g_RenderDevice = nullptr;

//=============================================================================
// Phase 8: Integration with existing system
//=============================================================================

// Forward declaration for legacy renderer creation
namespace fg {
    IImGuiRender* CreateLegacyImGuiRendererImpl()
    {
        return xr_new<dxImGuiRender>();
    }
}

// Initialize the ImGui renderer based on available backends
void InitializeImGuiRenderer(fg::RenderDevice* renderDevice)
{
    g_RenderDevice = renderDevice;

    if (renderDevice && renderDevice->IsInitialized())
    {
        fg::ImGuiRendererNVRHI* nvrhiRenderer = xr_new<fg::ImGuiRendererNVRHI>(renderDevice);
        g_ImGuiRenderer.reset(nvrhiRenderer);

        fg::RImplementation.SetImGuiRendererNVRHI(nvrhiRenderer);

        Msg("* ImGui: Using NVRHI renderer (supports DX11/DX12/Vulkan)");
    }
    else
    {
        // Fallback to legacy implementation
        g_ImGuiRenderer.reset(fg::CreateLegacyImGuiRendererImpl());
        Msg("* ImGui: Using legacy DX11 renderer");
    }
}

// Get the current ImGui renderer
IImGuiRender* GetImGuiRenderer()
{
    return g_ImGuiRenderer.get();
}

// Shutdown and cleanup
void ShutdownImGuiRenderer()
{
    g_ImGuiRenderer.reset();
    g_RenderDevice = nullptr;
}

//=============================================================================
// Command List Provider Interface
//=============================================================================

// This interface needs to be implemented by the renderer to provide command lists
class ImGuiCommandListProvider
{
public:
    virtual nvrhi::ICommandList* GetImGuiCommandList() = 0;
    virtual void SubmitImGuiCommands(nvrhi::ICommandList* cmdList) = 0;
};

// Global command list provider (set by the renderer)
static ImGuiCommandListProvider* g_CommandListProvider = nullptr;

void SetImGuiCommandListProvider(ImGuiCommandListProvider* provider)
{
    g_CommandListProvider = provider;
}

//=============================================================================
// Modified ImGuiRendererNVRHI to work with command list provider
//=============================================================================

namespace fg {

// Extension to support command list acquisition
class ImGuiRendererNVRHI_Integrated : public ImGuiRendererNVRHI
{
public:
    ImGuiRendererNVRHI_Integrated(RenderDevice* device)
        : ImGuiRendererNVRHI(device)
    {
    }

    void Render(ImDrawData* data) override
    {
        if (!data || data->TotalVtxCount == 0)
            return;

        // Get command list from provider
        if (!g_CommandListProvider)
        {
            Msg("! ImGui: No command list provider set");
            return;
        }

        nvrhi::ICommandList* cmdList = g_CommandListProvider->GetImGuiCommandList();
        if (!cmdList)
        {
            Msg("! ImGui: Failed to get command list from provider");
            return;
        }

        // Render using base implementation
        RenderDrawData(data, cmdList);

        // Submit commands
        g_CommandListProvider->SubmitImGuiCommands(cmdList);
    }
};
} // namespace fg

//=============================================================================
// Integration with existing dxRenderFactory
//=============================================================================

// This needs to be called from dxRenderFactory::CreateImGuiRender()
IImGuiRender* CreateModernImGuiRenderer()
{
    if (g_RenderDevice && g_RenderDevice->IsInitialized())
    {
        auto renderer = fg::ImGuiRendererFactory::Create(g_RenderDevice);
        return renderer.release(); // Transfer ownership
    }

    return nullptr;
}

} // namespace xray::render
