// D3D12Backend.cpp
// DirectX 12 backend implementation with bindless texture support
#include "stdafx.h"
#include "D3D12Backend.h"

#include "xrCore/Threading/TaskManager.hpp"

#include <d3d12.h>
#include <d3d12sdklayers.h>  // For ID3D12Debug1
#include <dxgi1_4.h>
#include <dxgi1_5.h>  // For IDXGIFactory5 and DXGI_FEATURE_PRESENT_ALLOW_TEARING
#include <nvrhi/d3d12.h>
#include <nvrhi/validation.h>
#include <SDL3/SDL.h>

// Link D3D12 libraries
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

// ═══════════════════════════════════════════════════════
//  NVRHI MESSAGE CALLBACK
// ═══════════════════════════════════════════════════════
// Captures NVRHI errors/warnings and logs them via engine's Msg system

class NVRHIMessageCallback : public nvrhi::IMessageCallback {
public:
    void message(nvrhi::MessageSeverity severity, const char* messageText) override {
        switch (severity) {
        case nvrhi::MessageSeverity::Info:
            Msg("* [NVRHI] %s", messageText);
            break;
        case nvrhi::MessageSeverity::Warning:
            Msg("! [NVRHI] WARNING: %s", messageText);
            break;
        case nvrhi::MessageSeverity::Error:
            Msg("! [NVRHI] ERROR: %s", messageText);
            break;
        case nvrhi::MessageSeverity::Fatal:
            Msg("! [NVRHI] FATAL: %s", messageText);
            R_ASSERT2(false, messageText);  // Trigger debugger break
            break;
        }
    }
};

// Static instance (must outlive the NVRHI device)
static NVRHIMessageCallback s_nvrhiMessageCallback;

D3D12Backend::D3D12Backend() = default;

D3D12Backend::~D3D12Backend() {
    Shutdown();
}

bool D3D12Backend::Initialize(SDL_Window* window, u32 width, u32 height, bool enableValidation) {
    if (m_initialized) {
        Msg("! [D3D12Backend] Already initialized");
        return false;
    }

    if (!window) {
        Msg("! [D3D12Backend] No window provided");
        return false;
    }

    Msg("* [D3D12Backend] Initializing...");

    // Get HWND from SDL window
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    if (!hwnd) {
        Msg("! [D3D12Backend] Failed to get HWND: %s", SDL_GetError());
        return false;
    }

    if (!CreateDXGIFactory(enableValidation)) {
        Shutdown();
        return false;
    }

    if (!SelectAdapter()) {
        Shutdown();
        return false;
    }

    if (!CreateDevice()) {
        Shutdown();
        return false;
    }

    if (!CreateCommandQueue()) {
        Shutdown();
        return false;
    }

    if (!CreateSwapChain(hwnd, width, height)) {
        Shutdown();
        return false;
    }

    // Create NVRHI device wrapper
    nvrhi::d3d12::DeviceDesc deviceDesc;
    deviceDesc.pDevice = m_d3d12Device;
    deviceDesc.pGraphicsCommandQueue = m_commandQueue;
    deviceDesc.pComputeCommandQueue = m_computeQueue;
    deviceDesc.errorCB = &s_nvrhiMessageCallback;  // Error/warning logging
    deviceDesc.enableHeapDirectlyIndexed = true;   // Enable bindless

    nvrhi::DeviceHandle baseDevice = nvrhi::d3d12::createDevice(deviceDesc);
    if (!baseDevice) {
        Msg("! [D3D12Backend] Failed to create NVRHI base device");
        Shutdown();
        return false;
    }

    // Wrap with validation layer for detailed error messages
    m_nvrhiDevice = nvrhi::validation::createValidationLayer(baseDevice);
    if (!m_nvrhiDevice) {
        Msg("! [D3D12Backend] Failed to create NVRHI validation layer");
        Shutdown();
        return false;
    }

    Msg("* [D3D12Backend] NVRHI validation layer enabled");

    // Create per-frame command list (for rendering)
    nvrhi::CommandListParameters cmdParams;
    cmdParams.enableImmediateExecution = false; // D3D12 uses deferred execution
    m_commandList = m_nvrhiDevice->createCommandList(cmdParams);

    if (!m_commandList) {
        Msg("! [D3D12Backend] Failed to create command list");
        Shutdown();
        return false;
    }

    // Create persistent upload command list (reused for out-of-frame uploads)
    // This prevents exhausting D3D12's command allocator pool during level loading
    m_uploadCommandList = m_nvrhiDevice->createCommandList(cmdParams);
    if (!m_uploadCommandList) {
        Msg("! [D3D12Backend] Failed to create upload command list");
        Shutdown();
        return false;
    }

    if (m_computeQueue) {
        nvrhi::CommandListParameters computeParams;
        computeParams.enableImmediateExecution = false;
        computeParams.setQueueType(nvrhi::CommandQueue::Compute);
        m_computeCommandList = m_nvrhiDevice->createCommandList(computeParams);
        if (m_computeCommandList) {
            Msg("* [D3D12Backend] Async compute enabled (dedicated compute queue)");
        } else {
            Msg("! [D3D12Backend] Failed to create compute command list (async compute disabled)");
        }
    }

    // Query capabilities
    QueryCapabilities();

    // Create back buffer textures
    CreateBackBufferTextures();

    // Create bindless resources
    CreateBindlessResources();

    m_initialized = true;
    Msg("* [D3D12Backend] Initialized successfully");
    Msg("*   Bindless textures: Yes (max %u)", m_capabilities.maxBindlessResources);

    return true;
}

