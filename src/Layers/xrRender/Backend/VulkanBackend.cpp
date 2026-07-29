#include "stdafx.h"
#include "VulkanBackend.h"

#include "xrCore/Threading/TaskManager.hpp"

#include <algorithm>

#if defined(__APPLE__)
#include <pthread/qos.h>
#endif

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <nvrhi/vulkan.h>
#include <nvrhi/validation.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#if defined(XRAY_USE_DLSS)
#include "Layers/xrRender/Upscaling/StreamlineDLSS.h"
#endif

extern ENGINE_API int ps_r_dlss_fg;
extern ENGINE_API int ps_r_upscale;
extern ENGINE_API int ps_r_reflex;
extern int ps_fps_limit;
extern int ps_fps_limit_in_menu;

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

class NVRHIVulkanMessageCallback : public nvrhi::IMessageCallback {
public:
    void message(nvrhi::MessageSeverity severity, const char* messageText) override {
        switch (severity) {
        case nvrhi::MessageSeverity::Info:
            Msg("* [NVRHI-VK] %s", messageText);
            break;
        case nvrhi::MessageSeverity::Warning:
            Msg("! [NVRHI-VK] WARNING: %s", messageText);
            break;
        case nvrhi::MessageSeverity::Error:
            Msg("! [NVRHI-VK] ERROR: %s", messageText);
            break;
        case nvrhi::MessageSeverity::Fatal:
            Msg("! [NVRHI-VK] FATAL: %s", messageText);
            R_ASSERT2(false, messageText);
            break;
        }
    }
};

static NVRHIVulkanMessageCallback s_nvrhiVkMessageCallback;

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        Msg("! [Vulkan] ERROR: %s", callbackData->pMessage);
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        Msg("! [Vulkan] WARNING: %s", callbackData->pMessage);
    return VK_FALSE;
}

VulkanBackend::VulkanBackend() = default;

VulkanBackend::~VulkanBackend() {
    Shutdown();
}

bool VulkanBackend::Initialize(SDL_Window* window, u32 width, u32 height, bool enableValidation) {
    if (m_initialized) {
        Msg("! [VulkanBackend] Already initialized");
        return false;
    }
    if (!window) {
        Msg("! [VulkanBackend] No window provided");
        return false;
    }

    Msg("* [VulkanBackend] Initializing...");
    m_validationEnabled = enableValidation;

    if (!CreateInstance(window, enableValidation)) { Shutdown(); return false; }
    Msg("* [VulkanBackend] Instance created, m_instance=%p", m_instance);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance, vkGetInstanceProcAddr);
    Msg("* [VulkanBackend] Dispatcher instance-init done, vkCreateDevice=%p vkCreateSemaphore=%p",
        VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateDevice, VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateSemaphore);

    if (!CreateSurface(window)) { Shutdown(); return false; }
    if (!SelectPhysicalDevice()) { Shutdown(); return false; }
    if (!CreateLogicalDevice()) { Shutdown(); return false; }
    Msg("* [VulkanBackend] Device created, m_device=%p", m_device);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance, vkGetInstanceProcAddr, m_device, vkGetDeviceProcAddr);
    Msg("* [VulkanBackend] Dispatcher device-init done, vkCreateSemaphore=%p vkDestroySemaphore=%p vkCreateCommandPool=%p",
        VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateSemaphore,
        VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroySemaphore,
        VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateCommandPool);

    if (!CreateSwapChain(width, height)) { Shutdown(); return false; }

    Msg("* [VulkanBackend] About to create NVRHI device...");
    Msg("* [VulkanBackend]   graphicsQueue=%p family=%u", m_graphicsQueue, m_graphicsQueueFamily);
    Msg("* [VulkanBackend]   computeQueue=%p family=%u", m_computeQueue, m_computeQueueFamily);

    nvrhi::vulkan::DeviceDesc deviceDesc;
    deviceDesc.errorCB = &s_nvrhiVkMessageCallback;
    deviceDesc.instance = m_instance;
    deviceDesc.physicalDevice = m_physicalDevice;
    deviceDesc.device = m_device;
    deviceDesc.graphicsQueue = m_graphicsQueue;
    deviceDesc.graphicsQueueIndex = static_cast<int>(m_graphicsQueueFamily);
    if (m_computeQueue) {
        deviceDesc.computeQueue = m_computeQueue;
        deviceDesc.computeQueueIndex = static_cast<int>(m_computeQueueFamily);
    }

    Uint32 sdlExtCount = 0;
    const char* const* sdlExts = SDL_Vulkan_GetInstanceExtensions(&sdlExtCount);
    xr_vector<const char*> instanceExts(sdlExts, sdlExts + sdlExtCount);
    instanceExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    deviceDesc.instanceExtensions = instanceExts.data();
    deviceDesc.numInstanceExtensions = instanceExts.size();

    xr_vector<const char*> nvrhiDeviceExts = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    };
    if (m_featureRayTracing) {
        nvrhiDeviceExts.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        nvrhiDeviceExts.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        nvrhiDeviceExts.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    }
    deviceDesc.deviceExtensions = nvrhiDeviceExts.data();
    deviceDesc.numDeviceExtensions = nvrhiDeviceExts.size();
    deviceDesc.bufferDeviceAddressSupported = m_featureBufferDeviceAddress;

    m_nvrhiVulkanDevice = nvrhi::vulkan::createDevice(deviceDesc);
    if (!m_nvrhiVulkanDevice) {
        Msg("! [VulkanBackend] Failed to create NVRHI Vulkan device");
        Shutdown();
        return false;
    }

    if (strstr(Core.Params, "-no_nvrhi_validation")) {
        m_nvrhiDevice = m_nvrhiVulkanDevice;
        Msg("* [VulkanBackend] NVRHI validation layer disabled (-no_nvrhi_validation)");
    } else {
        m_nvrhiDevice = nvrhi::validation::createValidationLayer(m_nvrhiVulkanDevice);
        if (!m_nvrhiDevice) {
            Msg("! [VulkanBackend] Failed to create NVRHI validation layer");
            Shutdown();
            return false;
        }
        Msg("* [VulkanBackend] NVRHI validation layer enabled");
    }

    nvrhi::CommandListParameters cmdParams;
    cmdParams.enableImmediateExecution = false;
    for (u32 i = 0; i < 2; ++i) {
        m_commandLists[i] = m_nvrhiDevice->createCommandList(cmdParams);
        if (!m_commandLists[i]) {
            Msg("! [VulkanBackend] Failed to create command list");
            Shutdown();
            return false;
        }
    }

    m_asyncSubmit = strstr(Core.Params, "-async_submit") != nullptr;
    if (m_asyncSubmit && ps_r_upscale == 2 && ps_r_dlss_fg != 0)
    {
        m_asyncSubmit = false;
        Msg("! [VulkanBackend] -async_submit disabled — incompatible with DLSS-FG dual present");
    }
    if (m_asyncSubmit) {
        m_submitRun = true;
        m_submitThread = std::thread([this] { SubmitThreadMain(); });
        Msg("* [VulkanBackend] async submit thread enabled (-async_submit)");
    }

    m_uploadCommandList = m_nvrhiDevice->createCommandList(cmdParams);
    if (!m_uploadCommandList) {
        Msg("! [VulkanBackend] Failed to create upload command list");
        Shutdown();
        return false;
    }

    if (m_computeQueue) {
        nvrhi::CommandListParameters computeParams;
        computeParams.enableImmediateExecution = false;
        computeParams.setQueueType(nvrhi::CommandQueue::Compute);
        m_computeCommandList = m_nvrhiDevice->createCommandList(computeParams);
        if (m_computeCommandList) {
            Msg("* [VulkanBackend] Async compute enabled");
        } else {
            Msg("! [VulkanBackend] Failed to create compute command list (async compute disabled)");
        }
    }

    QueryCapabilities();
    CreateBackBufferTextures();
    CreateSyncObjects();
    CreateBindlessResources();

    m_initialized = true;
    Msg("* [VulkanBackend] Initialized successfully");
    Msg("*   Bindless textures: %s (max %u)",
        m_capabilities.bindlessTextures ? "Yes" : "No",
        m_capabilities.maxBindlessResources);
    Msg("*   drawIndirectCount: %s, multiDrawIndirect: %s",
        m_capabilities.drawIndirectCount ? "Yes" : "No",
        m_capabilities.multiDrawIndirect ? "Yes" : "No");
    return true;
}

