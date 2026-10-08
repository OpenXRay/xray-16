#include "src/Layers/xrRenderVK/VulkanProbe.h"

#include <cassert>
#include <cstdint>
#include <cstring>

namespace
{
int load_result{};
int loads{};
int unloads{};
int destroyed_instances{};
VkResult instance_result{VK_SUCCESS};
VkResult enumeration_result{VK_SUCCESS};
uint32_t physical_devices{1};
bool loader_ready{};
bool no_loader_proc{};
bool no_create_proc{};
bool no_enumerate_proc{};
bool graphics_queue{true};
bool swapchain_extension{true};
bool supported_format{true};
int sdl_load_result{-1};
int sdl_loads{};
int sdl_unloads{};

VkResult VKAPI_CALL create_instance(const VkInstanceCreateInfo*,
    const VkAllocationCallbacks*, VkInstance* output)
{
    if (instance_result == VK_SUCCESS)
        *output = reinterpret_cast<VkInstance>(std::uintptr_t(1));
    return instance_result;
}

void VKAPI_CALL destroy_instance(VkInstance, const VkAllocationCallbacks*)
{
    ++destroyed_instances;
}

VkResult VKAPI_CALL enumerate_devices(VkInstance, uint32_t* count, VkPhysicalDevice* devices)
{
    *count = physical_devices;
    if (devices && physical_devices) devices[0] = reinterpret_cast<VkPhysicalDevice>(std::uintptr_t(2));
    return enumeration_result;
}

void VKAPI_CALL queue_families(VkPhysicalDevice, uint32_t* count, VkQueueFamilyProperties* queues)
{
    *count = 1;
    if (queues)
    {
        queues[0] = {};
        queues[0].queueCount = 1;
        queues[0].queueFlags = graphics_queue ? VK_QUEUE_GRAPHICS_BIT : VK_QUEUE_COMPUTE_BIT;
    }
}

VkResult VKAPI_CALL device_extensions(VkPhysicalDevice, const char*, uint32_t* count,
    VkExtensionProperties* extensions)
{
    *count = 1;
    if (extensions)
    {
        extensions[0] = {};
        std::strcpy(extensions[0].extensionName,
            swapchain_extension ? VK_KHR_SWAPCHAIN_EXTENSION_NAME : "VK_EXT_other");
    }
    return VK_SUCCESS;
}

void VKAPI_CALL physical_properties(VkPhysicalDevice, VkPhysicalDeviceProperties* properties)
{
    *properties = {};
    properties->limits.maxPushConstantsSize = 128;
    properties->limits.maxColorAttachments = 2;
    properties->limits.maxImageDimension2D = 4096;
    properties->limits.maxPerStageDescriptorSamplers = 8;
    properties->limits.maxPerStageDescriptorSampledImages = 8;
}
void VKAPI_CALL format_properties(VkPhysicalDevice, VkFormat format, VkFormatProperties* properties)
{
    *properties = {};
    if (format == VK_FORMAT_R8G8B8A8_UNORM)
        properties->optimalTilingFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
            (supported_format ? VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT : 0);
    if (format == VK_FORMAT_D32_SFLOAT)
        properties->optimalTilingFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
}

PFN_vkVoidFunction VKAPI_CALL get_instance_proc(VkInstance, const char* name)
{
    if (std::strcmp(name, "vkCreateInstance") == 0 && !no_create_proc)
        return reinterpret_cast<PFN_vkVoidFunction>(create_instance);
    if (std::strcmp(name, "vkDestroyInstance") == 0)
        return reinterpret_cast<PFN_vkVoidFunction>(destroy_instance);
    if (std::strcmp(name, "vkEnumeratePhysicalDevices") == 0 && !no_enumerate_proc)
        return reinterpret_cast<PFN_vkVoidFunction>(enumerate_devices);
    if (std::strcmp(name, "vkGetPhysicalDeviceQueueFamilyProperties") == 0)
        return reinterpret_cast<PFN_vkVoidFunction>(queue_families);
    if (std::strcmp(name, "vkEnumerateDeviceExtensionProperties") == 0)
        return reinterpret_cast<PFN_vkVoidFunction>(device_extensions);
    if (std::strcmp(name, "vkGetPhysicalDeviceProperties") == 0)
        return reinterpret_cast<PFN_vkVoidFunction>(physical_properties);
    if (std::strcmp(name, "vkGetPhysicalDeviceFormatProperties") == 0)
        return reinterpret_cast<PFN_vkVoidFunction>(format_properties);
    return nullptr;
}

int load_library(const char*)
{
    ++loads;
    if (load_result == 0)
        loader_ready = true;
    return load_result;
}

void unload_library()
{
    ++unloads;
    loader_ready = false;
}

PFN_vkGetInstanceProcAddr get_instance_proc_addr()
{
    return loader_ready && !no_loader_proc ? get_instance_proc : nullptr;
}
}

