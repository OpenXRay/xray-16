#include "stdafx.h"
#include "NVRHIDevice.h"

namespace xray::render::r4::nvrhi_wrapper
{

NVRHIDevice::NVRHIDevice() = default;

NVRHIDevice::~NVRHIDevice()
{
    Shutdown();
}

bool NVRHIDevice::Initialize(ID3D11Device* d3d11Device, ID3D11DeviceContext* d3d11Context)
{
    VERIFY(d3d11Device != nullptr);
    VERIFY(d3d11Context != nullptr);

    if (m_initialized)
    {
        Msg("! [NVRHI] Already initialized");
        return false;
    }

    try
    {
        Msg("~ [NVRHI] Initializing device wrapper...");

        // Create NVRHI device descriptor
        nvrhi::d3d11::DeviceDesc deviceDesc;
        deviceDesc.device = d3d11Device;
        deviceDesc.context = d3d11Context;

        // Wrap existing device
        m_device = nvrhi::d3d11::createDevice(deviceDesc);

        if (!m_device)
        {
            Msg("! [NVRHI] Failed to create device wrapper");
            return false;
        }

        // Create immediate execution command list
        nvrhi::CommandListParameters cmdListParams;
        cmdListParams.enableImmediateExecution = true;

        m_commandList = m_device->createCommandList(cmdListParams);

        if (!m_commandList)
        {
            Msg("! [NVRHI] Failed to create command list");
            m_device = nullptr;
            return false;
        }

        m_initialized = true;

        // Log device info
        const nvrhi::DeviceDesc& desc = m_device->getDeviceDesc();
        Msg("~ [NVRHI] Device initialized");
        Msg("~   Graphics API: %s", desc.graphicsAPI == nvrhi::GraphicsAPI::D3D11 ? "D3D11" : "Unknown");

        return true;
    }
    catch (const std::exception& e)
    {
        Msg("! [NVRHI] Exception during initialization: %s", e.what());
        return false;
    }
}

void NVRHIDevice::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    Msg("~ [NVRHI] Shutting down device wrapper...");

    WaitForIdle();

    m_commandList = nullptr;
    m_device = nullptr;

    m_initialized = false;

    Msg("~ [NVRHI] Device wrapper shutdown complete");
}

void NVRHIDevice::ExecuteCommandList(nvrhi::ICommandList* commandList)
{
    VERIFY(m_initialized);
    VERIFY(commandList != nullptr);

    m_device->executeCommandList(commandList);
}

void NVRHIDevice::WaitForIdle()
{
    if (m_device)
    {
        m_device->waitForIdle();
    }
}

} // namespace xray::render::r4::nvrhi_wrapper
