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

// Tests that the system Vulkan loader can create an instance and enumerate at
// least one physical device. Surface and presentation support are checked once
// the engine has created its SDL Vulkan window.
bool probe_vulkan_loader(std::string& error, const VulkanLoaderDispatch& dispatch);
bool probe_vulkan_loader(std::string& error);
}
