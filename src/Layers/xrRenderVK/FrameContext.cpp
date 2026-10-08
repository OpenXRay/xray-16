#include "FrameContext.h"
#if defined(XR_PLATFORM_ANDROID)
#include "xrEngine/x_ray.h"
#endif

#include <algorithm>
#include <limits>

namespace xray::render::vulkan
{
namespace
{
void crash_stage(const char* stage)
{
#if defined(XR_PLATFORM_ANDROID)
    android_set_load_context(stage);
    android_set_vulkan_stage(stage);
#else
    (void)stage;
#endif
}
}
bool mark_surface_lost(VkResult result, bool& surface_lost, FrameStatus& status)
{
    if (result != VK_ERROR_SURFACE_LOST_KHR)
        return false;
    surface_lost = true;
    status = FrameStatus::RecreateRequired;
    return true;
}

namespace
{
bool complete(const FrameDispatch& vk)
{
    return vk.get_surface_capabilities && vk.get_surface_formats && vk.create_swapchain && vk.destroy_swapchain &&
        vk.get_swapchain_images && vk.acquire_next_image && vk.queue_present && vk.create_image_view &&
        vk.destroy_image_view && vk.get_format_properties && vk.get_memory_properties &&
        vk.create_image && vk.destroy_image &&
        vk.get_image_memory_requirements && vk.allocate_memory && vk.free_memory && vk.bind_image_memory &&
        vk.create_render_pass && vk.destroy_render_pass && vk.create_framebuffer &&
        vk.destroy_framebuffer && vk.create_command_pool && vk.destroy_command_pool && vk.allocate_command_buffers &&
        vk.free_command_buffers &&
        vk.reset_command_buffer && vk.begin_command_buffer && vk.cmd_begin_render_pass && vk.cmd_end_render_pass &&
        vk.cmd_clear_attachments &&
        vk.cmd_copy_image_to_buffer && vk.cmd_copy_image && vk.cmd_pipeline_barrier &&
        vk.end_command_buffer && vk.create_semaphore && vk.destroy_semaphore && vk.create_fence && vk.destroy_fence &&
        vk.wait_for_fences && vk.reset_fences && vk.queue_submit && vk.device_wait_idle;
}

template <typename T, typename Query>
bool enumerate(Query query, std::vector<T>& values)
{
    for (unsigned attempt = 0; attempt != 3; ++attempt)
    {
        uint32_t count = 0;
        if (query(&count, nullptr) != VK_SUCCESS || count == 0)
            return false;
        values.resize(count);
        const VkResult result = query(&count, values.data());
        if (result == VK_SUCCESS)
        {
            values.resize(count);
            return !values.empty();
        }
        if (result != VK_INCOMPLETE)
            return false;
    }
    return false;
}

template <typename T>
T load_instance_proc(VkInstance instance, PFN_vkGetInstanceProcAddr get_proc, const char* name)
{
    return reinterpret_cast<T>(get_proc(instance, name));
}

template <typename T>
T load_device_proc(VkDevice device, PFN_vkGetDeviceProcAddr get_proc, const char* name)
{
    return reinterpret_cast<T>(get_proc(device, name));
}
}

bool load_frame_dispatch(VkInstance instance, PFN_vkGetInstanceProcAddr get_instance_proc,
    VkDevice device, PFN_vkGetDeviceProcAddr get_device_proc, FrameDispatch& dispatch, std::string& error)
{
    dispatch = {};
    if (!instance || !get_instance_proc || !device || !get_device_proc)
    {
        error = "Vulkan frame dispatch requires valid instance and device handles";
        return false;
    }

#define XRAY_LOAD_INSTANCE(member, name) \
    dispatch.member = load_instance_proc<decltype(dispatch.member)>(instance, get_instance_proc, name)
#define XRAY_LOAD_DEVICE(member, name) \
    dispatch.member = load_device_proc<decltype(dispatch.member)>(device, get_device_proc, name)
    XRAY_LOAD_INSTANCE(get_surface_capabilities, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    XRAY_LOAD_INSTANCE(get_surface_formats, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    XRAY_LOAD_INSTANCE(get_format_properties, "vkGetPhysicalDeviceFormatProperties");
    XRAY_LOAD_INSTANCE(get_memory_properties, "vkGetPhysicalDeviceMemoryProperties");
    XRAY_LOAD_DEVICE(create_swapchain, "vkCreateSwapchainKHR");
    XRAY_LOAD_DEVICE(destroy_swapchain, "vkDestroySwapchainKHR");
    XRAY_LOAD_DEVICE(get_swapchain_images, "vkGetSwapchainImagesKHR");
    XRAY_LOAD_DEVICE(acquire_next_image, "vkAcquireNextImageKHR");
    XRAY_LOAD_DEVICE(queue_present, "vkQueuePresentKHR");
    XRAY_LOAD_DEVICE(create_image_view, "vkCreateImageView");
    XRAY_LOAD_DEVICE(destroy_image_view, "vkDestroyImageView");
    XRAY_LOAD_DEVICE(create_image, "vkCreateImage");
    XRAY_LOAD_DEVICE(destroy_image, "vkDestroyImage");
    XRAY_LOAD_DEVICE(get_image_memory_requirements, "vkGetImageMemoryRequirements");
    XRAY_LOAD_DEVICE(allocate_memory, "vkAllocateMemory");
    XRAY_LOAD_DEVICE(free_memory, "vkFreeMemory");
    XRAY_LOAD_DEVICE(bind_image_memory, "vkBindImageMemory");
    XRAY_LOAD_DEVICE(create_render_pass, "vkCreateRenderPass");
    XRAY_LOAD_DEVICE(destroy_render_pass, "vkDestroyRenderPass");
    XRAY_LOAD_DEVICE(create_framebuffer, "vkCreateFramebuffer");
    XRAY_LOAD_DEVICE(destroy_framebuffer, "vkDestroyFramebuffer");
    XRAY_LOAD_DEVICE(create_command_pool, "vkCreateCommandPool");
    XRAY_LOAD_DEVICE(destroy_command_pool, "vkDestroyCommandPool");
    XRAY_LOAD_DEVICE(allocate_command_buffers, "vkAllocateCommandBuffers");
    XRAY_LOAD_DEVICE(free_command_buffers, "vkFreeCommandBuffers");
    XRAY_LOAD_DEVICE(reset_command_buffer, "vkResetCommandBuffer");
    XRAY_LOAD_DEVICE(begin_command_buffer, "vkBeginCommandBuffer");
    XRAY_LOAD_DEVICE(cmd_begin_render_pass, "vkCmdBeginRenderPass");
    XRAY_LOAD_DEVICE(cmd_end_render_pass, "vkCmdEndRenderPass");
    XRAY_LOAD_DEVICE(cmd_clear_attachments, "vkCmdClearAttachments");
    XRAY_LOAD_DEVICE(cmd_copy_image_to_buffer, "vkCmdCopyImageToBuffer");
    XRAY_LOAD_DEVICE(cmd_copy_image, "vkCmdCopyImage");
    XRAY_LOAD_DEVICE(cmd_pipeline_barrier, "vkCmdPipelineBarrier");
    XRAY_LOAD_DEVICE(end_command_buffer, "vkEndCommandBuffer");
    XRAY_LOAD_DEVICE(create_semaphore, "vkCreateSemaphore");
    XRAY_LOAD_DEVICE(destroy_semaphore, "vkDestroySemaphore");
    XRAY_LOAD_DEVICE(create_fence, "vkCreateFence");
    XRAY_LOAD_DEVICE(destroy_fence, "vkDestroyFence");
    XRAY_LOAD_DEVICE(wait_for_fences, "vkWaitForFences");
    XRAY_LOAD_DEVICE(reset_fences, "vkResetFences");
    XRAY_LOAD_DEVICE(queue_submit, "vkQueueSubmit");
    XRAY_LOAD_DEVICE(device_wait_idle, "vkDeviceWaitIdle");
    XRAY_LOAD_DEVICE(create_query_pool, "vkCreateQueryPool");
    XRAY_LOAD_DEVICE(destroy_query_pool, "vkDestroyQueryPool");
    XRAY_LOAD_DEVICE(cmd_reset_query_pool, "vkCmdResetQueryPool");
    XRAY_LOAD_DEVICE(cmd_write_timestamp, "vkCmdWriteTimestamp");
    XRAY_LOAD_DEVICE(get_query_pool_results, "vkGetQueryPoolResults");
#undef XRAY_LOAD_DEVICE
#undef XRAY_LOAD_INSTANCE

    if (!complete(dispatch))
    {
        dispatch = {};
        error = "required Vulkan frame procedures are unavailable";
        return false;
    }
    error.clear();
    return true;
}

bool FrameContext::recreate(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
    VkExtent2D requested_extent, std::string& error)
{
    if (!m_device || !m_queue || !m_command_pool || !physical_device || !surface)
    {
        error = "Vulkan frame context cannot recreate an uninitialized swapchain";
        return false;
    }
    if (!wait_idle())
    {
        error = "could not wait for Vulkan device before swapchain recreation";
        return false;
    }

    const VkFormat previous_format = m_format;
    if (!m_commands.empty())
        m_vk.free_command_buffers(m_device, m_command_pool,
            static_cast<uint32_t>(m_commands.size()), m_commands.data());
    m_commands.clear();
    destroy_swapchain_resources();
    m_device_lost = false;

    if (!create_swapchain(physical_device, surface, requested_extent, error))
        goto failed;
    if (m_format != previous_format)
    {
        error = "Vulkan surface format changed; existing renderer pipelines must be rebuilt";
        goto failed;
    }
    if (!create_render_targets(error) || !create_commands(error) || !create_sync(error))
        goto failed;
    m_current_frame = 0;
    m_surface_lost = false;
    error.clear();
    return true;

failed:
    destroy_swapchain_resources();
    return false;
}

bool FrameContext::release_swapchain()
{
    if (!m_device || !m_command_pool || !wait_idle())
        return false;
    if (!m_commands.empty())
        m_vk.free_command_buffers(m_device, m_command_pool,
            static_cast<uint32_t>(m_commands.size()), m_commands.data());
    m_commands.clear();
    destroy_swapchain_resources();
    return true;
}

bool FrameContext::wait_idle()
{
    if (!m_device || !m_vk.device_wait_idle)
        return false;
    const VkResult result = m_vk.device_wait_idle(m_device);
    m_device_lost = result == VK_ERROR_DEVICE_LOST;
    return result == VK_SUCCESS;
}

FrameContext::~FrameContext()
{
    destroy();
}

bool FrameContext::initialize(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface,
    VkQueue queue, uint32_t queue_family, VkExtent2D requested_extent,
    const FrameDispatch& dispatch, std::string& error, bool allow_readback, bool use_depth,
    bool preserve_prepass_depth, bool postprocess,
    float timestamp_period_ns, uint32_t timestamp_valid_bits)
{
    destroy();
    if (!physical_device || !device || !surface || !queue || queue_family == UINT32_MAX)
    {
        error = "Vulkan frame context requires valid device, surface and queue handles";
        return false;
    }
    if (!complete(dispatch))
    {
        error = "Vulkan frame context dispatch is incomplete";
        return false;
    }

    m_device = device;
    m_queue = queue;
    m_queue_family = queue_family;
    m_vk = dispatch;
    m_device_lost = false;
    m_surface_lost = false;
    m_allow_readback = allow_readback;
    m_preserve_prepass_depth = preserve_prepass_depth;
    m_postprocess = postprocess;
    if (timestamp_period_ns > 0.f && timestamp_valid_bits &&
        m_vk.create_query_pool && m_vk.destroy_query_pool && m_vk.cmd_reset_query_pool &&
        m_vk.cmd_write_timestamp && m_vk.get_query_pool_results)
    {
        VkQueryPoolCreateInfo query_info{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        query_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        query_info.queryCount = 2 * FramesInFlight;
        if (m_vk.create_query_pool(m_device, &query_info, nullptr, &m_timestamp_pool) == VK_SUCCESS)
        {
            m_timestamp_period_ns = timestamp_period_ns;
            m_timestamp_valid_bits = timestamp_valid_bits;
        }
    }
    if (postprocess && !allow_readback)
    {
        error = "Vulkan postprocess requires swapchain transfer-source support";
        destroy();
        return false;
    }
    if (preserve_prepass_depth && !use_depth)
    {
        error = "prepass depth requires a depth attachment";
        destroy();
        return false;
    }
    if (use_depth || postprocess)
        m_vk.get_memory_properties(physical_device, &m_memory_properties);
    if (use_depth)
    {
        const VkFormat candidates[]{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM};
        for (VkFormat candidate : candidates)
        {
            VkFormatProperties properties{};
            m_vk.get_format_properties(physical_device, candidate, &properties);
            const VkFormatFeatureFlags required = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
                VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
            if ((properties.optimalTilingFeatures & required) == required)
            {
                m_depth_format = candidate;
                break;
            }
        }
        if (m_depth_format == VK_FORMAT_UNDEFINED)
        {
            error = "Vulkan device has no depth format that supports attachment and sampling";
            destroy();
            return false;
        }
    }

    if (!create_swapchain(physical_device, surface, requested_extent, error) ||
        !create_render_targets(error) || !create_commands(error) || !create_sync(error))
    {
        destroy();
        return false;
    }
    error.clear();
    return true;
}

bool FrameContext::create_swapchain(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
    VkExtent2D requested_extent, std::string& error)
{
    VkSurfaceCapabilitiesKHR capabilities{};
    if (m_vk.get_surface_capabilities(physical_device, surface, &capabilities) != VK_SUCCESS)
    {
        error = "could not query Vulkan surface capabilities";
        return false;
    }
    if (!(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
    {
        error = "Vulkan surface does not support color attachments";
        return false;
    }
    if (m_allow_readback && !(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT))
    {
        error = "Vulkan smoke surface does not support transfer-source swapchain images";
        return false;
    }

    std::vector<VkSurfaceFormatKHR> formats;
    if (!enumerate<VkSurfaceFormatKHR>([&](uint32_t* count, VkSurfaceFormatKHR* values)
        { return m_vk.get_surface_formats(physical_device, surface, count, values); }, formats))
    {
        error = "Vulkan surface has no supported formats";
        return false;
    }

    VkSurfaceFormatKHR surface_format = formats.front();
    if (formats.size() == 1 && formats.front().format == VK_FORMAT_UNDEFINED)
    {
        surface_format.format = m_format != VK_FORMAT_UNDEFINED ? m_format : VK_FORMAT_B8G8R8A8_UNORM;
        surface_format.colorSpace = formats.front().colorSpace;
    }
    else
    {
        auto preferred = std::find_if(formats.begin(), formats.end(), [&](const VkSurfaceFormatKHR& format)
        {
            return m_format != VK_FORMAT_UNDEFINED && format.format == m_format &&
                format.colorSpace == m_color_space;
        });
        if (preferred == formats.end())
            preferred = std::find_if(formats.begin(), formats.end(), [&](const VkSurfaceFormatKHR& format)
            {
                return m_format != VK_FORMAT_UNDEFINED && format.format == m_format;
            });
        if (preferred == formats.end())
            preferred = std::find_if(formats.begin(), formats.end(), [](const VkSurfaceFormatKHR& format)
            {
                return format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                    format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            });
        if (preferred != formats.end())
            surface_format = *preferred;
    }
    if (m_allow_readback && surface_format.format != VK_FORMAT_B8G8R8A8_UNORM &&
        surface_format.format != VK_FORMAT_R8G8B8A8_UNORM)
    {
        const auto compatible = std::find_if(formats.begin(), formats.end(), [](const VkSurfaceFormatKHR& format)
        {
            return format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM;
        });
        if (compatible == formats.end())
        {
            error = "Vulkan smoke surface has no 8-bit RGBA swapchain format for pixel readback";
            return false;
        }
        surface_format = *compatible;
    }

    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        m_extent = capabilities.currentExtent;
    else
    {
        m_extent.width = std::clamp(requested_extent.width,
            capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        m_extent.height = std::clamp(requested_extent.height,
            capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    if (!m_extent.width || !m_extent.height)
    {
        error = "Vulkan surface has zero extent";
        return false;
    }

    VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (!(capabilities.supportedCompositeAlpha & composite_alpha))
    {
        constexpr std::array<VkCompositeAlphaFlagBitsKHR, 3> alternatives{
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
        };
        const auto supported = std::find_if(alternatives.begin(), alternatives.end(), [&](auto candidate)
        {
            return capabilities.supportedCompositeAlpha & candidate;
        });
        if (supported == alternatives.end())
        {
            error = "Vulkan surface has no supported composite alpha mode";
            return false;
        }
        composite_alpha = *supported;
    }

    uint32_t image_count = capabilities.minImageCount;
    if (image_count < std::numeric_limits<uint32_t>::max())
        ++image_count;
    if (capabilities.maxImageCount && image_count > capabilities.maxImageCount)
        image_count = capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = surface;
    info.minImageCount = image_count;
    info.imageFormat = surface_format.format;
    info.imageColorSpace = surface_format.colorSpace;
    info.imageExtent = m_extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        (m_allow_readback ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0);
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // The engine renders upright in the Android window's logical orientation.
    // With currentTransform on rotated displays, the compositor interprets
    // those pixels as already rotated, yielding a sideways, reduced frame.
    info.preTransform = (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ?
        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : capabilities.currentTransform;
    info.compositeAlpha = composite_alpha;
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    info.clipped = VK_TRUE;
    if (m_vk.create_swapchain(m_device, &info, nullptr, &m_swapchain) != VK_SUCCESS)
    {
        error = "vkCreateSwapchainKHR failed";
        return false;
    }
    m_format = surface_format.format;
    m_color_space = surface_format.colorSpace;

    if (!enumerate<VkImage>([&](uint32_t* count, VkImage* values)
        { return m_vk.get_swapchain_images(m_device, m_swapchain, count, values); }, m_images))
    {
        error = "could not enumerate Vulkan swapchain images";
        return false;
    }
    return true;
}

bool FrameContext::create_render_targets(std::string& error)
{
    m_scene_depth_attached = false;
    VkAttachmentDescription attachments[2]{};
    attachments[0].format = m_format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = m_allow_readback ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL :
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[1].format = m_depth_format;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = m_preserve_prepass_depth ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = m_preserve_prepass_depth ?
        VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = m_preserve_prepass_depth ?
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color;
    if (m_depth_format != VK_FORMAT_UNDEFINED)
        subpass.pDepthStencilAttachment = &depth;

    std::array<VkSubpassDependency, 2> dependencies{};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    if (m_depth_format != VK_FORMAT_UNDEFINED)
    {
        dependencies[0].srcStageMask |= m_preserve_prepass_depth ?
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask |= m_preserve_prepass_depth ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0;
        dependencies[0].dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = m_allow_readback ? VK_PIPELINE_STAGE_TRANSFER_BIT :
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = m_allow_readback ? VK_ACCESS_TRANSFER_READ_BIT : 0;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = m_depth_format != VK_FORMAT_UNDEFINED ? 2 : 1;
    render_pass_info.pAttachments = attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
    render_pass_info.pDependencies = dependencies.data();
    if (m_vk.create_render_pass(m_device, &render_pass_info, nullptr, &m_render_pass) != VK_SUCCESS)
    {
        error = "vkCreateRenderPass failed";
        return false;
    }

    m_image_views.reserve(m_images.size());
    m_depth_images.reserve(m_images.size());
    m_depth_memories.reserve(m_images.size());
    m_depth_views.reserve(m_images.size());
    m_framebuffers.reserve(m_images.size());
    for (VkImage image : m_images)
    {
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = m_format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.layerCount = 1;
        VkImageView view = VK_NULL_HANDLE;
        if (m_vk.create_image_view(m_device, &view_info, nullptr, &view) != VK_SUCCESS)
        {
            error = "vkCreateImageView failed";
            return false;
        }
        m_image_views.push_back(view);

        VkImageView framebuffer_views[2]{view, VK_NULL_HANDLE};
        if (m_depth_format != VK_FORMAT_UNDEFINED)
        {
            VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            image_info.imageType = VK_IMAGE_TYPE_2D;
            image_info.format = m_depth_format;
            image_info.extent = {m_extent.width, m_extent.height, 1};
            image_info.mipLevels = image_info.arrayLayers = 1;
            image_info.samples = VK_SAMPLE_COUNT_1_BIT;
            image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
            image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VkImage depth_image = VK_NULL_HANDLE;
            if (m_vk.create_image(m_device, &image_info, nullptr, &depth_image) != VK_SUCCESS)
            {
                error = "vkCreateImage failed for depth attachment";
                return false;
            }
            m_depth_images.push_back(depth_image);
            VkMemoryRequirements requirements{};
            m_vk.get_image_memory_requirements(m_device, depth_image, &requirements);
            uint32_t memory_type = UINT32_MAX;
            for (uint32_t i = 0; i < m_memory_properties.memoryTypeCount; ++i)
                if ((requirements.memoryTypeBits & (1u << i)) &&
                    (m_memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
                {
                    memory_type = i;
                    break;
                }
            if (memory_type == UINT32_MAX)
            {
                error = "no device-local memory type for depth attachment";
                return false;
            }
            VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            allocation.allocationSize = requirements.size;
            allocation.memoryTypeIndex = memory_type;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            if (m_vk.allocate_memory(m_device, &allocation, nullptr, &memory) != VK_SUCCESS)
            {
                error = "vkAllocateMemory failed for depth attachment";
                return false;
            }
            m_depth_memories.push_back(memory);
            if (m_vk.bind_image_memory(m_device, depth_image, memory, 0) != VK_SUCCESS)
            {
                error = "vkBindImageMemory failed for depth attachment";
                return false;
            }
            VkImageViewCreateInfo depth_view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            depth_view_info.image = depth_image;
            depth_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            depth_view_info.format = m_depth_format;
            depth_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depth_view_info.subresourceRange.levelCount = depth_view_info.subresourceRange.layerCount = 1;
            VkImageView depth_view = VK_NULL_HANDLE;
            if (m_vk.create_image_view(m_device, &depth_view_info, nullptr, &depth_view) != VK_SUCCESS)
            {
                error = "vkCreateImageView failed for depth attachment";
                return false;
            }
            m_depth_views.push_back(depth_view);
            framebuffer_views[1] = depth_view;
        }

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = m_render_pass;
        framebuffer_info.attachmentCount = m_depth_format != VK_FORMAT_UNDEFINED ? 2 : 1;
        framebuffer_info.pAttachments = framebuffer_views;
        framebuffer_info.width = m_extent.width;
        framebuffer_info.height = m_extent.height;
        framebuffer_info.layers = 1;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        if (m_vk.create_framebuffer(m_device, &framebuffer_info, nullptr, &framebuffer) != VK_SUCCESS)
        {
            error = "vkCreateFramebuffer failed";
            return false;
        }
        m_framebuffers.push_back(framebuffer);
    }
    if (m_postprocess && !create_postprocess_targets(error)) return false;
    return true;
}

bool FrameContext::create_postprocess_targets(std::string& error)
{
    VkAttachmentDescription color{};
    color.format = m_format;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    color.finalLayout = m_allow_readback ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL :
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    const VkAttachmentReference reference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &reference;
    VkSubpassDependency deps[2]{};
    deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass = 0;
    deps[0].srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[1].srcSubpass = 0;
    deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    pass.attachmentCount = 1;
    pass.pAttachments = &color;
    pass.subpassCount = 1;
    pass.pSubpasses = &subpass;
    pass.dependencyCount = 2;
    pass.pDependencies = deps;
    if (m_vk.create_render_pass(m_device, &pass, nullptr, &m_composite_pass) != VK_SUCCESS)
    {
        error = "could not create Vulkan postprocess render pass";
        return false;
    }
    for (size_t i = 0; i < m_images.size(); ++i)
    {
        VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = m_format;
        info.extent = {m_extent.width, m_extent.height, 1};
        info.mipLevels = info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkImage image = VK_NULL_HANDLE;
        if (m_vk.create_image(m_device, &info, nullptr, &image) != VK_SUCCESS)
        {
            error = "could not create Vulkan postprocess source image";
            return false;
        }
        m_post_images.push_back(image);
        VkMemoryRequirements requirements{};
        m_vk.get_image_memory_requirements(m_device, image, &requirements);
        uint32_t type = UINT32_MAX;
        for (uint32_t n = 0; n < m_memory_properties.memoryTypeCount; ++n)
            if ((requirements.memoryTypeBits & (1u << n)) &&
                (m_memory_properties.memoryTypes[n].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
            {
                type = n;
                break;
            }
        if (type == UINT32_MAX)
        {
            error = "no device-local memory for Vulkan postprocess source";
            return false;
        }
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = type;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        if (m_vk.allocate_memory(m_device, &allocation, nullptr, &memory) != VK_SUCCESS)
        {
            error = "could not allocate Vulkan postprocess source";
            return false;
        }
        m_post_memories.push_back(memory);
        if (m_vk.bind_image_memory(m_device, image, memory, 0) != VK_SUCCESS)
        {
            error = "could not bind Vulkan postprocess source";
            return false;
        }
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = image;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = m_format;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.levelCount = view.subresourceRange.layerCount = 1;
        VkImageView source = VK_NULL_HANDLE;
        if (m_vk.create_image_view(m_device, &view, nullptr, &source) != VK_SUCCESS)
        {
            error = "could not create Vulkan postprocess source view";
            return false;
        }
        m_post_views.push_back(source);
        VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer.renderPass = m_composite_pass;
        framebuffer.attachmentCount = 1;
        framebuffer.pAttachments = &m_image_views[i];
        framebuffer.width = m_extent.width;
        framebuffer.height = m_extent.height;
        framebuffer.layers = 1;
        VkFramebuffer output = VK_NULL_HANDLE;
        if (m_vk.create_framebuffer(m_device, &framebuffer, nullptr, &output) != VK_SUCCESS)
        {
            error = "could not create Vulkan postprocess framebuffer";
            return false;
        }
        m_composite_framebuffers.push_back(output);
    }
    return true;
}

bool FrameContext::attach_scene_depth(const std::vector<VkImageView>& views, std::string& error)
{
    if (!m_preserve_prepass_depth || !m_render_pass ||
        (!views.empty() && (views.size() != m_image_views.size() ||
            std::any_of(views.begin(), views.end(), [](VkImageView view) { return !view; }))))
    {
        error = "invalid borrowed Vulkan scene depth views";
        return false;
    }
    std::vector<VkFramebuffer> replacements;
    replacements.reserve(m_image_views.size());
    for (size_t i = 0; i < m_image_views.size(); ++i)
    {
        const VkImageView attachments[]{m_image_views[i], views.empty() ? m_depth_views[i] : views[i]};
        VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        info.renderPass = m_render_pass;
        info.attachmentCount = 2;
        info.pAttachments = attachments;
        info.width = m_extent.width;
        info.height = m_extent.height;
        info.layers = 1;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        if (m_vk.create_framebuffer(m_device, &info, nullptr, &framebuffer) != VK_SUCCESS)
        {
            for (auto created : replacements) m_vk.destroy_framebuffer(m_device, created, nullptr);
            error = "could not attach Vulkan scene depth to swapchain";
            return false;
        }
        replacements.push_back(framebuffer);
    }
    for (auto old : m_framebuffers) m_vk.destroy_framebuffer(m_device, old, nullptr);
    m_framebuffers.swap(replacements);
    m_scene_depth_attached = !views.empty();
    error.clear();
    return true;
}

bool FrameContext::enable_interpass(std::string& error)
{
    if (m_overlay_pass) return true;
    if (!m_device || !m_render_pass || !m_allow_readback || !m_scene_depth_attached ||
        m_depth_format == VK_FORMAT_UNDEFINED)
    { error = "water interpass requires a readable color target and scene depth"; return false; }
    VkAttachmentDescription attachments[2]{};
    attachments[0].format = m_format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    attachments[1].format = m_depth_format;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    const VkAttachmentReference color{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    const VkAttachmentReference depth{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color;
    subpass.pDepthStencilAttachment = &depth;
    const VkSubpassDependency dependencies[]{
    {
        VK_SUBPASS_EXTERNAL, 0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        0},
    {0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_TRANSFER_READ_BIT, 0}};
    VkRenderPassCreateInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    info.attachmentCount = 2;
    info.pAttachments = attachments;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 2;
    info.pDependencies = dependencies;
    if (m_vk.create_render_pass(m_device, &info, nullptr, &m_overlay_pass) != VK_SUCCESS)
    { error = "cannot create Vulkan water overlay pass"; return false; }
    error.clear();
    return true;
}

void FrameContext::clear_depth(VkCommandBuffer command) const
{
    VkClearAttachment attachment{};
    attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    attachment.clearValue.depthStencil = {1.0f, 0};
    VkClearRect rect{};
    rect.rect.extent = m_extent;
    rect.layerCount = 1;
    m_vk.cmd_clear_attachments(command, 1, &attachment, 1, &rect);
}

bool FrameContext::create_commands(std::string& error)
{
    if (!m_command_pool)
    {
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = m_queue_family;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        if (m_vk.create_command_pool(m_device, &pool_info, nullptr, &m_command_pool) != VK_SUCCESS)
        {
            error = "vkCreateCommandPool failed";
            return false;
        }
    }

    m_commands.resize(m_images.size());
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = m_command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = static_cast<uint32_t>(m_commands.size());
    if (m_vk.allocate_command_buffers(m_device, &allocate_info, m_commands.data()) != VK_SUCCESS)
    {
        error = "vkAllocateCommandBuffers failed";
        return false;
    }

    return true;
}

bool FrameContext::create_sync(std::string& error)
{
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (uint32_t frame = 0; frame < FramesInFlight; ++frame)
    {
        if (m_vk.create_semaphore(m_device, &semaphore_info, nullptr, &m_image_available[frame]) != VK_SUCCESS ||
            m_vk.create_fence(m_device, &fence_info, nullptr, &m_frame_fences[frame]) != VK_SUCCESS)
        {
            error = "could not create Vulkan frame synchronization objects";
            return false;
        }
    }

    m_render_finished.resize(m_images.size());
    for (VkSemaphore& semaphore : m_render_finished)
        if (m_vk.create_semaphore(m_device, &semaphore_info, nullptr, &semaphore) != VK_SUCCESS)
        {
            error = "could not create Vulkan presentation semaphores";
            return false;
        }
    m_image_fences.resize(m_images.size(), VK_NULL_HANDLE);
    return true;
}

bool FrameContext::render_frame(const VkClearColorValue& clear, FrameStatus& status, std::string& error,
    FrameRecorder recorder, void* user_data, FrameReadbackRecorder readback, void* readback_data,
    FramePrepassRecorder prepass, void* prepass_data, bool clear_target,
    FrameRecorder compositor, void* compositor_data,
    FrameInterpassRecorder interpass, void* interpass_data,
    FrameRecorder overlay, void* overlay_data)
{
    status = FrameStatus::Presented;
    if (!m_device || !m_swapchain)
    {
        error = "Vulkan frame context is not initialized";
        return false;
    }
    if (m_preserve_prepass_depth && !m_scene_depth_attached)
    {
        error = "Vulkan swapchain has no attached scene depth";
        return false;
    }
    if ((interpass || overlay) && (!m_overlay_pass || !interpass || !overlay))
    { error = "water overlay pass requires capture and overlay recorders"; return false; }

    crash_stage("vulkan frame: wait fence");
    VkFence frame_fence = m_frame_fences[m_current_frame];
    const VkResult fence_wait = m_vk.wait_for_fences(m_device, 1, &frame_fence, VK_TRUE, UINT64_MAX);
    if (fence_wait != VK_SUCCESS)
    {
        m_device_lost = fence_wait == VK_ERROR_DEVICE_LOST;
        error = "could not wait for Vulkan frame fence";
        return false;
    }
    if (m_timestamp_pool && m_timestamp_submitted[m_current_frame])
    {
        uint64_t ticks[2]{};
        if (m_vk.get_query_pool_results(m_device, m_timestamp_pool, 2 * m_current_frame, 2,
                sizeof(ticks), ticks, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS)
        {
            const uint64_t mask = m_timestamp_valid_bits == 64 ? UINT64_MAX :
                ((uint64_t{1} << m_timestamp_valid_bits) - 1);
            const uint64_t elapsed = (ticks[1] - ticks[0]) & mask;
            m_last_gpu_ms = static_cast<double>(elapsed) * m_timestamp_period_ns / 1000000.0;
        }
        m_timestamp_submitted[m_current_frame] = false;
    }

    crash_stage("vulkan frame: acquire image");
    uint32_t image_index = 0;
    const VkResult acquire = m_vk.acquire_next_image(m_device, m_swapchain, UINT64_MAX,
        m_image_available[m_current_frame], VK_NULL_HANDLE, &image_index);
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR)
    {
        status = FrameStatus::RecreateRequired;
        error.clear();
        return true;
    }
    if (mark_surface_lost(acquire, m_surface_lost, status))
    {
        error.clear();
        return true;
    }
    if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR)
    {
        m_device_lost = acquire == VK_ERROR_DEVICE_LOST;
        error = "vkAcquireNextImageKHR failed";
        return false;
    }
    if (image_index >= m_commands.size())
    {
        error = "Vulkan returned an invalid swapchain image index";
        return false;
    }

    VkFence image_fence = m_image_fences[image_index];
    if (image_fence && image_fence != frame_fence)
    {
        const VkResult image_fence_wait = m_vk.wait_for_fences(
            m_device, 1, &image_fence, VK_TRUE, UINT64_MAX);
        if (image_fence_wait != VK_SUCCESS)
        {
            m_device_lost = image_fence_wait == VK_ERROR_DEVICE_LOST;
            error = "could not wait for the previous Vulkan image submission";
            return false;
        }
    }
    crash_stage("vulkan frame: begin command buffer");
    VkCommandBuffer command = m_commands[image_index];
    const VkResult reset_command = m_vk.reset_command_buffer(command, 0);
    if (reset_command != VK_SUCCESS)
    {
        m_device_lost = reset_command == VK_ERROR_DEVICE_LOST;
        error = "could not reset Vulkan command buffer";
        return false;
    }
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    const VkResult begin_result = m_vk.begin_command_buffer(command, &begin_info);
    if (begin_result != VK_SUCCESS)
    {
        m_device_lost = begin_result == VK_ERROR_DEVICE_LOST;
        error = "vkBeginCommandBuffer failed";
        return false;
    }
    if (m_timestamp_pool)
    {
        m_vk.cmd_reset_query_pool(command, m_timestamp_pool, 2 * m_current_frame, 2);
        m_vk.cmd_write_timestamp(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            m_timestamp_pool, 2 * m_current_frame);
    }
    if (prepass)
    {
        crash_stage("vulkan frame: record prepass");
        const FrameRecordingContext frame{command, VK_NULL_HANDLE, VK_NULL_HANDLE,
            m_extent, image_index, m_current_frame};
        prepass(frame, prepass_data);
    }
    VkClearValue clear_values[2]{};
    clear_values[0].color = clear;
    clear_values[1].depthStencil = {1.0f, 0};
    VkRenderPassBeginInfo render_info{};
    render_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_info.renderPass = m_render_pass;
    render_info.framebuffer = m_framebuffers[image_index];
    render_info.renderArea.extent = m_extent;
    render_info.clearValueCount = m_depth_format != VK_FORMAT_UNDEFINED ? 2 : 1;
    render_info.pClearValues = clear_values;
    m_vk.cmd_begin_render_pass(command, &render_info, VK_SUBPASS_CONTENTS_INLINE);
    if (clear_target)
    {
        VkClearAttachment attachment{};
        attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        attachment.colorAttachment = 0;
        attachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        VkClearRect rect{};
        rect.rect.extent = m_extent;
        rect.layerCount = 1;
        m_vk.cmd_clear_attachments(command, 1, &attachment, 1, &rect);
    }
    if (recorder)
    {
        crash_stage("vulkan frame: record scene");
        const FrameRecordingContext frame{command, m_render_pass, m_framebuffers[image_index], m_extent,
            image_index, m_current_frame};
        recorder(frame, user_data);
    }
    m_vk.cmd_end_render_pass(command);
    if (interpass)
    {
        crash_stage("vulkan frame: record overlay");
        interpass(command, m_images[image_index], image_index, interpass_data);
        render_info.renderPass = m_overlay_pass;
        m_vk.cmd_begin_render_pass(command, &render_info, VK_SUBPASS_CONTENTS_INLINE);
        const FrameRecordingContext overlay_frame{command, m_overlay_pass,
            m_framebuffers[image_index], m_extent, image_index, m_current_frame};
        overlay(overlay_frame, overlay_data);
        m_vk.cmd_end_render_pass(command);
    }
    if (m_postprocess)
    {
        crash_stage("vulkan frame: record postprocess");
        if (!compositor)
        {
            error = "Vulkan postprocess compositor was not provided";
            return false;
        }
        VkImageMemoryBarrier to_copy{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        to_copy.srcAccessMask = 0;
        to_copy.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_copy.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Discard the previous use of this image.
        to_copy.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_copy.srcQueueFamilyIndex = to_copy.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_copy.image = m_post_images[image_index];
        to_copy.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        to_copy.subresourceRange.levelCount = to_copy.subresourceRange.layerCount = 1;
        m_vk.cmd_pipeline_barrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &to_copy);
        VkImageCopy region{};
        region.srcSubresource.aspectMask = region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.layerCount = region.dstSubresource.layerCount = 1;
        region.extent = {m_extent.width, m_extent.height, 1};
        m_vk.cmd_copy_image(command, m_images[image_index], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            m_post_images[image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        to_copy.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_copy.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        to_copy.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_copy.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        m_vk.cmd_pipeline_barrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &to_copy);
        VkRenderPassBeginInfo composite{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        composite.renderPass = m_composite_pass;
        composite.framebuffer = m_composite_framebuffers[image_index];
        composite.renderArea.extent = m_extent;
        m_vk.cmd_begin_render_pass(command, &composite, VK_SUBPASS_CONTENTS_INLINE);
        const FrameRecordingContext context{command, m_composite_pass,
            m_composite_framebuffers[image_index], m_extent, image_index, m_current_frame};
        compositor(context, compositor_data);
        m_vk.cmd_end_render_pass(command);
    }
    if (m_allow_readback)
    {
        if (readback)
            readback(command, m_images[image_index], m_extent, readback_data);
        VkImageMemoryBarrier present_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        present_barrier.srcAccessMask = readback ? VK_ACCESS_TRANSFER_READ_BIT : 0;
        present_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        present_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        present_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        present_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        present_barrier.image = m_images[image_index];
        present_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        present_barrier.subresourceRange.levelCount = 1;
        present_barrier.subresourceRange.layerCount = 1;
        m_vk.cmd_pipeline_barrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &present_barrier);
    }
    if (m_timestamp_pool)
        m_vk.cmd_write_timestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            m_timestamp_pool, 2 * m_current_frame + 1);
    crash_stage("vulkan frame: end command buffer");
    const VkResult end_result = m_vk.end_command_buffer(command);
    if (end_result != VK_SUCCESS)
    {
        m_device_lost = end_result == VK_ERROR_DEVICE_LOST;
        error = "vkEndCommandBuffer failed";
        return false;
    }
    const VkResult reset_fence = m_vk.reset_fences(m_device, 1, &frame_fence);
    if (reset_fence != VK_SUCCESS)
    {
        m_device_lost = reset_fence == VK_ERROR_DEVICE_LOST;
        error = "could not reset Vulkan frame fence";
        return false;
    }

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &m_image_available[m_current_frame];
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &m_render_finished[image_index];
    crash_stage("vulkan frame: queue submit");
    const VkResult submit_result = m_vk.queue_submit(m_queue, 1, &submit, frame_fence);
    if (submit_result != VK_SUCCESS)
    {
        m_device_lost = submit_result == VK_ERROR_DEVICE_LOST;
        error = "vkQueueSubmit failed";
        return false;
    }
    m_image_fences[image_index] = frame_fence;
    if (m_timestamp_pool) m_timestamp_submitted[m_current_frame] = true;

    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &m_render_finished[image_index];
    present.swapchainCount = 1;
    present.pSwapchains = &m_swapchain;
    present.pImageIndices = &image_index;
    crash_stage("vulkan frame: queue present");
    const VkResult result = m_vk.queue_present(m_queue, &present);
    crash_stage("vulkan frame: presented");
    m_current_frame = (m_current_frame + 1) % FramesInFlight;
    if (mark_surface_lost(result, m_surface_lost, status))
    {
        error.clear();
        return true;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR)
    {
        m_device_lost = result == VK_ERROR_DEVICE_LOST;
        error = "vkQueuePresentKHR failed";
        return false;
    }
    // Android may report SUBOPTIMAL every frame when the supported identity
    // transform differs from the display's natural rotation. Presentation is
    // valid; rebuilding the same swapchain cannot resolve that condition.
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        status = FrameStatus::RecreateRequired;
    error.clear();
    return true;
}

void FrameContext::destroy()
{
    if (m_device && m_vk.device_wait_idle)
    {
        const VkResult result = m_vk.device_wait_idle(m_device);
        m_device_lost = result == VK_ERROR_DEVICE_LOST;
    }
    if (m_device)
    {
        destroy_swapchain_resources();
        if (m_timestamp_pool) m_vk.destroy_query_pool(m_device, m_timestamp_pool, nullptr);
        if (m_command_pool)
            m_vk.destroy_command_pool(m_device, m_command_pool, nullptr);
    }
    m_device = VK_NULL_HANDLE;
    m_queue = VK_NULL_HANDLE;
    m_queue_family = UINT32_MAX;
    m_vk = {};
    m_swapchain = VK_NULL_HANDLE;
    m_render_pass = VK_NULL_HANDLE;
    m_command_pool = VK_NULL_HANDLE;
    m_timestamp_pool = VK_NULL_HANDLE;
    m_timestamp_period_ns = 0.f;
    m_timestamp_valid_bits = 0;
    m_timestamp_submitted = {};
    m_last_gpu_ms.reset();
    m_extent = {};
    m_format = VK_FORMAT_UNDEFINED;
    m_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    m_depth_format = VK_FORMAT_UNDEFINED;
    m_memory_properties = {};
    m_allow_readback = false;
    m_preserve_prepass_depth = false;
    m_postprocess = false;
    m_scene_depth_attached = false;
    m_images.clear();
    m_image_views.clear();
    m_depth_images.clear();
    m_depth_memories.clear();
    m_depth_views.clear();
    m_framebuffers.clear();
    m_post_images.clear();
    m_post_memories.clear();
    m_post_views.clear();
    m_composite_framebuffers.clear();
    m_composite_pass = VK_NULL_HANDLE;
    m_commands.clear();
    m_render_finished.clear();
    m_image_available = {};
    m_frame_fences = {};
    m_image_fences.clear();
    m_current_frame = 0;
    m_surface_lost = false;
}

void FrameContext::destroy_swapchain_resources()
{
    if (!m_device)
        return;
    for (VkFramebuffer framebuffer : m_framebuffers)
        if (framebuffer) m_vk.destroy_framebuffer(m_device, framebuffer, nullptr);
    for (VkFramebuffer framebuffer : m_composite_framebuffers)
        if (framebuffer) m_vk.destroy_framebuffer(m_device, framebuffer, nullptr);
    for (VkImageView view : m_post_views)
        if (view) m_vk.destroy_image_view(m_device, view, nullptr);
    for (VkImage image : m_post_images)
        if (image) m_vk.destroy_image(m_device, image, nullptr);
    for (VkDeviceMemory memory : m_post_memories)
        if (memory) m_vk.free_memory(m_device, memory, nullptr);
    if (m_composite_pass) m_vk.destroy_render_pass(m_device, m_composite_pass, nullptr);
    if (m_overlay_pass) m_vk.destroy_render_pass(m_device, m_overlay_pass, nullptr);
    for (VkImageView view : m_image_views)
        if (view) m_vk.destroy_image_view(m_device, view, nullptr);
    for (VkImageView view : m_depth_views)
        if (view) m_vk.destroy_image_view(m_device, view, nullptr);
    for (VkImage image : m_depth_images)
        if (image) m_vk.destroy_image(m_device, image, nullptr);
    for (VkDeviceMemory memory : m_depth_memories)
        if (memory) m_vk.free_memory(m_device, memory, nullptr);
    if (m_render_pass) m_vk.destroy_render_pass(m_device, m_render_pass, nullptr);
    for (VkSemaphore semaphore : m_render_finished)
        if (semaphore) m_vk.destroy_semaphore(m_device, semaphore, nullptr);
    for (VkSemaphore semaphore : m_image_available)
        if (semaphore) m_vk.destroy_semaphore(m_device, semaphore, nullptr);
    for (VkFence fence : m_frame_fences)
        if (fence) m_vk.destroy_fence(m_device, fence, nullptr);
    if (m_swapchain) m_vk.destroy_swapchain(m_device, m_swapchain, nullptr);
    m_swapchain = VK_NULL_HANDLE;
    m_render_pass = VK_NULL_HANDLE;
    m_overlay_pass = VK_NULL_HANDLE;
    m_composite_pass = VK_NULL_HANDLE;
    m_extent = {};
    m_images.clear();
    m_image_views.clear();
    m_depth_images.clear();
    m_depth_memories.clear();
    m_depth_views.clear();
    m_framebuffers.clear();
    m_post_images.clear();
    m_post_memories.clear();
    m_post_views.clear();
    m_composite_framebuffers.clear();
    m_scene_depth_attached = false;
    m_render_finished.clear();
    m_image_available = {};
    m_frame_fences = {};
    m_image_fences.clear();
}
}
