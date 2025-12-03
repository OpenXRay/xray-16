// D3D11Backend.cpp
// DirectX 11 backend implementation (legacy, no true bindless)
// Kept for compatibility during migration to D3D12/Vulkan
#include "stdafx.h"
#include "D3D11Backend.h"

#include <d3d11_1.h>
#include <dxgi1_2.h>

// For ID3DUserDefinedAnnotation
#include <d3d11.h>

namespace xray::render::ng {

D3D11Backend::D3D11Backend() = default;

D3D11Backend::~D3D11Backend() {
    Shutdown();
}

bool D3D11Backend::Initialize(const BackendInitParams& params) {
    if (m_initialized) {
        Msg("! [D3D11Backend] Already initialized");
        return false;
    }

    Msg("* [D3D11Backend] Initializing...");

    // Either wrap existing device or create new one
    bool success = false;
    if (params.existingD3D11Device && params.existingD3D11Context) {
        success = WrapExistingDevice(params);
    } else {
        success = CreateDeviceAndSwapChain(params);
    }

    if (!success) {
        Shutdown();
        return false;
    }

    // Create NVRHI device wrapper
    nvrhi::d3d11::DeviceDesc deviceDesc;
    deviceDesc.context = m_d3d11Context;
    deviceDesc.messageCallback = nullptr;

    m_device = nvrhi::d3d11::createDevice(deviceDesc);
    if (!m_device) {
        Msg("! [D3D11Backend] Failed to create NVRHI device");
        Shutdown();
        return false;
    }

    // Create immediate command list
    nvrhi::CommandListParameters cmdParams;
    cmdParams.enableImmediateExecution = true;
    m_commandList = m_device->createCommandList(cmdParams);

    if (!m_commandList) {
        Msg("! [D3D11Backend] Failed to create command list");
        Shutdown();
        return false;
    }

    // Query capabilities
    QueryCapabilities();

    // Create back buffer texture wrapper if we have a swap chain
    if (m_swapChain) {
        CreateBackBufferTexture();
    }

    m_initialized = true;
    Msg("* [D3D11Backend] Initialized successfully");
    Msg("*   Bindless textures: %s", m_capabilities.bindlessTextures ? "Yes" : "No (D3D11 limitation)");

    return true;
}

void D3D11Backend::Shutdown() {
    if (!m_initialized && !m_device)
        return;

    Msg("* [D3D11Backend] Shutting down...");

    WaitForIdle();

    // Release NVRHI resources
    for (auto& bb : m_backBuffers)
        bb = nullptr;
    m_commandList = nullptr;
    m_device = nullptr;

    // Release annotation interface
    if (m_annotation) {
        m_annotation->Release();
        m_annotation = nullptr;
    }

    if (m_ownsDevice) {
        if (m_swapChain) {
            m_swapChain->Release();
            m_swapChain = nullptr;
        }
        if (m_d3d11Context) {
            m_d3d11Context->Release();
            m_d3d11Context = nullptr;
        }
        if (m_d3d11Device) {
            m_d3d11Device->Release();
            m_d3d11Device = nullptr;
        }
    }

    m_initialized = false;
    Msg("* [D3D11Backend] Shutdown complete");
}

bool D3D11Backend::WrapExistingDevice(const BackendInitParams& params) {
    m_d3d11Device = params.existingD3D11Device;
    m_d3d11Context = params.existingD3D11Context;
    m_ownsDevice = false;

    // We don't own the swap chain in this case - CHW manages it
    m_swapChain = nullptr;

    Msg("* [D3D11Backend] Wrapping existing D3D11 device");
    return true;
}

bool D3D11Backend::CreateDeviceAndSwapChain(const BackendInitParams& params) {
    // This path creates a new device - used when not wrapping existing CHW
    // For now, we primarily use WrapExistingDevice during migration

    Msg("! [D3D11Backend] CreateDeviceAndSwapChain not yet implemented");
    Msg("!   Use existingD3D11Device/Context params to wrap CHW's device");
    return false;

    // TODO: Implement full device creation if needed
    // This would involve:
    // 1. CreateDXGIFactory
    // 2. EnumAdapters
    // 3. D3D11CreateDeviceAndSwapChain
}

void D3D11Backend::CreateBackBufferTexture() {
    if (!m_swapChain || !m_device)
        return;

    // Get back buffer from swap chain
    ID3D11Texture2D* backBufferTex = nullptr;
    HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferTex);
    if (FAILED(hr) || !backBufferTex)
        return;

    D3D11_TEXTURE2D_DESC desc;
    backBufferTex->GetDesc(&desc);

    m_backBufferWidth = desc.Width;
    m_backBufferHeight = desc.Height;

    // Wrap with NVRHI
    nvrhi::TextureDesc nvrhiDesc;
    nvrhiDesc.width = desc.Width;
    nvrhiDesc.height = desc.Height;
    nvrhiDesc.format = nvrhi::Format::RGBA8_UNORM; // Typical swap chain format
    nvrhiDesc.isRenderTarget = true;
    nvrhiDesc.debugName = "BackBuffer";

    m_backBuffers[0] = m_device->createHandleForNativeTexture(
        nvrhi::ObjectTypes::D3D11_Resource,
        nvrhi::Object(backBufferTex),
        nvrhiDesc
    );

