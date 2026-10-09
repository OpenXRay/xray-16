#include "VulkanHardware.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

namespace xray::render::vulkan
{
namespace
{
bool supports_extension(const std::vector<VkExtensionProperties>& extensions, const char* name)
{
    return std::any_of(extensions.begin(), extensions.end(), [name](const VkExtensionProperties& extension)
    {
        return std::strcmp(extension.extensionName, name) == 0;
    });
}

bool has_swapchain_extension(VkPhysicalDevice device, PFN_vkEnumerateDeviceExtensionProperties enumerate,
    std::vector<VkExtensionProperties>& extensions)
{
    uint32_t count = 0;
    if (enumerate(device, nullptr, &count, nullptr) != VK_SUCCESS || count == 0)
        return false;

    extensions.resize(count);
    const VkResult result = enumerate(device, nullptr, &count, extensions.data());
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
        return false;
    extensions.resize(count);
    return supports_extension(extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

int64_t score_device(const VkPhysicalDeviceProperties& properties, uint64_t local_memory_bytes,
    uint32_t queue_count)
{
    int64_t score = static_cast<int64_t>(std::min<uint64_t>(local_memory_bytes >> 20, 65535));
    switch (properties.deviceType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: score += 1'000'000; break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score += 800'000; break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: score += 400'000; break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU: score += 100'000; break;
    default: break;
    }
    score += static_cast<int64_t>(properties.limits.maxImageDimension2D);
    score += static_cast<int64_t>(queue_count) * 100;
    return score;
}
}

bool supports_game_formats(VkPhysicalDevice device, PFN_vkGetPhysicalDeviceFormatProperties get,
    const VkPhysicalDeviceProperties& properties, std::string& error)
{
    if (!get || properties.limits.maxPushConstantsSize < 128 ||
        properties.limits.maxColorAttachments < 2 ||
        properties.limits.maxImageDimension2D < 1024 ||
        properties.limits.maxPerStageDescriptorSamplers < 8 ||
        properties.limits.maxPerStageDescriptorSampledImages < 8)
    {
        error = "Vulkan gameplay limits or format query are insufficient";
        return false;
    }
    const auto supports = [&](VkFormat format, VkFormatFeatureFlags required)
    {
        VkFormatProperties available{};
        get(device, format, &available);
        return (available.optimalTilingFeatures & required) == required;
    };
    const auto color = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    if (!supports(VK_FORMAT_R8G8B8A8_UNORM, color) ||
        (!supports(VK_FORMAT_D32_SFLOAT, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) &&
         !supports(VK_FORMAT_D16_UNORM, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)))
    {
        error = "Vulkan gameplay attachment or sampled depth formats are unavailable";
        return false;
    }
    error.clear();
    return true;
}

bool select_physical_device(VkInstance instance, VkSurfaceKHR surface,
    const HardwareDispatch& vk, PhysicalDevice& selected, std::string& error)
{
    selected = {};
    if (!vk.enumerate_physical_devices || !vk.get_queue_families || !vk.get_surface_support ||
        !vk.enumerate_device_extensions || !vk.get_physical_properties || !vk.get_physical_features ||
        !vk.get_memory_properties || !vk.get_format_properties)
    {
        error = "required Vulkan hardware procedures are unavailable";
        return false;
    }

    uint32_t physical_count = 0;
    VkResult result = vk.enumerate_physical_devices(instance, &physical_count, nullptr);
    if (result != VK_SUCCESS || physical_count == 0)
    {
        error = "no Vulkan physical device is available";
        return false;
    }

    std::vector<VkPhysicalDevice> devices(physical_count);
    result = vk.enumerate_physical_devices(instance, &physical_count, devices.data());
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        error = "could not enumerate Vulkan physical devices";
        return false;
    }

    int64_t best_score = std::numeric_limits<int64_t>::min();
    std::vector<VkExtensionProperties> extensions;
    for (VkPhysicalDevice device : devices)
    {
        if (!has_swapchain_extension(device, vk.enumerate_device_extensions, extensions))
            continue;

        uint32_t family_count = 0;
        vk.get_queue_families(device, &family_count, nullptr);
        std::vector<VkQueueFamilyProperties> families(family_count);
        vk.get_queue_families(device, &family_count, families.data());
        for (uint32_t family = 0; family < family_count; ++family)
        {
            if (families[family].queueCount == 0 || !(families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT))
                continue;

            VkBool32 presentation_supported = VK_FALSE;
            if (vk.get_surface_support(device, family, surface, &presentation_supported) != VK_SUCCESS ||
                !presentation_supported)
                continue;

            PhysicalDevice candidate;
            candidate.handle = device;
            candidate.graphics_present_family = family;
            vk.get_physical_properties(device, &candidate.properties);
            std::string requirements_error;
            if (!supports_game_formats(device, vk.get_format_properties,
                    candidate.properties, requirements_error))
                continue;
            vk.get_physical_features(device, &candidate.features);
            vk.get_memory_properties(device, &candidate.memory);
            for (uint32_t heap = 0; heap < candidate.memory.memoryHeapCount; ++heap)
                if (candidate.memory.memoryHeaps[heap].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                    candidate.local_memory_bytes += candidate.memory.memoryHeaps[heap].size;
            candidate.score = score_device(candidate.properties, candidate.local_memory_bytes,
                families[family].queueCount);

            if (!selected.handle || candidate.score > best_score)
            {
                selected = candidate;
                best_score = candidate.score;
            }
        }
    }

    if (!selected.handle)
    {
        error = "no Vulkan graphics queue supports the surface and VK_KHR_swapchain";
        return false;
    }
    error.clear();
    return true;
}

bool create_logical_device(const PhysicalDevice& physical_device, const DeviceDispatch& vk,
    VkDevice& device, VkQueue& graphics_queue, std::string& error)
{
    device = VK_NULL_HANDLE;
    graphics_queue = VK_NULL_HANDLE;
    if (!physical_device.handle || physical_device.graphics_present_family == UINT32_MAX ||
        !vk.create_device || !vk.get_device_proc)
    {
        error = "Vulkan device selection or creation procedures are invalid";
        return false;
    }

    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_info.queueFamilyIndex = physical_device.graphics_present_family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;
    const char* extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = 1;
    device_info.ppEnabledExtensionNames = extensions;
    // The world sampler enables this only on GPUs advertising the feature.
    VkPhysicalDeviceFeatures enabled_features{};
    enabled_features.samplerAnisotropy = physical_device.features.samplerAnisotropy;
    device_info.pEnabledFeatures = &enabled_features;

    if (vk.create_device(physical_device.handle, &device_info, nullptr, &device) != VK_SUCCESS)
    {
        error = "vkCreateDevice failed for the selected Vulkan GPU";
        return false;
    }

    const auto get_queue = reinterpret_cast<PFN_vkGetDeviceQueue>(vk.get_device_proc(device, "vkGetDeviceQueue"));
    if (!get_queue)
    {
        error = "vkGetDeviceQueue is unavailable";
        const auto destroy = reinterpret_cast<PFN_vkDestroyDevice>(vk.get_device_proc(device, "vkDestroyDevice"));
        if (destroy)
            destroy(device, nullptr);
        device = VK_NULL_HANDLE;
        return false;
    }
    get_queue(device, physical_device.graphics_present_family, 0, &graphics_queue);
    if (!graphics_queue)
    {
        error = "the selected Vulkan graphics queue could not be acquired";
        const auto destroy = reinterpret_cast<PFN_vkDestroyDevice>(vk.get_device_proc(device, "vkDestroyDevice"));
        if (destroy)
            destroy(device, nullptr);
        device = VK_NULL_HANDLE;
        return false;
    }
    error.clear();
    return true;
}
}
