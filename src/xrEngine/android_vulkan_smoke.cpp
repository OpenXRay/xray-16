#include "stdafx.h"

#if defined(XR_PLATFORM_ANDROID)

#include "android_vulkan_smoke.h"
#include "../Layers/xrRenderVK/DdsTexture.h"
#include "../Layers/xrRenderVK/TextureUpload.h"
#include "../Layers/xrRenderVK/VulkanHardware.h"

#include <SDL.h>

#if __has_include(<vulkan/vulkan.h>)
#define XRAY_ANDROID_HAS_VULKAN_HEADERS 1
#include <vulkan/vulkan.h>
#endif

#include <dlfcn.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

#ifndef SDL_WINDOW_VULKAN
#define SDL_WINDOW_VULKAN 0x10000000u
#endif

namespace AndroidVulkanSmoke
{
#if defined(XRAY_ANDROID_HAS_VULKAN_HEADERS)
namespace
{
using GetInstanceExtensions = SDL_bool (*)(SDL_Window*, unsigned int*, const char**);
using CreateSurface = SDL_bool (*)(SDL_Window*, VkInstance, VkSurfaceKHR*);

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

bool has_extension(const std::vector<const char*>& extensions, const char* name)
{
    return std::any_of(extensions.begin(), extensions.end(), [name](const char* extension)
    {
        return std::strcmp(extension, name) == 0;
    });
}

}
#endif

bool Run(std::string& reason)
{
#if !defined(XRAY_ANDROID_HAS_VULKAN_HEADERS)
    reason = "Vulkan headers are not provided by the Android toolchain";
    Msg("! [renderer-vulkan] %s", reason.c_str());
    return false;
#else
    void* vulkan_library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!vulkan_library)
    {
        reason = "libvulkan.so is unavailable";
        Msg("! [renderer-vulkan] %s", reason.c_str());
        return false;
    }

    SDL_Window* window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSemaphore acquire_semaphore = VK_NULL_HANDLE;
    VkSemaphore render_semaphore = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    xray::render::vulkan::DdsTexture game_dds;
    std::string game_dds_name;
    xray::render::vulkan::UploadedTexture game_texture;
    xray::render::vulkan::TextureUploadDispatch upload_dispatch;
    std::vector<xray::render::vulkan::PendingTextureUpload> pending_uploads;
    xray::render::vulkan::ImageStateTracker image_states;
    std::vector<VkImageView> image_views;
    std::vector<VkFramebuffer> framebuffers;

    PFN_vkDestroyInstance destroy_instance = nullptr;
    PFN_vkDestroySurfaceKHR destroy_surface = nullptr;
    PFN_vkDestroyDevice destroy_device = nullptr;
    PFN_vkDeviceWaitIdle device_wait_idle = nullptr;
    PFN_vkDestroySwapchainKHR destroy_swapchain = nullptr;
    PFN_vkDestroySemaphore destroy_semaphore = nullptr;
    PFN_vkDestroyCommandPool destroy_command_pool = nullptr;
    PFN_vkDestroyRenderPass destroy_render_pass = nullptr;
    PFN_vkDestroyImageView destroy_image_view = nullptr;
    PFN_vkDestroyFramebuffer destroy_framebuffer = nullptr;

    auto cleanup = [&]
    {
        if (device && device_wait_idle)
            device_wait_idle(device);
        if (device && !pending_uploads.empty())
            xray::render::vulkan::wait_for_uploads(device, command_pool, upload_dispatch, pending_uploads);
        if (device && game_texture.image)
        {
            image_states.forget_image(game_texture.image);
            xray::render::vulkan::destroy_texture(device, upload_dispatch, game_texture);
        }
        if (device && destroy_framebuffer)
            for (VkFramebuffer framebuffer : framebuffers)
                destroy_framebuffer(device, framebuffer, nullptr);
        if (device && render_pass && destroy_render_pass)
            destroy_render_pass(device, render_pass, nullptr);
        if (device && destroy_image_view)
            for (VkImageView view : image_views)
                destroy_image_view(device, view, nullptr);
        if (device && command_pool && destroy_command_pool)
            destroy_command_pool(device, command_pool, nullptr);
        if (device && render_semaphore && destroy_semaphore)
            destroy_semaphore(device, render_semaphore, nullptr);
        if (device && acquire_semaphore && destroy_semaphore)
            destroy_semaphore(device, acquire_semaphore, nullptr);
        if (device && swapchain && destroy_swapchain)
            destroy_swapchain(device, swapchain, nullptr);
        if (device && destroy_device)
            destroy_device(device, nullptr);
        if (instance && surface && destroy_surface)
            destroy_surface(instance, surface, nullptr);
        if (instance && destroy_instance)
            destroy_instance(instance, nullptr);
        if (window)
            SDL_DestroyWindow(window);
        dlclose(vulkan_library);
    };

