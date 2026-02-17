#pragma once

#include "xrEngine/IRenderBackend.h"
#include <nvrhi/nvrhi.h>
#include <vulkan/vulkan.h>

struct SDL_Window;

class VulkanBackend : public IRenderBackend {
public:
    VulkanBackend();
    ~VulkanBackend() override;

    bool Initialize(SDL_Window* window, u32 width, u32 height, bool enableValidation = false);
    void Shutdown() override;

    API GetAPI() const override { return API::Vulkan; }
    pcstr GetAPIName() const override { return "Vulkan"; }

    bool IsInitialized() const override { return m_initialized; }
    void WaitForIdle() override;
    DeviceState GetDeviceState() const override;

    nvrhi::IDevice* GetDevice() const override { return m_nvrhiDevice.Get(); }
    nvrhi::ICommandList* GetCommandList() const override { return m_commandList.Get(); }
    nvrhi::ICommandList* CreateCommandList() override;

    void ExecuteCommandList(nvrhi::ICommandList* commandList) override;
    void ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) override;

    void UploadBufferData(nvrhi::IBuffer* buffer, const void* data, size_t size) override;

    nvrhi::ITexture* GetBackBuffer() override;
    u32 GetCurrentBackBufferIndex() const override { return m_currentImageIndex; }
    u32 GetBackBufferCount() const override { return BACK_BUFFER_COUNT; }
    std::pair<u32, u32> GetBackBufferSize() const override { return {m_backBufferWidth, m_backBufferHeight}; }
    void Present(bool vsync) override;
    void ResizeSwapChain(u32 width, u32 height) override;

    void BeginFrame() override;
    void EndFrame() override;
    bool IsInFrame() const override { return m_inFrame; }

    const Capabilities& GetCapabilities() const override { return m_capabilities; }
    Capabilities& GetMutableCapabilities() override { return m_capabilities; }

    u32 RegisterBindlessTexture(nvrhi::ITexture* texture) override;
    void UnregisterBindlessTexture(u32 index) override;
    nvrhi::IBindingLayout* GetBindlessLayout() const override { return m_bindlessLayout.Get(); }
    nvrhi::IDescriptorTable* GetBindlessDescriptorTable() const override { return m_bindlessDescriptorTable.Get(); }

    void BeginDebugEvent(pcstr name) override;
    void EndDebugEvent() override;
    void SetMarker(pcstr name) override;

private:
    static constexpr u32 BACK_BUFFER_COUNT = 2;
    static constexpr u32 MAX_BINDLESS_TEXTURES = 65536;

    bool CreateInstance(bool enableValidation);
    bool SelectPhysicalDevice();
    bool CreateSurface(SDL_Window* window);
    bool CreateLogicalDevice();
    bool CreateSwapChain(u32 width, u32 height);
    void DestroySwapChain();
    void CreateBackBufferTextures();
    void CreateSyncObjects();
    void DestroySyncObjects();
    void CreateBindlessResources();
    void QueryCapabilities();

    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    u32 m_graphicsQueueFamily = UINT32_MAX;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    VkSemaphore m_imageAvailable[BACK_BUFFER_COUNT] = {};
    VkSemaphore m_renderFinished[BACK_BUFFER_COUNT] = {};

    nvrhi::DeviceHandle m_nvrhiDevice;
    nvrhi::DeviceHandle m_nvrhiVulkanDevice;
    nvrhi::CommandListHandle m_commandList;
    nvrhi::CommandListHandle m_uploadCommandList;
    xr_vector<VkImage> m_swapchainImages;
    nvrhi::TextureHandle m_backBuffers[BACK_BUFFER_COUNT];

    nvrhi::BindingLayoutHandle m_bindlessLayout;
    nvrhi::DescriptorTableHandle m_bindlessDescriptorTable;
    xr_vector<u32> m_freeBindlessIndices;
    u32 m_nextBindlessIndex = 0;

    bool m_initialized = false;
    bool m_inFrame = false;
    bool m_validationEnabled = false;
    Capabilities m_capabilities;
    u32 m_backBufferWidth = 0;
    u32 m_backBufferHeight = 0;
    u32 m_currentImageIndex = 0;
    u32 m_currentFrameIndex = 0;
    VkFormat m_swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;

    Task* m_gcTask = nullptr;
};