void D3D12Backend::Shutdown() {
    if (!m_initialized && !m_nvrhiDevice)
        return;

    Msg("* [D3D12Backend] Shutting down...");

    WaitForIdle();

    // Release NVRHI resources
    m_bindlessDescriptorTable = nullptr;
    m_bindlessLayout = nullptr;
    for (auto& bb : m_backBuffers)
        bb = nullptr;
    m_commandList = nullptr;
    m_computeCommandList = nullptr;
    m_uploadCommandList = nullptr;
    m_nvrhiDevice = nullptr;

    // Release D3D12 resources
    if (m_swapChain) {
        m_swapChain->Release();
        m_swapChain = nullptr;
    }
    if (m_computeQueue) {
        m_computeQueue->Release();
        m_computeQueue = nullptr;
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

bool D3D12Backend::CreateDXGIFactory(bool enableValidation) {
    UINT dxgiFlags = 0;
    if (enableValidation) {
        dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;

        // Enable D3D12 debug layer
        ID3D12Debug* debugController = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();

            // Enable GPU-based validation for more detailed errors
            ID3D12Debug1* debugController1 = nullptr;
            if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1)))) {
                debugController1->SetEnableGPUBasedValidation(TRUE);
                debugController1->SetEnableSynchronizedCommandQueueValidation(TRUE);
                debugController1->Release();
                Msg("* [D3D12Backend] GPU-based validation enabled");
            }

            debugController->Release();
            Msg("* [D3D12Backend] Debug layer enabled");
        }
    }

    HRESULT hr = CreateDXGIFactory2(dxgiFlags, IID_PPV_ARGS(&m_dxgiFactory));
    if (FAILED(hr)) {
        Msg("! [D3D12Backend] Failed to create DXGI factory");
        return false;
    }

    // Check for tearing support (required for uncapped framerate in windowed mode)
    IDXGIFactory5* factory5 = nullptr;
    if (SUCCEEDED(m_dxgiFactory->QueryInterface(IID_PPV_ARGS(&factory5)))) {
        BOOL allowTearing = FALSE;
        hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        m_tearingSupported = SUCCEEDED(hr) && allowTearing;
        factory5->Release();
        Msg("* [D3D12Backend] Tearing support: %s", m_tearingSupported ? "Yes" : "No");
    }

    return true;
}

bool D3D12Backend::SelectAdapter() {
    IDXGIAdapter1* adapter = nullptr;
    for (UINT i = 0; m_dxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // Skip software adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            adapter->Release();
            continue;
        }

        // Check if adapter supports D3D12
        if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr))) {
            m_adapter = adapter;
            Msg("* [D3D12Backend] Using adapter: %S", desc.Description);

            // Store vendor/device IDs
            m_capabilities.id_vendor = desc.VendorId;
            m_capabilities.id_device = desc.DeviceId;
            return true;
        }
        adapter->Release();
    }

    Msg("! [D3D12Backend] No D3D12-capable adapter found");
    return false;
}

