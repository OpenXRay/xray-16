#pragma once

#include "FrameContext.h"
#include "VulkanHardware.h"

struct SDL_Window;

namespace xray::render::vulkan
{
// Owns an SDL window's Vulkan instance, surface, logical device and swapchain.
// The caller owns the window and all resources built from device() and must
// destroy those resources (after waiting for idle) before destroy().
class VulkanWindowDevice
{
public:
    VulkanWindowDevice() = default;
    ~VulkanWindowDevice() { destroy(); }
    VulkanWindowDevice(const VulkanWindowDevice&) = delete;
    VulkanWindowDevice& operator=(const VulkanWindowDevice&) = delete;

    bool initialize(SDL_Window* window, VkExtent2D extent, bool allow_readback,
        std::string& error, bool use_depth = false, bool preserve_prepass_depth = false,
        bool postprocess = false);
    bool recreate_frame(VkExtent2D extent, std::string& error);
    bool recreate_surface(VkExtent2D extent, std::string& error);
    void destroy();
    VkDevice device() const { return m_device; }
    VkInstance instance() const { return m_instance; }
    VkQueue queue() const { return m_queue; }
    const PhysicalDevice& physical() const { return m_physical; }
    FrameContext& frame() { return m_frame; }
    const FrameContext& frame() const { return m_frame; }
    PFN_vkGetInstanceProcAddr instance_proc() const { return m_instance_proc; }
    PFN_vkGetDeviceProcAddr device_proc() const { return m_device_proc; }

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    SDL_Window* m_window{};
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;
    PhysicalDevice m_physical{};
    FrameContext m_frame;
    PFN_vkGetInstanceProcAddr m_instance_proc{};
    PFN_vkGetDeviceProcAddr m_device_proc{};
    PFN_vkDestroyInstance m_destroy_instance{};
    PFN_vkDestroySurfaceKHR m_destroy_surface{};
    PFN_vkDestroyDevice m_destroy_device{};
    PFN_vkDeviceWaitIdle m_wait_idle{};
};
}
