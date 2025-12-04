#pragma once

#include "xrEngine/IRenderBackend.h"
#include <nvrhi/nvrhi.h>

// Forward declarations
struct ID3D11Device;
struct ID3D11DeviceContext;
class ENGINE_API IRenderBackend;
// ═══════════════════════════════════════════════════════
//  D3D11 BACKEND WRAPPER
// ═══════════════════════════════════════════════════════
// Wraps the existing CHW (HW global) to implement IRenderBackend.
// This is a migration shim - allows gradual transition from HW.* to GEnv.Backend->*
//
// Usage:
//   1. Create after CHW::CreateDevice()
//   2. Set GEnv.Backend = wrapper
//   3. Gradually replace HW.* calls with GEnv.Backend->*
//   4. Eventually replace CHW entirely with D3D12Backend

class D3D11BackendWrapper : public IRenderBackend
{
public:
    D3D11BackendWrapper();
    ~D3D11BackendWrapper() override;

    // Initialize by wrapping existing D3D11 device from CHW
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    // ═══════ IRenderBackend Implementation ═══════
    API GetAPI() const override { return API::D3D11; }
    pcstr GetAPIName() const override { return "D3D11"; }

    bool IsInitialized() const override { return m_initialized; }
    void WaitForIdle() override;

    nvrhi::IDevice* GetDevice() const override { return m_nvrhiDevice.Get(); }
    nvrhi::ICommandList* GetCommandList() const override { return m_commandList.Get(); }

    // Command execution
    void ExecuteCommandList(nvrhi::ICommandList* commandList) override;

    nvrhi::ITexture* GetBackBuffer() override;
    void Present(bool vsync) override;
    std::pair<u32, u32> GetBackBufferSize() const override;

    void BeginFrame() override;
    void EndFrame() override;

    const Capabilities& GetCapabilities() const override { return m_capabilities; }
    Capabilities& GetMutableCapabilities() override { return m_capabilities; }
    void UpdateCapabilities() override;

    // Bindless not supported on D3D11
    u32 RegisterBindlessTexture(nvrhi::ITexture* texture) override { return UINT32_MAX; }
    void UnregisterBindlessTexture(u32 index) override {}

    void BeginDebugEvent(pcstr name) override;
    void EndDebugEvent() override;

    // ═══════ D3D11-Specific Access (for legacy code migration) ═══════
    ID3D11Device* GetD3D11Device() const { return m_d3d11Device; }
    ID3D11DeviceContext* GetD3D11Context() const { return m_d3d11Context; }

private:
    void QueryCapabilities();

    // Wrapped D3D11 objects (owned by CHW, not us)
    ID3D11Device* m_d3d11Device = nullptr;
    ID3D11DeviceContext* m_d3d11Context = nullptr;

    // NVRHI wrapper
    nvrhi::DeviceHandle m_nvrhiDevice;
    nvrhi::CommandListHandle m_commandList;
    nvrhi::TextureHandle m_backBuffer;

    // State
    bool m_initialized = false;
    Capabilities m_capabilities;

    // Debug annotation
    struct ID3DUserDefinedAnnotation* m_annotation = nullptr;
};