void VulkanBackend::Shutdown() {
    if (!m_initialized && !m_nvrhiDevice)
        return;

    Msg("* [VulkanBackend] Shutting down...");
    WaitForIdle();

    if (m_submitThread.joinable()) {
        {
            std::lock_guard<std::mutex> lk(m_submitMutex);
            m_submitRun = false;
        }
        m_submitCv.notify_one();
        m_submitThread.join();
    }

    m_bindlessDescriptorTable = nullptr;
    m_bindlessLayout = nullptr;
    for (auto& bb : m_backBuffers)
        bb = nullptr;
    m_commandLists[0] = nullptr;
    m_commandLists[1] = nullptr;
    m_computeCommandList = nullptr;
    m_uploadCommandList = nullptr;

    m_nvrhiDevice = nullptr;
    if (m_nvrhiVulkanDevice) {
        m_nvrhiVulkanDevice->waitForIdle();
        m_nvrhiVulkanDevice->runGarbageCollection();
    }
    m_nvrhiVulkanDevice = nullptr;

    DestroyLatencyObjects();
    DestroySyncObjects();
    DestroySwapChain();

    if (m_device) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }
    if (m_surface) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
    if (m_debugMessenger) {
        auto destroyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyFunc)
            destroyFunc(m_instance, m_debugMessenger, nullptr);
        m_debugMessenger = VK_NULL_HANDLE;
    }
    if (m_instance) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }

    m_initialized = false;
    Msg("* [VulkanBackend] Shutdown complete");
}

bool VulkanBackend::CreateInstance(SDL_Window* window, bool enableValidation) {
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "OpenXRay";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "X-Ray Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    Uint32 sdlExtCount = 0;
    const char* const* sdlExts = SDL_Vulkan_GetInstanceExtensions(&sdlExtCount);
    if (!sdlExts) {
        Msg("! [VulkanBackend] SDL_Vulkan_GetInstanceExtensions failed: %s", SDL_GetError());
        return false;
    }
    xr_vector<const char*> extensions(sdlExts, sdlExts + sdlExtCount);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#if defined(XRAY_USE_DLSS)
    xray::render::fg::Streamline_PreInstanceInit();
    xray::render::fg::Streamline_GetRequiredInstanceExtensions(extensions);
#endif

    xr_vector<const char*> layers;
    if (enableValidation) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = static_cast<u32>(layers.size());
    createInfo.ppEnabledLayerNames = layers.data();

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        Msg("! [VulkanBackend] vkCreateInstance failed: %d", result);
        return false;
    }

    if (enableValidation) {
        auto createFunc = (PFN_vkCreateDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
        if (createFunc) {
            VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
            debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugInfo.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugInfo.pfnUserCallback = VulkanDebugCallback;
            createFunc(m_instance, &debugInfo, nullptr, &m_debugMessenger);
            Msg("* [VulkanBackend] Validation layer enabled");
        }
    }

    Msg("* [VulkanBackend] Vulkan instance created (API 1.2)");
    return true;
}

bool VulkanBackend::SelectPhysicalDevice() {
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        Msg("! [VulkanBackend] No Vulkan-capable GPUs found");
        return false;
    }

    xr_vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (auto& dev : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);

        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            m_physicalDevice = dev;
            m_capabilities.id_vendor = props.vendorID;
            m_capabilities.id_device = props.deviceID;
            Msg("* [VulkanBackend] Using GPU: %s", props.deviceName);
            return true;
        }
    }

    m_physicalDevice = devices[0];
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
    m_capabilities.id_vendor = props.vendorID;
    m_capabilities.id_device = props.deviceID;
    Msg("* [VulkanBackend] Using GPU (fallback): %s", props.deviceName);
    return true;
}

bool VulkanBackend::CreateSurface(SDL_Window* window) {
    if (!SDL_Vulkan_CreateSurface(window, m_instance, nullptr, &m_surface)) {
        Msg("! [VulkanBackend] SDL_Vulkan_CreateSurface failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool VulkanBackend::CreateLogicalDevice() {
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
    xr_vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

    m_graphicsQueueFamily = UINT32_MAX;
    m_computeQueueFamily = UINT32_MAX;
    for (u32 i = 0; i < queueFamilyCount; i++) {
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface, &presentSupport);

        if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
            m_graphicsQueueFamily = i;
        }

        if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
            !(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            m_computeQueueFamily = i;
        }
    }

    if (m_graphicsQueueFamily == UINT32_MAX) {
        Msg("! [VulkanBackend] No graphics+present queue family found");
        return false;
    }

    // If no dedicated compute family, try using a second queue from graphics family
    bool useGraphicsFamilyForCompute = false;
    if (m_computeQueueFamily == UINT32_MAX) {
        if (queueFamilies[m_graphicsQueueFamily].queueCount >= 2) {
            m_computeQueueFamily = m_graphicsQueueFamily;
            useGraphicsFamilyForCompute = true;
        }
    }

    float queuePriorities[2] = { 1.0f, 1.0f };

    xr_vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    VkDeviceQueueCreateInfo graphicsQueueInfo = {};
    graphicsQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueInfo.queueFamilyIndex = m_graphicsQueueFamily;
    graphicsQueueInfo.queueCount = useGraphicsFamilyForCompute ? 2 : 1;
    graphicsQueueInfo.pQueuePriorities = queuePriorities;
    queueCreateInfos.push_back(graphicsQueueInfo);

    if (m_computeQueueFamily != UINT32_MAX && !useGraphicsFamilyForCompute) {
        VkDeviceQueueCreateInfo computeQueueInfo = {};
        computeQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        computeQueueInfo.queueFamilyIndex = m_computeQueueFamily;
        computeQueueInfo.queueCount = 1;
        computeQueueInfo.pQueuePriorities = queuePriorities;
        queueCreateInfos.push_back(computeQueueInfo);
    }

    u32 availExtCount = 0;
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &availExtCount, nullptr);
    xr_vector<VkExtensionProperties> availExts(availExtCount);
    if (availExtCount)
        vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &availExtCount, availExts.data());
    auto hasExt = [&](const char* name) -> bool {
        for (const auto& e : availExts)
            if (!strcmp(e.extensionName, name))
                return true;
        return false;
    };

    xr_vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    };
#if defined(XR_PLATFORM_APPLE)
    deviceExtensions.push_back("VK_KHR_portability_subset");
