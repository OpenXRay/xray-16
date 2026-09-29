#include "src/Layers/xrRenderVK/GBufferTargets.h"

#include <cassert>

using namespace xray::render::vulkan;

namespace
{
uint32_t images, image_views, framebuffers, passes, samplers, begun, ended;
VkResult VKAPI_PTR create_image(VkDevice, const VkImageCreateInfo* info,
    const VkAllocationCallbacks*, VkImage* output)
{
    assert(info->usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT));
    *output = reinterpret_cast<VkImage>(uintptr_t(++images));
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_image(VkDevice, VkImage, const VkAllocationCallbacks*) { --images; }
void VKAPI_PTR requirements(VkDevice, VkImage, VkMemoryRequirements* output)
{
    *output = {4096, 256, 1};
}
VkResult VKAPI_PTR allocate(VkDevice, const VkMemoryAllocateInfo* info,
    const VkAllocationCallbacks*, VkDeviceMemory* output)
{
    assert(info->memoryTypeIndex == 0);
    *output = reinterpret_cast<VkDeviceMemory>(uintptr_t(info->allocationSize));
    return VK_SUCCESS;
}
void VKAPI_PTR free_memory(VkDevice, VkDeviceMemory, const VkAllocationCallbacks*) {}
VkResult VKAPI_PTR bind(VkDevice, VkImage, VkDeviceMemory, VkDeviceSize) { return VK_SUCCESS; }
VkResult VKAPI_PTR create_view(VkDevice, const VkImageViewCreateInfo*,
    const VkAllocationCallbacks*, VkImageView* output)
{
    *output = reinterpret_cast<VkImageView>(uintptr_t(++image_views));
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_view(VkDevice, VkImageView, const VkAllocationCallbacks*) { --image_views; }
void VKAPI_PTR formats(VkPhysicalDevice, VkFormat, VkFormatProperties* output)
{
    output->optimalTilingFeatures = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
}
VkResult VKAPI_PTR create_framebuffer(VkDevice, const VkFramebufferCreateInfo* info,
    const VkAllocationCallbacks*, VkFramebuffer* output)
{
    assert(info->attachmentCount == 3 && info->width == 640);
    *output = reinterpret_cast<VkFramebuffer>(uintptr_t(++framebuffers));
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_framebuffer(VkDevice, VkFramebuffer, const VkAllocationCallbacks*) { --framebuffers; }
VkResult VKAPI_PTR create_pass(VkDevice, const VkRenderPassCreateInfo* info,
    const VkAllocationCallbacks*, VkRenderPass* output)
{
    assert(info->attachmentCount == 3);
    *output = reinterpret_cast<VkRenderPass>(uintptr_t(++passes));
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_pass(VkDevice, VkRenderPass, const VkAllocationCallbacks*) { --passes; }
VkResult VKAPI_PTR create_sampler(VkDevice, const VkSamplerCreateInfo* info,
    const VkAllocationCallbacks*, VkSampler* output)
{
    assert(info->minFilter == VK_FILTER_NEAREST);
    *output = reinterpret_cast<VkSampler>(uintptr_t(++samplers));
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_sampler(VkDevice, VkSampler, const VkAllocationCallbacks*) { --samplers; }
void VKAPI_PTR begin(VkCommandBuffer, const VkRenderPassBeginInfo* info, VkSubpassContents)
{
    assert(info->clearValueCount == 3 && info->framebuffer);
    ++begun;
}
void VKAPI_PTR end(VkCommandBuffer) { ++ended; }
}

int main()
{
    FrameDispatch dispatch{};
    dispatch.create_image = create_image;
    dispatch.destroy_image = destroy_image;
    dispatch.get_image_memory_requirements = requirements;
    dispatch.allocate_memory = allocate;
    dispatch.free_memory = free_memory;
    dispatch.bind_image_memory = bind;
    dispatch.create_image_view = create_view;
    dispatch.destroy_image_view = destroy_view;
    dispatch.get_format_properties = formats;
    dispatch.create_framebuffer = create_framebuffer;
    dispatch.destroy_framebuffer = destroy_framebuffer;
    dispatch.create_render_pass = create_pass;
    dispatch.destroy_render_pass = destroy_pass;
    dispatch.cmd_begin_render_pass = begin;
    dispatch.cmd_end_render_pass = end;
    VkPhysicalDeviceMemoryProperties memory{};
    memory.memoryTypeCount = 1;
    memory.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    GBufferTargets targets;
    std::string error;
    assert(targets.initialize(reinterpret_cast<VkPhysicalDevice>(uintptr_t(1)),
        reinterpret_cast<VkDevice>(uintptr_t(2)), {640, 480}, 2, VK_FORMAT_D24_UNORM_S8_UINT,
        memory, dispatch, create_sampler, destroy_sampler, error));
    assert(images == 6 && image_views == 6 && framebuffers == 2 && passes == 1 && samplers == 1);
    FrameRecordingContext frame{reinterpret_cast<VkCommandBuffer>(uintptr_t(3)),
        VK_NULL_HANDLE, VK_NULL_HANDLE, {640, 480}, 1, 0};
    FrameRecordingContext geometry;
    assert(targets.begin(frame, geometry) && geometry.render_pass == targets.render_pass());
    targets.end(frame.command_buffer);
    assert(begun == 1 && ended == 1);
    assert(!targets.begin({frame.command_buffer, VK_NULL_HANDLE, VK_NULL_HANDLE, {640, 480}, 2, 0}, geometry));
    targets.destroy();
    assert(!images && !image_views && !framebuffers && !passes && !samplers);
}
