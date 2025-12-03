// D3D12Backend.cpp
// DirectX 12 backend implementation with bindless texture support
#include "stdafx.h"
#include "D3D12Backend.h"

#include <d3d12.h>
#include <dxgi1_4.h>
#include <SDL.h>
#include <SDL_syswm.h>

// Link D3D12 libraries
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

namespace xray::render::ng {

D3D12Backend::D3D12Backend() = default;

D3D12Backend::~D3D12Backend() {
    Shutdown();
}

bool D3D12Backend::Initialize(const BackendInitParams& params) {
    if (m_initialized) {
        Msg("! [D3D12Backend] Already initialized");
        return false;
    }

    Msg("* [D3D12Backend] Initializing...");

    if (!CreateDevice(params)) {
        Shutdown();
        return false;
    }

    if (!CreateCommandQueue()) {
        Shutdown();
        return false;
    }

    if (!CreateFence()) {
        Shutdown();
        return false;
    }

    if (!CreateSwapChain(params)) {
        Shutdown();
        return false;
    }

    // Create NVRHI device wrapper
    nvrhi::d3d12::DeviceDesc deviceDesc;
    deviceDesc.pDevice = m_d3d12Device;
    deviceDesc.pGraphicsCommandQueue = m_commandQueue;
    deviceDesc.errorCB = nullptr;
    deviceDesc.enableHeapDirectlyIndexed = true;  // Enable bindless

    m_device = nvrhi::d3d12::createDevice(deviceDesc);
    if (!m_device) {
        Msg("! [D3D12Backend] Failed to create NVRHI device");
        Shutdown();
        return false;
    }

    // Create command list
    nvrhi::CommandListParameters cmdParams;
    cmdParams.enableImmediateExecution = false; // D3D12 uses deferred execution
    m_commandList = m_device->createCommandList(cmdParams);

    if (!m_commandList) {
        Msg("! [D3D12Backend] Failed to create command list");
        Shutdown();
        return false;
    }

    // Query capabilities
    QueryCapabilities();

    // Create back buffer textures
    CreateBackBufferTextures();

    // Create bindless resources
    CreateBindlessResources();

    m_initialized = true;
    Msg("* [D3D12Backend] Initialized successfully");
    Msg("*   Bindless textures: Yes (max %u)", m_capabilities.maxBindlessTextures);

    return true;
}

void D3D12Backend::Shutdown() {
    if (!m_initialized && !m_device)
        return;

    Msg("* [D3D12Backend] Shutting down...");

    WaitForIdle();

    // Release NVRHI resources
    m_bindlessDescriptorTable = nullptr;
    m_bindlessLayout = nullptr;
    for (auto& bb : m_backBuffers)
        bb = nullptr;
    m_commandList = nullptr;
    m_device = nullptr;

    // Release D3D12 resources
    if (m_fenceEvent) {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
    if (m_fence) {
        m_fence->Release();
        m_fence = nullptr;
    }
    if (m_swapChain) {
        m_swapChain->Release();
        m_swapChain = nullptr;
    }
    if (m_commandQueue) {
        m_commandQueue->Release();
        m_commandQueue = nullptr;
    }
    if (m_d3d12Device) {
        m_d3d12Device->Release();
        m_d3d12Device = nullptr;
    }
    if (m_adapter) {
        m_adapter->Release();
        m_adapter = nullptr;
    }
    if (m_dxgiFactory) {
        m_dxgiFactory->Release();
        m_dxgiFactory = nullptr;
    }

    m_initialized = false;
    Msg("* [D3D12Backend] Shutdown complete");
}

bool D3D12Backend::CreateDevice(const BackendInitParams& params) {
    // Create DXGI factory
    UINT dxgiFlags = 0;
    if (params.enableValidation) {
        dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;

        // Enable D3D12 debug layer
        ID3D12Debug* debugController = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
            debugController->Release();
            Msg("* [D3D12Backend] Debug layer enabled");
        }
    }

    HRESULT hr = CreateDXGIFactory2(dxgiFlags, IID_PPV_ARGS(&m_dxgiFactory));
    if (FAILED(hr)) {
        Msg("! [D3D12Backend] Failed to create DXGI factory");
        return false;
    }

    // Find best adapter
    IDXGIAdapter1* adapter = nullptr;
    for (UINT i = 0; m_dxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // Skip software adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            adapter->Release();
            continue;
        }

        // Try to create device
        hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_d3d12Device));
        if (SUCCEEDED(hr)) {
            m_adapter = adapter;
            Msg("* [D3D12Backend] Using adapter: %S", desc.Description);
            break;
        }
        adapter->Release();
    }

    if (!m_d3d12Device) {
        Msg("! [D3D12Backend] No D3D12-capable adapter found");
        return false;
    }

    return true;
}