bool D3D12Backend::CreateDevice() {
    HRESULT hr = D3D12CreateDevice(m_adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_d3d12Device));
    if (FAILED(hr)) {
        Msg("! [D3D12Backend] Failed to create D3D12 device");
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
        Msg("! [D3D12Backend] Failed to create graphics command queue");
        return false;
    }

    D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {};
    computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    computeQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    hr = m_d3d12Device->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&m_computeQueue));
    if (FAILED(hr)) {
        Msg("! [D3D12Backend] Failed to create compute command queue (async compute disabled)");
        m_computeQueue = nullptr;
    }

    return true;
}

bool D3D12Backend::CreateSwapChain(HWND hwnd, u32 width, u32 height) {
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = BACK_BUFFER_COUNT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    // Enable tearing for uncapped framerate in windowed mode
    swapChainDesc.Flags = m_tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

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

    m_backBufferWidth = width;
    m_backBufferHeight = height;
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
        desc.keepInitialState = true;
        desc.initialState = nvrhi::ResourceStates::Present;  // Swapchain starts in Present state

        m_backBuffers[i] = m_nvrhiDevice->createHandleForNativeTexture(
            nvrhi::ObjectTypes::D3D12_Resource,
            nvrhi::Object(backBuffer),
            desc
        );

        backBuffer->Release();
    }
}

void D3D12Backend::CreateBindlessResources() {
    Msg("* [D3D12Backend] Creating bindless resources...");

    nvrhi::BindlessLayoutDesc bindlessDesc;
    bindlessDesc.visibility = nvrhi::ShaderType::All;
    bindlessDesc.firstSlot = 0;
    bindlessDesc.maxCapacity = MAX_BINDLESS_TEXTURES;
    bindlessDesc.registerSpaces = { nvrhi::BindingLayoutItem::Texture_SRV(1) };

    m_bindlessLayout = m_nvrhiDevice->createBindlessLayout(bindlessDesc);
    if (!m_bindlessLayout) {
        Msg("! [D3D12Backend] Failed to create bindless layout - createBindlessLayout returned null");
        Msg("! [D3D12Backend] This may indicate SM6.6/ResourceBindingTier3 is not supported");
        return;
    }
    Msg("* [D3D12Backend] Bindless layout created: %p", m_bindlessLayout.Get());

    // Create descriptor table
    m_bindlessDescriptorTable = m_nvrhiDevice->createDescriptorTable(m_bindlessLayout);
    if (!m_bindlessDescriptorTable) {
        Msg("! [D3D12Backend] Failed to create bindless descriptor table");
        return;
    }
    Msg("* [D3D12Backend] Bindless descriptor table created: %p", m_bindlessDescriptorTable.Get());

    // Resize the descriptor table to allocate the actual descriptors
    // Without this, capacity=0 and all writeDescriptorTable calls will fail
    m_nvrhiDevice->resizeDescriptorTable(m_bindlessDescriptorTable, MAX_BINDLESS_TEXTURES, false);
    Msg("* [D3D12Backend] Bindless descriptor table resized to %u entries", MAX_BINDLESS_TEXTURES);

    Msg("* [D3D12Backend] Bindless resources created successfully (max %u textures)", MAX_BINDLESS_TEXTURES);
}

void D3D12Backend::QueryCapabilities() {
    // Modern features
    m_capabilities.bindlessTextures = true;
    m_capabilities.maxBindlessResources = MAX_BINDLESS_TEXTURES;
    m_capabilities.shaderModel = 60;  // SM6.0 for D3D12

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

    // Legacy caps (D3D12 always supports these)
    m_capabilities.geometry.dwRegisters = 256;
    m_capabilities.geometry.dwInstructions = 65535;
    m_capabilities.geometry.dwClipPlanes = 6;
    m_capabilities.geometry.dwVertexCache = 24;
    m_capabilities.geometry.bVTF = true;

    m_capabilities.raster.dwRegisters = 256;
    m_capabilities.raster.dwInstructions = 65535;
    m_capabilities.raster.dwStages = 16;
    m_capabilities.raster.dwMRT_count = 8;
    m_capabilities.raster.b_MRT_mixdepth = true;

    m_capabilities.raster_major = 6;
    m_capabilities.raster_minor = 0;
    m_capabilities.raster_profile = "ps_6_0";
    m_capabilities.geometry_major = 6;
    m_capabilities.geometry_minor = 0;
    m_capabilities.geometry_profile = "vs_6_0";
    m_capabilities.iGPUNum = 1;

    m_capabilities.hasStencil = true;
    m_capabilities.hasScissor = true;
    m_capabilities.hasFixedPipeline = false;
    m_capabilities.useCombinedSamplers = true;
}

