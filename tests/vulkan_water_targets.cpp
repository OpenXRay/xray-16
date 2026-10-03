#include "src/Layers/xrRenderVK/WaterTargets.h"
#include "vulkan_mock_device.h"

#include <cassert>

using namespace xray::render::vulkan;
namespace
{
uint32_t copies{}, barriers{};
void VKAPI_PTR buffer_requirements(VkDevice device, VkBuffer buffer,
    VkMemoryRequirements* out)
{
    vk_mock::requirements(device, buffer, out);
    out->memoryTypeBits = 2;
}
void VKAPI_PTR formats(VkPhysicalDevice, VkFormat, VkFormatProperties* out)
{ out->optimalTilingFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT; }
VkResult VKAPI_PTR image(VkDevice device, const VkImageCreateInfo* info,
    const VkAllocationCallbacks* callbacks, VkImage* result)
{
    assert(info->format == VK_FORMAT_B8G8R8A8_UNORM &&
        info->usage == (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
    return vk_mock::create_image(device, info, callbacks, result);
}
void VKAPI_PTR copy(VkCommandBuffer, VkImage source, VkImageLayout source_layout,
    VkImage target, VkImageLayout target_layout, uint32_t count, const VkImageCopy* region)
{
    assert(source && vk_mock::images.count(target) && count == 1);
    assert(source_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
        target_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        region->extent.width == 800 && region->extent.height == 600);
    ++copies;
}
void VKAPI_PTR barrier(VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags,
    VkDependencyFlags, uint32_t, const VkMemoryBarrier*, uint32_t,
    const VkBufferMemoryBarrier*, uint32_t count, const VkImageMemoryBarrier* images)
{
    assert(count == 2 && images[0].image && images[1].image);
    if (barriers == 0)
        assert(images[0].oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            images[0].newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    else
        assert(images[0].oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            images[0].newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    ++barriers;
}
}
int main()
{
    FrameDispatch dispatch{};
    dispatch.create_image = image;
    dispatch.get_format_properties = formats;
    dispatch.destroy_image = vk_mock::destroy_image;
    dispatch.get_image_memory_requirements = vk_mock::image_requirements;
    dispatch.allocate_memory = vk_mock::allocate;
    dispatch.free_memory = vk_mock::free_memory;
    dispatch.bind_image_memory = vk_mock::bind_image;
    dispatch.create_image_view = vk_mock::image_view;
    dispatch.destroy_image_view = vk_mock::destroy_view;
    dispatch.cmd_copy_image = copy;
    dispatch.cmd_pipeline_barrier = barrier;
    VkPhysicalDeviceMemoryProperties memory{};
    memory.memoryTypeCount = 2;
    memory.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    memory.memoryTypes[1].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto buffers = vk_mock::buffer_dispatch();
    buffers.get_buffer_memory_requirements = buffer_requirements;
    WaterTargets targets;
    std::string error;
    assert(targets.initialize(vk_mock::handle<VkPhysicalDevice>(4),
        vk_mock::handle<VkDevice>(1), VK_FORMAT_B8G8R8A8_UNORM,
        {800, 600}, 2, memory, dispatch, buffers, error));
    assert(vk_mock::images.size() == 4 && targets.refraction(0) && targets.reflection(1));
    WaterSceneUniform scene{};
    scene.camera_position[3] = 1.f;
    assert(targets.uniform(1) && targets.write_uniform(1, scene, error));
    assert(targets.capture(vk_mock::handle<VkCommandBuffer>(2),
        vk_mock::handle<VkImage>(3), 1));
    assert(copies == 2 && barriers == 2);
    assert(!targets.capture(vk_mock::handle<VkCommandBuffer>(2),
        vk_mock::handle<VkImage>(3), 2));
    targets.destroy();
    assert(vk_mock::images.empty() && vk_mock::buffers.empty() && vk_mock::allocations.empty());
}
