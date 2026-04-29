#pragma once

namespace xray::render::fg::nvrhi_wrapper
{

class NVRHIDevice
{
public:
    NVRHIDevice();
    ~NVRHIDevice();

    // Initialize by wrapping existing D3D11 device
    bool Initialize(ID3D11Device* d3d11Device, ID3D11DeviceContext* d3d11Context);

    void Shutdown();

    bool IsInitialized() const { return m_initialized; }

    nvrhi::IDevice* GetDevice() const { return m_device.Get(); }
    nvrhi::ICommandList* GetCommandList() const { return m_commandList.Get(); }

    void ExecuteCommandList(nvrhi::ICommandList* commandList);
    void WaitForIdle();

private:
    nvrhi::DeviceHandle m_device;
    nvrhi::CommandListHandle m_commandList;
    bool m_initialized = false;

    // Prevent copying
    NVRHIDevice(const NVRHIDevice&) = delete;
    NVRHIDevice& operator=(const NVRHIDevice&) = delete;
};

} // namespace xray::render::fg::nvrhi_wrapper