    backBufferTex->Release();
}

void D3D11Backend::QueryCapabilities() {
    // D3D11 limitations - no true bindless
    m_capabilities.bindlessTextures = false;
    m_capabilities.bindlessSamplers = false;
    m_capabilities.meshShaders = false;             // D3D12+ only
    m_capabilities.rayTracing = false;              // D3D12+ only
    m_capabilities.variableRateShading = false;
    m_capabilities.computeShaders = true;           // D3D11 supports compute
    m_capabilities.maxBindlessTextures = 128;       // SRV slot limit
    m_capabilities.maxBindlessSamplers = 16;        // Sampler slot limit

    // Shader model
    m_capabilities.shaderModel = ShaderModel::SM_5_0;

    // Legacy caps for migration
    m_capabilities.maxVertexShaderRegisters = 256;
    m_capabilities.maxVertexShaderInstructions = 65535;
    m_capabilities.maxClipPlanes = 6;
    m_capabilities.vertexCacheSize = 24;
    m_capabilities.vertexTextureFetch = true;

    m_capabilities.maxPixelShaderRegisters = 256;
    m_capabilities.maxPixelShaderInstructions = 65535;
    m_capabilities.maxTextureStages = 16;
    m_capabilities.maxRenderTargets = 8;
    m_capabilities.mixedDepthMRT = true;
    m_capabilities.nonPowerOf2Textures = true;
    m_capabilities.cubemapSupport = true;

    // Format support
    m_capabilities.preferredColorFormat = nvrhi::Format::RGBA8_UNORM;
    m_capabilities.preferredDepthFormat = nvrhi::Format::D24S8;

    // Query GPU info if device exists
    if (m_d3d11Device) {
        IDXGIDevice* dxgiDevice = nullptr;
        if (SUCCEEDED(m_d3d11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice))) {
            IDXGIAdapter* adapter = nullptr;
            if (SUCCEEDED(dxgiDevice->GetAdapter(&adapter))) {
                DXGI_ADAPTER_DESC desc;
                if (SUCCEEDED(adapter->GetDesc(&desc))) {
                    m_capabilities.vendorId = desc.VendorId;
                    m_capabilities.deviceId = desc.DeviceId;
                }
                adapter->Release();
            }
            dxgiDevice->Release();
        }
    }

    m_capabilities.gpuCount = 1;
    m_capabilities.hasStencil = true;
    m_capabilities.maxStencilValue = 255;
    m_capabilities.hasScissor = true;
    m_capabilities.hasTableFog = false;
    m_capabilities.hasFixedPipeline = false;
    m_capabilities.useCombinedSamplers = true;  // D3D11 uses combined texture+sampler model

    // Setup annotation interface for profiling
    if (m_d3d11Context && !m_annotation) {
        m_d3d11Context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation),
                                       reinterpret_cast<void**>(&m_annotation));
    }
}

nvrhi::ITexture* D3D11Backend::GetCurrentBackBuffer() {
    return m_backBuffers[m_currentBackBufferIndex].Get();
}

void D3D11Backend::Present(bool vsync) {
    if (m_swapChain) {
        m_swapChain->Present(vsync ? 1 : 0, 0);
    }
}

void D3D11Backend::ResizeSwapChain(u32 width, u32 height) {
    if (!m_swapChain)
        return;

    // Release old back buffers
    for (auto& bb : m_backBuffers)
        bb = nullptr;

    // Resize
    HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        Msg("! [D3D11Backend] Failed to resize swap chain");
        return;
    }

    m_backBufferWidth = width;
    m_backBufferHeight = height;

    // Recreate back buffer wrapper
    CreateBackBufferTexture();
}

void D3D11Backend::BeginFrame() {
    // D3D11 doesn't need explicit frame begin
    m_commandList->open();
}

void D3D11Backend::EndFrame() {
    m_commandList->close();
    m_device->executeCommandList(m_commandList);
}

void D3D11Backend::WaitForIdle() {
    if (m_device) {
        m_device->waitForIdle();
    }
}

void D3D11Backend::ExecuteCommandList(nvrhi::ICommandList* commandList) {
    if (m_device && commandList) {
        m_device->executeCommandList(commandList);
    }
}

void D3D11Backend::ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) {
    if (!m_device)
        return;

    for (u32 i = 0; i < count; ++i) {
        if (commandLists[i]) {
            m_device->executeCommandList(commandLists[i]);
        }
    }
}

nvrhi::CommandListHandle D3D11Backend::CreateCommandList(nvrhi::CommandListParameters params) {
    if (!m_device)
        return nullptr;

    // D3D11 always uses immediate execution
    params.enableImmediateExecution = true;
    return m_device->createCommandList(params);
}

void D3D11Backend::BeginEvent(const char* name) {
    if (m_annotation) {
        // Convert to wide string
        wchar_t wname[256];
        MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, 256);
        m_annotation->BeginEvent(wname);
    }
}

void D3D11Backend::EndEvent() {
    if (m_annotation) {
        m_annotation->EndEvent();
    }
}

void D3D11Backend::SetMarker(const char* name) {
    if (m_annotation) {
        wchar_t wname[256];
        MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, 256);
        m_annotation->SetMarker(wname);
    }
}

} // namespace xray::render::ng
