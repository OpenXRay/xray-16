#include "src/Layers/xrRenderVK/DeferredPass.h"

#include <cassert>
#include <cmath>
#include <limits>

using namespace xray::render::vulkan;

static VkResult VKAPI_PTR create_render_pass(VkDevice, const VkRenderPassCreateInfo* info,
    const VkAllocationCallbacks*, VkRenderPass* result)
{
    assert(info->attachmentCount == 3 && info->subpassCount == 1);
    assert(info->pSubpasses[0].colorAttachmentCount == 2);
    assert(info->pSubpasses[0].pDepthStencilAttachment->attachment == 2);
    assert(info->pAttachments[0].finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    assert(info->pAttachments[1].finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    assert(info->pAttachments[0].storeOp == VK_ATTACHMENT_STORE_OP_STORE);
    assert(info->pAttachments[1].storeOp == VK_ATTACHMENT_STORE_OP_STORE);
    assert(info->pAttachments[2].storeOp == VK_ATTACHMENT_STORE_OP_STORE);
    assert(info->pAttachments[2].finalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
    assert(info->dependencyCount == 2);
    assert(info->pDependencies[1].dstAccessMask & VK_ACCESS_SHADER_READ_BIT);
    assert(info->pDependencies[1].srcAccessMask & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
    assert(info->pDependencies[1].srcStageMask & VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    *result = reinterpret_cast<VkRenderPass>(uintptr_t(3));
    return VK_SUCCESS;
}

int main()
{
    DeferredEnvironment environment;
    environment.sun_direction[0] = 0.f;
    environment.sun_direction[1] = -4.f;
    environment.sun_direction[2] = 0.f;
    environment.sun_color[0] = 0.3f;
    environment.sun_color[1] = 0.6f;
    environment.sun_color[2] = 0.9f;
    environment.ambient_color[0] = 0.2f;
    environment.ambient_color[1] = 0.2f;
    environment.ambient_color[2] = 0.2f;
    environment.hemi_color[0] = 0.4f;
    environment.hemi_color[1] = 0.4f;
    environment.hemi_color[2] = 0.4f;
    const DeferredLight environment_light = make_environment_deferred_light(environment);
    assert(environment_light.direction_ambient[0] == 0.f);
    assert(environment_light.direction_ambient[1] == -1.f);
    assert(environment_light.direction_ambient[2] == 0.f);
    assert(environment_light.color[0] == 0.3f && environment_light.color[1] == 0.6f &&
        environment_light.color[2] == 0.9f);
    assert(environment_light.direction_ambient[3] > 0.29f && environment_light.direction_ambient[3] < 0.31f);
    environment.sun_direction[0] = environment.sun_direction[1] = environment.sun_direction[2] = 0.f;
    environment.sun_color[0] = -1.f;
    environment.ambient_color[0] = std::numeric_limits<float>::quiet_NaN();
    const DeferredLight safe_light = make_environment_deferred_light(environment);
    assert(safe_light.direction_ambient[1] == -1.f && safe_light.color[0] == 0.f);
    assert(std::isfinite(safe_light.direction_ambient[3]));

    VkRenderPass result{};
    std::string error;
    FrameDispatch dispatch{};
    assert(!create_gbuffer_render_pass(VK_NULL_HANDLE, VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_D24_UNORM_S8_UINT, dispatch, result, error));
    dispatch.create_render_pass = create_render_pass;
    assert(create_gbuffer_render_pass(reinterpret_cast<VkDevice>(uintptr_t(2)),
        VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_D24_UNORM_S8_UINT, dispatch, result, error));
    assert(result && error.empty());
}
