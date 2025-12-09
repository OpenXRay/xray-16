#pragma once

#include "xrEngine/IRenderBackend.h"
#include <nvrhi/nvrhi.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
class ENGINE_API IRenderBackend;

class D3D11BackendWrapper : public IRenderBackend
{
public:
    D3D11BackendWrapper();
    ~D3D11BackendWrapper() override;

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    API GetAPI() const override { return API::D3D11; }
    pcstr GetAPIName() const override { return "D3D11"; }

    bool IsInitialized() const override { return m_initialized; }
    void WaitForIdle() override;

    nvrhi::IDevice* GetDevice() const override { return m_nvrhiDevice.Get(); }
    nvrhi::ICommandList* GetCommandList() const override { return m_commandList.Get(); }

    void ExecuteCommandList(nvrhi::ICommandList* commandList) override;

    nvrhi::ITexture* GetBackBuffer() override;
    void Present(bool vsync) override;
    std::pair<u32, u32> GetBackBufferSize() const override;

    void BeginFrame() override;
    void EndFrame() override;
    bool IsInFrame() const override { return m_inFrame; }

    const Capabilities& GetCapabilities() const override { return m_capabilities; }
    Capabilities& GetMutableCapabilities() override { return m_capabilities; }
    void UpdateCapabilities() override;

    u32 RegisterBindlessTexture(nvrhi::ITexture* texture) override { return UINT32_MAX; }
    void UnregisterBindlessTexture(u32 index) override {}

    void UploadBufferData(nvrhi::IBuffer* buffer, const void* data, size_t size) override;

    void BeginDebugEvent(pcstr name) override;
    void EndDebugEvent() override;

    ID3D11Device* GetD3D11Device() const { return m_d3d11Device; }
    ID3D11DeviceContext* GetD3D11Context() const { return m_d3d11Context; }

private:
    void QueryCapabilities();

    ID3D11Device* m_d3d11Device = nullptr;
    ID3D11DeviceContext* m_d3d11Context = nullptr;

    nvrhi::DeviceHandle m_nvrhiDevice;
    nvrhi::CommandListHandle m_commandList;
    nvrhi::TextureHandle m_backBuffer;

    bool m_initialized = false;
    bool m_inFrame = false;
    Capabilities m_capabilities;

    struct ID3DUserDefinedAnnotation* m_annotation = nullptr;
};