#endif

    bool wantRT =
        hasExt(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) &&
        hasExt(VK_KHR_RAY_QUERY_EXTENSION_NAME) &&
        hasExt(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    if (wantRT) {
        deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    }
#if defined(XRAY_USE_DLSS)
    {
        xr_vector<const char*> ngxDevExts;
        xray::render::fg::Streamline_GetRequiredDeviceExtensions(m_physicalDevice, ngxDevExts);
        for (const char* e : ngxDevExts) {
            if (e && hasExt(e))
                deviceExtensions.push_back(e);
        }
    }
#endif

    const bool wantPresentId = hasExt(VK_KHR_PRESENT_ID_EXTENSION_NAME);
    const bool wantLowLatency2 = wantPresentId && hasExt(VK_NV_LOW_LATENCY_2_EXTENSION_NAME);
    if (wantPresentId)
        deviceExtensions.push_back(VK_KHR_PRESENT_ID_EXTENSION_NAME);
    if (wantLowLatency2)
        deviceExtensions.push_back(VK_NV_LOW_LATENCY_2_EXTENSION_NAME);

    VkPhysicalDeviceVulkan12Features vulkan12Features = {};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.drawIndirectCount = VK_TRUE;
    vulkan12Features.descriptorIndexing = VK_TRUE;
    vulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    vulkan12Features.runtimeDescriptorArray = VK_TRUE;
    vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
    vulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;
    vulkan12Features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    vulkan12Features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    vulkan12Features.timelineSemaphore = VK_TRUE;
    vulkan12Features.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures = {};
    asFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    asFeatures.accelerationStructure = VK_TRUE;

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {};
    rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    rayQueryFeatures.rayQuery = VK_TRUE;

    VkPhysicalDevicePresentIdFeaturesKHR presentIdFeatures = {};
    presentIdFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR;
    presentIdFeatures.presentId = VK_TRUE;

    VkPhysicalDeviceSynchronization2Features sync2Features = {};
    sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2Features.synchronization2 = VK_TRUE;
    vulkan12Features.pNext = &sync2Features;

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures = {};
    dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
    sync2Features.pNext = &dynamicRenderingFeatures;

    VkPhysicalDeviceVulkan11Features vulkan11Features = {};
    vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vulkan11Features.shaderDrawParameters = VK_TRUE;
    dynamicRenderingFeatures.pNext = &vulkan11Features;

    void** featureTail = &vulkan11Features.pNext;
    if (wantRT) {
        vulkan11Features.pNext = &asFeatures;
        asFeatures.pNext = &rayQueryFeatures;
        featureTail = &rayQueryFeatures.pNext;
    }
    if (wantPresentId)
        *featureTail = &presentIdFeatures;

    VkPhysicalDeviceFeatures2 features2 = {};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &vulkan12Features;
    features2.features.samplerAnisotropy = VK_TRUE;
    features2.features.fillModeNonSolid = VK_TRUE;
    features2.features.multiDrawIndirect = VK_TRUE;
    features2.features.drawIndirectFirstInstance = VK_TRUE;
    features2.features.independentBlend = VK_TRUE;
    features2.features.shaderStorageImageReadWithoutFormat = VK_TRUE;
    features2.features.shaderStorageImageWriteWithoutFormat = VK_TRUE;
    features2.features.multiViewport = VK_TRUE;
    features2.features.textureCompressionBC = VK_TRUE;
    features2.features.imageCubeArray = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &features2;
    deviceCreateInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.enabledExtensionCount = static_cast<u32>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    {
        VkPhysicalDeviceVulkan12Features sup12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        VkPhysicalDeviceVulkan11Features sup11 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
        VkPhysicalDeviceAccelerationStructureFeaturesKHR supAS = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
        VkPhysicalDeviceRayQueryFeaturesKHR supRQ = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR };
        VkPhysicalDevicePresentIdFeaturesKHR supPresentId = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR };
        VkPhysicalDeviceFeatures2 sup2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        sup2.pNext = &sup12;
        sup12.pNext = &sup11;
        void** supTail = &sup11.pNext;
        if (wantRT) {
            sup11.pNext = &supAS;
            supAS.pNext = &supRQ;
            supTail = &supRQ.pNext;
        }
        if (wantPresentId)
            *supTail = &supPresentId;
        vkGetPhysicalDeviceFeatures2(m_physicalDevice, &sup2);

#define CLAMP12(F) do { if (vulkan12Features.F && !sup12.F) { Msg("! [VulkanBackend] vk12 feature unsupported: " #F); vulkan12Features.F = VK_FALSE; } } while(0)
        CLAMP12(drawIndirectCount);
        CLAMP12(descriptorIndexing);
        CLAMP12(shaderSampledImageArrayNonUniformIndexing);
        CLAMP12(runtimeDescriptorArray);
        CLAMP12(descriptorBindingPartiallyBound);
        CLAMP12(descriptorBindingVariableDescriptorCount);
        CLAMP12(descriptorBindingSampledImageUpdateAfterBind);
        CLAMP12(descriptorBindingStorageBufferUpdateAfterBind);
        CLAMP12(timelineSemaphore);
        CLAMP12(bufferDeviceAddress);
#undef CLAMP12
#define CLAMPF(F) do { if (features2.features.F && !sup2.features.F) { Msg("! [VulkanBackend] feature unsupported: " #F); features2.features.F = VK_FALSE; } } while(0)
        CLAMPF(samplerAnisotropy);
        CLAMPF(fillModeNonSolid);
        CLAMPF(multiDrawIndirect);
        CLAMPF(independentBlend);
        CLAMPF(shaderStorageImageReadWithoutFormat);
        CLAMPF(shaderStorageImageWriteWithoutFormat);
        CLAMPF(multiViewport);
        CLAMPF(textureCompressionBC);
        CLAMPF(imageCubeArray);
#undef CLAMPF
        if (!features2.features.textureCompressionBC)
            Msg("! [VulkanBackend] textureCompressionBC unavailable — BC1-BC7 textures will fail");
        if (vulkan11Features.shaderDrawParameters && !sup11.shaderDrawParameters) {
            Msg("! [VulkanBackend] vk11 feature unsupported: shaderDrawParameters");
            vulkan11Features.shaderDrawParameters = VK_FALSE;
        }
        if (wantRT) {
            if (!supAS.accelerationStructure) {
                Msg("! [VulkanBackend] accelerationStructure unsupported");
                asFeatures.accelerationStructure = VK_FALSE;
            }
            if (!supRQ.rayQuery) {
                Msg("! [VulkanBackend] rayQuery unsupported");
                rayQueryFeatures.rayQuery = VK_FALSE;
            }
        }

        m_featureDrawIndirectCount = vulkan12Features.drawIndirectCount == VK_TRUE;
        m_featureMultiDrawIndirect = features2.features.multiDrawIndirect == VK_TRUE;
        m_featureDescriptorIndexing = vulkan12Features.descriptorIndexing == VK_TRUE;
        m_featureShaderDrawParameters = vulkan11Features.shaderDrawParameters == VK_TRUE;
        m_featureBufferDeviceAddress = vulkan12Features.bufferDeviceAddress == VK_TRUE;
        m_featureRayTracing = wantRT &&
            asFeatures.accelerationStructure == VK_TRUE &&
            rayQueryFeatures.rayQuery == VK_TRUE &&
            m_featureBufferDeviceAddress;

        if (wantPresentId && !supPresentId.presentId)
        {
            Msg("! [VulkanBackend] presentId feature unsupported");
            presentIdFeatures.presentId = VK_FALSE;
            m_presentIdAvailable = false;
        }
        else
            m_presentIdAvailable = wantPresentId && presentIdFeatures.presentId == VK_TRUE;
        m_latencyAvailable = wantLowLatency2 && m_presentIdAvailable;

        Msg("* [VulkanBackend] feature clamp summary:");
        Msg("*   drawIndirectCount      = %s (device: %s)",
            m_featureDrawIndirectCount ? "ENABLED" : "DISABLED",
            sup12.drawIndirectCount ? "supported" : "unsupported");
        Msg("*   multiDrawIndirect      = %s (device: %s)",
            m_featureMultiDrawIndirect ? "ENABLED" : "DISABLED",
            sup2.features.multiDrawIndirect ? "supported" : "unsupported");
        Msg("*   descriptorIndexing     = %s (device: %s)",
            m_featureDescriptorIndexing ? "ENABLED" : "DISABLED",
            sup12.descriptorIndexing ? "supported" : "unsupported");
        Msg("*   shaderDrawParameters   = %s (device: %s)",
            m_featureShaderDrawParameters ? "ENABLED" : "DISABLED",
            sup11.shaderDrawParameters ? "supported" : "unsupported");
        Msg("*   bufferDeviceAddress    = %s",
            m_featureBufferDeviceAddress ? "ENABLED" : "DISABLED");
        Msg("*   rayTracing (AS+RQ)     = %s",
            m_featureRayTracing ? "ENABLED" : "DISABLED");
        Msg("*   presentId              = %s",
            m_presentIdAvailable ? "ENABLED" : "DISABLED");
        Msg("*   lowLatency2 (Reflex)   = %s",
            m_latencyAvailable ? "ENABLED" : "DISABLED");
        Msg("*   runtimeDescriptorArray = %s",
            vulkan12Features.runtimeDescriptorArray ? "ENABLED" : "DISABLED");
        Msg("*   partiallyBound         = %s",
            vulkan12Features.descriptorBindingPartiallyBound ? "ENABLED" : "DISABLED");
    }

    VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
    if (result != VK_SUCCESS && wantRT) {
        Msg("! [VulkanBackend] vkCreateDevice failed with RT (%d), retrying without RT", result);
        wantRT = false;
        m_featureRayTracing = false;
        asFeatures.accelerationStructure = VK_FALSE;
        rayQueryFeatures.rayQuery = VK_FALSE;
        vulkan11Features.pNext = wantPresentId ? &presentIdFeatures : nullptr;
        deviceExtensions.clear();
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
#if defined(XR_PLATFORM_APPLE)
        deviceExtensions.push_back("VK_KHR_portability_subset");
#endif
#if defined(XRAY_USE_DLSS)
        {
            xr_vector<const char*> ngxDevExts;
            xray::render::fg::Streamline_GetRequiredDeviceExtensions(m_physicalDevice, ngxDevExts);
            for (const char* e : ngxDevExts) {
                if (e && hasExt(e))
                    deviceExtensions.push_back(e);
            }
        }
#endif
        if (wantPresentId)
            deviceExtensions.push_back(VK_KHR_PRESENT_ID_EXTENSION_NAME);
        if (wantLowLatency2)
            deviceExtensions.push_back(VK_NV_LOW_LATENCY_2_EXTENSION_NAME);
        deviceCreateInfo.enabledExtensionCount = static_cast<u32>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
        result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
    }
    if (result != VK_SUCCESS) {
        Msg("! [VulkanBackend] vkCreateDevice failed: %d", result);
        if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY || result == VK_ERROR_INITIALIZATION_FAILED) {
            Msg("! [VulkanBackend] GPU busy/OOM — kill leftover xr_3da (often Ctrl+Z stopped): pkill -9 xr_3da");
        }
        return false;
    }

    vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);

    if (m_computeQueueFamily != UINT32_MAX) {
        u32 computeQueueIndex = useGraphicsFamilyForCompute ? 1 : 0;
        vkGetDeviceQueue(m_device, m_computeQueueFamily, computeQueueIndex, &m_computeQueue);
        Msg("* [VulkanBackend] Compute queue: family %u, index %u%s",
            m_computeQueueFamily, computeQueueIndex,
            useGraphicsFamilyForCompute ? " (shared family)" : " (dedicated)");
    } else {
        Msg("* [VulkanBackend] No compute queue available (async compute disabled)");
    }

    InitLatencyExtension();

    Msg("* [VulkanBackend] Logical device created (graphics family %u)", m_graphicsQueueFamily);
    return true;
}

