#include "LocalShadowTargets.h"

namespace xray::render::vulkan
{
bool LocalShadowTargets::initialize(VkDevice device, VkFormat format, uint32_t count,
    const VkPhysicalDeviceMemoryProperties& memory, const FrameDispatch& dispatch,
    const BufferResourceDispatch& buffers, PFN_vkCreateSampler create_sampler,
    PFN_vkDestroySampler destroy_sampler, std::string& error,
    VkDeviceSize uniform_alignment)
{
    destroy();
    if (!device || format == VK_FORMAT_UNDEFINED || !count || count > 16 ||
        !dispatch.create_image || !dispatch.destroy_image ||
        !dispatch.get_image_memory_requirements || !dispatch.allocate_memory ||
        !dispatch.free_memory || !dispatch.bind_image_memory ||
        !dispatch.create_image_view || !dispatch.destroy_image_view ||
        !dispatch.create_framebuffer || !dispatch.destroy_framebuffer ||
        !dispatch.create_render_pass || !dispatch.destroy_render_pass ||
        !dispatch.cmd_begin_render_pass || !dispatch.cmd_end_render_pass ||
        !create_sampler || !destroy_sampler)
    { error = "local shadow targets need complete image procedures"; return false; }
    device_ = device;
    vk_ = dispatch;
    const VkDeviceSize alignment = uniform_alignment ? uniform_alignment : 1;
    uniform_stride_ = (sizeof(LocalLightUniform) + alignment - 1) / alignment * alignment;
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
    { error = "cannot create local shadow render pass"; destroy(); return false; }
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler.magFilter = sampler.minFilter = VK_FILTER_NEAREST;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (create_sampler(device, &sampler, nullptr, &sampler_) != VK_SUCCESS)
    { error = "cannot create local shadow sampler"; destroy(); return false; }
    targets_.resize(count);
    for (Target& target : targets_)
    {
        VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = format;
        image.extent = {LocalShadowSize, LocalShadowSize, 1};
        image.mipLevels = 1;
        image.arrayLayers = LocalShadowSlots * LocalShadowFaces;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vk_.create_image(device, &image, nullptr, &target.image) != VK_SUCCESS)
        { error = "cannot create local shadow array"; destroy(); return false; }
        VkMemoryRequirements requirements{};
        vk_.get_image_memory_requirements(device, target.image, &requirements);
        uint32_t type = UINT32_MAX;
        for (uint32_t i = 0; i < memory.memoryTypeCount; ++i)
            if ((requirements.memoryTypeBits & (1u << i)) &&
                (memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
            { type = i; break; }
        if (type == UINT32_MAX)
        { error = "no device-local memory for local shadows"; destroy(); return false; }
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = type;
        if (vk_.allocate_memory(device, &allocation, nullptr, &target.memory) != VK_SUCCESS ||
            vk_.bind_image_memory(device, target.image, target.memory, 0) != VK_SUCCESS)
        { error = "cannot bind local shadow array"; destroy(); return false; }
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = target.image;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        view.format = format;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        view.subresourceRange.levelCount = 1;
        view.subresourceRange.layerCount = LocalShadowSlots * LocalShadowFaces;
        if (vk_.create_image_view(device, &view, nullptr, &target.array_view) != VK_SUCCESS)
        { error = "cannot create local shadow array view"; destroy(); return false; }
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.subresourceRange.layerCount = 1;
        for (uint32_t layer = 0; layer < target.layer_views.size(); ++layer)
        {
            view.subresourceRange.baseArrayLayer = layer;
            if (vk_.create_image_view(device, &view, nullptr, &target.layer_views[layer]) != VK_SUCCESS)
            { error = "cannot create local shadow layer view"; destroy(); return false; }
            VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            framebuffer.renderPass = pass_;
            framebuffer.attachmentCount = 1;
            framebuffer.pAttachments = &target.layer_views[layer];
            framebuffer.width = framebuffer.height = LocalShadowSize;
            framebuffer.layers = 1;
            if (vk_.create_framebuffer(device, &framebuffer, nullptr,
                    &target.framebuffers[layer]) != VK_SUCCESS)
            { error = "cannot create local shadow framebuffer"; destroy(); return false; }
        }
        if (!target.constants.initialize(device, uniform_stride_ * LocalLightCapacity,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                memory, buffers, error))
        { destroy(); return false; }
    }
    error.clear();
    return true;
}

bool LocalShadowTargets::write_light(uint32_t image, uint32_t index,
    const LocalLightUniform& value, std::string& error)
{
    return image < targets_.size() && index < LocalLightCapacity &&
        targets_[image].constants.write(VkDeviceSize(index) * uniform_stride_, &value, sizeof(value), error);
}

bool LocalShadowTargets::initialize_layers(const FrameRecordingContext& frame)
{
    if (!frame.command_buffer || frame.image_index >= targets_.size()) return false;
    Target& target = targets_[frame.image_index];
    if (target.initialized) return true;
    for (uint32_t layer = 0; layer < target.framebuffers.size(); ++layer)
    {
        VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        begin.renderPass = pass_;
        begin.framebuffer = target.framebuffers[layer];
        begin.renderArea.extent = {LocalShadowSize, LocalShadowSize};
        VkClearValue clear{};
        clear.depthStencil = {1.f, 0};
        begin.clearValueCount = 1;
        begin.pClearValues = &clear;
        vk_.cmd_begin_render_pass(frame.command_buffer, &begin, VK_SUBPASS_CONTENTS_INLINE);
        vk_.cmd_end_render_pass(frame.command_buffer);
    }
    target.initialized = true;
    return true;
}

bool LocalShadowTargets::begin_face(const FrameRecordingContext& frame, uint32_t slot,
    uint32_t face, FrameRecordingContext& shadow_frame) const
{
    if (!frame.command_buffer || frame.image_index >= targets_.size() ||
        slot >= LocalShadowSlots || face >= LocalShadowFaces) return false;
    const VkFramebuffer framebuffer = targets_[frame.image_index].framebuffers[slot * LocalShadowFaces + face];
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    begin.renderPass = pass_;
    begin.framebuffer = framebuffer;
    begin.renderArea.extent = {LocalShadowSize, LocalShadowSize};
    VkClearValue clear{};
    clear.depthStencil = {1.f, 0};
    begin.clearValueCount = 1;
    begin.pClearValues = &clear;
    vk_.cmd_begin_render_pass(frame.command_buffer, &begin, VK_SUBPASS_CONTENTS_INLINE);
    shadow_frame = {frame.command_buffer, pass_, framebuffer, {LocalShadowSize, LocalShadowSize},
        frame.image_index, frame.frame_index};
    return true;
}

void LocalShadowTargets::end(VkCommandBuffer command) const
{
    if (command && pass_) vk_.cmd_end_render_pass(command);
}

VkImageView LocalShadowTargets::view(uint32_t image) const
{
    return image < targets_.size() ? targets_[image].array_view : VK_NULL_HANDLE;
}
VkBuffer LocalShadowTargets::uniform(uint32_t image) const
{
    return image < targets_.size() ? targets_[image].constants.handle() : VK_NULL_HANDLE;
}

void LocalShadowTargets::destroy()
{
    if (device_)
    {
        for (Target& target : targets_)
        {
            target.constants.destroy();
            for (auto framebuffer : target.framebuffers)
                if (framebuffer) vk_.destroy_framebuffer(device_, framebuffer, nullptr);
            for (auto view : target.layer_views)
                if (view) vk_.destroy_image_view(device_, view, nullptr);
            if (target.array_view) vk_.destroy_image_view(device_, target.array_view, nullptr);
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
    destroy_sampler_ = nullptr;
    vk_ = {};
    uniform_stride_ = sizeof(LocalLightUniform);
}
}
