#include "stdafx.h"
#include "ImGuiRendererNVRHI.h"
#include "RenderContext/RenderDevice.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"

namespace xray::render {

static xr_unique_ptr<IImGuiRender> g_ImGuiRenderer;
static fg::RenderDevice* g_RenderDevice = nullptr;

void InitializeImGuiRenderer(fg::RenderDevice* renderDevice)
{
    g_RenderDevice = renderDevice;
    R_ASSERT2(renderDevice && renderDevice->IsInitialized(), "ImGui: RenderDevice required");

    fg::ImGuiRendererNVRHI* nvrhiRenderer = xr_new<fg::ImGuiRendererNVRHI>(renderDevice);
    g_ImGuiRenderer.reset(nvrhiRenderer);
    fg::RImplementation.SetImGuiRendererNVRHI(nvrhiRenderer);
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