    auto fail = [&](const std::string& message)
    {
        reason = message;
        Msg("! [renderer-vulkan] %s", reason.c_str());
        cleanup();
        return false;
    };

    const auto get_instance_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        dlsym(vulkan_library, "vkGetInstanceProcAddr"));
    if (!get_instance_proc)
        return fail("vkGetInstanceProcAddr is unavailable");

    const auto get_instance_extensions = reinterpret_cast<GetInstanceExtensions>(
        dlsym(RTLD_DEFAULT, "SDL_Vulkan_GetInstanceExtensions"));
    const auto create_surface = reinterpret_cast<CreateSurface>(
        dlsym(RTLD_DEFAULT, "SDL_Vulkan_CreateSurface"));
    if (!get_instance_extensions || !create_surface)
        return fail("SDL was built without Vulkan window support");

    window = SDL_CreateWindow("OpenXRay Vulkan surface smoke", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 960, 540, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window)
    {
        reason = SDL_GetError();
        return fail(reason);
    }

    unsigned int extension_count = 0;
    if (!get_instance_extensions(window, &extension_count, nullptr) || extension_count == 0)
        return fail("SDL returned no Vulkan instance extensions");

    std::vector<const char*> extensions(extension_count);
    if (!get_instance_extensions(window, &extension_count, extensions.data()))
        return fail("SDL could not enumerate Vulkan instance extensions");
    if (!has_extension(extensions, VK_KHR_SURFACE_EXTENSION_NAME))
        return fail("SDL Vulkan extensions do not include VK_KHR_surface");

    VkApplicationInfo application_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    application_info.pApplicationName = "OpenXRay";
    application_info.applicationVersion = VK_MAKE_VERSION(0, 9, 0);
    application_info.pEngineName = "OpenXRay";
    application_info.engineVersion = VK_MAKE_VERSION(0, 9, 0);
    application_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo = &application_info;
    instance_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instance_info.ppEnabledExtensionNames = extensions.data();

    const auto create_instance = reinterpret_cast<PFN_vkCreateInstance>(
        get_instance_proc(VK_NULL_HANDLE, "vkCreateInstance"));
    if (!create_instance || create_instance(&instance_info, nullptr, &instance) != VK_SUCCESS)
        return fail("vkCreateInstance failed");

    destroy_instance = load_instance_proc<PFN_vkDestroyInstance>(instance, get_instance_proc, "vkDestroyInstance");
    destroy_surface = load_instance_proc<PFN_vkDestroySurfaceKHR>(instance, get_instance_proc, "vkDestroySurfaceKHR");
    const auto get_physical_properties = load_instance_proc<PFN_vkGetPhysicalDeviceProperties>(
        instance, get_instance_proc, "vkGetPhysicalDeviceProperties");
    const auto get_physical_features = load_instance_proc<PFN_vkGetPhysicalDeviceFeatures>(
        instance, get_instance_proc, "vkGetPhysicalDeviceFeatures");
    const auto get_memory_properties = load_instance_proc<PFN_vkGetPhysicalDeviceMemoryProperties>(
        instance, get_instance_proc, "vkGetPhysicalDeviceMemoryProperties");
    const auto get_format_properties = load_instance_proc<PFN_vkGetPhysicalDeviceFormatProperties>(
        instance, get_instance_proc, "vkGetPhysicalDeviceFormatProperties");
    const auto get_surface_capabilities = load_instance_proc<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
        instance, get_instance_proc, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    const auto get_surface_formats = load_instance_proc<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
        instance, get_instance_proc, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    const auto get_present_modes = load_instance_proc<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
        instance, get_instance_proc, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    const auto get_device_proc = load_instance_proc<PFN_vkGetDeviceProcAddr>(
        instance, get_instance_proc, "vkGetDeviceProcAddr");
    if (!destroy_instance || !destroy_surface || !get_surface_capabilities || !get_surface_formats ||
        !get_present_modes || !get_device_proc || !get_physical_properties ||
        !get_physical_features || !get_memory_properties || !get_format_properties)
        return fail("required Vulkan instance procedures are unavailable");

    if (!create_surface(window, instance, &surface))
        return fail("SDL could not create a Vulkan window surface");

    const auto enumerate_physical_devices = load_instance_proc<PFN_vkEnumeratePhysicalDevices>(
        instance, get_instance_proc, "vkEnumeratePhysicalDevices");
    const auto get_queue_families = load_instance_proc<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
        instance, get_instance_proc, "vkGetPhysicalDeviceQueueFamilyProperties");
    const auto get_surface_support = load_instance_proc<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
        instance, get_instance_proc, "vkGetPhysicalDeviceSurfaceSupportKHR");
    const auto enumerate_device_extensions = load_instance_proc<PFN_vkEnumerateDeviceExtensionProperties>(
        instance, get_instance_proc, "vkEnumerateDeviceExtensionProperties");
    xray::render::vulkan::HardwareDispatch hardware_dispatch{
        enumerate_physical_devices, get_queue_families, get_surface_support, enumerate_device_extensions,
        get_physical_properties, get_physical_features, get_memory_properties
    };
    xray::render::vulkan::PhysicalDevice physical_selection;
    std::string hardware_error;
    if (!xray::render::vulkan::select_physical_device(instance, surface, hardware_dispatch,
            physical_selection, hardware_error))
        return fail(hardware_error);
    const VkPhysicalDevice physical_device = physical_selection.handle;
    const uint32_t queue_family = physical_selection.graphics_present_family;
    const VkPhysicalDeviceProperties& physical_properties = physical_selection.properties;
    const VkPhysicalDeviceFeatures& physical_features = physical_selection.features;
    const VkPhysicalDeviceMemoryProperties& memory_properties = physical_selection.memory;
    const VkDeviceSize device_local_bytes = physical_selection.local_memory_bytes;

    const auto format_features = [&](VkFormat format)
    {
        VkFormatProperties properties{};
        if (get_format_properties)
            get_format_properties(physical_device, format, &properties);
        return properties.optimalTilingFeatures;
    };
    const bool rgba8_attachment = format_features(VK_FORMAT_R8G8B8A8_UNORM) &
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    const bool rgba16f_attachment = format_features(VK_FORMAT_R16G16B16A16_SFLOAT) &
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    const bool r32f_attachment = format_features(VK_FORMAT_R32_SFLOAT) &
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    const bool d24s8_attachment = format_features(VK_FORMAT_D24_UNORM_S8_UINT) &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    const bool d32s8_attachment = format_features(VK_FORMAT_D32_SFLOAT_S8_UINT) &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    Msg("[renderer-vulkan] device='%s' api=%u.%u.%u driver=0x%x vendor=0x%x device=0x%x queue=%u",
        physical_properties.deviceName,
        VK_VERSION_MAJOR(physical_properties.apiVersion), VK_VERSION_MINOR(physical_properties.apiVersion),
        VK_VERSION_PATCH(physical_properties.apiVersion), physical_properties.driverVersion,
        physical_properties.vendorID, physical_properties.deviceID, queue_family);
    Msg("[renderer-vulkan] limits: image2D=%u colorAttachments=%u samplers/stage=%u pushConstants=%u localMemory=%lluMiB",
        physical_properties.limits.maxImageDimension2D, physical_properties.limits.maxColorAttachments,
        physical_properties.limits.maxPerStageDescriptorSamplers,
        physical_properties.limits.maxPushConstantsSize,
        static_cast<unsigned long long>(device_local_bytes / (1024ull * 1024ull)));
    Msg("[renderer-vulkan] features: anisotropy=%u BC=%u ETC2=%u ASTC_LDR=%u geometry=%u tessellation=%u",
        physical_features.samplerAnisotropy, physical_features.textureCompressionBC,
        physical_features.textureCompressionETC2, physical_features.textureCompressionASTC_LDR,
        physical_features.geometryShader, physical_features.tessellationShader);
    Msg("[renderer-vulkan] attachment formats: RGBA8=%u RGBA16F=%u R32F=%u D24S8=%u D32S8=%u",
        rgba8_attachment, rgba16f_attachment, r32f_attachment, d24s8_attachment, d32s8_attachment);

    if (FS.get_path("$game_textures$"))
    {
        FS_FileSet files;
        FS.file_list(files, "$game_textures$", FS_ListFiles | FS_RootOnly, "*.dds");
        for (const auto& file : files)
        {
            string_path path;
            FS.update_path(path, "$game_textures$", file.name.c_str());
            IReader* reader = FS.r_open(path);
            if (!reader)
                continue;
            xray::render::vulkan::DdsTexture texture;
            std::string decode_error;
            const bool decoded = xray::render::vulkan::decode_dds(reader->pointer(), reader->length(),
                physical_features.textureCompressionBC != VK_FALSE, texture, decode_error);
            FS.r_close(reader);
            if (!decoded)
                continue;
            const bool sampled = (format_features(texture.format) & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
            Msg("[renderer-vulkan] game DDS '%s': %ux%u, %zu mips, format=%u, sampled=%u",
                file.name.c_str(), texture.extent.width, texture.extent.height, texture.copies.size(),
                static_cast<unsigned int>(texture.format), sampled);
            if (sampled && texture.pixels.size() <= 8u * 1024u * 1024u &&
                texture.extent.width <= physical_properties.limits.maxImageDimension2D &&
                texture.extent.height <= physical_properties.limits.maxImageDimension2D)
            {
                game_dds = std::move(texture);
                game_dds_name = file.name.c_str();
                break;
            }
        }
    }

    const auto create_device = load_instance_proc<PFN_vkCreateDevice>(instance, get_instance_proc, "vkCreateDevice");
    xray::render::vulkan::DeviceDispatch device_dispatch{create_device, get_device_proc};
    std::string device_error;
    if (!xray::render::vulkan::create_logical_device(physical_selection, device_dispatch,
            device, queue, device_error))
        return fail(device_error);

    destroy_device = load_device_proc<PFN_vkDestroyDevice>(device, get_device_proc, "vkDestroyDevice");
    device_wait_idle = load_device_proc<PFN_vkDeviceWaitIdle>(device, get_device_proc, "vkDeviceWaitIdle");
    const auto create_swapchain = load_device_proc<PFN_vkCreateSwapchainKHR>(device, get_device_proc, "vkCreateSwapchainKHR");
    const auto get_swapchain_images = load_device_proc<PFN_vkGetSwapchainImagesKHR>(device, get_device_proc, "vkGetSwapchainImagesKHR");
    const auto acquire_next_image = load_device_proc<PFN_vkAcquireNextImageKHR>(device, get_device_proc, "vkAcquireNextImageKHR");
    const auto queue_present = load_device_proc<PFN_vkQueuePresentKHR>(device, get_device_proc, "vkQueuePresentKHR");
    destroy_swapchain = load_device_proc<PFN_vkDestroySwapchainKHR>(device, get_device_proc, "vkDestroySwapchainKHR");
    const auto create_semaphore = load_device_proc<PFN_vkCreateSemaphore>(device, get_device_proc, "vkCreateSemaphore");
    destroy_semaphore = load_device_proc<PFN_vkDestroySemaphore>(device, get_device_proc, "vkDestroySemaphore");
    const auto create_image_view = load_device_proc<PFN_vkCreateImageView>(device, get_device_proc, "vkCreateImageView");
    destroy_image_view = load_device_proc<PFN_vkDestroyImageView>(device, get_device_proc, "vkDestroyImageView");
    const auto create_render_pass = load_device_proc<PFN_vkCreateRenderPass>(device, get_device_proc, "vkCreateRenderPass");
    destroy_render_pass = load_device_proc<PFN_vkDestroyRenderPass>(device, get_device_proc, "vkDestroyRenderPass");
    const auto create_framebuffer = load_device_proc<PFN_vkCreateFramebuffer>(device, get_device_proc, "vkCreateFramebuffer");
    destroy_framebuffer = load_device_proc<PFN_vkDestroyFramebuffer>(device, get_device_proc, "vkDestroyFramebuffer");
    const auto create_command_pool = load_device_proc<PFN_vkCreateCommandPool>(device, get_device_proc, "vkCreateCommandPool");
    destroy_command_pool = load_device_proc<PFN_vkDestroyCommandPool>(device, get_device_proc, "vkDestroyCommandPool");
    const auto allocate_command_buffers = load_device_proc<PFN_vkAllocateCommandBuffers>(device, get_device_proc, "vkAllocateCommandBuffers");
    const auto begin_command_buffer = load_device_proc<PFN_vkBeginCommandBuffer>(device, get_device_proc, "vkBeginCommandBuffer");
    const auto cmd_begin_render_pass = load_device_proc<PFN_vkCmdBeginRenderPass>(device, get_device_proc, "vkCmdBeginRenderPass");
    const auto cmd_end_render_pass = load_device_proc<PFN_vkCmdEndRenderPass>(device, get_device_proc, "vkCmdEndRenderPass");
    const auto end_command_buffer = load_device_proc<PFN_vkEndCommandBuffer>(device, get_device_proc, "vkEndCommandBuffer");
    const auto queue_submit = load_device_proc<PFN_vkQueueSubmit>(device, get_device_proc, "vkQueueSubmit");
    if (!destroy_device || !device_wait_idle || !create_swapchain || !get_swapchain_images ||
        !acquire_next_image || !queue_present || !destroy_swapchain || !create_semaphore || !destroy_semaphore ||
        !create_image_view || !destroy_image_view || !create_render_pass || !destroy_render_pass ||
        !create_framebuffer || !destroy_framebuffer || !create_command_pool || !destroy_command_pool ||
        !allocate_command_buffers || !begin_command_buffer || !cmd_begin_render_pass || !cmd_end_render_pass ||
        !end_command_buffer || !queue_submit)
        return fail("required Vulkan device procedures are unavailable");

    VkSurfaceCapabilitiesKHR capabilities{};
    if (get_surface_capabilities(physical_device, surface, &capabilities) != VK_SUCCESS)
        return fail("could not query Vulkan surface capabilities");

    uint32_t format_count = 0;
    if (get_surface_formats(physical_device, surface, &format_count, nullptr) != VK_SUCCESS || format_count == 0)
        return fail("Vulkan surface has no supported formats");
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    get_surface_formats(physical_device, surface, &format_count, formats.data());

    uint32_t present_mode_count = 0;
    if (get_present_modes(physical_device, surface, &present_mode_count, nullptr) != VK_SUCCESS || present_mode_count == 0)
        return fail("Vulkan surface has no present modes");
    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    get_present_modes(physical_device, surface, &present_mode_count, present_modes.data());

    VkExtent2D extent = capabilities.currentExtent;
    if (extent.width == std::numeric_limits<uint32_t>::max())
    {
        extent.width = 960;
        extent.height = 540;
        extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (!(capabilities.supportedCompositeAlpha & composite_alpha))
    {
        const VkCompositeAlphaFlagBitsKHR alternatives[] = {
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        };
        for (VkCompositeAlphaFlagBitsKHR candidate : alternatives)
            if (capabilities.supportedCompositeAlpha & candidate)
            {
                composite_alpha = candidate;
                break;
            }
    }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    if (std::find(present_modes.begin(), present_modes.end(), present_mode) == present_modes.end())
        present_mode = present_modes.front();

    uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount && image_count > capabilities.maxImageCount)
        image_count = capabilities.maxImageCount;
    VkSwapchainCreateInfoKHR swapchain_info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_info.surface = surface;
    swapchain_info.minImageCount = image_count;
    swapchain_info.imageFormat = formats.front().format;
    swapchain_info.imageColorSpace = formats.front().colorSpace;
    swapchain_info.imageExtent = extent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.preTransform = capabilities.currentTransform;
    swapchain_info.compositeAlpha = composite_alpha;
    swapchain_info.presentMode = present_mode;
    swapchain_info.clipped = VK_TRUE;
    if (create_swapchain(device, &swapchain_info, nullptr, &swapchain) != VK_SUCCESS)
        return fail("vkCreateSwapchainKHR failed");

    uint32_t swapchain_image_count = 0;
    if (get_swapchain_images(device, swapchain, &swapchain_image_count, nullptr) != VK_SUCCESS || swapchain_image_count == 0)
        return fail("Vulkan swapchain has no images");
    std::vector<VkImage> images(swapchain_image_count);
    if (get_swapchain_images(device, swapchain, &swapchain_image_count, images.data()) != VK_SUCCESS)
        return fail("could not enumerate Vulkan swapchain images");

    // A genuine Vulkan frame: render pass owns the color attachment and
    // transitions it to PRESENT_SRC before the presentation queue sees it.
    VkAttachmentDescription color_attachment{};
    color_attachment.format = formats.front().format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference color_reference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo pass_info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    pass_info.attachmentCount = 1;
    pass_info.pAttachments = &color_attachment;
    pass_info.subpassCount = 1;
    pass_info.pSubpasses = &subpass;
    pass_info.dependencyCount = 1;
    pass_info.pDependencies = &dependency;
    if (create_render_pass(device, &pass_info, nullptr, &render_pass) != VK_SUCCESS)
        return fail("vkCreateRenderPass failed");

    image_views.reserve(swapchain_image_count);
    framebuffers.reserve(swapchain_image_count);
    for (VkImage image : images)
    {
        VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.image = image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = formats.front().format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.layerCount = 1;
        VkImageView view = VK_NULL_HANDLE;
        if (create_image_view(device, &view_info, nullptr, &view) != VK_SUCCESS)
            return fail("vkCreateImageView failed");
        image_views.push_back(view);

        VkFramebufferCreateInfo framebuffer_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = &image_views.back();
        framebuffer_info.width = extent.width;
        framebuffer_info.height = extent.height;
        framebuffer_info.layers = 1;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        if (create_framebuffer(device, &framebuffer_info, nullptr, &framebuffer) != VK_SUCCESS)
            return fail("vkCreateFramebuffer failed");
        framebuffers.push_back(framebuffer);
    }

    VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.queueFamilyIndex = queue_family;
    if (create_command_pool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS)
        return fail("vkCreateCommandPool failed");

#define XRAY_LOAD_UPLOAD(member, name) \
    upload_dispatch.member = load_device_proc<decltype(upload_dispatch.member)>(device, get_device_proc, name)
    XRAY_LOAD_UPLOAD(create_buffer, "vkCreateBuffer");
    XRAY_LOAD_UPLOAD(destroy_buffer, "vkDestroyBuffer");
    XRAY_LOAD_UPLOAD(get_buffer_memory_requirements, "vkGetBufferMemoryRequirements");
    XRAY_LOAD_UPLOAD(create_image, "vkCreateImage");
    XRAY_LOAD_UPLOAD(destroy_image, "vkDestroyImage");
    XRAY_LOAD_UPLOAD(get_image_memory_requirements, "vkGetImageMemoryRequirements");
    XRAY_LOAD_UPLOAD(allocate_memory, "vkAllocateMemory");
    XRAY_LOAD_UPLOAD(free_memory, "vkFreeMemory");
    XRAY_LOAD_UPLOAD(bind_buffer_memory, "vkBindBufferMemory");
    XRAY_LOAD_UPLOAD(bind_image_memory, "vkBindImageMemory");
    XRAY_LOAD_UPLOAD(map_memory, "vkMapMemory");
    XRAY_LOAD_UPLOAD(unmap_memory, "vkUnmapMemory");
    XRAY_LOAD_UPLOAD(create_image_view, "vkCreateImageView");
    XRAY_LOAD_UPLOAD(destroy_image_view, "vkDestroyImageView");
    XRAY_LOAD_UPLOAD(allocate_command_buffers, "vkAllocateCommandBuffers");
    XRAY_LOAD_UPLOAD(free_command_buffers, "vkFreeCommandBuffers");
    XRAY_LOAD_UPLOAD(begin_command_buffer, "vkBeginCommandBuffer");
    XRAY_LOAD_UPLOAD(end_command_buffer, "vkEndCommandBuffer");
    XRAY_LOAD_UPLOAD(cmd_pipeline_barrier, "vkCmdPipelineBarrier");
    XRAY_LOAD_UPLOAD(cmd_copy_buffer_to_image, "vkCmdCopyBufferToImage");
    XRAY_LOAD_UPLOAD(queue_submit, "vkQueueSubmit");
    XRAY_LOAD_UPLOAD(create_fence, "vkCreateFence");
    XRAY_LOAD_UPLOAD(destroy_fence, "vkDestroyFence");
    XRAY_LOAD_UPLOAD(get_fence_status, "vkGetFenceStatus");
    XRAY_LOAD_UPLOAD(wait_for_fences, "vkWaitForFences");
#undef XRAY_LOAD_UPLOAD
    if (!upload_dispatch.create_buffer || !upload_dispatch.destroy_buffer ||
        !upload_dispatch.get_buffer_memory_requirements || !upload_dispatch.create_image ||
        !upload_dispatch.destroy_image || !upload_dispatch.get_image_memory_requirements ||
        !upload_dispatch.allocate_memory || !upload_dispatch.free_memory ||
        !upload_dispatch.bind_buffer_memory || !upload_dispatch.bind_image_memory ||
        !upload_dispatch.map_memory || !upload_dispatch.unmap_memory ||
        !upload_dispatch.create_image_view || !upload_dispatch.destroy_image_view ||
        !upload_dispatch.allocate_command_buffers || !upload_dispatch.free_command_buffers ||
        !upload_dispatch.begin_command_buffer || !upload_dispatch.end_command_buffer ||
        !upload_dispatch.cmd_pipeline_barrier || !upload_dispatch.cmd_copy_buffer_to_image ||
        !upload_dispatch.queue_submit || !upload_dispatch.create_fence || !upload_dispatch.destroy_fence ||
        !upload_dispatch.get_fence_status || !upload_dispatch.wait_for_fences)
        return fail("Vulkan image upload procedures are unavailable");

    if (!game_dds.pixels.empty())
    {
        std::string upload_error;
        if (!xray::render::vulkan::upload_texture(device, queue, command_pool, memory_properties,
                upload_dispatch, game_dds, game_texture, pending_uploads, image_states, upload_error))
            return fail("game DDS Vulkan upload failed: " + upload_error);
        Msg("[renderer-vulkan] game DDS uploaded to sampled GPU image: '%s'", game_dds_name.c_str());
    }
    std::vector<VkCommandBuffer> commands(swapchain_image_count);
    VkCommandBufferAllocateInfo allocate_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocate_info.commandPool = command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = swapchain_image_count;
    if (allocate_command_buffers(device, &allocate_info, commands.data()) != VK_SUCCESS)
        return fail("vkAllocateCommandBuffers failed");

    for (uint32_t index = 0; index < swapchain_image_count; ++index)
    {
        VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        if (begin_command_buffer(commands[index], &begin_info) != VK_SUCCESS)
            return fail("vkBeginCommandBuffer failed");
        VkClearValue clear{};
        clear.color.float32[0] = 0.08f;
        clear.color.float32[1] = 0.18f;
        clear.color.float32[2] = 0.32f;
        clear.color.float32[3] = 1.0f;
        VkRenderPassBeginInfo render_info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        render_info.renderPass = render_pass;
        render_info.framebuffer = framebuffers[index];
        render_info.renderArea.extent = extent;
        render_info.clearValueCount = 1;
        render_info.pClearValues = &clear;
        cmd_begin_render_pass(commands[index], &render_info, VK_SUBPASS_CONTENTS_INLINE);
        cmd_end_render_pass(commands[index]);
        if (end_command_buffer(commands[index]) != VK_SUCCESS)
            return fail("vkEndCommandBuffer failed");
    }

    VkSemaphoreCreateInfo semaphore_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    if (create_semaphore(device, &semaphore_info, nullptr, &acquire_semaphore) != VK_SUCCESS ||
        create_semaphore(device, &semaphore_info, nullptr, &render_semaphore) != VK_SUCCESS)
        return fail("vkCreateSemaphore failed");

    uint32_t image_index = 0;
    const VkResult acquire_result = acquire_next_image(device, swapchain, UINT64_MAX, acquire_semaphore,
        VK_NULL_HANDLE, &image_index);
    if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR)
        return fail("vkAcquireNextImageKHR failed");

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &acquire_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &commands[image_index];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &render_semaphore;
    if (queue_submit(queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
        return fail("vkQueueSubmit failed");

    VkPresentInfoKHR present_info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &image_index;
    const VkResult present_result = queue_present(queue, &present_info);
    if (present_result != VK_SUCCESS && present_result != VK_SUBOPTIMAL_KHR)
        return fail("vkQueuePresentKHR failed");
    if (device_wait_idle(device) != VK_SUCCESS)
        return fail("Vulkan queue did not become idle after present");

    Msg("[renderer-vulkan] render-pass clear and present PASS: %s, Vulkan %u.%u.%u",
        physical_properties.deviceName,
        VK_VERSION_MAJOR(physical_properties.apiVersion), VK_VERSION_MINOR(physical_properties.apiVersion),
        VK_VERSION_PATCH(physical_properties.apiVersion));
    reason = "Vulkan command buffer, render-pass clear, queue submit and present passed";
    cleanup();
    return true;
#endif
}
}

#endif
