#pragma once

#include "SubmitTokenRing.h"
#include "BackendCompletion.h"
#include "xrCore/Threading/Task.hpp"
#include "xrEngine/IRenderBackend.h"
#include <nvrhi/nvrhi.h>
#include <vulkan/vulkan.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

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
    MemoryBudget GetMemoryBudget() const override;

    nvrhi::IDevice* GetDevice() const override { return m_nvrhiDevice.Get(); }
    nvrhi::ICommandList* GetCommandList() const override { return m_currentGraphics; }
    nvrhi::ICommandList* CreateCommandList() override;

    bool HasAsyncCompute() const override { return m_asyncComputeEnabled; }
    nvrhi::ICommandList* AcquireComputeCommandList() override;
    u32 SubmitCompute(nvrhi::ICommandList* commandList, const u32* waitTokens, u32 numWaitTokens) override;
    void AddGraphicsWait(u32 token) override { m_graphicsWaits.Add(token); }
    u32 SplitGraphics() override;
    u32 LastGraphicsToken() const override { return m_tokens.last[u32(nvrhi::CommandQueue::Graphics)]; }
    u32 LastComputeToken() const override { return m_tokens.last[u32(nvrhi::CommandQueue::Compute)]; }

    void ExecuteCommandList(nvrhi::ICommandList* commandList) override;
    void ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) override;

    bool SupportsSubmissionLeases() const override { return true; }
    u64 OpenSubmissionLease() override { return m_completion.OpenLease(); }
    void CloseSubmissionLease(u64 lease) override { m_completion.CloseLease(lease); }
    SubmissionLeaseState PollSubmissionLease(u64 lease) override;
    bool IsSubmissionLeaseSubmitted(u64 lease) const override;
    void ReleaseSubmissionLease(u64 lease) override { m_completion.ReleaseLease(lease); }
    u32 GetPendingSubmissionCount() const override { return m_completion.PendingTicketCount(); }

    void UploadBufferData(nvrhi::IBuffer* buffer, const void* data, size_t size) override;

    nvrhi::ITexture* GetBackBuffer() override;
    u32 GetCurrentBackBufferIndex() const override { return m_currentImageIndex; }
    u32 GetBackBufferCount() const override { return static_cast<u32>(m_backBuffers.size()); }
    std::pair<u32, u32> GetBackBufferSize() const override { return {m_backBufferWidth, m_backBufferHeight}; }
    void Present(bool vsync) override;
    void ResizeSwapChain(u32 width, u32 height) override;

    void BeginFrame() override;
    void EndFrame() override;
    bool IsInFrame() const override { return m_inFrame; }
    bool GetSubmitThreadTimings(SubmitThreadTimings& out) const override;

    const Capabilities& GetCapabilities() const override { return m_capabilities; }
    Capabilities& GetMutableCapabilities() override { return m_capabilities; }

    u32 RegisterBindlessTexture(nvrhi::ITexture* texture) override;
    void UnregisterBindlessTexture(u32 index) override;
    nvrhi::ITexture* GetBindlessTexture(u32 index) override;
    bool RetainBindlessTextures(const u32* indices, u32 count) override;
    void ReleaseBindlessTextures(const u32* indices, u32 count) override;
    nvrhi::IBindingLayout* GetBindlessLayout() const override { return m_bindlessLayout.Get(); }
    nvrhi::IDescriptorTable* GetBindlessDescriptorTable() const override { return m_bindlessDescriptorTable.Get(); }

    void BeginDebugEvent(pcstr name) override;
    void EndDebugEvent() override;
    void SetMarker(pcstr name) override;

