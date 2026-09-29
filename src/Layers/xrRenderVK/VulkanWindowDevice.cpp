#include "VulkanWindowDevice.h"

#include <SDL.h>
#include <dlfcn.h>

#include <algorithm>
#include <cstring>
#include <vector>

namespace xray::render::vulkan
{
namespace
{
using GetExtensions = SDL_bool (*)(SDL_Window*, unsigned int*, const char**);
using CreateSurface = SDL_bool (*)(SDL_Window*, VkInstance, VkSurfaceKHR*);

template <typename T> T load_instance_proc(VkInstance instance, PFN_vkGetInstanceProcAddr get, const char* name)
{
    return reinterpret_cast<T>(get(instance, name));
}
}

bool VulkanWindowDevice::initialize(SDL_Window* window, VkExtent2D extent,
    bool allow_readback, std::string& error)
{
    destroy();
    if (!window || !extent.width || !extent.height)
    {
        error = "Vulkan device requires an SDL window and a nonzero extent";
        return false;
    }
    const auto fail = [&]() { destroy(); return false; };
    m_library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!m_library)
    {
        error = "libvulkan.so is unavailable";
        return fail();
    }
    m_instance_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(m_library, "vkGetInstanceProcAddr"));
    const auto get_extensions = reinterpret_cast<GetExtensions>(dlsym(RTLD_DEFAULT,
        "SDL_Vulkan_GetInstanceExtensions"));
    const auto create_surface = reinterpret_cast<CreateSurface>(dlsym(RTLD_DEFAULT,
        "SDL_Vulkan_CreateSurface"));
    if (!m_instance_proc || !get_extensions || !create_surface)
    {
        error = "Vulkan loader or SDL Vulkan surface procedures are unavailable";
        return fail();
    }
    unsigned int extension_count = 0;
    if (!get_extensions(window, &extension_count, nullptr) || !extension_count)
    {
        error = "SDL returned no Vulkan instance extensions";
        return fail();
    }
    std::vector<const char*> extensions(extension_count);
    if (!get_extensions(window, &extension_count, extensions.data()))
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
    if (!create_surface(window, m_instance, &m_surface))
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
            m_physical.graphics_present_family, extent, frame_dispatch, error, allow_readback))
        return fail();
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
    if (m_library)
        dlclose(m_library);
    m_library = nullptr;
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
}
}
