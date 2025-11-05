#include "stdafx.h"
#include "VulkanDevice.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <set>
#include <string>
#include <cstring>
#include <cstdio>

// Simple logging replacement
#define Msg(...) printf(__VA_ARGS__), printf("\n")

namespace xray {
namespace animation {
namespace renderer {

// Debug callback for validation layers
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {

    if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        Msg("! [Vulkan Validation] %s", callback_data->pMessage);
    }
    return VK_FALSE;
}

VulkanDevice::VulkanDevice() {
}

VulkanDevice::~VulkanDevice() {
    Shutdown();
}

bool VulkanDevice::Initialize(GLFWwindow* window, bool enable_validation) {
    window_ = window;
    validation_enabled_ = enable_validation;

    Msg("* Initializing Vulkan device...");

    if (!CreateInstance(enable_validation)) return false;
    if (enable_validation && !CreateDebugMessenger()) return false;
    if (!CreateSurface(window)) return false;
    if (!PickPhysicalDevice()) return false;
    if (!CreateLogicalDevice()) return false;
    if (!CreateAllocator()) return false;  // MUST be before CreateDepthResources
    if (!CreateSwapchain()) return false;
    if (!CreateImageViews()) return false;
    if (!CreateDepthResources()) return false;
    if (!CreateCommandPool()) return false;
    if (!CreateSyncObjects()) return false;

    Msg("* Vulkan device initialized successfully");
    return true;
}

void VulkanDevice::Shutdown() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);

        // Cleanup sync objects
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (render_finished_semaphores_[i]) vkDestroySemaphore(device_, render_finished_semaphores_[i], nullptr);
            if (image_available_semaphores_[i]) vkDestroySemaphore(device_, image_available_semaphores_[i], nullptr);
            if (in_flight_fences_[i]) vkDestroyFence(device_, in_flight_fences_[i], nullptr);
        }

        if (command_pool_) vkDestroyCommandPool(device_, command_pool_, nullptr);

        CleanupSwapchain();

        if (allocator_) {
            vmaDestroyAllocator(allocator_);
            allocator_ = VK_NULL_HANDLE;
        }

        if (device_) vkDestroyDevice(device_, nullptr);
    }

    if (surface_) vkDestroySurfaceKHR(instance_, surface_, nullptr);

    if (debug_messenger_) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance_, debug_messenger_, nullptr);
        }
    }

    if (instance_) vkDestroyInstance(instance_, nullptr);

    instance_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    surface_ = VK_NULL_HANDLE;
    debug_messenger_ = VK_NULL_HANDLE;
}

void VulkanDevice::BeginFrame() {
    // Wait for previous frame to finish
    vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);

    // Acquire next swapchain image
    VkResult result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
        image_available_semaphores_[current_frame_], VK_NULL_HANDLE, &current_image_index_);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);
        RecreateSwapchain(width, height);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        Msg("! Failed to acquire swapchain image");
        return;
    }

    // Check if a previous frame is using this image
    if (images_in_flight_[current_image_index_] != VK_NULL_HANDLE) {
        vkWaitForFences(device_, 1, &images_in_flight_[current_image_index_], VK_TRUE, UINT64_MAX);
    }
    images_in_flight_[current_image_index_] = in_flight_fences_[current_frame_];
}

void VulkanDevice::EndFrame() {
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_finished_semaphores_[current_frame_];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain_;
    present_info.pImageIndices = &current_image_index_;

    VkResult result = vkQueuePresentKHR(present_queue_, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);
        RecreateSwapchain(width, height);
    } else if (result != VK_SUCCESS) {
        Msg("! Failed to present swapchain image");
    }

    current_frame_ = (current_frame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanDevice::WaitIdle() {
    if (device_) {
        vkDeviceWaitIdle(device_);
    }
}

void VulkanDevice::RecreateSwapchain(int width, int height) {
    vkDeviceWaitIdle(device_);

    CleanupSwapchain();

    CreateSwapchain();
    CreateImageViews();
    CreateDepthResources();
}

VkImage VulkanDevice::GetCurrentSwapchainImage() const {
    if (current_image_index_ < swapchain_images_.size()) {
        return swapchain_images_[current_image_index_];
    }
    return VK_NULL_HANDLE;
}

VkImageView VulkanDevice::GetCurrentSwapchainImageView() const {
    if (current_image_index_ < swapchain_image_views_.size()) {
        return swapchain_image_views_[current_image_index_];
    }
    return VK_NULL_HANDLE;
}

// ============================================================================
// Initialization Helpers
// ============================================================================

bool VulkanDevice::CreateInstance(bool enable_validation) {
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "ozz Animation Viewer";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "X-Ray Animation";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    // Get required GLFW extensions
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    if (enable_validation) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    if (enable_validation) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
        create_info.ppEnabledLayerNames = validation_layers_.data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance(&create_info, nullptr, &instance_);
    if (result != VK_SUCCESS) {
        Msg("! Failed to create Vulkan instance (error: %d)", result);
        return false;
    }

    return true;
}

bool VulkanDevice::CreateDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = DebugCallback;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        VkResult result = func(instance_, &create_info, nullptr, &debug_messenger_);
        if (result != VK_SUCCESS) {
            Msg("! Failed to create debug messenger");
            return false;
        }
    } else {
        Msg("! Debug messenger extension not available");
        return false;
    }

    return true;
}

