#pragma once

#include "RenderBackend.h"
#include <nvrhi/d3d11.h>

namespace xray::render::ng {

class ECORE_API D3D11Backend : public IRenderBackend {
public:
    D3D11Backend();
    ~D3D11Backend() override;

    bool Initialize(const BackendInitParams& params) override;
    void Shutdown() override;
    bool IsInitialized() const override { return m_initialized; }

    GraphicsAPI GetAPI() const override { return GraphicsAPI::D3D11; }
    const BackendCapabilities& GetCapabilities() const override { return m_capabilities; }

    nvrhi::IDevice* GetDevice() const override { return m_device.Get(); }
    nvrhi::ICommandList* GetImmediateCommandList() const override { return m_commandList.Get(); }

    nvrhi::CommandListHandle CreateCommandList(
        nvrhi::CommandListParameters params = nvrhi::CommandListParameters()) override;

    nvrhi::ITexture* GetCurrentBackBuffer() override;
    u32 GetCurrentBackBufferIndex() const override { return m_currentBackBufferIndex; }
    u32 GetBackBufferCount() const override { return m_backBufferCount; }
    std::pair<u32, u32> GetBackBufferSize() const override { return {m_backBufferWidth, m_backBufferHeight}; }
    void Present(bool vsync) override;
    void ResizeSwapChain(u32 width, u32 height) override;

    void BeginFrame() override;
    void EndFrame() override;
    void WaitForIdle() override;

    void ExecuteCommandList(nvrhi::ICommandList* commandList) override;
    void ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) override;

    void BeginEvent(const char* name) override;
    void EndEvent() override;
    void SetMarker(const char* name) override;

    ID3D11Device* GetD3D11Device() const { return m_d3d11Device; }
    ID3D11DeviceContext* GetD3D11Context() const { return m_d3d11Context; }
    IDXGISwapChain* GetSwapChain() const { return m_swapChain; }

private:
    static constexpr u32 MAX_BACK_BUFFERS = 3;

    bool CreateDeviceAndSwapChain(const BackendInitParams& params);
    bool WrapExistingDevice(const BackendInitParams& params);
    void CreateBackBufferTexture();
    void QueryCapabilities();

    ID3D11Device* m_d3d11Device = nullptr;
    ID3D11DeviceContext* m_d3d11Context = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    bool m_ownsDevice = false;

    nvrhi::DeviceHandle m_device;
    nvrhi::CommandListHandle m_commandList;
    nvrhi::TextureHandle m_backBuffers[MAX_BACK_BUFFERS];

    bool m_initialized = false;
    BackendCapabilities m_capabilities;
    u32 m_backBufferWidth = 0;
    u32 m_backBufferHeight = 0;
    u32 m_backBufferCount = 1;
    u32 m_currentBackBufferIndex = 0;

    ID3DUserDefinedAnnotation* m_annotation = nullptr;
};

}
