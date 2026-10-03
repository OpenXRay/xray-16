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
    return pass.initialize(device, geometry_pass, light_pass, vertex.handle(), fragment.handle(),
        alpha_test_fragment.handle(), transparent_fragment.handle(), light_vertex.handle(), light_fragment.handle(),
        weather_fragment.handle(), pass_dispatch, error);
}
}
