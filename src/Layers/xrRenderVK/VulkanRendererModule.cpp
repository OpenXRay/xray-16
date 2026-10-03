#include "xrEngine/stdafx.h"
#include "Include/xrRender/xrRender.h"
#include "VulkanProbe.h"
#include "VulkanLevelRender.h"
#include "VulkanGameDevice.h"
#include "VulkanRenderFactory.h"
#include "VulkanDrawUtils.h"
#include "VulkanDebugRender.h"

namespace xray::render::vulkan
{
namespace
{
class VulkanRendererModule final : public RendererModule
{
    xr_vector<std::pair<pcstr, int>> modes;
    bool probe_attempted{};
    bool loader_available{};
    std::string probe_error;
    VulkanGameDevice device;
    VulkanLevelRender renderer{device};
    VulkanRenderFactory factory{device};
    VulkanDrawUtils draw_utils{device};
#ifdef DEBUG
    VulkanDebugRender debug_render{device};
#endif

public:
    const xr_vector<std::pair<pcstr, int>>& ObtainSupportedModes() override
    {
        if (!probe_attempted)
        {
            probe_attempted = true;
            loader_available = probe_vulkan_loader(probe_error);
            if (loader_available)
                modes.emplace_back("renderer_vulkan", 7);
            else
                Log("~ [renderer-vulkan] unavailable: %s", probe_error.c_str());
        }
        return modes;
    }

    bool CheckGameRequirements() override
    {
        if (!probe_attempted)
            ObtainSupportedModes();
        if (!loader_available)
            return false;
        // The shared game shader resource manager still dispatches through
        // GLES RImplementation. Enabling selection before its Vulkan pipeline
        // is wired would initialize GL shader resources in a Vulkan window.
        Log("! [renderer-vulkan] gameplay shader resource pipeline is not ready");
        return false;
    }

    void SetupEnv(pcstr mode) override
    {
        R_ASSERT2(mode && xr_strcmp(mode, "renderer_vulkan") == 0,
            "Vulkan module received another renderer mode");
        R_ASSERT2(!GEnv.Render || GEnv.Render == &renderer,
            "Another renderer is still bound to GEnv");
        GEnv.Render = &renderer;
        GEnv.RenderFactory = &factory;
        GEnv.UIRender = &device.ui();
        GEnv.DU = &draw_utils;
#ifdef DEBUG
        GEnv.DRender = &debug_render;
#endif
    }

    void ClearEnv() override
    {
        if (GEnv.Render != &renderer) return;
        renderer.Destroy();
        GEnv.DRender = nullptr;
        GEnv.DU = nullptr;
        GEnv.UIRender = nullptr;
        GEnv.RenderFactory = nullptr;
        GEnv.Render = nullptr;
    }
};
VulkanRendererModule module;
}

RendererModule* GetRendererModule() { return &module; }
}
