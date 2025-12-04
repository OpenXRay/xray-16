// D3D11BackendWrapper.cpp
// Wraps CHW's D3D11 device to implement IRenderBackend interface
// This enables gradual migration from HW.* to GEnv.Backend->*
#include "stdafx.h"
#include "D3D11BackendWrapper.h"

#include <d3d11_1.h>
#include <dxgi.h>
#include <nvrhi/d3d11.h>

// Access to CHW for swap chain only
#include "Layers/xrRenderDX11/dx11HW.h"

// HW is in the render namespace (needed for swap chain access)
using xray::render::RENDER_NAMESPACE::HW;
D3D11BackendWrapper::D3D11BackendWrapper() = default;

D3D11BackendWrapper::~D3D11BackendWrapper()
{
    Shutdown();
}

bool D3D11BackendWrapper::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
    if (m_initialized)
    {
        Msg("! [D3D11BackendWrapper] Already initialized");
        return false;
    }

    if (!device || !context)
    {
        Msg("! [D3D11BackendWrapper] Invalid device or context");
        return false;
    }

    Msg("* [D3D11BackendWrapper] Initializing (wrapping CHW device)...");

    m_d3d11Device = device;
    m_d3d11Context = context;

    // Create NVRHI device wrapper
    nvrhi::d3d11::DeviceDesc deviceDesc;
    deviceDesc.context = m_d3d11Context;
    deviceDesc.messageCallback = nullptr;

    m_nvrhiDevice = nvrhi::d3d11::createDevice(deviceDesc);
    if (!m_nvrhiDevice)
    {
        Msg("! [D3D11BackendWrapper] Failed to create NVRHI device");
        return false;
    }

    // Create immediate command list
    nvrhi::CommandListParameters cmdParams;
    cmdParams.enableImmediateExecution = true;
    m_commandList = m_nvrhiDevice->createCommandList(cmdParams);

    if (!m_commandList)
    {
        Msg("! [D3D11BackendWrapper] Failed to create command list");
        m_nvrhiDevice = nullptr;
        return false;
    }

    // Get debug annotation interface
    m_d3d11Context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation),
        reinterpret_cast<void**>(&m_annotation));

    // Query capabilities from CHW
    QueryCapabilities();

    m_initialized = true;
    Msg("* [D3D11BackendWrapper] Initialized successfully");
    Msg("*   API: D3D11 (wrapped from CHW)");
    Msg("*   Bindless: No (D3D11 limitation)");

    return true;
}

void D3D11BackendWrapper::Shutdown()
{
    if (!m_initialized)
        return;

    Msg("* [D3D11BackendWrapper] Shutting down...");

    if (m_annotation)
    {
        m_annotation->Release();
        m_annotation = nullptr;
    }

    m_backBuffer = nullptr;
    m_commandList = nullptr;
    m_nvrhiDevice = nullptr;

    // Don't release device/context - they're owned by CHW
    m_d3d11Device = nullptr;
    m_d3d11Context = nullptr;

    m_initialized = false;
}

void D3D11BackendWrapper::WaitForIdle()
{
    if (m_d3d11Context)
    {
        m_d3d11Context->Flush();
    }
}

nvrhi::ITexture* D3D11BackendWrapper::GetBackBuffer()
{
    // Get back buffer from CHW's swap chain
    if (!m_backBuffer && HW.m_pSwapChain)
    {
        ID3D11Texture2D* backBufferTex = nullptr;
        HRESULT hr = HW.m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferTex));
        if (SUCCEEDED(hr) && backBufferTex)
        {
            m_backBuffer = m_nvrhiDevice->createHandleForNativeTexture(
                nvrhi::ObjectTypes::D3D11_Resource,
                nvrhi::Object(backBufferTex),
                nvrhi::TextureDesc()
                    .setWidth(HW.m_ChainDesc.BufferDesc.Width)
                    .setHeight(HW.m_ChainDesc.BufferDesc.Height)
                    .setFormat(nvrhi::Format::RGBA8_UNORM)
                    .setIsRenderTarget(true)
                    .setDebugName("BackBuffer")
            );
            backBufferTex->Release();
        }
    }
    return m_backBuffer.Get();
}

void D3D11BackendWrapper::Present(bool vsync)
{
    // Delegate to CHW
    HW.Present();
}

std::pair<u32, u32> D3D11BackendWrapper::GetBackBufferSize() const
{
    return HW.GetSurfaceSize();
}

void D3D11BackendWrapper::BeginFrame()
{
    m_commandList->open();
}

void D3D11BackendWrapper::EndFrame()
{
    m_commandList->close();
    m_nvrhiDevice->executeCommandList(m_commandList);
}