extern "C" int SDL_Vulkan_LoadLibrary(const char*)
{
    ++sdl_loads;
    return sdl_load_result;
}

extern "C" void SDL_Vulkan_UnloadLibrary()
{
    ++sdl_unloads;
}

extern "C" void* SDL_Vulkan_GetVkGetInstanceProcAddr()
{
    return nullptr;
}

int main()
{
    using xray::render::vulkan::probe_vulkan_loader;
    const xray::render::vulkan::VulkanLoaderDispatch dispatch{
        load_library, unload_library, get_instance_proc_addr};
    std::string error;

    load_result = -1;
    assert(!probe_vulkan_loader(error, dispatch) && !error.empty());
    assert(loads == 1 && unloads == 0);

    load_result = 0;
    loader_ready = false;
    no_loader_proc = true;
    assert(!probe_vulkan_loader(error, dispatch) && !error.empty());
    assert(loads == 2 && unloads == 1);

    no_loader_proc = false;
    no_create_proc = true;
    assert(!probe_vulkan_loader(error, dispatch) && !error.empty());
    assert(loads == 3 && unloads == 2);

    no_create_proc = false;
    instance_result = VK_ERROR_INITIALIZATION_FAILED;
    assert(!probe_vulkan_loader(error, dispatch) && !error.empty());
    assert(loads == 4 && unloads == 3);

    instance_result = VK_SUCCESS;
    no_enumerate_proc = true;
    assert(!probe_vulkan_loader(error, dispatch) && !error.empty());
    assert(loads == 5 && unloads == 4 && destroyed_instances == 1);

    no_enumerate_proc = false;
    enumeration_result = VK_ERROR_INITIALIZATION_FAILED;
    assert(!probe_vulkan_loader(error, dispatch) && !error.empty());
    assert(loads == 6 && unloads == 5 && destroyed_instances == 2);

    enumeration_result = VK_SUCCESS;
    physical_devices = 0;
    assert(!probe_vulkan_loader(error, dispatch) && !error.empty());
    assert(loads == 7 && unloads == 6 && destroyed_instances == 3);

    physical_devices = 1;
    loader_ready = true;
    graphics_queue = false;
    assert(!probe_vulkan_loader(error, dispatch) && !error.empty());
    graphics_queue = true;
    swapchain_extension = false;
    assert(!probe_vulkan_loader(error, dispatch) && !error.empty());
    swapchain_extension = true;
    supported_format = false;
    assert(!probe_vulkan_loader(error, dispatch) && !error.empty());
    supported_format = true;
    assert(probe_vulkan_loader(error, dispatch) && error.empty());
    assert(loads == 7 && unloads == 6 && destroyed_instances == 7);

    loader_ready = false;
    assert(probe_vulkan_loader(error, dispatch) && error.empty());
    assert(loads == 8 && unloads == 7 && destroyed_instances == 8);

    assert(!probe_vulkan_loader(error) && !error.empty());
    assert(sdl_loads == 1 && sdl_unloads == 0);
}
