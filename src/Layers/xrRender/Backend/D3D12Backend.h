// D3D12Backend.h
// DirectX 12 backend implementation with true bindless texture support
// This is the PRIMARY TARGET backend for GPU-driven rendering
#pragma once

#include "RenderBackend.h"
#include <nvrhi/d3d12.h>

struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12Fence;
struct IDXGISwapChain3;
struct IDXGIFactory4;
struct IDXGIAdapter1;

namespace xray::render::ng {

class ECORE_API D3D12Backend : public IRenderBackend {
public:
    D3D12Backend();
    ~D3D12Backend() override;

    // ═══════ IRenderBackend Implementation ═══════
    bool Initialize(const BackendInitParams& params) override;
    void Shutdown() override;
    bool IsInitialized() const override { return m_initialized; }

    GraphicsAPI GetAPI() const override { return GraphicsAPI::D3D12; }
    const BackendCapabilities& GetCapabilities() const override { return m_capabilities; }

    nvrhi::IDevice* GetDevice() const override { return m_device.Get(); }
    nvrhi::ICommandList* GetImmediateCommandList() const override { return m_commandList.Get(); }

    nvrhi::CommandListHandle CreateCommandList(
        nvrhi::CommandListParameters params = nvrhi::CommandListParameters()) override;

    nvrhi::ITexture* GetCurrentBackBuffer() override;
    u32 GetCurrentBackBufferIndex() const override { return m_currentBackBufferIndex; }
    u32 GetBackBufferCount() const override { return BACK_BUFFER_COUNT; }
    std::pair<u32, u32> GetBackBufferSize() const override { return {m_backBufferWidth, m_backBufferHeight}; }
    void Present(bool vsync) override;
    void ResizeSwapChain(u32 width, u32 height) override;

    void BeginFrame() override;
    void EndFrame() override;
    void WaitForIdle() override;

    void ExecuteCommandList(nvrhi::ICommandList* commandList) override;
    void ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) override;

    // ═══════ Bindless Resources (D3D12 feature) ═══════
    u32 RegisterBindlessTexture(nvrhi::ITexture* texture) override;
    void UnregisterBindlessTexture(u32 index) override;
    nvrhi::IBindingLayout* GetBindlessLayout() const override { return m_bindlessLayout.Get(); }
    nvrhi::IDescriptorTable* GetBindlessDescriptorTable() const override { return m_bindlessDescriptorTable.Get(); }

    // ═══════ Debug/Profiling ═══════
    void BeginEvent(const char* name) override;
    void EndEvent() override;
    void SetMarker(const char* name) override;

    // ═══════ D3D12-Specific Access ═══════
    ID3D12Device* GetD3D12Device() const { return m_d3d12Device; }
    ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue; }
    IDXGISwapChain3* GetSwapChain() const { return m_swapChain; }

private:
    static constexpr u32 BACK_BUFFER_COUNT = 2;
    static constexpr u32 MAX_BINDLESS_TEXTURES = 65536;

    bool CreateDevice(const BackendInitParams& params);
    bool CreateSwapChain(const BackendInitParams& params);
    bool CreateCommandQueue();
    bool CreateFence();
    void CreateBackBufferTextures();
    void CreateBindlessResources();
    void QueryCapabilities();
    void WaitForFence(u64 fenceValue);
    void MoveToNextFrame();

    // DXGI
    IDXGIFactory4* m_dxgiFactory = nullptr;
    IDXGIAdapter1* m_adapter = nullptr;
    IDXGISwapChain3* m_swapChain = nullptr;

    // D3D12 objects
    ID3D12Device* m_d3d12Device = nullptr;
    ID3D12CommandQueue* m_commandQueue = nullptr;
    ID3D12Fence* m_fence = nullptr;
    void* m_fenceEvent = nullptr;
    u64 m_fenceValues[BACK_BUFFER_COUNT] = {};
    u64 m_currentFenceValue = 0;

    // NVRHI wrapper
    nvrhi::DeviceHandle m_device;
    nvrhi::CommandListHandle m_commandList;
    nvrhi::TextureHandle m_backBuffers[BACK_BUFFER_COUNT];

    // Bindless resources
    nvrhi::BindingLayoutHandle m_bindlessLayout;
    nvrhi::DescriptorTableHandle m_bindlessDescriptorTable;
    xr_vector<u32> m_freeBindlessIndices;
    u32 m_nextBindlessIndex = 0;

    // State
    bool m_initialized = false;
    BackendCapabilities m_capabilities;
    u32 m_backBufferWidth = 0;
    u32 m_backBufferHeight = 0;
    u32 m_currentBackBufferIndex = 0;
};

} // namespace xray::render::ng
