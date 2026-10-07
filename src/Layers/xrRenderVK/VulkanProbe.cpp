#include "VulkanProbe.h"
#include "VulkanHardware.h"

#include <SDL_vulkan.h>

#include <algorithm>
#include <cstring>
#include <vector>

namespace xray::render::vulkan
{
namespace
{
int load_sdl_vulkan(const char* path)
{
    return SDL_Vulkan_LoadLibrary(path);
}

void unload_sdl_vulkan()
{
    SDL_Vulkan_UnloadLibrary();
}

PFN_vkGetInstanceProcAddr get_sdl_instance_proc_addr()
{
    return reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
}
}

bool probe_vulkan_loader(std::string& error, const VulkanLoaderDispatch& dispatch)
{
    if (!dispatch.get_instance_proc_addr)
    {
        error = "Vulkan loader procedure lookup is unavailable";
        return false;
    }
    auto get_instance_proc = dispatch.get_instance_proc_addr();
    bool loaded_here = false;
    if (!get_instance_proc)
    {
        if (!dispatch.load_library || !dispatch.unload_library || dispatch.load_library(nullptr) != 0)
        {
            error = "SDL could not load the platform Vulkan loader";
            return false;
        }
        loaded_here = true;
        get_instance_proc = dispatch.get_instance_proc_addr();
    }
    struct LoaderGuard
    {
        const VulkanLoaderDispatch& dispatch;
        bool loaded_here;
        ~LoaderGuard()
        {
            if (loaded_here)
                dispatch.unload_library();
        }
    } loader_guard{dispatch, loaded_here};
    if (!get_instance_proc)
    {
        error = "vkGetInstanceProcAddr is unavailable";
        return false;
    }

    const auto create_instance = reinterpret_cast<PFN_vkCreateInstance>(
        get_instance_proc(VK_NULL_HANDLE, "vkCreateInstance"));
    if (!create_instance)
    {
        error = "vkCreateInstance is unavailable";
        return false;
    }

    VkApplicationInfo application{};
    application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application.pApplicationName = "OpenXRay Vulkan probe";
    application.pEngineName = "OpenXRay";
    application.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &application;

    VkInstance instance = VK_NULL_HANDLE;
    const VkResult create_result = create_instance(&create_info, nullptr, &instance);
    if (create_result != VK_SUCCESS)
    {
        error = "vkCreateInstance failed (VkResult " + std::to_string(create_result) + ")";
        return false;
    }

    const auto destroy_instance = reinterpret_cast<PFN_vkDestroyInstance>(
        get_instance_proc(instance, "vkDestroyInstance"));
    if (!destroy_instance)
    {
        error = "vkDestroyInstance is unavailable";
        return false;
    }
    const auto enumerate_devices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        get_instance_proc(instance, "vkEnumeratePhysicalDevices"));
    uint32_t device_count = 0;
    const VkResult enumerate_result = enumerate_devices ?
        enumerate_devices(instance, &device_count, nullptr) : VK_ERROR_INITIALIZATION_FAILED;
    if (enumerate_result != VK_SUCCESS || device_count == 0)
    {
        destroy_instance(instance, nullptr);
        error = enumerate_result == VK_SUCCESS ? "no Vulkan physical device is available" :
            "vkEnumeratePhysicalDevices failed (VkResult " + std::to_string(enumerate_result) + ")";
        return false;
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    const VkResult fetch_result = enumerate_devices(instance, &device_count, devices.data());
    const auto get_queues = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
        get_instance_proc(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    const auto get_extensions = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
        get_instance_proc(instance, "vkEnumerateDeviceExtensionProperties"));
    const auto get_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
        get_instance_proc(instance, "vkGetPhysicalDeviceProperties"));
    const auto get_formats = reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties>(
        get_instance_proc(instance, "vkGetPhysicalDeviceFormatProperties"));
    bool suitable = false;
    if (fetch_result == VK_SUCCESS && get_queues && get_extensions && get_properties && get_formats)
        for (uint32_t i = 0; i < device_count && !suitable; ++i)
        {
            uint32_t queue_count = 0, extension_count = 0;
            get_queues(devices[i], &queue_count, nullptr);
            if (!queue_count || get_extensions(devices[i], nullptr, &extension_count, nullptr) != VK_SUCCESS ||
                !extension_count) continue;
            std::vector<VkQueueFamilyProperties> queues(queue_count);
            std::vector<VkExtensionProperties> extensions(extension_count);
            get_queues(devices[i], &queue_count, queues.data());
            if (get_extensions(devices[i], nullptr, &extension_count, extensions.data()) != VK_SUCCESS)
                continue;
            const bool graphics = std::any_of(queues.begin(), queues.begin() + queue_count,
                [](const auto& queue) { return queue.queueCount && (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT); });
            const bool swapchain = std::any_of(extensions.begin(), extensions.begin() + extension_count,
                [](const auto& extension) { return std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0; });
            if (graphics && swapchain)
            {
                VkPhysicalDeviceProperties properties{};
                get_properties(devices[i], &properties);
                std::string requirements_error;
                suitable = supports_game_formats(devices[i], get_formats, properties, requirements_error);
            }
        }
    destroy_instance(instance, nullptr);
    if (!suitable)
    {
        error = fetch_result != VK_SUCCESS ? "could not enumerate Vulkan physical devices" :
            "no Vulkan device with graphics queue, VK_KHR_swapchain and gameplay formats/limits is available";
        return false;
    }

    error.clear();
    return true;
}

bool probe_vulkan_loader(std::string& error)
{
    static const VulkanLoaderDispatch sdl_dispatch{
        load_sdl_vulkan, unload_sdl_vulkan, get_sdl_instance_proc_addr};
    return probe_vulkan_loader(error, sdl_dispatch);
}
}
