#include "xrEngine/stdafx.h"
#include "DeferredShaderFactory.h"
#include "DeferredShaders.h"
#include "WeatherShaders.h"
#include "VulkanGameShaders.h"

namespace xray::render::vulkan
{
void configure_engine_shader_resources(VkDevice device, const ShaderModuleDispatch &dispatch, GameShaderResources &resources)
{
    resources.configure(device, dispatch, [](const std::string &name, std::vector<uint8_t> &bytes, std::string &reason) {
        string_path path;
        if (!FS.exist(path, "$game_shaders$", name.c_str()))
        {
            reason = "missing precompiled SPIR-V: " + name;
            return false;
        }
        IReader *reader = FS.r_open(path);
        if (!reader)
        {
            reason = "could not open SPIR-V: " + name;
            return false;
        }
        const auto *begin = static_cast<const uint8_t *>(reader->pointer());
        bytes.clear();
        if (reader->length())
            bytes.assign(begin, begin + reader->length());
        FS.r_close(reader);
        return true;
    });
}

bool DeferredShaderFactory::create(VkDevice device, const ShaderModuleDispatch &shader_dispatch, const ScenePassDispatch &pass_dispatch,
                                   VkRenderPass geometry_pass, VkRenderPass light_pass, DeferredPass &pass, GameShaderResources &resources, std::string &error)
{
    ShaderModule vertex, fragment, alpha_test_fragment, transparent_fragment, light_vertex, light_fragment, weather_fragment;
    if (!vertex.initialize(device, shader_dispatch, deferred_shaders::GBufferVertex, sizeof(deferred_shaders::GBufferVertex), error) ||
        !fragment.initialize(device, shader_dispatch, deferred_shaders::GBufferFragment, sizeof(deferred_shaders::GBufferFragment), error) ||
        !alpha_test_fragment.initialize(device, shader_dispatch, game_shaders::CutoutFragment, sizeof(game_shaders::CutoutFragment), error) ||
        !transparent_fragment.initialize(device, shader_dispatch, game_shaders::TransparentFragment, sizeof(game_shaders::TransparentFragment), error) ||
        !light_vertex.initialize(device, shader_dispatch, deferred_shaders::LightVertex, sizeof(deferred_shaders::LightVertex), error) ||
        !light_fragment.initialize(device, shader_dispatch, deferred_shaders::LightFragment, sizeof(deferred_shaders::LightFragment), error) ||
        !weather_fragment.initialize(device, shader_dispatch, weather_shaders::Fragment, sizeof(weather_shaders::Fragment), error))
        return false;
    if (!pass.initialize(device, geometry_pass, light_pass, vertex.handle(), fragment.handle(), alpha_test_fragment.handle(), transparent_fragment.handle(),
                         light_vertex.handle(), light_fragment.handle(), weather_fragment.handle(), pass_dispatch, error))
        return false;
    return reload_game_pipelines(device, shader_dispatch, pass, resources, error);
}

bool DeferredShaderFactory::reload_game_pipelines(VkDevice device, const ShaderModuleDispatch &shader_dispatch, DeferredPass &pass,
                                                  GameShaderResources &resources, std::string &error)
{
    pass.begin_game_pipeline_reload();

    // Startup and on-demand legacy SVS/SPS pairs use the same device-owned
    // resource cache and VFS resolver as IRender::shader_compile.
    (void)device;
    (void)shader_dispatch;
    struct Pair
    {
        const char *vertex;
        const char *fragment;
        SurfaceMode mode;
    };
    constexpr Pair pairs[]{{"level_opaque", "level_opaque", SurfaceMode::Opaque},          {"object_opaque", "object_opaque", SurfaceMode::Opaque},
                           {"level_cutout", "level_cutout", SurfaceMode::AlphaTest},       {"object_cutout", "object_cutout", SurfaceMode::AlphaTest},
                           {"object_blended", "object_blended", SurfaceMode::Transparent}, {"object_double_sided", "object_double_sided", SurfaceMode::Opaque},
                           {"tree_opaque", "level_opaque", SurfaceMode::Opaque},           {"progressive_opaque", "level_opaque", SurfaceMode::Opaque}};
    for (const auto& pair : pairs)
    {
        const std::string vertex_name = std::string("vk\\") + pair.vertex + ".vs";
        const std::string fragment_name = std::string("vk\\") + pair.fragment + ".ps";
        if (!resources.pipeline(pass, vertex_name, fragment_name, pair.mode, false, false, error))
        {
            pass.abort_game_pipeline_reload();
            return false;
        }
    }
    struct SkinnedFragment { const char* name; SurfaceMode mode; };
    constexpr SkinnedFragment skinned_fragments[]{
        {"object_opaque", SurfaceMode::Opaque},
        {"object_cutout", SurfaceMode::AlphaTest},
        {"object_blended", SurfaceMode::Transparent}
    };
    for (unsigned weights = 1; weights <= 4; ++weights)
    {
        const std::string vertex_name = "vk\\skinned_" + std::to_string(weights) + ".vs";
        for (const auto &variant : skinned_fragments)
        {
            const std::string fragment_name = std::string("vk\\") + variant.name + ".ps";
            if (!resources.pipeline(pass, vertex_name, fragment_name, variant.mode, false, true, error))
            {
                pass.abort_game_pipeline_reload();
                return false;
            }
        }
    }
    if (!resources.pipeline(pass, "vk\\hud_skinned_4.vs", "vk\\object_blended.ps", SurfaceMode::Opaque, true, true, error))
    {
        pass.abort_game_pipeline_reload();
        return false;
    }
    pass.commit_game_pipeline_reload();
    error.clear();
    return true;
}
} // namespace xray::render::vulkan