u32 D3D12Backend::RegisterBindlessTexture(nvrhi::ITexture* texture) {
    if (!m_bindlessDescriptorTable || !texture)
        return UINT32_MAX;

    auto it = m_bindlessTextureMap.find(texture);
    if (it != m_bindlessTextureMap.end())
        return it->second;

    u32 slot;
    if (!m_freeBindlessIndices.empty()) {
        slot = m_freeBindlessIndices.back();
        m_freeBindlessIndices.pop_back();
    } else {
        if (m_nextBindlessIndex >= MAX_BINDLESS_TEXTURES) {
            Msg("! [D3D12Backend] Bindless texture limit reached");
            return UINT32_MAX;
        }
        slot = m_nextBindlessIndex++;
    }

    nvrhi::BindingSetItem item = nvrhi::BindingSetItem::Texture_SRV(0, texture);
    item.slot = slot;

    if (!m_nvrhiDevice->writeDescriptorTable(m_bindlessDescriptorTable, item)) {
        m_freeBindlessIndices.push_back(slot);
        return UINT32_MAX;
    }

    m_bindlessTextureMap[texture] = slot;
    return slot;
}

void D3D12Backend::UnregisterBindlessTexture(u32 index) {
    if (index >= MAX_BINDLESS_TEXTURES)
        return;
    for (auto it = m_bindlessTextureMap.begin(); it != m_bindlessTextureMap.end(); ++it) {
        if (it->second == index) {
            m_bindlessTextureMap.erase(it);
            break;
        }
    }
    m_freeBindlessIndices.push_back(index);
}

nvrhi::ITexture* D3D12Backend::GetBackBuffer() {
    return m_backBuffers[m_currentBackBufferIndex].Get();
}

void D3D12Backend::Present(bool vsync) {
    ZoneScopedN("D3D12Backend::Present");
    if (m_swapChain) {
        UINT syncInterval = vsync ? 1 : 0;
        UINT presentFlags = 0;

        // Use DXGI_PRESENT_ALLOW_TEARING for uncapped framerate when vsync is off
        if (!vsync && m_tearingSupported) {
            presentFlags = DXGI_PRESENT_ALLOW_TEARING;
        }

        m_swapChain->Present(syncInterval, presentFlags);
    }
}

