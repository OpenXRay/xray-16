#include "xrEngine/stdafx.h"
#include "Include/xrRender/xrRender.h"
#include "VulkanProbe.h"
#include "VulkanLevelRender.h"
#include "VulkanGameDevice.h"
#include "VulkanRenderFactory.h"
#include "VulkanDrawUtils.h"
#include "VulkanDebugRender.h"
#include "VulkanSettings.h"
#include "xrEngine/XR_IOConsole.h"
#include "xrEngine/xr_ioc_cmd.h"
#include <SDL.h>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <cmath>

namespace xray::render::vulkan
{
namespace
{
const xr_token quality_tokens[]{{"Minimum", 0}, {"Low", 1}, {"Default", 2}, {"High", 3}, {"Extreme", 4}, {nullptr, 0}};

class VulkanPreset final : public CCC_Token
{
  public:
    VulkanPreset() : CCC_Token("_preset", &quality_preset, quality_tokens)
    {
    }
    void Execute(pcstr args) override
    {
        CCC_Token::Execute(args);
        if (get_token_id(quality_tokens, args) >= 0)
            set_vulkan_quality(quality_preset);
    }
};

void register_vulkan_options()
{
    static VulkanPreset preset;
    static CCC_Float detail_time("r_vk_detail_prepare_ms", &detail_prepare_ms, 0.1f, 20.f);
    Console->AddCommand(&detail_time);
    static CCC_Integer light_grid("r_vk_light_grid", &use_light_grid, 0, 1);
    Console->AddCommand(&light_grid);
    static CCC_Integer detail_updates("r_vk_detail_update_cells", &detail_update_cells, 1, 256);
    static CCC_Integer point_shadows("r_vk_point_shadows", &point_shadow_budget, 0, 2);
    static CCC_Integer spot_shadows("r_vk_spot_shadows", &spot_shadow_budget, 0, 4);
    static CCC_Integer ros_rays("r_vk_ros_ray_budget", &ros_ray_budget, 0, 2048);
    static CCC_Float sky_scale("r2_dhemi_sky_scale", &hemi_sky_scale, 0.f, 100.f);
    static CCC_Float light_scale("r2_dhemi_light_scale", &hemi_light_scale, 0.f, 100.f);
    static CCC_Float light_flow("r2_dhemi_light_flow", &hemi_light_flow, 0.f, 1.f);
    static CCC_Float smooth("r2_dhemi_smooth", &hemi_smooth, 0.f, 10.f);
    static CCC_Integer anisotropy("r__tf_aniso", &texture_anisotropy, 1, 16);
    static CCC_Integer details("r__detail_radius", &detail_radius, 0, 80);
    static CCC_Float ambient("r2_sun_lumscale_amb", &ambient_scale, 0.f, 3.f);
    static CCC_Float hemisphere("r2_sun_lumscale_hemi", &hemisphere_scale, 0.f, 3.f);
    static CCC_Float sun("r2_sun_lumscale", &sun_scale, 0.f, 3.f);
    static CCC_Integer sun_switch("r2_sun", &sun_enabled, 0, 1);
    static CCC_Integer tone_switch("r2_tonemap", &tonemap_enabled, 0, 1);
    static CCC_Float tone_gray("r2_tonemap_middlegray", &tonemap_middlegray, 0.f, 2.f);
    static CCC_Float tone_amount("r2_tonemap_amount", &tonemap_amount, 0.f, 1.f);
    apply_vulkan_launch_settings(Core.Params);
    IConsole_Command *options[]{&point_shadows, &spot_shadows, &ros_rays, &sky_scale,
                                &light_scale,   &light_flow,   &smooth};
    for (auto *option : options)
        Console->AddCommand(option);
    Console->AddCommand(&preset);
    Console->AddCommand(&details);
    Console->AddCommand(&anisotropy);
    Console->AddCommand(&detail_updates);
    Console->AddCommand(&ambient);
    Console->AddCommand(&hemisphere);
    Console->AddCommand(&sun);
    Console->AddCommand(&sun_switch);
    Console->AddCommand(&tone_switch);
    Console->AddCommand(&tone_gray);
    Console->AddCommand(&tone_amount);
}

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
    const xr_vector<std::pair<pcstr, int>> &ObtainSupportedModes() override
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
        constexpr pcstr required[]{"vk\\level_opaque.vs",        "vk\\level_opaque.ps",        "vk\\object_opaque.vs",
                                   "vk\\object_opaque.ps",       "vk\\level_cutout.vs",        "vk\\level_cutout.ps",
                                   "vk\\object_cutout.vs",       "vk\\object_cutout.ps",       "vk\\object_blended.vs",
                                   "vk\\object_blended.ps",      "vk\\skinned_blended.ps",     "vk\\hud_blended.ps",
                                   "vk\\object_double_sided.vs", "vk\\object_double_sided.ps", "vk\\tree_opaque.vs",
                                   "vk\\progressive_opaque.vs",  "vk\\skinned_1.vs",           "vk\\skinned_2.vs",
                                   "vk\\skinned_3.vs",           "vk\\skinned_4.vs",           "vk\\hud_skinned_1.vs",
                                   "vk\\hud_skinned_2.vs",       "vk\\hud_skinned_3.vs",       "vk\\hud_skinned_4.vs"};
        for (pcstr name : required)
        {
            string_path path;
            if (!FS.exist(path, "$game_shaders$", name, ".spv"))
            {
                Msg("! [renderer-vulkan] missing gameplay shader: %s.spv", name);
                return false;
            }
            IReader *reader = FS.r_open(path);
            const size_t length = xr_strlen(name);
            const uint32_t stage = length >= 3 && name[length - 2] == 'v' ? 0u : 4u;
            const bool valid = reader && has_spirv_entry(reader->pointer(), reader->length(), stage, "main");
            if (reader)
                FS.r_close(reader);
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
        R_ASSERT2(mode && xr_strcmp(mode, "renderer_vulkan") == 0, "Vulkan module received another renderer mode");
        R_ASSERT2(!GEnv.Render || GEnv.Render == &renderer, "Another renderer is still bound to GEnv");
        GEnv.Render = &renderer;
        register_vulkan_options();
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
        if (GEnv.Render != &renderer)
            return;
        Msg("[renderer-vulkan] module.clear begin");
        renderer.Destroy();
        GEnv.DRender = nullptr;
        GEnv.DU = nullptr;
        GEnv.UIRender = nullptr;
        GEnv.RenderFactory = nullptr;
        GEnv.Render = nullptr;
    }
};
} // namespace

RendererModule *GetRendererModule()
{
    // xrCore's logging lock can be destroyed before global renderer objects.
    // The engine explicitly calls ClearEnv during renderer shutdown; keep the
    // module alive until process exit instead of running its destructor after
    // xrCore's static teardown and logging through an invalid lock.
    static auto *module = new VulkanRendererModule;
    return module;
}
} // namespace xray::render::vulkan
