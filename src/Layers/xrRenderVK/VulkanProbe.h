#pragma once

#include <vulkan/vulkan.h>

#include <string>

namespace xray::render::vulkan
{
struct VulkanLoaderDispatch
{
    int (*load_library)(const char* path){};
    void (*unload_library)(){};
    PFN_vkGetInstanceProcAddr (*get_instance_proc_addr)(){};
};

// Tests the loader, instance and a graphics device with the swapchain extension.
// Surface presentation support is checked once SDL has created the game window.
bool probe_vulkan_loader(std::string& error, const VulkanLoaderDispatch& dispatch);
bool probe_vulkan_loader(std::string& error);
}