bool VulkanDevice::CreateSurface(GLFWwindow* window) {
    VkResult result = glfwCreateWindowSurface(instance_, window, nullptr, &surface_);
    if (result != VK_SUCCESS) {
        Msg("! Failed to create window surface");
        return false;
    }
    return true;
}

bool VulkanDevice::PickPhysicalDevice() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);

    if (device_count == 0) {
        Msg("! No Vulkan-capable GPUs found");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());

    Msg("====================================================================");
    Msg("=== GPU DETECTION DEBUG ===");
    Msg("Found %u Vulkan device(s)", device_count);
    Msg("--------------------------------------------------------------------");

    // Rate all devices and pick the best one
    int best_score = 0;
    for (size_t i = 0; i < devices.size(); i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);

        const char* device_type_str = "UNKNOWN";
        switch(props.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   device_type_str = "DISCRETE_GPU"; break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: device_type_str = "INTEGRATED_GPU"; break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    device_type_str = "VIRTUAL_GPU"; break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:            device_type_str = "CPU (Software)"; break;
            default: break;
        }

        int score = RateDeviceSuitability(devices[i]);

        Msg("Device #%zu: %s", i, props.deviceName);
        Msg("  Type: %s", device_type_str);
        Msg("  Vendor ID: 0x%04X", props.vendorID);
        Msg("  Device ID: 0x%04X", props.deviceID);
        Msg("  API Version: %u.%u.%u",
            VK_VERSION_MAJOR(props.apiVersion),
            VK_VERSION_MINOR(props.apiVersion),
            VK_VERSION_PATCH(props.apiVersion));
        Msg("  Driver Version: %u", props.driverVersion);
        Msg("  Suitability Score: %d", score);
        Msg("--------------------------------------------------------------------");

        if (score > best_score) {
            best_score = score;
            physical_device_ = devices[i];
        }
    }

    if (physical_device_ == VK_NULL_HANDLE) {
        Msg("! No suitable GPU found");
        Msg("====================================================================");
        return false;
    }

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device_, &properties);

    const char* selected_type = "UNKNOWN";
    switch(properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   selected_type = "DISCRETE_GPU"; break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: selected_type = "INTEGRATED_GPU"; break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    selected_type = "VIRTUAL_GPU"; break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:            selected_type = "CPU (Software)"; break;
        default: break;
    }

    Msg("*** SELECTED GPU: %s ***", properties.deviceName);
    Msg("    Type: %s", selected_type);
    Msg("    Score: %d", best_score);
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
        Msg("    ⚠️  WARNING: Using CPU software rendering - performance will be poor!");
    }
    Msg("====================================================================");

    queue_family_indices_ = FindQueueFamilies(physical_device_);

    return true;
}

