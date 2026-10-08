#include "xrEngine/stdafx.h"
#include "Include/xrRender/xrRender.h"
#include "VulkanProbe.h"
#include "VulkanLevelRender.h"
#include "VulkanGameDevice.h"
#include "VulkanRenderFactory.h"
#include "VulkanDrawUtils.h"
#include "VulkanDebugRender.h"
#include <SDL.h>
#include <cstring>

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
            if (std::strstr(Core.Params, "-vk_validation"))
                SDL_setenv("XRAY_VK_VALIDATION", "1", 1);
            probe_attempted = true;
            Msg("[renderer-vulkan] probe.begin validation=%s", SDL_getenv("XRAY_VK_VALIDATION") ? "requested" : "off");
            loader_available = probe_vulkan_loader(probe_error);
            if (loader_available)
            {
                modes.emplace_back("renderer_vulkan", 7);
                Msg("[renderer-vulkan] probe.ready mode=renderer_vulkan");
            }
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
        Msg("[renderer-vulkan] requirements.begin shader-root=$game_shaders$");
        // The Vulkan game path owns its model/material resources and does not
        // instantiate the GLES CResourceManager. Check the VFS assets before
        // selecting a Vulkan SDL window; an incomplete install may use GLES
        // in auto mode, while an explicit request will report the failure.
        constexpr pcstr required[]{
            "vk\\level_opaque.vs", "vk\\level_opaque.ps",
            "vk\\object_opaque.vs", "vk\\object_opaque.ps",
            "vk\\level_cutout.vs", "vk\\level_cutout.ps",
            "vk\\object_cutout.vs", "vk\\object_cutout.ps",
            "vk\\object_blended.vs", "vk\\object_blended.ps", "vk\\skinned_blended.ps", "vk\\hud_blended.ps",
            "vk\\object_double_sided.vs", "vk\\object_double_sided.ps",
            "vk\\tree_opaque.vs", "vk\\progressive_opaque.vs",
            "vk\\skinned_1.vs", "vk\\skinned_2.vs",
            "vk\\skinned_3.vs", "vk\\skinned_4.vs", "vk\\hud_skinned_1.vs",
            "vk\\hud_skinned_2.vs", "vk\\hud_skinned_3.vs", "vk\\hud_skinned_4.vs"
        };
        for (pcstr name : required)
        {
            string_path path;
            if (!FS.exist(path, "$game_shaders$", name, ".spv"))
            {
                Msg("! [renderer-vulkan] missing gameplay shader: %s.spv", name);
                return false;
            }
            IReader* reader = FS.r_open(path);
            const size_t length = xr_strlen(name);
            const uint32_t stage = length >= 3 && name[length - 2] == 'v' ? 0u : 4u;
            const bool valid = reader && has_spirv_entry(reader->pointer(), reader->length(), stage, "main");
            if (reader) FS.r_close(reader);
            if (!valid)
            {
                Msg("! [renderer-vulkan] invalid gameplay shader stage/entry: %s.spv", name);
                return false;
            }
        }
        Msg("[renderer-vulkan] requirements.ready shader-count=%zu", sizeof(required) / sizeof(*required));
        return true;
    }

    void SetupEnv(pcstr mode) override
    {
        R_ASSERT2(mode && xr_strcmp(mode, "renderer_vulkan") == 0,
            "Vulkan module received another renderer mode");
        R_ASSERT2(!GEnv.Render || GEnv.Render == &renderer,
            "Another renderer is still bound to GEnv");
        GEnv.Render = &renderer;
        Msg("[renderer-vulkan] module.selected mode=%s", mode);
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
        Msg("[renderer-vulkan] module.clear begin");
        renderer.Destroy();
        GEnv.DRender = nullptr;
        GEnv.DU = nullptr;
        GEnv.UIRender = nullptr;
        GEnv.RenderFactory = nullptr;
        GEnv.Render = nullptr;
    }
};
}

RendererModule* GetRendererModule()
{
    // xrCore's logging lock can be destroyed before global renderer objects.
    // The engine explicitly calls ClearEnv during renderer shutdown; keep the
    // module alive until process exit instead of running its destructor after
    // xrCore's static teardown and logging through an invalid lock.
    static auto* module = new VulkanRendererModule;
    return module;
}
}