void D3D12Backend::ResizeSwapChain(u32 width, u32 height) {
    WaitForIdle();

    // Release back buffers
    for (auto& bb : m_backBuffers)
        bb = nullptr;

    // Resize (preserve tearing flag)
    UINT swapChainFlags = m_tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    HRESULT hr = m_swapChain->ResizeBuffers(
        BACK_BUFFER_COUNT,
        width,
        height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        swapChainFlags
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
    ZoneScopedN("D3D12::BeginFrame");

    // ═══════════════════════════════════════════════════════
    //  ASYNC GC: Wait for previous frame's garbage collection
    // ═══════════════════════════════════════════════════════
    // GC was launched at end of previous frame and runs in parallel
    // with CPU game logic. Wait here before any NVRHI operations.
    if (m_gcTask) {
        ZoneScopedN("D3D12::WaitForGC");
        TaskScheduler->Wait(*m_gcTask);
        m_gcTask = nullptr;
    }

    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    {
        ZoneScopedN("D3D12::CommandListOpen");
        m_commandList->open();
    }
    m_inFrame = true;
}

void D3D12Backend::EndFrame() {
    ZoneScopedN("D3D12::EndFrame");

    m_inFrame = false;

    {
        ZoneScopedN("D3D12::CommandListClose");
        m_commandList->close();
    }

    // NVRHI handles fence signaling internally
    {
        ZoneScopedN("D3D12::ExecuteCommandList");
        m_lastGraphicsInstanceID = m_nvrhiDevice->executeCommandList(m_commandList);
    }

    // ═══════════════════════════════════════════════════════
    //  ASYNC GC: Launch garbage collection on background thread
    // ═══════════════════════════════════════════════════════
    // After executeCommandList, no more NVRHI device operations this frame.
    // GC can safely run in parallel with CPU game logic (FrameMove, AI, physics).
    // Will be waited on at start of next BeginFrame.
    //
    // NOTE: No ZoneScopedN here! The profiler resets zones at frame boundary,
    // and this task spans across frames, which corrupts the zone hierarchy.
    nvrhi::IDevice* device = m_nvrhiDevice;
    m_gcTask = &TaskScheduler->AddTask([device] {
        device->runGarbageCollection();
    });
}

void D3D12Backend::WaitForIdle() {
    // Wait for any pending async GC first
    if (m_gcTask) {
        TaskScheduler->Wait(*m_gcTask);
        m_gcTask = nullptr;
    }

    // Use NVRHI's built-in wait which handles its internal fences
    m_nvrhiDevice->waitForIdle();

    // Also run garbage collection to release any pending resources
    m_nvrhiDevice->runGarbageCollection();
}

void D3D12Backend::ExecuteCommandList(nvrhi::ICommandList* commandList) {
    if (m_nvrhiDevice && commandList) {
        m_nvrhiDevice->executeCommandList(commandList);
    }
}

u64 D3D12Backend::ExecuteComputeCommandList(nvrhi::ICommandList* commandList) {
    if (!m_nvrhiDevice || !commandList || !m_computeQueue)
        return 0;
    return m_nvrhiDevice->executeCommandList(commandList, nvrhi::CommandQueue::Compute);
}

void D3D12Backend::QueueWaitForCompute(u64 instanceID) {
    if (!m_nvrhiDevice || !m_computeQueue || instanceID == 0)
        return;
    m_nvrhiDevice->queueWaitForCommandList(nvrhi::CommandQueue::Graphics, nvrhi::CommandQueue::Compute, instanceID);
}

void D3D12Backend::ComputeWaitForPreviousGraphics() {
    if (!m_nvrhiDevice || !m_computeQueue || m_lastGraphicsInstanceID == 0)
        return;
    m_nvrhiDevice->queueWaitForCommandList(nvrhi::CommandQueue::Compute, nvrhi::CommandQueue::Graphics, m_lastGraphicsInstanceID);
}

void D3D12Backend::ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) {
    if (!m_nvrhiDevice)
        return;

    for (u32 i = 0; i < count; ++i) {
        if (commandLists[i]) {
            m_nvrhiDevice->executeCommandList(commandLists[i]);
        }
    }
}

void D3D12Backend::UploadBufferData(nvrhi::IBuffer* buffer, const void* data, size_t size) {
    if (!buffer || !data || size == 0)
        return;

    if (m_inFrame) {
        // During frame: use main command list (batched, single execute at EndFrame)
        m_commandList->writeBuffer(buffer, data, size);
    } else {
        // Outside frame (initialization): use persistent upload command list
        // CRITICAL: Must reuse same command list to avoid exhausting D3D12's
        // command allocator pool during model loading (hundreds of buffers)
        m_uploadCommandList->open();
        m_uploadCommandList->writeBuffer(buffer, data, size);
        m_uploadCommandList->close();
        m_nvrhiDevice->executeCommandList(m_uploadCommandList);
    }
}

nvrhi::ICommandList* D3D12Backend::CreateCommandList() {
    // TODO: Implement when parallel command list recording is needed
    // The caller would need to manage the returned pointer's lifetime via RefCountPtr
    return nullptr;
}

void D3D12Backend::BeginDebugEvent(pcstr name) {
    // PIX events for D3D12
    // NVRHI handles PIX internally when available
}

void D3D12Backend::EndDebugEvent() {
    // PIX events for D3D12
}

void D3D12Backend::SetMarker(pcstr name) {
    // PIX marker for D3D12
}

DeviceState D3D12Backend::GetDeviceState() const {
    if (!m_initialized || !m_d3d12Device)
        return DeviceState::Lost;

    // Check for device removal
    HRESULT hr = m_d3d12Device->GetDeviceRemovedReason();
    if (FAILED(hr)) {
        Msg("! [D3D12Backend] Device removed: 0x%08X", hr);
        return DeviceState::Lost;
    }

    return DeviceState::Normal;
}
