#pragma once

#include "DeferredPass.h"
#include "ShaderModule.h"

namespace xray::render::vulkan
{
// Owns modules after pipeline creation, including the failure path.
class DeferredShaderFactory
{
public:
    bool create(VkDevice device, const ShaderModuleDispatch& shader_dispatch,
        const ScenePassDispatch& pass_dispatch, VkRenderPass geometry_pass,
        VkRenderPass light_pass, DeferredPass& pass, std::string& error);
};
}
