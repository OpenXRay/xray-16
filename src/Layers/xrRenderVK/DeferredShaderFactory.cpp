#include "xrEngine/stdafx.h"
#include "DeferredShaderFactory.h"
#include "DeferredShaders.h"
#include "WeatherShaders.h"
#include "VulkanGameShaders.h"

namespace xray::render::vulkan
{
bool DeferredShaderFactory::create(VkDevice device, const ShaderModuleDispatch& shader_dispatch,
    const ScenePassDispatch& pass_dispatch, VkRenderPass geometry_pass,
    VkRenderPass light_pass, DeferredPass& pass, std::string& error)
{
    ShaderModule vertex, fragment, alpha_test_fragment, transparent_fragment, light_vertex, light_fragment, weather_fragment;
    if (!vertex.initialize(device, shader_dispatch, deferred_shaders::GBufferVertex,
            sizeof(deferred_shaders::GBufferVertex), error) ||
        !fragment.initialize(device, shader_dispatch, deferred_shaders::GBufferFragment,
            sizeof(deferred_shaders::GBufferFragment), error) ||
        !alpha_test_fragment.initialize(device, shader_dispatch,
            game_shaders::CutoutFragment,
            sizeof(game_shaders::CutoutFragment), error) ||
        !transparent_fragment.initialize(device, shader_dispatch,
            game_shaders::TransparentFragment, sizeof(game_shaders::TransparentFragment), error) ||
        !light_vertex.initialize(device, shader_dispatch, deferred_shaders::LightVertex,
            sizeof(deferred_shaders::LightVertex), error) ||
        !light_fragment.initialize(device, shader_dispatch, deferred_shaders::LightFragment,
            sizeof(deferred_shaders::LightFragment), error) ||
        !weather_fragment.initialize(device, shader_dispatch, weather_shaders::Fragment,
            sizeof(weather_shaders::Fragment), error)) return false;
    if (!pass.initialize(device, geometry_pass, light_pass, vertex.handle(), fragment.handle(),
        alpha_test_fragment.handle(), transparent_fragment.handle(), light_vertex.handle(), light_fragment.handle(),
        weather_fragment.handle(), pass_dispatch, error)) return false;
    return reload_game_pipelines(device, shader_dispatch, pass, error);
}

bool DeferredShaderFactory::reload_game_pipelines(VkDevice device, const ShaderModuleDispatch& shader_dispatch,
    DeferredPass& pass, std::string& error)
{
    pass.begin_game_pipeline_reload();

    // Game variants are read through the mounted shader VFS so the APK and
    // installed game use exactly the same SPIR-V assets. The cache owns shader
    // modules until every pipeline has been created; the pipelines then own
    // their compiled executable and modules may be released.
    ShaderModuleCache modules;
    const auto load = [&](const char* name, uint32_t stage, ShaderModule*& result) -> bool
    {
        string_path path;
        if (!FS.exist(path, "$game_shaders$", name, ".spv"))
        {
            error = std::string("missing Vulkan game shader: ") + name;
            return false;
        }
        IReader* reader = FS.r_open(path);
        if (!reader)
        {
            error = std::string("could not open Vulkan game shader: ") + path;
            return false;
        }
        const bool valid = modules.load(path, device, shader_dispatch, reader->pointer(), reader->length(),
            stage, "main", result, error);
        FS.r_close(reader);
        return valid;
    };
    struct Pair { const char* vertex; const char* fragment; SurfaceMode mode; };
    constexpr Pair pairs[]{
        {"level_opaque", "level_opaque", SurfaceMode::Opaque},
        {"object_opaque", "object_opaque", SurfaceMode::Opaque},
        {"level_cutout", "level_cutout", SurfaceMode::AlphaTest},
        {"object_cutout", "object_cutout", SurfaceMode::AlphaTest},
        {"object_blended", "object_blended", SurfaceMode::Transparent},
        {"object_double_sided", "object_double_sided", SurfaceMode::Opaque},
        {"tree_opaque", "level_opaque", SurfaceMode::Opaque},
        {"progressive_opaque", "level_opaque", SurfaceMode::Opaque}
    };
    for (const auto& pair : pairs)
    {
        const std::string vertex_name = std::string("vk\\") + pair.vertex + ".vs";
        const std::string fragment_name = std::string("vk\\") + pair.fragment + ".ps";
        ShaderModule* vs = nullptr;
        ShaderModule* ps = nullptr;
        if (!load(vertex_name.c_str(), 0, vs) || !load(fragment_name.c_str(), 4, ps))
        {
            pass.abort_game_pipeline_reload();
            return false;
        }
        if (!pass.create_game_pipeline(vertex_name, fragment_name,
                vs->handle(), ps->handle(), pair.mode, false, error))
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
        ShaderModule* vs = nullptr;
        if (!load(vertex_name.c_str(), 0, vs)) { pass.abort_game_pipeline_reload(); return false; }
        for (const auto& variant : skinned_fragments)
        {
            const std::string fragment_name = std::string("vk\\") + variant.name + ".ps";
            ShaderModule* ps = nullptr;
            if (!load(fragment_name.c_str(), 4, ps) ||
                !pass.create_game_pipeline(vertex_name, fragment_name,
                    vs->handle(), ps->handle(), variant.mode, false, error, true))
            { pass.abort_game_pipeline_reload(); return false; }
        }
    }
    ShaderModule* hud_vs = nullptr;
    ShaderModule* hud_ps = nullptr;
    if (!load("vk\\hud_skinned_4.vs", 0, hud_vs) ||
        !load("vk\\object_blended.ps", 4, hud_ps) ||
        !pass.create_game_pipeline("vk\\hud_skinned_4.vs", "vk\\object_blended.ps",
            hud_vs->handle(), hud_ps->handle(), SurfaceMode::Opaque, true, error, true))
    { pass.abort_game_pipeline_reload(); return false; }
    pass.commit_game_pipeline_reload();
    error.clear();
    return true;
}
}
