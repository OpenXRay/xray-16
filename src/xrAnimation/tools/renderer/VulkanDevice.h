#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <cstdint>

struct SDL_Window;

namespace xray {
namespace animation {
namespace renderer {

class VulkanDevice {
public:
    VulkanDevice();
    ~VulkanDevice();

    bool Initialize(SDL_Window* window, bool enable_validation = true);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void WaitIdle();

    void RecreateSwapchain(int width, int height);

    VkInstance GetInstance() const { return instance_; }
    VkPhysicalDevice GetPhysicalDevice() const { return physical_device_; }
    VkDevice GetDevice() const { return device_; }
    VkQueue GetGraphicsQueue() const { return graphics_queue_; }
    VkQueue GetPresentQueue() const { return present_queue_; }
    VkSwapchainKHR GetSwapchain() const { return swapchain_; }
    VkCommandPool GetCommandPool() const { return command_pool_; }
    VmaAllocator GetAllocator() const { return allocator_; }

    VkImage GetCurrentSwapchainImage() const;
    VkImageView GetCurrentSwapchainImageView() const;
    VkImageView GetDepthImageView() const { return depth_image_view_; }
    uint32_t GetCurrentFrameIndex() const { return current_frame_; }
    uint32_t GetSwapchainImageCount() const { return static_cast<uint32_t>(swapchain_images_.size()); }
    uint32_t GetCurrentImageIndex() const { return current_image_index_; }

    VkSemaphore GetImageAvailableSemaphore() const { return image_available_semaphores_[current_frame_]; }
    VkSemaphore GetRenderFinishedSemaphore() const { return render_finished_semaphores_[current_frame_]; }
    VkFence GetInFlightFence() const { return in_flight_fences_[current_frame_]; }

    VkFormat GetSwapchainFormat() const { return swapchain_format_; }
    VkExtent2D GetSwapchainExtent() const { return swapchain_extent_; }

    struct QueueFamilyIndices {
        uint32_t graphics_family = UINT32_MAX;
        uint32_t present_family = UINT32_MAX;

        bool IsComplete() const {
            return graphics_family != UINT32_MAX && present_family != UINT32_MAX;
        }
    };

    QueueFamilyIndices GetQueueFamilyIndices() const { return queue_family_indices_; }

private:
    bool CreateInstance(bool enable_validation);
    bool CreateDebugMessenger();
    bool CreateSurface(SDL_Window* window);
    bool PickPhysicalDevice();
    bool CreateLogicalDevice();
    bool CreateSwapchain();
    bool CreateImageViews();
    bool CreateDepthResources();
    bool CreateSyncObjects();
    bool CreateCommandPool();
    bool CreateAllocator();

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    int RateDeviceSuitability(VkPhysicalDevice device);

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_modes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    VkFormat FindDepthFormat();
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    void CleanupSwapchain();

    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;

    VkQueue graphics_queue_ = VK_NULL_HANDLE;
    VkQueue present_queue_ = VK_NULL_HANDLE;
    QueueFamilyIndices queue_family_indices_;

    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_image_views_;
    VkFormat swapchain_format_;
    VkExtent2D swapchain_extent_;

    VkImage depth_image_ = VK_NULL_HANDLE;
    VmaAllocation depth_allocation_ = VK_NULL_HANDLE;
    VkImageView depth_image_view_ = VK_NULL_HANDLE;
    VkFormat depth_format_;

    VkCommandPool command_pool_ = VK_NULL_HANDLE;

    VmaAllocator allocator_ = VK_NULL_HANDLE;

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> image_available_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    std::vector<VkFence> in_flight_fences_;
    std::vector<VkFence> images_in_flight_;
    uint32_t current_frame_ = 0;
    uint32_t current_image_index_ = 0;

    bool validation_enabled_ = false;
    SDL_Window* window_ = nullptr;

    std::vector<const char*> device_extensions_ = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    const std::vector<const char*> validation_layers_ = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkPhysicalDeviceVulkan13Features vulkan_13_features_ = {};
    VkPhysicalDeviceVulkan12Features vulkan_12_features_ = {};
    VkPhysicalDeviceVulkan11Features vulkan_11_features_ = {};
    VkPhysicalDeviceFeatures2 device_features_2_ = {};

    std::vector<VkExtensionProperties> available_device_extensions_;
};

}
}
}
