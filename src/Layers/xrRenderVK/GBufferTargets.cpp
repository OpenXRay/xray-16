#include "GBufferTargets.h"
#include "SunShadowTargets.h"

namespace xray::render::vulkan
{
bool GBufferTargets::create_attachment(VkFormat format, VkImageUsageFlags usage,
    VkImageAspectFlags aspect, Attachment& target, std::string& error)
{
    VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = format;
    image.extent = {extent_.width, extent_.height, 1};
    image.mipLevels = image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = usage;
    image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vk_.create_image(device_, &image, nullptr, &target.image) != VK_SUCCESS)
    {
        error = "could not create Vulkan G-buffer image";
        return false;
    }
    VkMemoryRequirements requirements{};
    vk_.get_image_memory_requirements(device_, target.image, &requirements);
    uint32_t memory_type = UINT32_MAX;
    for (uint32_t i = 0; i < memory_.memoryTypeCount; ++i)
        if ((requirements.memoryTypeBits & (1u << i)) &&
            (memory_.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        {
            memory_type = i;
            break;
        }
    if (memory_type == UINT32_MAX)
    {
        error = "no device-local memory for G-buffer image";
        return false;
    }
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memory_type;
    if (vk_.allocate_memory(device_, &allocation, nullptr, &target.memory) != VK_SUCCESS ||
        vk_.bind_image_memory(device_, target.image, target.memory, 0) != VK_SUCCESS)
    {
        error = "could not allocate Vulkan G-buffer image memory";
        return false;
    }
    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.image = target.image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = format;
    view.subresourceRange.aspectMask = aspect;
    view.subresourceRange.levelCount = view.subresourceRange.layerCount = 1;
    if (vk_.create_image_view(device_, &view, nullptr, &target.view) != VK_SUCCESS)
    {
        error = "could not create Vulkan G-buffer image view";
        return false;
    }
    return true;
}

bool GBufferTargets::initialize(VkPhysicalDevice physical_device, VkDevice device, VkExtent2D extent, uint32_t image_count,
    VkFormat depth_format, const VkPhysicalDeviceMemoryProperties& memory,
    const FrameDispatch& dispatch, PFN_vkCreateSampler create_sampler,
    PFN_vkDestroySampler destroy_sampler, std::string& error)
{
    destroy();
    if (!physical_device || !device || !extent.width || !extent.height || !image_count ||
        image_count > 16 || depth_format == VK_FORMAT_UNDEFINED ||
        !dispatch.get_format_properties ||
        !dispatch.create_image || !dispatch.destroy_image || !dispatch.allocate_memory ||
        !dispatch.free_memory || !dispatch.bind_image_memory || !dispatch.get_image_memory_requirements ||
        !dispatch.create_image_view || !dispatch.destroy_image_view ||
        !dispatch.create_framebuffer || !dispatch.destroy_framebuffer ||
        !dispatch.create_render_pass || !dispatch.destroy_render_pass ||
        !dispatch.cmd_begin_render_pass || !dispatch.cmd_end_render_pass ||
        !dispatch.cmd_pipeline_barrier || !dispatch.cmd_copy_image ||
        !create_sampler || !destroy_sampler)
    {
        error = "G-buffer targets need a device, formats and complete image procedures";
        return false;
    }
    const VkFormat color = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormatProperties color_support{};
    dispatch.get_format_properties(physical_device, color, &color_support);
    if ((color_support.optimalTilingFeatures &
            (VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) !=
        (VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
    {
        error = "RGBA8 cannot be used as a sampled G-buffer attachment";
        return false;
    }
    VkFormatProperties depth_support{};
    dispatch.get_format_properties(physical_device, depth_format, &depth_support);
    if ((depth_support.optimalTilingFeatures &
            (VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) !=
        (VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
    {
        error = "selected depth format cannot be stored and sampled as a G-buffer attachment";
        return false;
    }
    device_ = device;
    extent_ = extent;
    memory_ = memory;
    vk_ = dispatch;
    destroy_sampler_ = destroy_sampler;
    if (!create_gbuffer_render_pass(device, color, color, depth_format, vk_, pass_, error))
    {
        destroy();
        return false;
    }
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler.magFilter = sampler.minFilter = VK_FILTER_NEAREST;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.maxLod = 0;
    if (create_sampler(device, &sampler, nullptr, &sampler_) != VK_SUCCESS)
    {
        error = "could not create G-buffer sampler";
        destroy();
        return false;
    }
    targets_.resize(image_count);
    for (auto& target : targets_)
    {
        if (!create_attachment(color, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT, target.albedo, error) ||
            !create_attachment(color, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT, target.normal, error) ||
            !create_attachment(depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT, target.depth, error) ||
            !create_attachment(depth_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT, target.sampled_depth, error))
        {
            destroy();
            return false;
        }
        const VkImageView views[]{target.albedo.view, target.normal.view, target.depth.view};
        VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer.renderPass = pass_;
        framebuffer.attachmentCount = 3;
        framebuffer.pAttachments = views;
        framebuffer.width = extent.width;
        framebuffer.height = extent.height;
        framebuffer.layers = 1;
        if (vk_.create_framebuffer(device, &framebuffer, nullptr, &target.framebuffer) != VK_SUCCESS)
        {
            if (error.empty()) error = "could not create G-buffer framebuffer";
            destroy();
            return false;
        }
    }
    error.clear();
    return true;
}

bool GBufferTargets::bind_lighting(DeferredPass& deferred, std::string& error)
{
    if (!device_ || !sampler_ || targets_.empty())
    {
        error = "G-buffer targets are not initialized";
        return false;
    }
    for (auto& target : targets_)
        if (!target.lighting && !deferred.gbuffer(target.albedo.view,
                target.normal.view, target.sampled_depth.view, sampler_, target.lighting, error)) return false;
    error.clear();
    return true;
}

void GBufferTargets::bind_sun_shadow(DeferredPass& deferred, const SunShadowTargets& shadow)
{
    for (uint32_t i = 0; i < targets_.size(); ++i)
        deferred.bind_sun_shadow(targets_[i].lighting, shadow.view(i), shadow.sampler(), shadow.uniform(i));
}

void GBufferTargets::release_lighting(DeferredPass& deferred)
{
    for (auto& target : targets_)
        deferred.release_gbuffer(target.lighting);
}

bool GBufferTargets::begin(const FrameRecordingContext& frame, FrameRecordingContext& geometry_frame) const
{
    if (!pass_ || !frame.command_buffer || frame.image_index >= targets_.size() ||
        frame.extent.width != extent_.width || frame.extent.height != extent_.height) return false;
    VkClearValue clear[3]{};
    clear[0].color = {{0, 0, 0, 0}};
    clear[1].color = {{0.5f, 0.5f, 1.0f, 0}};
    clear[2].depthStencil = {1.0f, 0};
    VkRenderPassBeginInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    info.renderPass = pass_;
    info.framebuffer = targets_[frame.image_index].framebuffer;
    info.renderArea.extent = extent_;
    info.clearValueCount = 3;
    info.pClearValues = clear;
    vk_.cmd_begin_render_pass(frame.command_buffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    geometry_frame = {frame.command_buffer, pass_, info.framebuffer,
        extent_, frame.image_index, frame.frame_index};
    return true;
}

void GBufferTargets::end(VkCommandBuffer command) const
{
    if (command && pass_) vk_.cmd_end_render_pass(command);
}

bool GBufferTargets::copy_depth(const FrameRecordingContext& frame) const
{
    if (!frame.command_buffer || frame.image_index >= targets_.size()) return false;
    const Target& target = targets_[frame.image_index];
    VkImageMemoryBarrier barriers[2]{};
    for (VkImageMemoryBarrier& barrier : barriers)
    {
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.levelCount = barrier.subresourceRange.layerCount = 1;
    }
    barriers[0].image = target.depth.image;
    barriers[0].oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[1].image = target.sampled_depth.image;
    barriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Discard the previous frame's copy.
    barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vk_.cmd_pipeline_barrier(frame.command_buffer,
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 2, barriers);
    VkImageCopy region{};
    region.srcSubresource.aspectMask = region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    region.srcSubresource.layerCount = region.dstSubresource.layerCount = 1;
    region.extent = {extent_.width, extent_.height, 1};
    vk_.cmd_copy_image(frame.command_buffer, target.depth.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        target.sampled_depth.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    barriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vk_.cmd_pipeline_barrier(frame.command_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 2, barriers);
    return true;
}

VkDescriptorSet GBufferTargets::lighting_set(uint32_t index) const
{
    return index < targets_.size() ? targets_[index].lighting : VK_NULL_HANDLE;
}

VkImageView GBufferTargets::depth_view(uint32_t index) const
{
    return index < targets_.size() ? targets_[index].depth.view : VK_NULL_HANDLE;
}

VkImageView GBufferTargets::sampled_depth_view(uint32_t index) const
{
    return index < targets_.size() ? targets_[index].sampled_depth.view : VK_NULL_HANDLE;
}

void GBufferTargets::destroy()
{
    if (device_)
    {
        for (auto& target : targets_)
        {
            if (target.framebuffer) vk_.destroy_framebuffer(device_, target.framebuffer, nullptr);
            for (Attachment* attachment : {&target.albedo, &target.normal, &target.depth, &target.sampled_depth})
            {
                if (attachment->view) vk_.destroy_image_view(device_, attachment->view, nullptr);
                if (attachment->image) vk_.destroy_image(device_, attachment->image, nullptr);
                if (attachment->memory) vk_.free_memory(device_, attachment->memory, nullptr);
            }
        }
        if (sampler_) destroy_sampler_(device_, sampler_, nullptr);
        if (pass_) vk_.destroy_render_pass(device_, pass_, nullptr);
    }
    targets_.clear();
    device_ = VK_NULL_HANDLE;
    pass_ = VK_NULL_HANDLE;
    sampler_ = VK_NULL_HANDLE;
    destroy_sampler_ = nullptr;
    vk_ = {};
}
}