bool VulkanBackend::CreateSwapChain(u32 width, u32 height) {
    VkSurfaceCapabilitiesKHR surfaceCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfaceCaps);

    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    xr_vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

    m_swapchainFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    bool foundFormat = false;
    for (const auto& fmt : formats) {
        if (fmt.format == VK_FORMAT_R8G8B8A8_UNORM) {
            m_swapchainFormat = fmt.format;
            colorSpace = fmt.colorSpace;
            foundFormat = true;
            break;
        }
    }
    if (!foundFormat) {
        for (const auto& fmt : formats) {
            if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM) {
                m_swapchainFormat = fmt.format;
                colorSpace = fmt.colorSpace;
                break;
            }
        }
    }

    const bool wantVSync = psDeviceFlags.test(rsVSync);
    const bool wantFg = ps_r_upscale == 2 && ps_r_dlss_fg != 0;
    m_createdWithVSync = wantVSync;
    m_createdWithFg = wantFg;
    const VkPresentModeKHR presentMode = SelectPresentMode(wantVSync, wantFg);
    m_presentMode = presentMode;
    Msg("* [VulkanBackend] Present mode: %s (vsync=%s fg=%s)",
        presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR ? "IMMEDIATE" :
        presentMode == VK_PRESENT_MODE_MAILBOX_KHR   ? "MAILBOX"   : "FIFO",
        wantVSync ? "on" : "off",
        wantFg ? "on" : "off");

    VkExtent2D extent = { width, height };
    if (surfaceCaps.currentExtent.width != UINT32_MAX)
        extent = surfaceCaps.currentExtent;

    u32 imageCount = wantFg ? BACK_BUFFER_COUNT : 3;
    if (imageCount < surfaceCaps.minImageCount)
        imageCount = surfaceCaps.minImageCount;
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR && imageCount < surfaceCaps.minImageCount + 1)
        imageCount = surfaceCaps.minImageCount + 1;
    if (surfaceCaps.maxImageCount > 0 && imageCount > surfaceCaps.maxImageCount)
        imageCount = surfaceCaps.maxImageCount;
    if (imageCount > BACK_BUFFER_COUNT)
        imageCount = BACK_BUFFER_COUNT;

    VkSwapchainLatencyCreateInfoNV latencyCreateInfo = {};
    latencyCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_LATENCY_CREATE_INFO_NV;
    latencyCreateInfo.latencyModeEnable = m_latencyAvailable ? VK_TRUE : VK_FALSE;

    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    if (m_latencyAvailable)
        swapchainInfo.pNext = &latencyCreateInfo;
    swapchainInfo.surface = m_surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = m_swapchainFormat;
    swapchainInfo.imageColorSpace = colorSpace;
    swapchainInfo.imageExtent = extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.preTransform = surfaceCaps.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(m_device, &swapchainInfo, nullptr, &m_swapchain);
    if (result != VK_SUCCESS) {
        Msg("! [VulkanBackend] vkCreateSwapchainKHR failed: %d", result);
        return false;
    }

    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());

    m_backBufferWidth = extent.width;
    m_backBufferHeight = extent.height;
    m_currentImageIndex = 0;
    m_oobPresentQueueNotified = false;
    m_appliedReflexMode = -1;

    if (m_latencyAvailable)
    {
        const int mode = ps_r_reflex;
        u32 intervalUs = 0;
        if (ps_fps_limit > 0)
            intervalUs = 1000000u / static_cast<u32>(ps_fps_limit);
        ApplyLowLatencyMode(mode, intervalUs);
    }

    Msg("* [VulkanBackend] Swapchain created: %ux%u, %u images, format %d",
        extent.width, extent.height, imageCount, m_swapchainFormat);
    return true;
}

void VulkanBackend::DestroySwapChain() {
    for (auto& bb : m_backBuffers)
        bb = nullptr;
    m_swapchainImages.clear();

    if (m_swapchain) {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
}

void VulkanBackend::CreateBackBufferTextures() {
    nvrhi::Format nvFormat = (m_swapchainFormat == VK_FORMAT_R8G8B8A8_UNORM)
        ? nvrhi::Format::RGBA8_UNORM
        : nvrhi::Format::BGRA8_UNORM;

    for (u32 i = 0; i < m_swapchainImages.size() && i < BACK_BUFFER_COUNT; i++) {
        nvrhi::TextureDesc desc;
        desc.width = m_backBufferWidth;
        desc.height = m_backBufferHeight;
        desc.format = nvFormat;
        desc.isRenderTarget = true;
        desc.debugName = "BackBuffer";
        desc.keepInitialState = true;
        desc.initialState = nvrhi::ResourceStates::Present;

        m_backBuffers[i] = m_nvrhiDevice->createHandleForNativeTexture(
            nvrhi::ObjectTypes::VK_Image,
            nvrhi::Object(m_swapchainImages[i]),
            desc
        );
    }
}

void VulkanBackend::CreateSyncObjects() {
    VkSemaphoreCreateInfo semInfo = {};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (u32 i = 0; i < BACK_BUFFER_COUNT; i++) {
        vkCreateSemaphore(m_device, &semInfo, nullptr, &m_imageAvailable[i]);
        vkCreateSemaphore(m_device, &semInfo, nullptr, &m_renderFinished[i]);
        vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFence[i]);
    }
}

void VulkanBackend::DestroySyncObjects() {
    if (!m_device)
        return;
    for (u32 i = 0; i < BACK_BUFFER_COUNT; i++) {
        if (m_imageAvailable[i]) {
            vkDestroySemaphore(m_device, m_imageAvailable[i], nullptr);
            m_imageAvailable[i] = VK_NULL_HANDLE;
        }
        if (m_renderFinished[i]) {
            vkDestroySemaphore(m_device, m_renderFinished[i], nullptr);
            m_renderFinished[i] = VK_NULL_HANDLE;
        }
        if (m_inFlightFence[i]) {
            vkDestroyFence(m_device, m_inFlightFence[i], nullptr);
            m_inFlightFence[i] = VK_NULL_HANDLE;
        }
    }
}

VkPresentModeKHR VulkanBackend::SelectPresentMode(bool wantVSync, bool wantFg) const
{
    if (wantVSync)
        return VK_PRESENT_MODE_FIFO_KHR;

    u32 presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);
    xr_vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (presentModeCount)
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, presentModes.data());

    bool hasImmediate = false, hasMailbox = false;
    for (auto mode : presentModes)
    {
        if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) hasImmediate = true;
        else if (mode == VK_PRESENT_MODE_MAILBOX_KHR) hasMailbox = true;
    }

    if (wantFg)
        return hasMailbox ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
    if (hasImmediate) return VK_PRESENT_MODE_IMMEDIATE_KHR;
    if (hasMailbox) return VK_PRESENT_MODE_MAILBOX_KHR;
    return VK_PRESENT_MODE_FIFO_KHR;
}