void D3D11BackendWrapper::ExecuteCommandList(nvrhi::ICommandList* commandList)
{
    if (m_nvrhiDevice && commandList)
    {
        m_nvrhiDevice->executeCommandList(commandList);
    }
}

void D3D11BackendWrapper::QueryCapabilities()
{
    // Modern capabilities (D3D11 limitations)
    m_capabilities.bindlessTextures = false;
    m_capabilities.meshShaders = false;
    m_capabilities.rayTracing = false;
    m_capabilities.variableRateShading = false;
    m_capabilities.maxBindlessResources = 0;
    m_capabilities.shaderModel = 50;  // SM 5.0

    // Query feature level from device
    D3D_FEATURE_LEVEL featureLevel = m_d3d11Device->GetFeatureLevel();

    // Geometry caps based on feature level
    switch (featureLevel)
    {
    case D3D_FEATURE_LEVEL_10_0:
        m_capabilities.geometry_profile = "vs_4_0";
        m_capabilities.geometry_major = 4;
        m_capabilities.geometry_minor = 0;
        break;
    case D3D_FEATURE_LEVEL_10_1:
        m_capabilities.geometry_profile = "vs_4_1";
        m_capabilities.geometry_major = 4;
        m_capabilities.geometry_minor = 1;
        break;
    case D3D_FEATURE_LEVEL_11_0:
    case D3D_FEATURE_LEVEL_11_1:
    default:
        m_capabilities.geometry_profile = "vs_5_0";
        m_capabilities.geometry_major = 5;
        m_capabilities.geometry_minor = 0;
        break;
    }
    m_capabilities.geometry.dwRegisters = 256;
    m_capabilities.geometry.dwInstructions = 256;
    m_capabilities.geometry.dwClipPlanes = 6;
    m_capabilities.geometry.dwVertexCache = 24;
    m_capabilities.geometry.bVTF = TRUE;

    // Raster caps based on feature level
    switch (featureLevel)
    {
    case D3D_FEATURE_LEVEL_10_0:
        m_capabilities.raster_profile = "ps_4_0";
        m_capabilities.raster_major = 4;
        m_capabilities.raster_minor = 0;
        break;
    case D3D_FEATURE_LEVEL_10_1:
        m_capabilities.raster_profile = "ps_4_1";
        m_capabilities.raster_major = 4;
        m_capabilities.raster_minor = 1;
        break;
    case D3D_FEATURE_LEVEL_11_0:
    case D3D_FEATURE_LEVEL_11_1:
    default:
        m_capabilities.raster_profile = "ps_5_0";
        m_capabilities.raster_major = 5;
        m_capabilities.raster_minor = 0;
        break;
    }
    m_capabilities.raster.dwStages = 15;
    m_capabilities.raster.dwMRT_count = 4;
    m_capabilities.raster.b_MRT_mixdepth = TRUE;
    m_capabilities.raster.dwInstructions = 256;
    m_capabilities.raster.dwRegisters = 256;

    // Fixed caps for D3D11
    m_capabilities.hasStencil = TRUE;
    m_capabilities.hasScissor = TRUE;
    m_capabilities.hasFixedPipeline = FALSE;
    m_capabilities.useCombinedSamplers = TRUE;
    m_capabilities.bForceGPU_REF = FALSE;
    m_capabilities.SceneMode = 0;
    m_capabilities.iGPUNum = 1;

    // GPU identification - query from DXGI adapter
    m_capabilities.id_vendor = 0;
    m_capabilities.id_device = 0;
    IDXGIDevice* dxgiDevice = nullptr;
    if (SUCCEEDED(m_d3d11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice)))
    {
        IDXGIAdapter* adapter = nullptr;
        if (SUCCEEDED(dxgiDevice->GetAdapter(&adapter)))
        {
            DXGI_ADAPTER_DESC desc;
            if (SUCCEEDED(adapter->GetDesc(&desc)))
            {
                m_capabilities.id_vendor = desc.VendorId;
                m_capabilities.id_device = desc.DeviceId;
            }
            adapter->Release();
        }
        dxgiDevice->Release();
    }
}

void D3D11BackendWrapper::UpdateCapabilities()
{
    // SceneMode can be toggled at runtime, other caps are fixed
    // Nothing to update for D3D11 - caps are determined at init time
}

void D3D11BackendWrapper::BeginDebugEvent(pcstr name)
{
    if (m_annotation)
    {
        wchar_t wname[256];
        MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, 256);
        m_annotation->BeginEvent(wname);
    }
}

void D3D11BackendWrapper::EndDebugEvent()
{
    if (m_annotation)
    {
        m_annotation->EndEvent();
    }
}

