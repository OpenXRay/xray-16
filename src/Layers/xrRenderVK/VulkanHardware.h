#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>

namespace xray::render::vulkan
{
struct HardwareDispatch
{
    PFN_vkEnumeratePhysicalDevices enumerate_physical_devices{};
    PFN_vkGetPhysicalDeviceQueueFamilyProperties get_queue_families{};
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR get_surface_support{};
    PFN_vkEnumerateDeviceExtensionProperties enumerate_device_extensions{};
    PFN_vkGetPhysicalDeviceProperties get_physical_properties{};
    PFN_vkGetPhysicalDeviceFeatures get_physical_features{};
    PFN_vkGetPhysicalDeviceMemoryProperties get_memory_properties{};
};

struct PhysicalDevice
{
    VkPhysicalDevice handle = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties properties{};
    VkPhysicalDeviceFeatures features{};
    VkPhysicalDeviceMemoryProperties memory{};
    uint32_t graphics_present_family = UINT32_MAX;
    uint64_t local_memory_bytes{};
    int64_t score{};
};

struct DeviceDispatch
{
    PFN_vkCreateDevice create_device{};
    PFN_vkGetDeviceProcAddr get_device_proc{};
};

bool select_physical_device(VkInstance instance, VkSurfaceKHR surface,
    const HardwareDispatch& vk, PhysicalDevice& selected, std::string& error);
bool create_logical_device(const PhysicalDevice& physical_device, const DeviceDispatch& vk,
    VkDevice& device, VkQueue& graphics_queue, std::string& error);
}
