#include "xrEngine/stdafx.h"
#include "Include/xrRender/xrRender.h"

namespace xray::render::vulkan
{
namespace
{
class VulkanRendererModule final : public RendererModule
{
    xr_vector<std::pair<pcstr, int>> modes{{"renderer_vulkan", 7}};

public:
    const xr_vector<std::pair<pcstr, int>>& ObtainSupportedModes() override { return modes; }

    bool CheckGameRequirements() override
    {
        // The device, swapchain, shader and texture building blocks live in
        // this target, but an IRender implementation and its companion factory,
        // UI and device renderers do not exist yet. Never bind GL objects to a
        // mode advertised as Vulkan, even when the Vulkan probe passes.
        Log("! [renderer-vulkan] gameplay pipeline unavailable (world/UI/device renderers missing)");
        return false;
    }

    void SetupEnv(pcstr) override
    {
        R_ASSERT2(false, "Vulkan gameplay pipeline is not implemented");
    }

    void ClearEnv() override {}
};
VulkanRendererModule module;
}

RendererModule* GetRendererModule() { return &module; }
}
