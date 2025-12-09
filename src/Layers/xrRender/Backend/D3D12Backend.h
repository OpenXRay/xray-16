// D3D12Backend.h
// DirectX 12 backend implementation with true bindless texture support
// This is the PRIMARY TARGET backend for GPU-driven rendering
#pragma once

#include "xrEngine/IRenderBackend.h"
#include <nvrhi/nvrhi.h>

struct ID3D12Device;
struct ID3D12CommandQueue;
struct IDXGISwapChain3;
struct IDXGIFactory4;
struct IDXGIAdapter1;
struct SDL_Window;

class D3D12Backend : public IRenderBackend {
public:
    D3D12Backend();
    ~D3D12Backend() override;

    // Initialize with SDL window
    bool Initialize(SDL_Window* window, u32 width, u32 height, bool enableValidation = false);
    void Shutdown() override;

    // ═══════ IRenderBackend Implementation ═══════
    API GetAPI() const override { return API::D3D12; }
    pcstr GetAPIName() const override { return "D3D12"; }

    bool IsInitialized() const override { return m_initialized; }
    void WaitForIdle() override;
    DeviceState GetDeviceState() const override;

    nvrhi::IDevice* GetDevice() const override { return m_nvrhiDevice.Get(); }
    nvrhi::ICommandList* GetCommandList() const override { return m_commandList.Get(); }

    nvrhi::ICommandList* CreateCommandList() override;

    void ExecuteCommandList(nvrhi::ICommandList* commandList) override;
    void ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) override;

    // ═══════ Buffer Upload ═══════
    void UploadBufferData(nvrhi::IBuffer* buffer, const void* data, size_t size) override;

    nvrhi::ITexture* GetBackBuffer() override;
    u32 GetCurrentBackBufferIndex() const override { return m_currentBackBufferIndex; }
    u32 GetBackBufferCount() const override { return BACK_BUFFER_COUNT; }
    std::pair<u32, u32> GetBackBufferSize() const override { return {m_backBufferWidth, m_backBufferHeight}; }
    void Present(bool vsync) override;
    void ResizeSwapChain(u32 width, u32 height) override;

    void BeginFrame() override;
    void EndFrame() override;
    bool IsInFrame() const override { return m_inFrame; }

    const Capabilities& GetCapabilities() const override { return m_capabilities; }
    Capabilities& GetMutableCapabilities() override { return m_capabilities; }
    void UpdateCapabilities() override {}

    // ═══════ Bindless Resources (D3D12 feature) ═══════
    u32 RegisterBindlessTexture(nvrhi::ITexture* texture) override;
    void UnregisterBindlessTexture(u32 index) override;
    nvrhi::IBindingLayout* GetBindlessLayout() const override { return m_bindlessLayout.Get(); }
    nvrhi::IDescriptorTable* GetBindlessDescriptorTable() const override { return m_bindlessDescriptorTable.Get(); }

    // ═══════ Debug/Profiling ═══════
    void BeginDebugEvent(pcstr name) override;
    void EndDebugEvent() override;
    void SetMarker(pcstr name) override;

    // ═══════ D3D12-Specific Access ═══════
    ID3D12Device* GetD3D12Device() const { return m_d3d12Device; }
    ID3D12CommandQueue* GetD3D12CommandQueue() const { return m_commandQueue; }
    IDXGISwapChain3* GetSwapChain() const { return m_swapChain; }

private:
    static constexpr u32 BACK_BUFFER_COUNT = 2;
    static constexpr u32 MAX_BINDLESS_TEXTURES = 65536;

    bool CreateDXGIFactory(bool enableValidation);
    bool SelectAdapter();
    bool CreateDevice();
    bool CreateCommandQueue();
    bool CreateSwapChain(HWND hwnd, u32 width, u32 height);
    void CreateBackBufferTextures();
    void CreateBindlessResources();
    void QueryCapabilities();

    // DXGI
    IDXGIFactory4* m_dxgiFactory = nullptr;
    IDXGIAdapter1* m_adapter = nullptr;
    IDXGISwapChain3* m_swapChain = nullptr;

    // D3D12 objects
    ID3D12Device* m_d3d12Device = nullptr;
    ID3D12CommandQueue* m_commandQueue = nullptr;

    // NVRHI wrapper
    nvrhi::DeviceHandle m_nvrhiDevice;
    nvrhi::CommandListHandle m_commandList;  // Per-frame command list for rendering
    nvrhi::CommandListHandle m_uploadCommandList;  // Persistent upload command list (out-of-frame)
    nvrhi::TextureHandle m_backBuffers[BACK_BUFFER_COUNT];

    // Bindless resources
    nvrhi::BindingLayoutHandle m_bindlessLayout;
    nvrhi::DescriptorTableHandle m_bindlessDescriptorTable;
    xr_vector<u32> m_freeBindlessIndices;
    u32 m_nextBindlessIndex = 0;

    // State
    bool m_initialized = false;
    bool m_inFrame = false;  // True between BeginFrame/EndFrame
    Capabilities m_capabilities;
    u32 m_backBufferWidth = 0;
    u32 m_backBufferHeight = 0;
    u32 m_currentBackBufferIndex = 0;
};
