#include "stdafx.h"
#include "ImGuiRendererNVRHI.h"
#include "Layers/xrRenderDX11/dxImGuiRender.h"
#include "RenderContext/RenderDevice.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"

namespace xray::render {

static xr_unique_ptr<IImGuiRender> g_ImGuiRenderer;

static fg::RenderDevice* g_RenderDevice = nullptr;

namespace fg {
    IImGuiRender* CreateLegacyImGuiRendererImpl()
    {
        return xr_new<dxImGuiRender>();
    }
}

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
        g_ImGuiRenderer.reset(fg::CreateLegacyImGuiRendererImpl());
        Msg("* ImGui: Using legacy DX11 renderer");
    }
}

IImGuiRender* GetImGuiRenderer()
{
    return g_ImGuiRenderer.get();
}

void ShutdownImGuiRenderer()
{
    g_ImGuiRenderer.reset();
    g_RenderDevice = nullptr;
}

class ImGuiCommandListProvider
{
public:
    virtual nvrhi::ICommandList* GetImGuiCommandList() = 0;
    virtual void SubmitImGuiCommands(nvrhi::ICommandList* cmdList) = 0;
};

static ImGuiCommandListProvider* g_CommandListProvider = nullptr;

void SetImGuiCommandListProvider(ImGuiCommandListProvider* provider)
{
    g_CommandListProvider = provider;
}

namespace fg {

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

        RenderDrawData(data, cmdList);

        g_CommandListProvider->SubmitImGuiCommands(cmdList);
    }
};
}

IImGuiRender* CreateModernImGuiRenderer()
{
    if (g_RenderDevice && g_RenderDevice->IsInitialized())
    {
        auto renderer = fg::ImGuiRendererFactory::Create(g_RenderDevice);
        return renderer.release();
    }

    return nullptr;
}

}