void VulkanBackend::InitLatencyExtension()
{
    m_vkSetLatencySleepModeNV = nullptr;
    m_vkLatencySleepNV = nullptr;
    m_vkSetLatencyMarkerNV = nullptr;
    m_vkQueueNotifyOutOfBandNV = nullptr;
    if (!m_latencyAvailable || !m_device)
        return;

    m_vkSetLatencySleepModeNV = reinterpret_cast<PFN_vkSetLatencySleepModeNV>(
        vkGetDeviceProcAddr(m_device, "vkSetLatencySleepModeNV"));
    m_vkLatencySleepNV = reinterpret_cast<PFN_vkLatencySleepNV>(
        vkGetDeviceProcAddr(m_device, "vkLatencySleepNV"));
    m_vkSetLatencyMarkerNV = reinterpret_cast<PFN_vkSetLatencyMarkerNV>(
        vkGetDeviceProcAddr(m_device, "vkSetLatencyMarkerNV"));
    m_vkQueueNotifyOutOfBandNV = reinterpret_cast<PFN_vkQueueNotifyOutOfBandNV>(
        vkGetDeviceProcAddr(m_device, "vkQueueNotifyOutOfBandNV"));

    if (!m_vkSetLatencySleepModeNV || !m_vkLatencySleepNV || !m_vkSetLatencyMarkerNV)
    {
        Msg("! [VulkanBackend] VK_NV_low_latency2 entry points missing");
        m_latencyAvailable = false;
        return;
    }

    VkSemaphoreTypeCreateInfo timelineInfo = {};
    timelineInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    timelineInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineInfo.initialValue = 0;
    VkSemaphoreCreateInfo semInfo = {};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semInfo.pNext = &timelineInfo;
    if (vkCreateSemaphore(m_device, &semInfo, nullptr, &m_latencySleepSemaphore) != VK_SUCCESS)
    {
        Msg("! [VulkanBackend] Failed to create Reflex sleep timeline semaphore");
        m_latencyAvailable = false;
        m_latencySleepSemaphore = VK_NULL_HANDLE;
        return;
    }
    m_latencySleepValue = 0;
    Msg("* [VulkanBackend] NVIDIA Reflex (VK_NV_low_latency2) ready");
}

void VulkanBackend::DestroyLatencyObjects()
{
    if (m_device && m_latencySleepSemaphore)
    {
        vkDestroySemaphore(m_device, m_latencySleepSemaphore, nullptr);
        m_latencySleepSemaphore = VK_NULL_HANDLE;
    }
    m_vkSetLatencySleepModeNV = nullptr;
    m_vkLatencySleepNV = nullptr;
    m_vkSetLatencyMarkerNV = nullptr;
    m_vkQueueNotifyOutOfBandNV = nullptr;
}

void VulkanBackend::ApplyLowLatencyMode(int mode, u32 minIntervalUs)
{
    if (!m_latencyAvailable || !m_vkSetLatencySleepModeNV || !m_swapchain)
        return;
    if (mode < 0) mode = 0;
    if (mode > 2) mode = 2;
    if (mode == m_appliedReflexMode && minIntervalUs == m_appliedReflexIntervalUs)
        return;

    VkLatencySleepModeInfoNV info = {};
    info.sType = VK_STRUCTURE_TYPE_LATENCY_SLEEP_MODE_INFO_NV;
    info.lowLatencyMode = (mode >= 1) ? VK_TRUE : VK_FALSE;
    info.lowLatencyBoost = (mode >= 2) ? VK_TRUE : VK_FALSE;
    info.minimumIntervalUs = minIntervalUs;
    const VkResult res = m_vkSetLatencySleepModeNV(m_device, m_swapchain, &info);
    if (res != VK_SUCCESS)
    {
        Msg("! [VulkanBackend] vkSetLatencySleepModeNV failed: %d", static_cast<int>(res));
        return;
    }
    m_appliedReflexMode = mode;
    m_appliedReflexIntervalUs = minIntervalUs;
}

void VulkanBackend::LatencySleep()
{
    if (!m_latencyAvailable || !m_vkLatencySleepNV || !m_swapchain || !m_latencySleepSemaphore)
        return;

    const u64 waitValue = ++m_latencySleepValue;
    VkLatencySleepInfoNV sleepInfo = {};
    sleepInfo.sType = VK_STRUCTURE_TYPE_LATENCY_SLEEP_INFO_NV;
    sleepInfo.signalSemaphore = m_latencySleepSemaphore;
    sleepInfo.value = waitValue;
    if (m_vkLatencySleepNV(m_device, m_swapchain, &sleepInfo) != VK_SUCCESS)
        return;

    VkSemaphoreWaitInfo waitInfo = {};
    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    waitInfo.semaphoreCount = 1;
    waitInfo.pSemaphores = &m_latencySleepSemaphore;
    waitInfo.pValues = &waitValue;
    vkWaitSemaphores(m_device, &waitInfo, UINT64_MAX);
}

void VulkanBackend::SetLatencyMarkerNV(VkLatencyMarkerNV marker, u64 presentId)
{
    if (!m_latencyAvailable || !m_vkSetLatencyMarkerNV || !m_swapchain)
        return;
    VkSetLatencyMarkerInfoNV info = {};
    info.sType = VK_STRUCTURE_TYPE_SET_LATENCY_MARKER_INFO_NV;
    info.presentID = presentId;
    info.marker = marker;
    m_vkSetLatencyMarkerNV(m_device, m_swapchain, &info);
}

void VulkanBackend::SetLatencyMarker(LatencyMarker marker)
{
    if (!m_latencyAvailable)
        return;
    VkLatencyMarkerNV vkMarker = VK_LATENCY_MARKER_SIMULATION_START_NV;
    switch (marker)
    {
    case LatencyMarker::SimulationStart: vkMarker = VK_LATENCY_MARKER_SIMULATION_START_NV; break;
    case LatencyMarker::SimulationEnd: vkMarker = VK_LATENCY_MARKER_SIMULATION_END_NV; break;
    case LatencyMarker::RenderSubmitStart: vkMarker = VK_LATENCY_MARKER_RENDERSUBMIT_START_NV; break;
    case LatencyMarker::RenderSubmitEnd: vkMarker = VK_LATENCY_MARKER_RENDERSUBMIT_END_NV; break;
    case LatencyMarker::PresentStart: vkMarker = VK_LATENCY_MARKER_PRESENT_START_NV; break;
    case LatencyMarker::PresentEnd: vkMarker = VK_LATENCY_MARKER_PRESENT_END_NV; break;
    case LatencyMarker::OutOfBandPresentStart: vkMarker = VK_LATENCY_MARKER_OUT_OF_BAND_PRESENT_START_NV; break;
    case LatencyMarker::OutOfBandPresentEnd: vkMarker = VK_LATENCY_MARKER_OUT_OF_BAND_PRESENT_END_NV; break;
    default: return;
    }
    SetLatencyMarkerNV(vkMarker, m_nextPresentId);
}

void VulkanBackend::AssociateLatencyPresentId(u64 presentId)
{
    if (!m_latencyAvailable || !m_device)
        return;
    VkLatencySubmissionPresentIdNV idInfo = {};
    idInfo.sType = VK_STRUCTURE_TYPE_LATENCY_SUBMISSION_PRESENT_ID_NV;
    idInfo.presentID = presentId;
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = &idInfo;
    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
}

void VulkanBackend::CreateBindlessResources() {
    Msg("* [VulkanBackend] Creating bindless resources...");

    nvrhi::BindlessLayoutDesc bindlessDesc;
    bindlessDesc.visibility = nvrhi::ShaderType::All;
    bindlessDesc.firstSlot = 0;
    bindlessDesc.maxCapacity = MAX_BINDLESS_TEXTURES;
    bindlessDesc.registerSpaces = { nvrhi::BindingLayoutItem::Texture_SRV(1) };

    m_bindlessLayout = m_nvrhiDevice->createBindlessLayout(bindlessDesc);
    if (!m_bindlessLayout) {
        Msg("! [VulkanBackend] Failed to create bindless layout");
        return;
    }

    m_bindlessDescriptorTable = m_nvrhiDevice->createDescriptorTable(m_bindlessLayout);
    if (!m_bindlessDescriptorTable) {
        Msg("! [VulkanBackend] Failed to create bindless descriptor table");
        return;
    }

    m_nvrhiDevice->resizeDescriptorTable(m_bindlessDescriptorTable, MAX_BINDLESS_TEXTURES, false);
    Msg("* [VulkanBackend] Bindless resources created (max %u textures)", MAX_BINDLESS_TEXTURES);
}

