#include "stdafx.h"

#if defined(XR_PLATFORM_ANDROID)

#include "android_vulkan_smoke.h"
#include "../Layers/xrRenderVK/FrameContext.h"
#include "../Layers/xrRenderVK/VulkanHardware.h"

#include <SDL.h>

#if __has_include(<vulkan/vulkan.h>)
#define XRAY_ANDROID_HAS_VULKAN_HEADERS 1
#include <vulkan/vulkan.h>
#endif

#include <dlfcn.h>

#include <algorithm>
#include <cstring>
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

struct FrameCallbackAudit
{
    uint32_t calls = 0;
    bool valid = true;
};

void audit_frame_callback(const xray::render::vulkan::FrameRecordingContext& frame, void* user_data)
{
    auto& audit = *static_cast<FrameCallbackAudit*>(user_data);
    audit.valid = audit.valid && frame.command_buffer && frame.render_pass && frame.framebuffer &&
        frame.extent.width && frame.extent.height &&
        frame.frame_index < xray::render::vulkan::FrameContext::FramesInFlight;
    ++audit.calls;
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
    xray::render::vulkan::FrameContext frame_context;

    PFN_vkDestroyInstance destroy_instance = nullptr;
    PFN_vkDestroySurfaceKHR destroy_surface = nullptr;
    PFN_vkDestroyDevice destroy_device = nullptr;

    auto cleanup = [&]
    {
        frame_context.destroy();
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

    VkApplicationInfo application_info{};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = "OpenXRay";
    application_info.applicationVersion = VK_MAKE_VERSION(0, 9, 0);
    application_info.pEngineName = "OpenXRay";
    application_info.engineVersion = VK_MAKE_VERSION(0, 9, 0);
    application_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
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
    const auto get_device_proc = load_instance_proc<PFN_vkGetDeviceProcAddr>(
        instance, get_instance_proc, "vkGetDeviceProcAddr");
    if (!destroy_instance || !destroy_surface || !get_device_proc || !get_physical_properties ||
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

    const auto create_device = load_instance_proc<PFN_vkCreateDevice>(instance, get_instance_proc, "vkCreateDevice");
    xray::render::vulkan::DeviceDispatch device_dispatch{create_device, get_device_proc};
    std::string device_error;
    if (!xray::render::vulkan::create_logical_device(physical_selection, device_dispatch,
            device, queue, device_error))
        return fail(device_error);

    destroy_device = load_device_proc<PFN_vkDestroyDevice>(device, get_device_proc, "vkDestroyDevice");
    xray::render::vulkan::FrameDispatch frame_dispatch;
    std::string frame_error;
    if (!destroy_device || !xray::render::vulkan::load_frame_dispatch(instance, get_instance_proc,
            device, get_device_proc, frame_dispatch, frame_error))
        return fail(frame_error.empty() ? "vkDestroyDevice is unavailable" : frame_error);
    if (!frame_context.initialize(physical_device, device, surface, queue, queue_family, {960, 540},
            frame_dispatch, frame_error))
        return fail(frame_error);

    VkClearColorValue clear{};
    clear.float32[0] = 0.08f;
    clear.float32[1] = 0.18f;
    clear.float32[2] = 0.32f;
    clear.float32[3] = 1.0f;
    FrameCallbackAudit callback_audit;
    for (uint32_t frame = 0; frame <= xray::render::vulkan::FrameContext::FramesInFlight; ++frame)
    {
        xray::render::vulkan::FrameStatus frame_status{};
        if (!frame_context.render_frame(clear, frame_status, frame_error,
                audit_frame_callback, &callback_audit))
            return fail(frame_error);
        if (frame_status != xray::render::vulkan::FrameStatus::Presented)
            return fail("Vulkan surface changed during the smoke test");
        if (!callback_audit.valid || callback_audit.calls != frame + 1)
            return fail("Vulkan frame recorder received an invalid frame context");
    }
    if (frame_dispatch.device_wait_idle(device) != VK_SUCCESS)
        return fail("Vulkan queue did not become idle after present");

    Msg("[renderer-vulkan] render-pass clear and present PASS: %s, Vulkan %u.%u.%u",
        physical_properties.deviceName,
        VK_VERSION_MAJOR(physical_properties.apiVersion), VK_VERSION_MINOR(physical_properties.apiVersion),
        VK_VERSION_PATCH(physical_properties.apiVersion));
    reason = "three Vulkan frame-context submits and presents passed";
    cleanup();
    return true;
#endif
}
}

#endif
