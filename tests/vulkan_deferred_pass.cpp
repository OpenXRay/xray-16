#include "src/Layers/xrRenderVK/DeferredPass.h"

#include <cassert>

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
