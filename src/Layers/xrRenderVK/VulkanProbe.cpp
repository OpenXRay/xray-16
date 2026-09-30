#include "VulkanProbe.h"

#include <vulkan/vulkan.h>

#include <dlfcn.h>

namespace xray::render::vulkan
{
bool probe_vulkan_loader(std::string& error)
{
    void* library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!library)
    {
        error = "libvulkan.so is unavailable";
        return false;
    }

    const auto get_instance_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        dlsym(library, "vkGetInstanceProcAddr"));
    if (!get_instance_proc)
    {
        error = "vkGetInstanceProcAddr is unavailable";
        dlclose(library);
        return false;
    }

    const auto create_instance = reinterpret_cast<PFN_vkCreateInstance>(
        get_instance_proc(VK_NULL_HANDLE, "vkCreateInstance"));
    if (!create_instance)
    {
        error = "vkCreateInstance is unavailable";
        dlclose(library);
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
        dlclose(library);
        return false;
    }

    const auto destroy_instance = reinterpret_cast<PFN_vkDestroyInstance>(
        get_instance_proc(instance, "vkDestroyInstance"));
    const auto enumerate_devices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        get_instance_proc(instance, "vkEnumeratePhysicalDevices"));
    uint32_t device_count = 0;
    const VkResult enumerate_result = enumerate_devices ?
        enumerate_devices(instance, &device_count, nullptr) : VK_ERROR_INITIALIZATION_FAILED;

    if (destroy_instance)
        destroy_instance(instance, nullptr);
    dlclose(library);

    if (enumerate_result != VK_SUCCESS || device_count == 0)
    {
        error = enumerate_result == VK_SUCCESS ? "no Vulkan physical device is available" :
            "vkEnumeratePhysicalDevices failed (VkResult " + std::to_string(enumerate_result) + ")";
        return false;
    }

    error.clear();
    return true;
}
}