bool D3D12Backend::CreateCommandQueue() {
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    HRESULT hr = m_d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
    if (FAILED(hr)) {
        Msg("! [D3D12Backend] Failed to create command queue");
        return false;
    }

    return true;
}

bool D3D12Backend::CreateFence() {
    HRESULT hr = m_d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    if (FAILED(hr)) {
        Msg("! [D3D12Backend] Failed to create fence");
        return false;
    }

    m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent) {
        Msg("! [D3D12Backend] Failed to create fence event");
        return false;
    }

    m_currentFenceValue = 1;
    return true;
}

bool D3D12Backend::CreateSwapChain(const BackendInitParams& params) {
    if (!params.window) {
        Msg("! [D3D12Backend] No window provided for swap chain");
        return false;
    }

    // Get HWND from SDL window
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(params.window, &wmInfo)) {
        Msg("! [D3D12Backend] Failed to get window info");
        return false;
    }
    HWND hwnd = wmInfo.info.win.window;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = params.backBufferWidth;
    swapChainDesc.Height = params.backBufferHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = BACK_BUFFER_COUNT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    IDXGISwapChain1* swapChain1 = nullptr;
    HRESULT hr = m_dxgiFactory->CreateSwapChainForHwnd(
        m_commandQueue,
        hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1
    );

    if (FAILED(hr)) {
        Msg("! [D3D12Backend] Failed to create swap chain");
        return false;
    }

    hr = swapChain1->QueryInterface(IID_PPV_ARGS(&m_swapChain));
    swapChain1->Release();

    if (FAILED(hr)) {
        Msg("! [D3D12Backend] Failed to query swap chain interface");
        return false;
    }

    m_backBufferWidth = params.backBufferWidth;
    m_backBufferHeight = params.backBufferHeight;
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    return true;
}

void D3D12Backend::CreateBackBufferTextures() {
    for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i) {
        ID3D12Resource* backBuffer = nullptr;
        HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
        if (FAILED(hr))
            continue;

        nvrhi::TextureDesc desc;
        desc.width = m_backBufferWidth;
        desc.height = m_backBufferHeight;
        desc.format = nvrhi::Format::RGBA8_UNORM;
        desc.isRenderTarget = true;
        desc.debugName = "BackBuffer";

        m_backBuffers[i] = m_device->createHandleForNativeTexture(
            nvrhi::ObjectTypes::D3D12_Resource,
            nvrhi::Object(backBuffer),
            desc
        );

        backBuffer->Release();
    }
}

void D3D12Backend::CreateBindlessResources() {
    // Create bindless layout for unbounded texture array
    nvrhi::BindlessLayoutDesc bindlessDesc;
    bindlessDesc.visibility = nvrhi::ShaderType::Pixel | nvrhi::ShaderType::Compute;
    bindlessDesc.firstSlot = 0;
    bindlessDesc.maxCapacity = MAX_BINDLESS_TEXTURES;
    bindlessDesc.registerSpaces.push_back(nvrhi::BindingLayoutItem::Texture_SRV(0)); // space0, t0[]

    m_bindlessLayout = m_device->createBindlessLayout(bindlessDesc);
    if (!m_bindlessLayout) {
        Msg("! [D3D12Backend] Failed to create bindless layout");
        return;
    }

    // Create descriptor table
    m_bindlessDescriptorTable = m_device->createDescriptorTable(m_bindlessLayout);
    if (!m_bindlessDescriptorTable) {
        Msg("! [D3D12Backend] Failed to create bindless descriptor table");
        return;
    }

    Msg("* [D3D12Backend] Bindless resources created (max %u textures)", MAX_BINDLESS_TEXTURES);
}

void D3D12Backend::QueryCapabilities() {
    m_capabilities.bindlessTextures = true;
    m_capabilities.bindlessSamplers = true;
    m_capabilities.maxBindlessTextures = MAX_BINDLESS_TEXTURES;
    m_capabilities.maxBindlessSamplers = 2048;

    // Check for mesh shaders
    D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
    if (SUCCEEDED(m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7)))) {
        m_capabilities.meshShaders = (options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED);
    }

    // Check for ray tracing
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    if (SUCCEEDED(m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)))) {
        m_capabilities.rayTracing = (options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED);
    }

    // Check for VRS
    D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
    if (SUCCEEDED(m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6)))) {
        m_capabilities.variableRateShading = (options6.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED);
    }
}