bool VulkanDevice::CreateLogicalDevice() {
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = {
        queue_family_indices_.graphics_family,
        queue_family_indices_.present_family
    };

    float queue_priority = 1.0f;
    for (uint32_t queue_family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    // Check available extensions
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &extension_count, nullptr);
    available_device_extensions_.resize(extension_count);
    vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &extension_count, available_device_extensions_.data());

    // Add push descriptors if available
    for (const auto& ext : available_device_extensions_) {
        if (strcmp(ext.extensionName, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME) == 0) {
            device_extensions_.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
            Msg("* Enabling VK_KHR_push_descriptor extension");
            break;
        }
    }

    // Setup Vulkan 1.3 feature chain
    vulkan_11_features_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vulkan_11_features_.pNext = nullptr;

    vulkan_12_features_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan_12_features_.pNext = &vulkan_11_features_;
    vulkan_12_features_.bufferDeviceAddress = VK_TRUE;  // Enable buffer device address

    vulkan_13_features_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan_13_features_.pNext = &vulkan_12_features_;
    vulkan_13_features_.dynamicRendering = VK_TRUE;  // Enable dynamic rendering
    vulkan_13_features_.synchronization2 = VK_TRUE;  // Enable better synchronization

    device_features_2_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    device_features_2_.pNext = &vulkan_13_features_;
    device_features_2_.features.samplerAnisotropy = VK_TRUE;
    device_features_2_.features.fillModeNonSolid = VK_TRUE;  // For line rendering

    // Query what the device supports
    vkGetPhysicalDeviceFeatures2(physical_device_, &device_features_2_);

    // Verify required features
    if (!vulkan_13_features_.dynamicRendering) {
        Msg("! Dynamic rendering not supported - please update your GPU drivers");
        return false;
    }

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pNext = &device_features_2_;
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pEnabledFeatures = nullptr;  // Use pNext chain instead
    create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions_.size());
    create_info.ppEnabledExtensionNames = device_extensions_.data();

    if (validation_enabled_) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
        create_info.ppEnabledLayerNames = validation_layers_.data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    VkResult result = vkCreateDevice(physical_device_, &create_info, nullptr, &device_);
    if (result != VK_SUCCESS) {
        Msg("! Failed to create logical device (error: %d)", result);
        return false;
    }

    vkGetDeviceQueue(device_, queue_family_indices_.graphics_family, 0, &graphics_queue_);
    vkGetDeviceQueue(device_, queue_family_indices_.present_family, 0, &present_queue_);

    Msg("* Vulkan 1.3 features enabled:");
    Msg("  - Dynamic Rendering: YES");
    Msg("  - Synchronization2: %s", vulkan_13_features_.synchronization2 ? "YES" : "NO");
    Msg("  - Buffer Device Address: %s", vulkan_12_features_.bufferDeviceAddress ? "YES" : "NO");

    return true;
}

bool VulkanDevice::CreateSwapchain() {
    SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(physical_device_);

    VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(swap_chain_support.formats);
    VkPresentModeKHR present_mode = ChooseSwapPresentMode(swap_chain_support.present_modes);
    VkExtent2D extent = ChooseSwapExtent(swap_chain_support.capabilities);

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface_;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queue_family_indices[] = {queue_family_indices_.graphics_family, queue_family_indices_.present_family};

    if (queue_family_indices_.graphics_family != queue_family_indices_.present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_);
    if (result != VK_SUCCESS) {
        Msg("! Failed to create swapchain");
        return false;
    }

    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);
    swapchain_images_.resize(image_count);
    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, swapchain_images_.data());

    swapchain_format_ = surface_format.format;
    swapchain_extent_ = extent;

    Msg("* Swapchain created: %dx%d, %d images", extent.width, extent.height, image_count);

    return true;
}

bool VulkanDevice::CreateImageViews() {
    swapchain_image_views_.resize(swapchain_images_.size());

    for (size_t i = 0; i < swapchain_images_.size(); i++) {
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swapchain_images_[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swapchain_format_;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(device_, &create_info, nullptr, &swapchain_image_views_[i]);
        if (result != VK_SUCCESS) {
            Msg("! Failed to create image view %zu", i);
            return false;
        }
    }

    return true;
}

bool VulkanDevice::CreateDepthResources() {
    depth_format_ = FindDepthFormat();

    Msg("* Creating depth buffer: %ux%u, format=%d", swapchain_extent_.width, swapchain_extent_.height, depth_format_);

    // Create depth image using VMA
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = swapchain_extent_.width;
    image_info.extent.height = swapchain_extent_.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = depth_format_;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    alloc_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkResult result = vmaCreateImage(allocator_, &image_info, &alloc_info, &depth_image_, &depth_allocation_, nullptr);
    if (result != VK_SUCCESS) {
        Msg("! Failed to create depth image via VMA (VkResult=%d)", result);
        return false;
    }

    // Create depth image view
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = depth_image_;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = depth_format_;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    result = vkCreateImageView(device_, &view_info, nullptr, &depth_image_view_);
    if (result != VK_SUCCESS) {
        Msg("! Failed to create depth image view (VkResult=%d)", result);
        return false;
    }

    Msg("* Depth buffer created successfully");
    return true;
}

bool VulkanDevice::CreateCommandPool() {
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = queue_family_indices_.graphics_family;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result = vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_);
    if (result != VK_SUCCESS) {
        Msg("! Failed to create command pool");
        return false;
    }

    return true;
}

