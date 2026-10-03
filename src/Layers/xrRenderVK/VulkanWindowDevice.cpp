#include "VulkanWindowDevice.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <algorithm>
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
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = app.pEngineName = "OpenXRay";
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo = &app;
    instance_info.enabledExtensionCount = extension_count;
    instance_info.ppEnabledExtensionNames = extensions.data();
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
            "vkGetPhysicalDeviceMemoryProperties")
    };
    if (!select_physical_device(m_instance, m_surface, hardware, m_physical, error))
        return fail();
    DeviceDispatch devices{load_instance_proc<PFN_vkCreateDevice>(m_instance, m_instance_proc,
        "vkCreateDevice"), m_device_proc};
    if (!create_logical_device(m_physical, devices, m_device, m_queue, error))
        return fail();
    m_destroy_device = reinterpret_cast<PFN_vkDestroyDevice>(m_device_proc(m_device, "vkDestroyDevice"));
    FrameDispatch frame_dispatch;
    if (!m_destroy_device || !load_frame_dispatch(m_instance, m_instance_proc, m_device,
            m_device_proc, frame_dispatch, error))
    {
        if (error.empty()) error = "vkDestroyDevice is unavailable";
        return fail();
    }
    m_wait_idle = frame_dispatch.device_wait_idle;
    if (!m_frame.initialize(m_physical.handle, m_device, m_surface, m_queue,
            m_physical.graphics_present_family, extent, frame_dispatch, error,
            allow_readback, use_depth, preserve_prepass_depth, postprocess))
        return fail();
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
    if (!m_window || !m_instance || !m_device || !m_surface ||
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
    if (m_destroy_surface)
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