void VulkanBackend::QueryCapabilities() {
    m_capabilities.bindlessTextures = m_featureDescriptorIndexing;
    m_capabilities.maxBindlessResources = MAX_BINDLESS_TEXTURES;
    m_capabilities.shaderModel = 60;
    m_capabilities.drawIndirectCount = m_featureDrawIndirectCount;
    m_capabilities.multiDrawIndirect = m_featureMultiDrawIndirect;
    m_capabilities.descriptorIndexing = m_featureDescriptorIndexing;
    m_capabilities.shaderDrawParameters = m_featureShaderDrawParameters;
    m_capabilities.rayTracing = m_featureRayTracing;

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &props);

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(m_physicalDevice, &features);

    m_capabilities.geometry.dwRegisters = 256;
    m_capabilities.geometry.dwInstructions = 65535;
    m_capabilities.geometry.dwClipPlanes = 6;
    m_capabilities.geometry.dwVertexCache = 24;
    m_capabilities.geometry.bVTF = true;

    m_capabilities.raster.dwRegisters = 256;
    m_capabilities.raster.dwInstructions = 65535;
    m_capabilities.raster.dwStages = 16;
    m_capabilities.raster.dwMRT_count = 8;
    m_capabilities.raster.b_MRT_mixdepth = true;

    m_capabilities.raster_major = 6;
    m_capabilities.raster_minor = 0;
    m_capabilities.raster_profile = "ps_6_0";
    m_capabilities.geometry_major = 6;
    m_capabilities.geometry_minor = 0;
    m_capabilities.geometry_profile = "vs_6_0";
    m_capabilities.iGPUNum = 1;

    m_capabilities.hasStencil = true;
    m_capabilities.hasScissor = true;
    m_capabilities.hasFixedPipeline = false;
    m_capabilities.useCombinedSamplers = false;
}

u32 VulkanBackend::RegisterBindlessTexture(nvrhi::ITexture* texture) {
    if (!m_bindlessDescriptorTable || !texture)
        return UINT32_MAX;

    auto it = m_bindlessTextureMap.find(texture);
    if (it != m_bindlessTextureMap.end())
        return it->second;

    u32 slot;
    if (!m_freeBindlessIndices.empty()) {
        slot = m_freeBindlessIndices.back();
        m_freeBindlessIndices.pop_back();
    } else {
        if (m_nextBindlessIndex >= MAX_BINDLESS_TEXTURES) {
            Msg("! [VulkanBackend] Bindless texture limit reached");
            return UINT32_MAX;
        }
        slot = m_nextBindlessIndex++;
    }

    nvrhi::BindingSetItem item = nvrhi::BindingSetItem::Texture_SRV(0, texture);
    item.slot = slot;

    if (!m_nvrhiDevice->writeDescriptorTable(m_bindlessDescriptorTable, item)) {
        m_freeBindlessIndices.push_back(slot);
        return UINT32_MAX;
    }

    m_bindlessTextureMap[texture] = slot;
    return slot;
}

void VulkanBackend::UnregisterBindlessTexture(u32 index) {
    if (index >= MAX_BINDLESS_TEXTURES)
        return;
    for (auto it = m_bindlessTextureMap.begin(); it != m_bindlessTextureMap.end(); ++it) {
        if (it->second == index) {
            m_bindlessTextureMap.erase(it);
            break;
        }
    }
    m_freeBindlessIndices.push_back(index);
}

nvrhi::ITexture* VulkanBackend::GetBackBuffer() {
    const u32 idx = (m_currentImageIndex < BACK_BUFFER_COUNT) ? m_currentImageIndex : 0;
    return m_backBuffers[idx].Get();
}

void VulkanBackend::RequestSwapchainRecreate(bool fromOutOfDate)
{
    if (fromOutOfDate)
    {
        m_suboptimalAccepted.store(false, std::memory_order_relaxed);
        m_swapchainDirty.store(true, std::memory_order_relaxed);
        return;
    }

    // SUBOPTIMAL is still presentable. Recreate once, then ignore until OUT_OF_DATE.
    if (m_suboptimalAccepted.load(std::memory_order_relaxed))
        return;
    m_swapchainDirty.store(true, std::memory_order_relaxed);
}

