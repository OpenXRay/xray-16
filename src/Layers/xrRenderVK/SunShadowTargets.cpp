#include "SunShadowTargets.h"

namespace xray::render::vulkan
{
bool SunShadowTargets::initialize(VkDevice device, VkFormat format, uint32_t image_count,
    const VkPhysicalDeviceMemoryProperties& memory, const FrameDispatch& dispatch,
    const BufferResourceDispatch& buffers, PFN_vkCreateSampler create_sampler,
    PFN_vkDestroySampler destroy_sampler, std::string& error)
{
    destroy();
    if (!device || format == VK_FORMAT_UNDEFINED || !image_count || image_count > 16 ||
        !dispatch.create_image || !dispatch.get_image_memory_requirements ||
        !dispatch.allocate_memory || !dispatch.bind_image_memory || !dispatch.create_image_view ||
        !dispatch.create_framebuffer || !dispatch.create_render_pass ||
        !dispatch.cmd_begin_render_pass || !dispatch.cmd_end_render_pass ||
        !create_sampler || !destroy_sampler)
    { error = "shadow targets need complete Vulkan image procedures"; return false; }
    device_ = device;
    vk_ = dispatch;
    destroy_sampler_ = destroy_sampler;
    VkAttachmentDescription depth{};
    depth.format = format;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    const VkAttachmentReference reference{0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pDepthStencilAttachment = &reference;
    const VkSubpassDependency dependencies[]{
        {VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT},
        {0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT}
    };
    VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    pass.attachmentCount = 1;
    pass.pAttachments = &depth;
    pass.subpassCount = 1;
    pass.pSubpasses = &subpass;
    pass.dependencyCount = 2;
    pass.pDependencies = dependencies;
    if (vk_.create_render_pass(device, &pass, nullptr, &pass_) != VK_SUCCESS)
    { error = "cannot create shadow render pass"; destroy(); return false; }
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler.magFilter = sampler.minFilter = VK_FILTER_NEAREST;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.maxLod = 0;
    if (create_sampler(device, &sampler, nullptr, &sampler_) != VK_SUCCESS)
    { error = "cannot create shadow sampler"; destroy(); return false; }
    targets_.resize(image_count);
    for (Target& target : targets_)
    {
        VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = format;
        image.extent = {Size, Size, 1};
        image.mipLevels = image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vk_.create_image(device, &image, nullptr, &target.image) != VK_SUCCESS)
        { error = "cannot create shadow depth image"; destroy(); return false; }
        VkMemoryRequirements requirements{};
        vk_.get_image_memory_requirements(device, target.image, &requirements);
        uint32_t type = UINT32_MAX;
        for (uint32_t i = 0; i < memory.memoryTypeCount; ++i)
            if ((requirements.memoryTypeBits & (1u << i)) &&
                (memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
            { type = i; break; }
        if (type == UINT32_MAX)
        { error = "no device-local memory for shadow image"; destroy(); return false; }
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = type;
        if (vk_.allocate_memory(device, &allocation, nullptr, &target.memory) != VK_SUCCESS ||
            vk_.bind_image_memory(device, target.image, target.memory, 0) != VK_SUCCESS)
        { error = "cannot bind shadow depth image"; destroy(); return false; }
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = target.image;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = format;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        view.subresourceRange.levelCount = view.subresourceRange.layerCount = 1;
        if (vk_.create_image_view(device, &view, nullptr, &target.view) != VK_SUCCESS)
        { error = "cannot create shadow image view"; destroy(); return false; }
        VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer.renderPass = pass_;
        framebuffer.attachmentCount = 1;
        framebuffer.pAttachments = &target.view;
        framebuffer.width = framebuffer.height = Size;
        framebuffer.layers = 1;
        if (vk_.create_framebuffer(device, &framebuffer, nullptr, &target.framebuffer) != VK_SUCCESS ||
            !target.constants.initialize(device, sizeof(SunShadowUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                memory, buffers, error))
        { if (error.empty()) error = "cannot create shadow framebuffer"; destroy(); return false; }
    }
    error.clear();
    return true;
}

bool SunShadowTargets::begin(const FrameRecordingContext& frame, const SunShadowUniform& constants,
    FrameRecordingContext& shadow_frame, std::string& error)
{
    if (!pass_ || !frame.command_buffer || frame.image_index >= targets_.size() ||
        !targets_[frame.image_index].constants.write(0, &constants, sizeof(constants), error)) return false;
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    begin.renderPass = pass_;
    begin.framebuffer = targets_[frame.image_index].framebuffer;
    begin.renderArea.extent = {Size, Size};
    begin.clearValueCount = 1;
    VkClearValue depth_clear{};
    depth_clear.depthStencil = {1.f, 0};
    begin.pClearValues = &depth_clear;
    vk_.cmd_begin_render_pass(frame.command_buffer, &begin, VK_SUBPASS_CONTENTS_INLINE);
    shadow_frame = {frame.command_buffer, pass_, begin.framebuffer, {Size, Size}, frame.image_index, frame.frame_index};
    error.clear();
    return true;
}

void SunShadowTargets::end(VkCommandBuffer command) const
{
    if (command && pass_) vk_.cmd_end_render_pass(command);
}

void SunShadowTargets::destroy()
{
    if (device_)
    {
        for (Target& target : targets_)
        {
            target.constants.destroy();
            if (target.framebuffer) vk_.destroy_framebuffer(device_, target.framebuffer, nullptr);
            if (target.view) vk_.destroy_image_view(device_, target.view, nullptr);
            if (target.image) vk_.destroy_image(device_, target.image, nullptr);
            if (target.memory) vk_.free_memory(device_, target.memory, nullptr);
        }
        if (sampler_) destroy_sampler_(device_, sampler_, nullptr);
        if (pass_) vk_.destroy_render_pass(device_, pass_, nullptr);
    }
    targets_.clear();
    device_ = VK_NULL_HANDLE;
    pass_ = VK_NULL_HANDLE;
    sampler_ = VK_NULL_HANDLE;
    vk_ = {};
    destroy_sampler_ = nullptr;
}
}
