#pragma once

#include "DeferredPass.h"
#include "ShaderModule.h"
#include "GameShaderResources.h"

namespace xray::render::vulkan
{
void configure_engine_shader_resources(VkDevice device, const ShaderModuleDispatch &dispatch, GameShaderResources &resources);
// Owns modules after pipeline creation, including the failure path.
class DeferredShaderFactory
{
  public:
    bool create(VkDevice device, const ShaderModuleDispatch &shader_dispatch, const ScenePassDispatch &pass_dispatch, VkRenderPass geometry_pass,
                VkRenderPass light_pass, DeferredPass &pass, GameShaderResources &resources, std::string &error,
                VkRenderPass shadow_pass = VK_NULL_HANDLE,
                VkRenderPass local_shadow_pass = VK_NULL_HANDLE);
    // The caller waits for submitted frames before swapping pipelines.
    bool reload_game_pipelines(VkDevice device, const ShaderModuleDispatch &shader_dispatch, DeferredPass &pass, GameShaderResources &resources,
                               std::string &error);
};
} // namespace xray::render::vulkan