bool VulkanBackend::RecreateSwapchainFromSurface()
{
    VkSurfaceCapabilitiesKHR caps = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);

    u32 w = m_backBufferWidth;
    u32 h = m_backBufferHeight;
    if (caps.currentExtent.width != UINT32_MAX)
    {
        w = caps.currentExtent.width;
        h = caps.currentExtent.height;
    }
    else
    {
        w = std::clamp(w, caps.minImageExtent.width, caps.maxImageExtent.width);
        h = std::clamp(h, caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    if (w == 0 || h == 0)
        return false;

    Msg("* [VulkanBackend] Recreating swapchain: %ux%u → %ux%u",
        m_backBufferWidth, m_backBufferHeight, w, h);
    ResizeSwapChain(w, h);
    m_swapchainDirty.store(false, std::memory_order_relaxed);
    m_suboptimalAccepted.store(true, std::memory_order_relaxed);
    return true;
}

void VulkanBackend::Present(bool /*vsync*/) {
    if (m_asyncSubmit)
        return;
    PresentInternal(false);
}

void VulkanBackend::PresentInternal(bool outOfBand)
{
    ZoneScopedN("VulkanBackend::Present");

    std::scoped_lock sc(m_swapchainMutex, m_queueMutex);

    const u64 presentId = m_nextPresentId++;
    AssociateLatencyPresentId(presentId);

    if (outOfBand)
    {
        if (m_vkQueueNotifyOutOfBandNV && !m_oobPresentQueueNotified)
        {
            VkOutOfBandQueueTypeInfoNV qinfo{};
            qinfo.sType = VK_STRUCTURE_TYPE_OUT_OF_BAND_QUEUE_TYPE_INFO_NV;
            qinfo.queueType = VK_OUT_OF_BAND_QUEUE_TYPE_PRESENT_NV;
            m_vkQueueNotifyOutOfBandNV(m_graphicsQueue, &qinfo);
            m_oobPresentQueueNotified = true;
        }
        SetLatencyMarkerNV(VK_LATENCY_MARKER_OUT_OF_BAND_PRESENT_START_NV, presentId);
    }
    else
        SetLatencyMarkerNV(VK_LATENCY_MARKER_PRESENT_START_NV, presentId);

    VkPresentIdKHR presentIdInfo = {};
    presentIdInfo.sType = VK_STRUCTURE_TYPE_PRESENT_ID_KHR;
    presentIdInfo.swapchainCount = 1;
    presentIdInfo.pPresentIds = &presentId;

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    if (m_presentIdAvailable)
        presentInfo.pNext = &presentIdInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_renderFinished[m_currentFrameIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &m_currentImageIndex;

    VkResult result = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        RequestSwapchainRecreate(true);
    else if (result == VK_SUBOPTIMAL_KHR)
        RequestSwapchainRecreate(false);

    if (outOfBand)
        SetLatencyMarkerNV(VK_LATENCY_MARKER_OUT_OF_BAND_PRESENT_END_NV, presentId);
    else
        SetLatencyMarkerNV(VK_LATENCY_MARKER_PRESENT_END_NV, presentId);

    VkSubmitInfo fenceSubmit = {};
    fenceSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vkQueueSubmit(m_graphicsQueue, 1, &fenceSubmit, m_inFlightFence[m_currentFrameIndex]);

    m_currentFrameIndex = (m_currentFrameIndex + 1) % BACK_BUFFER_COUNT;
}

bool VulkanBackend::PresentFrameGeneration(nvrhi::ITexture* interpolated, nvrhi::ITexture* real)
{
    (void)real;
    if (!interpolated || m_asyncSubmit)
        return false;

    ZoneScopedN("VulkanBackend::PresentFrameGeneration");

    const u32 prevFrame = (m_currentFrameIndex + BACK_BUFFER_COUNT - 1) % BACK_BUFFER_COUNT;
    {
        ZoneScopedN("VK::FG_WaitPrevPresentFence");
        vkWaitForFences(m_device, 1, &m_inFlightFence[prevFrame], VK_TRUE, UINT64_MAX);
    }

    {
        ZoneScopedN("VK::FG_WaitInFlightFence");
        vkWaitForFences(m_device, 1, &m_inFlightFence[m_currentFrameIndex], VK_TRUE, UINT64_MAX);
        vkResetFences(m_device, 1, &m_inFlightFence[m_currentFrameIndex]);
    }

    {
        std::lock_guard<std::mutex> sc(m_swapchainMutex);
        VkResult result = vkAcquireNextImageKHR(
            m_device, m_swapchain, UINT64_MAX,
            m_imageAvailable[m_currentFrameIndex], VK_NULL_HANDLE,
            &m_currentImageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RequestSwapchainRecreate(true);
            return false;
        }
        if (result == VK_SUBOPTIMAL_KHR)
            RequestSwapchainRecreate(false);
    }

    nvrhi::ITexture* dst = GetBackBuffer();
    if (!dst || dst->getDesc().format != interpolated->getDesc().format ||
        dst->getDesc().width != interpolated->getDesc().width ||
        dst->getDesc().height != interpolated->getDesc().height)
        return false;

    auto* cl = m_commandLists[1].Get();
    if (!cl)
        return false;

    {
        auto* vkDevice = static_cast<nvrhi::vulkan::IDevice*>(m_nvrhiVulkanDevice.Get());
        std::lock_guard<std::mutex> qk(m_queueMutex);
        vkDevice->queueWaitForSemaphore(
            nvrhi::CommandQueue::Graphics,
            m_imageAvailable[m_currentFrameIndex], 0);
        vkDevice->queueSignalSemaphore(
            nvrhi::CommandQueue::Graphics,
            m_renderFinished[m_currentFrameIndex], 0);

        cl->open();
        nvrhi::TextureSlice slice;
        cl->copyTexture(dst, slice, interpolated, slice);
        cl->close();
        m_nvrhiDevice->executeCommandList(cl);
    }

    PresentInternal(true);
    return true;
}

void VulkanBackend::ResizeSwapChain(u32 width, u32 height) {
    WaitForIdle();

    for (auto& bb : m_backBuffers)
        bb = nullptr;

    DestroySwapChain();
    CreateSwapChain(width, height);
    CreateBackBufferTextures();
    m_swapchainDirty.store(false, std::memory_order_relaxed);

    if (m_nvrhiDevice)
    {
        for (int i = 0; i < 3; ++i)
            m_nvrhiDevice->runGarbageCollection();
    }
}

void VulkanBackend::BeginFrame() {
    ZoneScopedN("VK::BeginFrame");

    const bool wantVSync = psDeviceFlags.test(rsVSync);
    const bool wantFg = ps_r_upscale == 2 && ps_r_dlss_fg != 0;
    if (m_asyncSubmit && wantFg)
    {
        FlushSubmits();
        m_asyncSubmit = false;
        Msg("! [VulkanBackend] async submit disabled — DLSS-FG enabled");
    }
    const VkPresentModeKHR desiredMode = SelectPresentMode(wantVSync, wantFg);
    if (wantVSync != m_createdWithVSync || wantFg != m_createdWithFg || desiredMode != m_presentMode)
        RequestSwapchainRecreate(true);

    if (m_latencyAvailable)
    {
        u32 intervalUs = 0;
        const int fpsCap = (Device.Paused() || g_pGameLevel == nullptr) ? ps_fps_limit_in_menu : ps_fps_limit;
        if (fpsCap > 0)
            intervalUs = 1000000u / static_cast<u32>(fpsCap);
        ApplyLowLatencyMode(ps_r_reflex, intervalUs);
    }

    if (m_gcTask) {
        ZoneScopedN("VK::WaitForGC");
        TaskScheduler->Wait(*m_gcTask);
        m_gcTask = nullptr;
    }

    if (m_asyncSubmit) {
        ZoneScopedN("VK::WaitSubmitSlot");
        std::unique_lock<std::mutex> lk(m_submitMutex);
        m_submitDoneCv.wait(lk, [&] { return !m_slotInFlight[m_recordSlot]; });
    }

    {
        ZoneScopedN("VK::WaitInFlightFence");
        vkWaitForFences(m_device, 1, &m_inFlightFence[m_currentFrameIndex], VK_TRUE, UINT64_MAX);
        vkResetFences(m_device, 1, &m_inFlightFence[m_currentFrameIndex]);
    }

    auto reSignalInFlightFence = [&]() {
        VkSubmitInfo fenceSubmit = {};
        fenceSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        std::lock_guard<std::mutex> qk(m_queueMutex);
        vkQueueSubmit(m_graphicsQueue, 1, &fenceSubmit, m_inFlightFence[m_currentFrameIndex]);
    };

    auto acquire = [&]() -> VkResult {
        ZoneScopedN("VK::AcquireNextImage");
        std::lock_guard<std::mutex> sc(m_swapchainMutex);
        return vkAcquireNextImageKHR(
            m_device, m_swapchain, UINT64_MAX,
            m_imageAvailable[m_currentFrameIndex], VK_NULL_HANDLE,
            &m_currentImageIndex);
    };

    // Present (possibly async) may have flagged a stale swapchain — recreate before acquire.
    if (m_swapchainDirty.load(std::memory_order_relaxed))
    {
        if (!RecreateSwapchainFromSurface())
        {
            reSignalInFlightFence();
            return;
        }
    }

    VkResult result = acquire();

    // OUT_OF_DATE must recreate. SUBOPTIMAL is presentable — recreate at most once
    // (same policy as Present), otherwise Linux drivers that always return SUBOPTIMAL
    // would recreate every frame.
    const bool acquireNeedsRecreate =
        result == VK_ERROR_OUT_OF_DATE_KHR ||
        (result == VK_SUBOPTIMAL_KHR && !m_suboptimalAccepted.load(std::memory_order_relaxed));

    if (acquireNeedsRecreate)
    {
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
            m_suboptimalAccepted.store(false, std::memory_order_relaxed);

        if (!RecreateSwapchainFromSurface())
        {
            reSignalInFlightFence();
            return;
        }

        result = acquire();
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            Msg("~ [VulkanBackend] Acquire still out of date after recreate — skipping frame");
            reSignalInFlightFence();
            return;
        }
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        Msg("! [VulkanBackend] vkAcquireNextImageKHR failed: %d", static_cast<int>(result));
        reSignalInFlightFence();
        return;
    }

    if (!m_asyncSubmit) {
        auto* vkDevice = static_cast<nvrhi::vulkan::IDevice*>(m_nvrhiVulkanDevice.Get());
        vkDevice->queueWaitForSemaphore(
            nvrhi::CommandQueue::Graphics,
            m_imageAvailable[m_currentFrameIndex], 0);
    }

    {
        ZoneScopedN("VK::CommandListOpen");
        m_commandLists[m_recordSlot]->open();
    }
    m_inFrame = true;
}

void VulkanBackend::EndFrame() {
    ZoneScopedN("VK::EndFrame");

    if (!m_inFrame)
        return;

    m_inFrame = false;

    if (m_asyncSubmit) {
        {
            ZoneScopedN("VK::CommandListClose");
            m_commandLists[m_recordSlot]->close();
        }

        SubmitJob job;
        job.cl = m_commandLists[m_recordSlot];
        job.imageAvailable = m_imageAvailable[m_currentFrameIndex];
        job.renderFinished = m_renderFinished[m_currentFrameIndex];
        job.fence = m_inFlightFence[m_currentFrameIndex];
        job.imageIndex = m_currentImageIndex;
        job.slot = m_recordSlot;
        job.enqueueTime = std::chrono::steady_clock::now();

        {
            std::lock_guard<std::mutex> lk(m_submitMutex);
            m_pendingJob = job;
            m_jobQueued = true;
            m_slotInFlight[m_recordSlot] = true;
        }
        m_submitCv.notify_one();

        m_recordSlot ^= 1;
        m_currentFrameIndex = (m_currentFrameIndex + 1) % BACK_BUFFER_COUNT;
        return;
    }

    auto* vkDevice = static_cast<nvrhi::vulkan::IDevice*>(m_nvrhiVulkanDevice.Get());
    {
        std::lock_guard<std::mutex> qk(m_queueMutex);
        vkDevice->queueSignalSemaphore(
            nvrhi::CommandQueue::Graphics,
            m_renderFinished[m_currentFrameIndex], 0);

        {
            ZoneScopedN("VK::CommandListClose");
            m_commandLists[m_recordSlot]->close();
        }

        {
            ZoneScopedN("VK::ExecuteCommandList");
            m_lastGraphicsInstanceID = m_nvrhiDevice->executeCommandList(m_commandLists[m_recordSlot]);
        }
    }

    nvrhi::IDevice* device = m_nvrhiDevice;
    m_gcTask = &TaskScheduler->AddTask([device] {
        if (device)
            device->runGarbageCollection();
    });
}

void VulkanBackend::SubmitThreadMain() {
#if defined(__APPLE__)
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif

    using Clock = std::chrono::steady_clock;
    auto usBetween = [](Clock::time_point a, Clock::time_point b) -> u64 {
        return static_cast<u64>(std::chrono::duration_cast<std::chrono::microseconds>(b - a).count());
    };

    for (;;) {
        SubmitJob job;
        {
            std::unique_lock<std::mutex> lk(m_submitMutex);
            m_submitCv.wait(lk, [&] { return m_jobQueued || !m_submitRun; });
            if (!m_submitRun && !m_jobQueued)
                return;
            job = m_pendingJob;
            m_jobQueued = false;
        }

        const auto tDequeue = Clock::now();
        m_stJobLatencyUs.store(usBetween(job.enqueueTime, tDequeue), std::memory_order_relaxed);

        auto* vkDevice = static_cast<nvrhi::vulkan::IDevice*>(m_nvrhiVulkanDevice.Get());
        {
            std::lock_guard<std::mutex> qk(m_queueMutex);
            const auto tLocked = Clock::now();
            m_stQueueLockUs.store(usBetween(tDequeue, tLocked), std::memory_order_relaxed);

            vkDevice->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics, job.imageAvailable, 0);
            vkDevice->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, job.renderFinished, 0);
            const auto tSem = Clock::now();
            m_stSemWaitUs.store(usBetween(tLocked, tSem), std::memory_order_relaxed);

            {
                ZoneScopedN("VK::ExecuteCommandList");
                m_lastGraphicsInstanceID = m_nvrhiDevice->executeCommandList(job.cl);
            }
            m_stEncodeUs.store(usBetween(tSem, Clock::now()), std::memory_order_relaxed);
        }

        {
            std::lock_guard<std::mutex> lk(m_submitMutex);
            m_slotInFlight[job.slot] = false;
        }
        m_submitDoneCv.notify_all();

        {
            ZoneScopedN("VulkanBackend::Present");
            const auto tPre = Clock::now();
            std::scoped_lock sc(m_swapchainMutex, m_queueMutex);
            const auto tPresentLocked = Clock::now();
            m_stPresentLockUs.store(usBetween(tPre, tPresentLocked), std::memory_order_relaxed);

            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &job.renderFinished;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &m_swapchain;
            presentInfo.pImageIndices = &job.imageIndex;

            VkResult result = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
            if (result == VK_ERROR_OUT_OF_DATE_KHR)
                RequestSwapchainRecreate(true);
            else if (result == VK_SUBOPTIMAL_KHR)
                RequestSwapchainRecreate(false);
            const auto tPresented = Clock::now();
            m_stPresentUs.store(usBetween(tPresentLocked, tPresented), std::memory_order_relaxed);

            VkSubmitInfo fenceSubmit = {};
            fenceSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            vkQueueSubmit(m_graphicsQueue, 1, &fenceSubmit, job.fence);
            m_stFenceUs.store(usBetween(tPresented, Clock::now()), std::memory_order_relaxed);
        }

        {
            ZoneScopedN("SubmitThread::GC");
            const auto tGc = Clock::now();
            m_nvrhiDevice->runGarbageCollection();
            m_stGcUs.store(usBetween(tGc, Clock::now()), std::memory_order_relaxed);
        }
    }
}