u32 D3D12Backend::RegisterBindlessTexture(nvrhi::ITexture* texture) {
    if (!m_bindlessDescriptorTable || !texture)
        return UINT32_MAX;

    // Get free index
    u32 index;
    if (!m_freeBindlessIndices.empty()) {
        index = m_freeBindlessIndices.back();
        m_freeBindlessIndices.pop_back();
    } else {
        if (m_nextBindlessIndex >= MAX_BINDLESS_TEXTURES) {
            Msg("! [D3D12Backend] Bindless texture limit reached");
            return UINT32_MAX;
        }
        index = m_nextBindlessIndex++;
    }

    // Write descriptor
    nvrhi::BindingSetItem item = nvrhi::BindingSetItem::Texture_SRV(0, texture);
    item.slot = index;

    if (!m_device->writeDescriptorTable(m_bindlessDescriptorTable, item)) {
        m_freeBindlessIndices.push_back(index);
        return UINT32_MAX;
    }

    return index;
}

void D3D12Backend::UnregisterBindlessTexture(u32 index) {
    if (index < MAX_BINDLESS_TEXTURES) {
        m_freeBindlessIndices.push_back(index);
    }
}

nvrhi::ITexture* D3D12Backend::GetCurrentBackBuffer() {
    return m_backBuffers[m_currentBackBufferIndex].Get();
}

void D3D12Backend::Present(bool vsync) {
    if (m_swapChain) {
        m_swapChain->Present(vsync ? 1 : 0, 0);
    }
}

void D3D12Backend::ResizeSwapChain(u32 width, u32 height) {
    WaitForIdle();

    // Release back buffers
    for (auto& bb : m_backBuffers)
        bb = nullptr;

    // Resize
    HRESULT hr = m_swapChain->ResizeBuffers(
        BACK_BUFFER_COUNT,
        width,
        height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        0
    );

    if (FAILED(hr)) {
        Msg("! [D3D12Backend] Failed to resize swap chain");
        return;
    }

    m_backBufferWidth = width;
    m_backBufferHeight = height;
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Recreate back buffer textures
    CreateBackBufferTextures();
}

void D3D12Backend::BeginFrame() {
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Wait for previous frame on this buffer to complete
    WaitForFence(m_fenceValues[m_currentBackBufferIndex]);

    m_commandList->open();
}

void D3D12Backend::EndFrame() {
    m_commandList->close();
    m_device->executeCommandList(m_commandList);

    // Signal fence
    m_fenceValues[m_currentBackBufferIndex] = m_currentFenceValue;
    m_commandQueue->Signal(m_fence, m_currentFenceValue);
    m_currentFenceValue++;
}

void D3D12Backend::WaitForFence(u64 fenceValue) {
    if (m_fence->GetCompletedValue() < fenceValue) {
        m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void D3D12Backend::WaitForIdle() {
    // Signal and wait for all frames
    u64 waitValue = m_currentFenceValue;
    m_commandQueue->Signal(m_fence, waitValue);
    m_currentFenceValue++;

    WaitForFence(waitValue);
}

void D3D12Backend::ExecuteCommandList(nvrhi::ICommandList* commandList) {
    if (m_device && commandList) {
        m_device->executeCommandList(commandList);
    }
}

void D3D12Backend::ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) {
    if (!m_device)
        return;

    for (u32 i = 0; i < count; ++i) {
        if (commandLists[i]) {
            m_device->executeCommandList(commandLists[i]);
        }
    }
}

nvrhi::CommandListHandle D3D12Backend::CreateCommandList(nvrhi::CommandListParameters params) {
    if (!m_device)
        return nullptr;

    // D3D12 uses deferred execution
    params.enableImmediateExecution = false;
    return m_device->createCommandList(params);
}

void D3D12Backend::BeginEvent(const char* name) {
    // PIX events for D3D12
    // Requires PIXBeginEvent from pix3.h, or use NVRHI's annotation
    if (m_commandList) {
        // NVRHI handles PIX internally when available
        // For now, use command list annotation if available
    }
}

void D3D12Backend::EndEvent() {
    // PIX events for D3D12
}

void D3D12Backend::SetMarker(const char* name) {
    // PIX marker for D3D12
}

} // namespace xray::render::ng
