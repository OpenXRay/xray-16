#include "WaterTargets.h"

namespace xray::render::vulkan
{
bool WaterTargets::make_image(Image& target, std::string& error)
{
    VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = format_;
    image.extent = {extent_.width, extent_.height, 1};
    image.mipLevels = image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vk_.create_image(device_, &image, nullptr, &target.image) != VK_SUCCESS)
    { error = "cannot create water color target"; return false; }
    VkMemoryRequirements requirements{};
    vk_.get_image_memory_requirements(device_, target.image, &requirements);
    uint32_t type = UINT32_MAX;
    for (uint32_t i = 0; i < memory_.memoryTypeCount; ++i)
        if ((requirements.memoryTypeBits & (1u << i)) &&
            (memory_.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        { type = i; break; }
    if (type == UINT32_MAX)
    { error = "no device-local memory for water color target"; return false; }
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = type;
    if (vk_.allocate_memory(device_, &allocation, nullptr, &target.memory) != VK_SUCCESS ||
        vk_.bind_image_memory(device_, target.image, target.memory, 0) != VK_SUCCESS)
    { error = "cannot allocate water color target"; return false; }
    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.image = target.image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = format_;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.levelCount = view.subresourceRange.layerCount = 1;
    if (vk_.create_image_view(device_, &view, nullptr, &target.view) != VK_SUCCESS)
    { error = "cannot create water color view"; return false; }
    return true;
}

bool WaterTargets::initialize(VkPhysicalDevice physical, VkDevice device, VkFormat format, VkExtent2D extent,
    uint32_t count, const VkPhysicalDeviceMemoryProperties& memory,
    const FrameDispatch& dispatch, const BufferResourceDispatch& buffers,
    std::string& error)
{
    destroy();
    if (!physical || !device || format == VK_FORMAT_UNDEFINED || !extent.width || !extent.height ||
        !count || count > 16 || !dispatch.create_image || !dispatch.destroy_image ||
        !dispatch.get_format_properties ||
        !dispatch.get_image_memory_requirements || !dispatch.allocate_memory ||
        !dispatch.free_memory || !dispatch.bind_image_memory ||
        !dispatch.create_image_view || !dispatch.destroy_image_view ||
        !dispatch.cmd_copy_image || !dispatch.cmd_pipeline_barrier)
    { error = "water targets require complete Vulkan image and copy procedures"; return false; }
    VkFormatProperties properties{};
    dispatch.get_format_properties(physical, format, &properties);
    if (!(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
    { error = "swapchain color format cannot be sampled by water targets"; return false; }
    device_ = device;
    format_ = format;
    extent_ = extent;
    memory_ = memory;
    vk_ = dispatch;
    targets_.resize(count);
    for (auto& target : targets_)
        if (!make_image(target.refraction, error) || !make_image(target.reflection, error) ||
            !target.constants.initialize(device, sizeof(WaterSceneUniform),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                memory, buffers, error))
        { destroy(); return false; }
    error.clear();
    return true;
}

bool WaterTargets::write_uniform(uint32_t index, const WaterSceneUniform& value,
    std::string& error)
{
    return index < targets_.size() &&
        targets_[index].constants.write(0, &value, sizeof(value), error);
}

bool WaterTargets::capture(VkCommandBuffer command, VkImage source, uint32_t index) const
{
    if (!command || !source || index >= targets_.size()) return false;
    const Target& target = targets_[index];
    VkImageMemoryBarrier transitions[2]{};
    const VkImage images[]{target.refraction.image, target.reflection.image};
    for (uint32_t i = 0; i < 2; ++i)
    {
        transitions[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        transitions[i].image = images[i];
        transitions[i].srcQueueFamilyIndex = transitions[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transitions[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        transitions[i].subresourceRange.levelCount = transitions[i].subresourceRange.layerCount = 1;
        transitions[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        transitions[i].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        transitions[i].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    vk_.cmd_pipeline_barrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 2, transitions);
    VkImageCopy region{};
    region.srcSubresource.aspectMask = region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.layerCount = region.dstSubresource.layerCount = 1;
    region.extent = {extent_.width, extent_.height, 1};
    for (VkImage image : images)
        vk_.cmd_copy_image(command, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    for (auto& transition : transitions)
    {
        transition.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        transition.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        transition.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        transition.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    vk_.cmd_pipeline_barrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, transitions);
    return true;
}

VkImageView WaterTargets::refraction(uint32_t index) const
{ return index < targets_.size() ? targets_[index].refraction.view : VK_NULL_HANDLE; }
VkImageView WaterTargets::reflection(uint32_t index) const
{ return index < targets_.size() ? targets_[index].reflection.view : VK_NULL_HANDLE; }
VkBuffer WaterTargets::uniform(uint32_t index) const
{ return index < targets_.size() ? targets_[index].constants.handle() : VK_NULL_HANDLE; }

void WaterTargets::destroy()
{
    if (device_)
        for (auto& target : targets_)
        {
            target.constants.destroy();
            for (Image* image : {&target.refraction, &target.reflection})
            {
                if (image->view) vk_.destroy_image_view(device_, image->view, nullptr);
                if (image->image) vk_.destroy_image(device_, image->image, nullptr);
                if (image->memory) vk_.free_memory(device_, image->memory, nullptr);
            }
        }
    targets_.clear();
    device_ = VK_NULL_HANDLE;
    format_ = VK_FORMAT_UNDEFINED;
    extent_ = {};
    vk_ = {};
}
}