private:
    static constexpr u32 BACK_BUFFER_COUNT = 3;
    static constexpr u32 MAX_BINDLESS_TEXTURES = 65536;

    bool CreateInstance(SDL_Window* window, bool enableValidation);
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
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    u32 m_computeQueueFamily = UINT32_MAX;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    xr_vector<const char*> m_deviceExtensions;

    VkSemaphore m_imageAvailable[BACK_BUFFER_COUNT] = {};
    xr_vector<VkSemaphore> m_renderFinished;
    u64 m_frameSubmissionIDs[BACK_BUFFER_COUNT] = {};

    nvrhi::DeviceHandle m_nvrhiDevice;
    nvrhi::DeviceHandle m_nvrhiVulkanDevice;

    struct CommandListPool {
        xr_vector<nvrhi::CommandListHandle> lists;
        u32 used = 0;
    };
    CommandListPool m_graphicsPools[2];
    CommandListPool m_computePools[2];
    u32 m_recordSlot = 0;
    nvrhi::ICommandList* m_currentGraphics = nullptr;
    nvrhi::CommandListHandle m_uploadCommandList;
    xr_vector<VkImage> m_swapchainImages;
    xr_vector<nvrhi::TextureHandle> m_backBuffers;

    nvrhi::BindingLayoutHandle m_bindlessLayout;
    nvrhi::DescriptorTableHandle m_bindlessDescriptorTable;
    xr_vector<u32> m_freeBindlessIndices;
    xr_map<nvrhi::ITexture*, u32> m_bindlessTextureMap;
    u32 m_nextBindlessIndex = 0;
    xr_vector<nvrhi::TextureHandle> m_bindlessTextureResources;
    xr_vector<u32> m_bindlessTextureReferences;
    std::mutex m_bindlessMutex;

    bool m_initialized = false;
    bool m_inFrame = false;
    bool m_presentPending = false;
    bool m_requestedVSync = false;
    bool m_swapchainVSync = false;
    std::atomic<bool> m_swapchainNeedsReset{ false };
    bool m_validationEnabled = false;
    Capabilities m_capabilities;
    u32 m_backBufferWidth = 0;
    u32 m_backBufferHeight = 0;
    u32 m_currentImageIndex = 0;
    u32 m_currentFrameIndex = 0;
    u32 m_acquiredImageCount = 0;
    u32 m_maxAcquiredImageCount = 1;
    VkFormat m_swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;

    TaskHandle m_gcTask;
    bool m_asyncComputeEnabled = false;
    bool m_frameGraphicsSubmitted = false;
    SubmitTokenRing m_tokens;
    SubmitWaitList m_graphicsWaits;

    struct SubmitJob {
        nvrhi::ICommandList* cl = nullptr;
        nvrhi::CommandQueue queue = nvrhi::CommandQueue::Graphics;
        u32 token = 0;
        u64 completion = 0;
        SubmitWaitList waits;
        VkSemaphore imageAvailable = VK_NULL_HANDLE;
        VkSemaphore renderFinished = VK_NULL_HANDLE;
        bool present = false;
        u32 frameIndex = 0;
        u32 imageIndex = 0;
        u32 slot = 0;
        std::chrono::steady_clock::time_point enqueueTime;
    };

    bool m_asyncSubmit = false;
    std::thread m_submitThread;
    std::mutex m_submitMutex;
    std::condition_variable m_submitCv;
    std::condition_variable m_submitDoneCv;
    std::deque<SubmitJob> m_jobs;
    bool m_submitActive = false;
    bool m_submitRun = false;
    bool m_slotInFlight[2] = {};
    bool m_frameSubmissionPending[BACK_BUFFER_COUNT] = {};
    std::mutex m_queueMutex;
    xray::render::backend::SubmissionTracker m_completion;
    std::mutex m_swapchainMutex;

    mutable std::atomic<u64> m_stJobLatencyUs{0};
    mutable std::atomic<u64> m_stQueueLockUs{0};
    mutable std::atomic<u64> m_stSemWaitUs{0};
    mutable std::atomic<u64> m_stEncodeUs{0};
    mutable std::atomic<u64> m_stPresentLockUs{0};
    mutable std::atomic<u64> m_stPresentUs{0};
    mutable std::atomic<u64> m_stGcUs{0};
    mutable std::atomic<u64> m_stSlotWaitUs{0};
    mutable std::atomic<u64> m_stCapacityWaitUs{0};
    mutable std::atomic<u64> m_stGpuWaitUs{0};
    mutable std::atomic<u64> m_stAcquireUs{0};
    void StoreMaxUs(std::atomic<u64>& slot, u64 value) const;

    nvrhi::ICommandList* AcquireFromPool(CommandListPool& pool, nvrhi::CommandQueue queue);
    u64 SubmitLocked(const SubmitJob& job);
    void RunGarbageCollection();
    void EnqueueJob(SubmitJob&& job);
    void SubmitThreadMain();
    void FlushSubmits();
};
