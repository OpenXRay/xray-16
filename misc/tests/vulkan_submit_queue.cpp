#include "stdafx.h"
#include "xrEngine/IRenderBackend.h"
#include <nvrhi/nvrhi.h>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <nvrhi/vulkan.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <future>

#define private public
#include "Layers/xrRender/Backend/VulkanBackend.h"
#undef private

static void require(bool condition, const char* description)
{
    if (!condition)
    {
        std::fprintf(stderr, "%s\n", description);
        std::_Exit(1);
    }
}

static void setup(VulkanBackend& backend)
{
    const char* extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME};
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.apiVersion = VK_API_VERSION_1_2;
    VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    info.pApplicationInfo = &app;
    info.enabledExtensionCount = 2;
    info.ppEnabledExtensionNames = extensions;
    require(vkCreateInstance(&info, nullptr, &backend.m_instance) == VK_SUCCESS, "Create Vulkan instance");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(backend.m_instance, vkGetInstanceProcAddr);
    VkHeadlessSurfaceCreateInfoEXT surface{VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT};
    auto createHeadless = reinterpret_cast<PFN_vkCreateHeadlessSurfaceEXT>(vkGetInstanceProcAddr(backend.m_instance, "vkCreateHeadlessSurfaceEXT"));
    require(createHeadless && createHeadless(backend.m_instance, &surface, nullptr, &backend.m_surface) == VK_SUCCESS, "VK_EXT_headless_surface is required");
    uint32_t count = 0;
    require(vkEnumeratePhysicalDevices(backend.m_instance, &count, nullptr) == VK_SUCCESS && count > 0, "Enumerate GPU");
    std::vector<VkPhysicalDevice> devices(count);
    require(vkEnumeratePhysicalDevices(backend.m_instance, &count, devices.data()) == VK_SUCCESS, "Select GPU");
    backend.m_physicalDevice = devices.front();
    require(backend.CreateLogicalDevice(), "Create backend Vulkan device");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(backend.m_instance, vkGetInstanceProcAddr, backend.m_device, vkGetDeviceProcAddr);
    require(backend.CreateSwapChain(64, 64), "Create headless swapchain");
    nvrhi::vulkan::DeviceDesc desc;
    desc.instance = backend.m_instance;
    desc.physicalDevice = backend.m_physicalDevice;
    desc.device = backend.m_device;
    desc.graphicsQueue = backend.m_graphicsQueue;
    desc.graphicsQueueIndex = backend.m_graphicsQueueFamily;
    desc.instanceExtensions = extensions;
    desc.numInstanceExtensions = 2;
    desc.deviceExtensions = backend.m_deviceExtensions.data();
    desc.numDeviceExtensions = backend.m_deviceExtensions.size();
    backend.m_nvrhiVulkanDevice = nvrhi::vulkan::createDevice(desc);
    backend.m_nvrhiDevice = backend.m_nvrhiVulkanDevice;
    backend.CreateBackBufferTextures();
    backend.CreateSyncObjects();
    nvrhi::CommandListParameters parameters;
    parameters.enableImmediateExecution = false;
    for (auto& commands : backend.m_commandLists)
        commands = backend.m_nvrhiDevice->createCommandList(parameters);
    backend.m_asyncSubmit = true;
    backend.m_initialized = true;
}

static void record(VulkanBackend& backend, nvrhi::IBuffer* completedFrames, u32 frame)
{
    backend.BeginFrame();
    require(backend.IsInFrame(), "Begin headless frame");
    auto* commands = backend.GetCommandList();
    commands->clearTextureFloat(backend.GetBackBuffer(), nvrhi::AllSubresources, nvrhi::Color(0.f));
    commands->setTextureState(backend.GetBackBuffer(), nvrhi::AllSubresources, nvrhi::ResourceStates::Present);
    commands->commitBarriers();
    const u32 value = frame + 1;
    commands->writeBuffer(completedFrames, &value, sizeof(value), frame * sizeof(value));
}

static void finish(VulkanBackend& backend)
{
    backend.EndFrame();
    backend.Present(false);
}

static void verifyCompleted(VulkanBackend& backend, nvrhi::IBuffer* completedFrames, u32 count)
{
    auto* values = static_cast<const u32*>(backend.GetDevice()->mapBuffer(completedFrames, nvrhi::CpuAccessMode::Read));
    require(values != nullptr, "Map completed frame payloads");
    for (u32 frame = 0; frame < count; ++frame)
        require(values[frame] == frame + 1, "A recorded frame payload was lost or overwritten");
    backend.GetDevice()->unmapBuffer(completedFrames);
}

int main()
{
    VulkanBackend backend;
    setup(backend);
    constexpr u32 frameCount = 130;
    nvrhi::BufferDesc desc;
    desc.byteSize = frameCount * sizeof(u32);
    desc.cpuAccess = nvrhi::CpuAccessMode::Read;
    desc.initialState = nvrhi::ResourceStates::CopyDest;
    desc.keepInitialState = true;
    auto completedFrames = backend.GetDevice()->createBuffer(desc);
    require(completedFrames != nullptr, "Create frame completion readback");
    std::vector<u32> initialValues(frameCount, 0);
    auto initialize = backend.GetDevice()->createCommandList();
    initialize->open();
    initialize->writeBuffer(completedFrames, initialValues.data(), desc.byteSize);
    initialize->close();
    backend.ExecuteCommandList(initialize);
    std::promise<void> firstFrameQueued;
    auto firstReady = firstFrameQueued.get_future();
    std::promise<void> secondFrameRecorded;
    auto ready = secondFrameRecorded.get_future();
    auto producer = std::async(std::launch::async, [&]
    {
        record(backend, completedFrames, 0);
        finish(backend);
        firstFrameQueued.set_value();
        record(backend, completedFrames, 1);
        secondFrameRecorded.set_value();
        finish(backend);
    });
    require(firstReady.wait_for(std::chrono::seconds(5)) == std::future_status::ready, "Producer must queue the first frame while the submit worker is delayed");
    if (backend.m_maxAcquiredImageCount >= 2)
        require(ready.wait_for(std::chrono::seconds(5)) == std::future_status::ready, "Producer must reach the second frame when acquisition capacity permits");
    producer.wait_for(std::chrono::milliseconds(100));
    backend.m_submitRun = true;
    backend.m_submitThread = std::thread([&] { backend.SubmitThreadMain(); });
    require(producer.wait_for(std::chrono::seconds(5)) == std::future_status::ready, "Producer must resume when the submit worker starts");
    producer.get();
    backend.WaitForIdle();
    verifyCompleted(backend, completedFrames, 2);
    for (u32 frame = 2; frame < frameCount; ++frame)
    {
        record(backend, completedFrames, frame);
        finish(backend);
    }
    backend.WaitForIdle();
    verifyCompleted(backend, completedFrames, frameCount);
    std::puts("Async submit: delayed-worker recovery and all 130 recorded frame payloads verified.");
}