bool VulkanDevice::CreateAllocator() {
    if (allocator_ != VK_NULL_HANDLE) {
        vmaDestroyAllocator(allocator_);
        allocator_ = VK_NULL_HANDLE;
    }

    VmaVulkanFunctions functions = {};
    functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    functions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
    functions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    functions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    functions.vkAllocateMemory = vkAllocateMemory;
    functions.vkFreeMemory = vkFreeMemory;
    functions.vkMapMemory = vkMapMemory;
    functions.vkUnmapMemory = vkUnmapMemory;
    functions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    functions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    functions.vkBindBufferMemory = vkBindBufferMemory;
    functions.vkBindImageMemory = vkBindImageMemory;
    functions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    functions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    functions.vkCreateBuffer = vkCreateBuffer;
    functions.vkDestroyBuffer = vkDestroyBuffer;
    functions.vkCreateImage = vkCreateImage;
    functions.vkDestroyImage = vkDestroyImage;
    functions.vkCmdCopyBuffer = vkCmdCopyBuffer;

    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.instance = instance_;
    allocator_info.physicalDevice = physical_device_;
    allocator_info.device = device_;
    allocator_info.pVulkanFunctions = &functions;
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;

    VkResult result = vmaCreateAllocator(&allocator_info, &allocator_);
    if (result != VK_SUCCESS) {
        Msg("! Failed to create VMA allocator (error: %d)", result);
        allocator_ = VK_NULL_HANDLE;
        return false;
    }

    return true;
}

bool VulkanDevice::CreateSyncObjects() {
    image_available_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences_.resize(MAX_FRAMES_IN_FLIGHT);
    images_in_flight_.resize(swapchain_images_.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device_, &semaphore_info, nullptr, &image_available_semaphores_[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device_, &semaphore_info, nullptr, &render_finished_semaphores_[i]) != VK_SUCCESS ||
            vkCreateFence(device_, &fence_info, nullptr, &in_flight_fences_[i]) != VK_SUCCESS) {
            Msg("! Failed to create synchronization objects");
            return false;
        }
    }

    return true;
}

// ============================================================================
// Helper Functions
// ============================================================================

VulkanDevice::QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    int i = 0;
    for (const auto& queue_family : queue_families) {
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);
        if (present_support) {
            indices.present_family = i;
        }

        if (indices.IsComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::set<std::string> required_extensions;
    for (const char* ext : device_extensions_) {
        required_extensions.insert(ext);
    }

    for (const auto& extension : available_extensions) {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

int VulkanDevice::RateDeviceSuitability(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceProperties(device, &device_properties);
    vkGetPhysicalDeviceFeatures(device, &device_features);

    int score = 0;

    // Discrete GPUs have a significant performance advantage
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += device_properties.limits.maxImageDimension2D;

    // Application can't function without geometry shaders
    if (!device_features.geometryShader) {
        return 0;
    }

    // Check queue families
    QueueFamilyIndices indices = FindQueueFamilies(device);
    if (!indices.IsComplete()) {
        return 0;
    }

    // Check extension support
    if (!CheckDeviceExtensionSupport(device)) {
        return 0;
    }

    // Check swapchain support
    SwapChainSupportDetails swapchain_support = QuerySwapChainSupport(device);
    if (swapchain_support.formats.empty() || swapchain_support.present_modes.empty()) {
        return 0;
    }

    return score;
}

VulkanDevice::SwapChainSupportDetails VulkanDevice::QuerySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);
    if (format_count != 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats.data());
    }

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);
    if (present_mode_count != 0) {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, details.present_modes.data());
    }

    return details;
}

VkSurfaceFormatKHR VulkanDevice::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats) {
    for (const auto& available_format : available_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_format;
        }
    }

    return available_formats[0];
}

VkPresentModeKHR VulkanDevice::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_modes) {
    for (const auto& available_mode : available_modes) {
        if (available_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanDevice::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);

        VkExtent2D actual_extent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actual_extent.width = std::max(capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actual_extent.width));
        actual_extent.height = std::max(capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actual_extent.height));

        return actual_extent;
    }
}

VkFormat VulkanDevice::FindDepthFormat() {
    return FindSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat VulkanDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device_, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    Msg("! Failed to find supported format");
    return VK_FORMAT_UNDEFINED;
}

void VulkanDevice::CleanupSwapchain() {
    // Destroy depth resources
    if (depth_image_view_) {
        vkDestroyImageView(device_, depth_image_view_, nullptr);
        depth_image_view_ = VK_NULL_HANDLE;
    }
    if (depth_image_ && allocator_) {
        vmaDestroyImage(allocator_, depth_image_, depth_allocation_);
        depth_image_ = VK_NULL_HANDLE;
        depth_allocation_ = VK_NULL_HANDLE;
    }

    // Destroy image views
    for (auto image_view : swapchain_image_views_) {
        vkDestroyImageView(device_, image_view, nullptr);
    }
    swapchain_image_views_.clear();

    // Destroy swapchain
    if (swapchain_) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }

    swapchain_images_.clear();
}

} // namespace renderer
} // namespace animation
} // namespace xray
