#include "src/Layers/xrRenderVK/LocalShadowTargets.h"
#include "vulkan_mock_device.h"

#include <cassert>

using namespace xray::render::vulkan;
namespace
{
uint32_t array_views{}, layer_views{}, framebuffers{}, passes{}, begins{}, ends{};
VkResult VKAPI_PTR image(VkDevice d, const VkImageCreateInfo* info,
    const VkAllocationCallbacks* a, VkImage* out)
{
    assert(info->arrayLayers == LocalShadowSlots * LocalShadowFaces);
    assert(info->extent.width == LocalShadowSize && info->extent.height == LocalShadowSize);
    assert((info->usage & VK_IMAGE_USAGE_SAMPLED_BIT) != 0);
    return vk_mock::create_image(d, info, a, out);
}
VkResult VKAPI_PTR view(VkDevice d, const VkImageViewCreateInfo* info,
    const VkAllocationCallbacks* a, VkImageView* out)
{
    if (info->viewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY)
    {
        assert(info->subresourceRange.layerCount == LocalShadowSlots * LocalShadowFaces);
        ++array_views;
    }
    else
    {
        assert(info->viewType == VK_IMAGE_VIEW_TYPE_2D &&
            info->subresourceRange.layerCount == 1);
        ++layer_views;
    }
    return vk_mock::image_view(d, info, a, out);
}
void VKAPI_PTR destroy_view(VkDevice d, VkImageView v, const VkAllocationCallbacks* a)
{
    vk_mock::destroy_view(d, v, a);
}
VkResult VKAPI_PTR framebuffer(VkDevice, const VkFramebufferCreateInfo* info,
    const VkAllocationCallbacks*, VkFramebuffer* out)
{
    assert(info->attachmentCount == 1 && info->width == LocalShadowSize);
    *out = vk_mock::handle<VkFramebuffer>(vk_mock::next_handle++);
    ++framebuffers;
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_framebuffer(VkDevice, VkFramebuffer, const VkAllocationCallbacks*)
{ --framebuffers; }
VkResult VKAPI_PTR render_pass(VkDevice, const VkRenderPassCreateInfo* info,
    const VkAllocationCallbacks*, VkRenderPass* out)
{
    assert(info->attachmentCount == 1 && info->pAttachments[0].storeOp == VK_ATTACHMENT_STORE_OP_STORE &&
        info->pAttachments[0].finalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
    *out = vk_mock::handle<VkRenderPass>(vk_mock::next_handle++);
    ++passes;
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_pass(VkDevice, VkRenderPass, const VkAllocationCallbacks*) { --passes; }
void VKAPI_PTR begin(VkCommandBuffer, const VkRenderPassBeginInfo* info, VkSubpassContents)
{ assert(info->clearValueCount == 1 && info->renderArea.extent.width == LocalShadowSize); ++begins; }
void VKAPI_PTR end(VkCommandBuffer) { ++ends; }
void VKAPI_PTR host_buffer_requirements(VkDevice device, VkBuffer buffer,
    VkMemoryRequirements* out)
{
    vk_mock::requirements(device, buffer, out);
    out->memoryTypeBits = 2;
}
}

int main()
{
    FrameDispatch dispatch{};
    dispatch.create_image = image;
    dispatch.destroy_image = vk_mock::destroy_image;
    dispatch.get_image_memory_requirements = vk_mock::image_requirements;
    dispatch.allocate_memory = vk_mock::allocate;
    dispatch.free_memory = vk_mock::free_memory;
    dispatch.bind_image_memory = vk_mock::bind_image;
    dispatch.create_image_view = view;
    dispatch.destroy_image_view = destroy_view;
    dispatch.create_framebuffer = framebuffer;
    dispatch.destroy_framebuffer = destroy_framebuffer;
    dispatch.create_render_pass = render_pass;
    dispatch.destroy_render_pass = destroy_pass;
    dispatch.cmd_begin_render_pass = begin;
    dispatch.cmd_end_render_pass = end;
    VkPhysicalDeviceMemoryProperties memory{};
    memory.memoryTypeCount = 2;
    memory.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    memory.memoryTypes[1].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    // The mock buffer's memoryTypeBits includes both types while the image requires device-local.
    auto buffers = vk_mock::buffer_dispatch();
    buffers.get_buffer_memory_requirements = host_buffer_requirements;
    LocalShadowTargets targets;
    std::string error;
    const VkDevice device = vk_mock::handle<VkDevice>(1);
    assert(targets.initialize(device, VK_FORMAT_D24_UNORM_S8_UINT, 2, memory, dispatch,
        buffers, vk_mock::sampler, vk_mock::destroy_sampler, error, 1024));
    assert(targets.uniform_stride() == 1024);
    assert(array_views == 2 && layer_views == 48 && framebuffers == 48 && passes == 1);
    FrameRecordingContext frame{vk_mock::handle<VkCommandBuffer>(2), VK_NULL_HANDLE,
        VK_NULL_HANDLE, {640,480}, 1, 0};
    assert(targets.initialize_layers(frame));
    assert(begins == 24 && ends == 24);
    assert(targets.initialize_layers(frame) && begins == 24);
    LocalLightUniform light{};
    light.position_range[3] = 25.f;
    assert(targets.write_light(1, 0, light, error));
    assert(!targets.write_light(1, LocalLightCapacity, light, error));
    FrameRecordingContext shadow;
    assert(targets.begin_face(frame, 3, 5, shadow));
    assert(shadow.render_pass == targets.render_pass() && shadow.extent.width == LocalShadowSize);
    targets.end(frame.command_buffer);
    assert(!targets.begin_face(frame, 4, 0, shadow));
    targets.destroy();
    assert(framebuffers == 0 && passes == 0 && vk_mock::images.empty() &&
        vk_mock::buffers.empty() && vk_mock::allocations.empty());
}
