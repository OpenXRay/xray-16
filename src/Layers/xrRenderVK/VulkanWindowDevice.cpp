#include "xrCore/stdafx.h"
#include "VulkanWindowDevice.h"
#include "xrCore/log.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

namespace xray::render::vulkan
{
namespace
{
template <typename T> T load_instance_proc(VkInstance instance, PFN_vkGetInstanceProcAddr get, const char* name)
{
    return reinterpret_cast<T>(get(instance, name));
}
template <typename T> bool contains_name(const char* expected, const std::vector<T>& items)
{
    return std::any_of(items.begin(), items.end(), [&](const auto& item)
        { return std::strcmp(expected, item.extensionName) == 0; });
}
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanWindowDevice::validation_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* data, void* user)
{
    auto* owner = static_cast<VulkanWindowDevice*>(user);
    const char* id = data && data->pMessageIdName ? data->pMessageIdName : "Vulkan";
    const char* message = data && data->pMessage ? data->pMessage : "(no message)";
    std::lock_guard<std::mutex> lock(owner->m_validation_mutex);
    const std::string key = std::string(id) + ": " + message;
    const unsigned repeats = ++owner->m_validation_repeats[key];
    // Preserve the first warning and exponentially spaced occurrence counts.
    // Per-draw driver hints otherwise bury errors and crash context in logs.
    if ((repeats & (repeats - 1)) != 0)
        return VK_FALSE;
    Msg("! [renderer-vulkan] validation occurrence=%u %s: %s", repeats, id, message);
    if (owner->m_validation_log)
    {
        std::fprintf(owner->m_validation_log, "occurrence=%u %s: %s\n", repeats, id, message);
        std::fflush(owner->m_validation_log);
    }
    return VK_FALSE;
}

bool VulkanWindowDevice::initialize(SDL_Window* window, VkExtent2D extent,
    bool allow_readback, std::string& error, bool use_depth, bool preserve_prepass_depth,
    bool postprocess)
{
    destroy();
    if (!window || !extent.width || !extent.height)
    {
        error = "Vulkan device requires an SDL window and a nonzero extent";
        return false;
    }
    m_window = window;
    const auto fail = [&]() { destroy(); return false; };
    // SDL loads the platform Vulkan loader for SDL_WINDOW_VULKAN windows and
    // selects the matching Win32/X11/Wayland/Android surface implementation.
    m_instance_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
    if (!m_instance_proc)
    {
        error = "SDL has no Vulkan loader for this window";
        return fail();
    }
    unsigned int extension_count = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr) || !extension_count)
    {
        error = "SDL returned no Vulkan instance extensions";
        return fail();
    }
    std::vector<const char*> extensions(extension_count);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data()))
    {
        error = "SDL could not enumerate Vulkan instance extensions";
        return fail();
    }
    extensions.resize(extension_count);
    if (std::none_of(extensions.begin(), extensions.end(), [](const char* name)
        { return std::strcmp(name, VK_KHR_SURFACE_EXTENSION_NAME) == 0; }))
    {
        error = "SDL Vulkan extensions do not include VK_KHR_surface";
        return fail();
    }
    std::vector<const char*> layers_enabled;
    VkValidationFeatureEnableEXT sync_feature = VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
    VkValidationFeaturesEXT validation_features{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
    // Android's debug-signed APK uses ReleaseMasterGold native code. Keep the
    // explicit opt-in available there; without a packaged layer it is inert.
    const char* requested = SDL_getenv("XRAY_VK_VALIDATION");
    if (requested && std::strcmp(requested, "0") != 0)
    {
        const auto enumerate_layers = load_instance_proc<PFN_vkEnumerateInstanceLayerProperties>(
            VK_NULL_HANDLE, m_instance_proc, "vkEnumerateInstanceLayerProperties");
        const auto enumerate_extensions = load_instance_proc<PFN_vkEnumerateInstanceExtensionProperties>(
            VK_NULL_HANDLE, m_instance_proc, "vkEnumerateInstanceExtensionProperties");
        uint32_t count = 0;
        if (enumerate_layers && enumerate_layers(&count, nullptr) == VK_SUCCESS)
        {
            std::vector<VkLayerProperties> available(count);
            if (enumerate_layers(&count, available.data()) == VK_SUCCESS &&
                std::any_of(available.begin(), available.end(), [](const auto& layer)
                { return std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0; }))
            {
                layers_enabled.push_back("VK_LAYER_KHRONOS_validation");
                const auto collect_extensions = [&](const char* layer)
                {
                    uint32_t extension_count = 0;
                    if (!enumerate_extensions || enumerate_extensions(layer, &extension_count, nullptr) != VK_SUCCESS)
                        return std::vector<VkExtensionProperties>{};
                    std::vector<VkExtensionProperties> result(extension_count);
                    if (enumerate_extensions(layer, &extension_count, result.data()) != VK_SUCCESS)
                        result.clear();
                    return result;
                };
                const auto global_extensions = collect_extensions(nullptr);
                const auto layer_extensions = collect_extensions("VK_LAYER_KHRONOS_validation");
                const auto available = [&](const char* name)
                { return contains_name(name, global_extensions) || contains_name(name, layer_extensions); };
                if (available(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
                    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                if (available(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME))
                {
                    extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
                    validation_features.enabledValidationFeatureCount = 1;
                    validation_features.pEnabledValidationFeatures = &sync_feature;
                }
            }
        }
        SDL_Log("[renderer-vulkan] validation %s",
            layers_enabled.empty() ? "requested but VK_LAYER_KHRONOS_validation unavailable" : "enabled");
    }
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = app.pEngineName = "OpenXRay";
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo = &app;
    instance_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instance_info.ppEnabledExtensionNames = extensions.data();
    instance_info.enabledLayerCount = static_cast<uint32_t>(layers_enabled.size());
    instance_info.ppEnabledLayerNames = layers_enabled.data();
    if (validation_features.enabledValidationFeatureCount)
        instance_info.pNext = &validation_features;
    const auto create_instance = load_instance_proc<PFN_vkCreateInstance>(VK_NULL_HANDLE,
        m_instance_proc, "vkCreateInstance");
    if (!create_instance || create_instance(&instance_info, nullptr, &m_instance) != VK_SUCCESS)
    {
        error = "vkCreateInstance failed";
        return fail();
    }
    m_destroy_instance = load_instance_proc<PFN_vkDestroyInstance>(m_instance, m_instance_proc,
        "vkDestroyInstance");
    m_destroy_surface = load_instance_proc<PFN_vkDestroySurfaceKHR>(m_instance, m_instance_proc,
        "vkDestroySurfaceKHR");
    m_device_proc = load_instance_proc<PFN_vkGetDeviceProcAddr>(m_instance, m_instance_proc,
        "vkGetDeviceProcAddr");
    if (!m_destroy_instance || !m_destroy_surface || !m_device_proc)
    {
        error = "required Vulkan instance procedures are unavailable";
        return fail();
    }
    if (!layers_enabled.empty() && std::any_of(extensions.begin(), extensions.end(),
            [](const char* name) { return std::strcmp(name, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0; }))
    {
        const auto create = load_instance_proc<PFN_vkCreateDebugUtilsMessengerEXT>(m_instance,
            m_instance_proc, "vkCreateDebugUtilsMessengerEXT");
        m_destroy_debug_messenger = load_instance_proc<PFN_vkDestroyDebugUtilsMessengerEXT>(m_instance,
            m_instance_proc, "vkDestroyDebugUtilsMessengerEXT");
        VkDebugUtilsMessengerCreateInfoEXT messenger{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        messenger.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        messenger.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        messenger.pfnUserCallback = validation_callback;
        messenger.pUserData = this;
        if (create && m_destroy_debug_messenger)
            create(m_instance, &messenger, nullptr, &m_debug_messenger);
        char* log_dir = SDL_GetPrefPath("OpenXRay", "validation");
        if (log_dir)
        {
            const std::string path = std::string(log_dir) + "vulkan-vuid.log";
            m_validation_log = std::fopen(path.c_str(), "a");
            SDL_free(log_dir);
        }
    }
    if (!SDL_Vulkan_CreateSurface(window, m_instance, &m_surface))
    {
        error = "SDL could not create a Vulkan window surface";
        return fail();
    }
    HardwareDispatch hardware{
        load_instance_proc<PFN_vkEnumeratePhysicalDevices>(m_instance, m_instance_proc, "vkEnumeratePhysicalDevices"),
        load_instance_proc<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(m_instance, m_instance_proc,
            "vkGetPhysicalDeviceQueueFamilyProperties"),
        load_instance_proc<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(m_instance, m_instance_proc,
            "vkGetPhysicalDeviceSurfaceSupportKHR"),
        load_instance_proc<PFN_vkEnumerateDeviceExtensionProperties>(m_instance, m_instance_proc,
            "vkEnumerateDeviceExtensionProperties"),
        load_instance_proc<PFN_vkGetPhysicalDeviceProperties>(m_instance, m_instance_proc,
            "vkGetPhysicalDeviceProperties"),
        load_instance_proc<PFN_vkGetPhysicalDeviceFeatures>(m_instance, m_instance_proc,
            "vkGetPhysicalDeviceFeatures"),
        load_instance_proc<PFN_vkGetPhysicalDeviceMemoryProperties>(m_instance, m_instance_proc,
            "vkGetPhysicalDeviceMemoryProperties"),
        load_instance_proc<PFN_vkGetPhysicalDeviceFormatProperties>(m_instance, m_instance_proc,
            "vkGetPhysicalDeviceFormatProperties")
    };
    if (!select_physical_device(m_instance, m_surface, hardware, m_physical, error))
        return fail();
    DeviceDispatch devices{load_instance_proc<PFN_vkCreateDevice>(m_instance, m_instance_proc,
        "vkCreateDevice"), m_device_proc};
    if (!create_logical_device(m_physical, devices, m_device, m_queue, error))
        return fail();
    SDL_Log("[renderer-vulkan] gpu.selected name='%s' vendor=0x%04x device=0x%04x driver=0x%x api=%u.%u queue=%u",
        m_physical.properties.deviceName, m_physical.properties.vendorID, m_physical.properties.deviceID,
        m_physical.properties.driverVersion, VK_VERSION_MAJOR(m_physical.properties.apiVersion),
        VK_VERSION_MINOR(m_physical.properties.apiVersion), m_physical.graphics_present_family);
    m_destroy_device = reinterpret_cast<PFN_vkDestroyDevice>(m_device_proc(m_device, "vkDestroyDevice"));
    FrameDispatch frame_dispatch;
    if (!m_destroy_device || !load_frame_dispatch(m_instance, m_instance_proc, m_device,
            m_device_proc, frame_dispatch, error))
    {
        if (error.empty()) error = "vkDestroyDevice is unavailable";
        return fail();
    }
    m_wait_idle = frame_dispatch.device_wait_idle;
    uint32_t timestamp_bits = 0;
    const auto queue_properties = load_instance_proc<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
        m_instance, m_instance_proc, "vkGetPhysicalDeviceQueueFamilyProperties");
    if (queue_properties)
    {
        uint32_t count = 0;
        queue_properties(m_physical.handle, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        if (count) queue_properties(m_physical.handle, &count, families.data());
        if (m_physical.graphics_present_family < count)
            timestamp_bits = families[m_physical.graphics_present_family].timestampValidBits;
    }
    if (!m_frame.initialize(m_physical.handle, m_device, m_surface, m_queue,
            m_physical.graphics_present_family, extent, frame_dispatch, error,
            allow_readback, use_depth, preserve_prepass_depth, postprocess,
            m_physical.properties.limits.timestampPeriod, timestamp_bits))
        return fail();
    SDL_Log("[renderer-vulkan] swapchain.ready extent=%ux%u images=%u format=%d timestamps=%u bits",
        m_frame.extent().width, m_frame.extent().height, m_frame.image_count(),
        static_cast<int>(m_frame.format()), timestamp_bits);
    error.clear();
    return true;
}

bool VulkanWindowDevice::recreate_frame(VkExtent2D extent, std::string& error)
{
    if (!m_device || !m_surface || !m_queue || !m_instance || !m_instance_proc || !m_device_proc ||
        !extent.width || !extent.height)
    {
        error = "Vulkan window device cannot recreate a frame for an invalid window or extent";
        return false;
    }
    if (!m_frame.recreate(m_physical.handle, m_surface, extent, error))
        return false;
    error.clear();
    return true;
}

bool VulkanWindowDevice::recreate_surface(VkExtent2D extent, std::string& error)
{
    SDL_Log("[renderer-vulkan] surface.recreate begin extent=%ux%u", extent.width, extent.height);
    if (!m_window || !m_instance || !m_device ||
        !m_physical.handle || !extent.width || !extent.height)
    {
        error = "Vulkan surface recreation requires a live window, device and drawable extent";
        return false;
    }
    // The native Android window may only be associated with one live Vulkan
    // surface/swapchain. Retire the old chain and surface before creating its
    // replacement from the resumed SDL window.
    const VkSurfaceKHR previous = m_surface;
    if (!m_frame.release_swapchain())
    {
        error = "could not release the old Vulkan swapchain before surface recreation";
        return false;
    }
    if (previous && m_destroy_surface)
        m_destroy_surface(m_instance, previous, nullptr);
    m_surface = VK_NULL_HANDLE;

    VkSurfaceKHR replacement = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(m_window, m_instance, &replacement) || !replacement)
    {
        error = "SDL could not create a replacement Vulkan surface";
        return false;
    }
    m_surface = replacement;
    const auto get_surface_support = load_instance_proc<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
        m_instance, m_instance_proc, "vkGetPhysicalDeviceSurfaceSupportKHR");
    VkBool32 supported = VK_FALSE;
    if (!get_surface_support || get_surface_support(m_physical.handle,
            m_physical.graphics_present_family, replacement, &supported) != VK_SUCCESS || !supported)
    {
        if (m_destroy_surface)
            m_destroy_surface(m_instance, replacement, nullptr);
        m_surface = VK_NULL_HANDLE;
        error = "selected Vulkan queue cannot present to the replacement surface";
        return false;
    }

    if (!m_frame.recreate(m_physical.handle, replacement, extent, error))
        return false;
    error.clear();
    return true;
}

void VulkanWindowDevice::destroy()
{
    if (m_device && m_wait_idle)
        m_wait_idle(m_device);
    m_frame.destroy();
    if (m_device && m_destroy_device)
        m_destroy_device(m_device, nullptr);
    if (m_surface && m_instance && m_destroy_surface)
        m_destroy_surface(m_instance, m_surface, nullptr);
    if (m_debug_messenger && m_destroy_debug_messenger)
        m_destroy_debug_messenger(m_instance, m_debug_messenger, nullptr);
    m_debug_messenger = VK_NULL_HANDLE;
    m_destroy_debug_messenger = nullptr;
    if (m_validation_log)
        std::fclose(m_validation_log);
    m_validation_log = nullptr;
    m_validation_repeats.clear();
    if (m_instance && m_destroy_instance)
        m_destroy_instance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;
    m_surface = VK_NULL_HANDLE;
    m_device = VK_NULL_HANDLE;
    m_queue = VK_NULL_HANDLE;
    m_physical = {};
    m_instance_proc = nullptr;
    m_device_proc = nullptr;
    m_destroy_instance = nullptr;
    m_destroy_surface = nullptr;
    m_destroy_device = nullptr;
    m_wait_idle = nullptr;
    m_window = nullptr;
}
}