bool VulkanBackend::GetSubmitThreadTimings(SubmitThreadTimings& out) const {
    if (!m_asyncSubmit)
        return false;
    out.jobLatencyUs = m_stJobLatencyUs.load(std::memory_order_relaxed);
    out.queueLockUs = m_stQueueLockUs.load(std::memory_order_relaxed);
    out.semWaitUs = m_stSemWaitUs.load(std::memory_order_relaxed);
    out.encodeUs = m_stEncodeUs.load(std::memory_order_relaxed);
    out.presentLockUs = m_stPresentLockUs.load(std::memory_order_relaxed);
    out.presentUs = m_stPresentUs.load(std::memory_order_relaxed);
    out.fenceUs = m_stFenceUs.load(std::memory_order_relaxed);
    out.gcUs = m_stGcUs.load(std::memory_order_relaxed);
    return true;
}

void VulkanBackend::FlushSubmits() {
    if (!m_asyncSubmit)
        return;
    std::unique_lock<std::mutex> lk(m_submitMutex);
    m_submitDoneCv.wait(lk, [&] { return !m_jobQueued && !m_slotInFlight[0] && !m_slotInFlight[1]; });
}

void VulkanBackend::WaitForIdle() {
    FlushSubmits();
    if (m_gcTask) {
        TaskScheduler->Wait(*m_gcTask);
        m_gcTask = nullptr;
    }
    if (m_nvrhiDevice)
        m_nvrhiDevice->waitForIdle();
    if (m_device)
        vkDeviceWaitIdle(m_device);
}

void VulkanBackend::LockDevice() {
    m_queueMutex.lock();
}

void VulkanBackend::UnlockDevice() {
    m_queueMutex.unlock();
}

void VulkanBackend::ExecuteCommandList(nvrhi::ICommandList* commandList) {
    if (m_nvrhiDevice && commandList) {
        std::lock_guard<std::mutex> qk(m_queueMutex);
        m_nvrhiDevice->executeCommandList(commandList);
    }
}

u64 VulkanBackend::ExecuteComputeCommandList(nvrhi::ICommandList* commandList) {
    if (!m_nvrhiDevice || !commandList || !m_computeQueue)
        return 0;
    return m_nvrhiDevice->executeCommandList(commandList, nvrhi::CommandQueue::Compute);
}

void VulkanBackend::QueueWaitForCompute(u64 instanceID) {
    if (!m_nvrhiDevice || !m_computeQueue || instanceID == 0)
        return;
    m_nvrhiDevice->queueWaitForCommandList(nvrhi::CommandQueue::Graphics, nvrhi::CommandQueue::Compute, instanceID);
}

void VulkanBackend::ComputeWaitForPreviousGraphics() {
    if (!m_nvrhiDevice || !m_computeQueue || m_lastGraphicsInstanceID == 0)
        return;
    m_nvrhiDevice->queueWaitForCommandList(nvrhi::CommandQueue::Compute, nvrhi::CommandQueue::Graphics, m_lastGraphicsInstanceID);
}

void VulkanBackend::ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) {
    if (!m_nvrhiDevice) return;
    for (u32 i = 0; i < count; i++) {
        if (commandLists[i])
            m_nvrhiDevice->executeCommandList(commandLists[i]);
    }
}

void VulkanBackend::UploadBufferData(nvrhi::IBuffer* buffer, const void* data, size_t size) {
    if (!buffer || !data || size == 0) return;

    if (m_inFrame) {
        m_commandLists[m_recordSlot]->writeBuffer(buffer, data, size);
    } else {
        m_nvrhiDevice->runGarbageCollection();
        m_uploadCommandList->open();
        m_uploadCommandList->writeBuffer(buffer, data, size);
        m_uploadCommandList->close();
        m_nvrhiDevice->executeCommandList(m_uploadCommandList);
    }
}

nvrhi::ICommandList* VulkanBackend::CreateCommandList() {
    return nullptr;
}

DeviceState VulkanBackend::GetDeviceState() const {
    if (!m_initialized || !m_device)
        return DeviceState::Lost;
    return DeviceState::Normal;
}

void VulkanBackend::BeginDebugEvent(pcstr name) {}
void VulkanBackend::EndDebugEvent() {}
void VulkanBackend::SetMarker(pcstr name) {}

IRenderBackend* CreateVulkanBackend(SDL_Window* window, u32 width, u32 height, bool enableValidation) {
    auto* backend = xr_new<VulkanBackend>();
    if (backend->Initialize(window, width, height, enableValidation))
        return backend;
    Msg("! [VulkanBackend] Initialization failed");
    xr_delete(backend);
    return nullptr;
}
