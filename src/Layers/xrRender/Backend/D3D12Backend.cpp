#include "stdafx.h"
#include "D3D12Backend.h"

#include "xrCore/Threading/TaskManager.hpp"

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_4.h>
#include <dxgi1_5.h>
#include <nvrhi/d3d12.h>
#include <nvrhi/validation.h>
#include <SDL3/SDL.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

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
            R_ASSERT2(false, messageText);
            break;
        }
    }
};

static NVRHIMessageCallback s_nvrhiMessageCallback;

namespace
{
DXGI_FORMAT SwapChainFormat(bool hdr)
{
    return hdr ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;
}

DXGI_COLOR_SPACE_TYPE SwapChainColorSpace(bool hdr)
{
    return hdr ? DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
}
}

D3D12Backend::D3D12Backend() = default;

D3D12Backend::~D3D12Backend() {
    Shutdown();
}

bool D3D12Backend::IsSubmissionLeaseSubmitted(u64 lease) const
{
    return m_completion.IsLeaseSubmitted(lease);
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
    m_window = window;

    Msg("* [D3D12Backend] Initializing...");

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

    nvrhi::d3d12::DeviceDesc deviceDesc;
    deviceDesc.pDevice = m_d3d12Device;
    deviceDesc.pGraphicsCommandQueue = m_commandQueue;
    deviceDesc.pComputeCommandQueue = m_computeQueue;
    deviceDesc.errorCB = &s_nvrhiMessageCallback;
    deviceDesc.enableHeapDirectlyIndexed = true;

    nvrhi::DeviceHandle baseDevice = nvrhi::d3d12::createDevice(deviceDesc);
    if (!baseDevice) {
        Msg("! [D3D12Backend] Failed to create NVRHI base device");
        Shutdown();
        return false;
    }

    if (!enableValidation) {
        m_nvrhiDevice = baseDevice;
    } else {
        m_nvrhiDevice = nvrhi::validation::createValidationLayer(baseDevice);
        if (!m_nvrhiDevice) {
            Msg("! [D3D12Backend] Failed to create NVRHI validation layer");
            Shutdown();
            return false;
        }

        Msg("* [D3D12Backend] NVRHI validation layer enabled");
    }

    m_completion.Initialize(m_nvrhiDevice);

    nvrhi::CommandListParameters cmdParams;
    cmdParams.enableImmediateExecution = false;
    m_commandList = m_nvrhiDevice->createCommandList(cmdParams);

    if (!m_commandList) {
        Msg("! [D3D12Backend] Failed to create command list");
        Shutdown();
        return false;
    }

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

    QueryCapabilities();

    CreateBackBufferTextures();

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

    m_bindless.Shutdown();
    m_completion.Shutdown();

    for (auto& bb : m_backBuffers)
        bb = nullptr;
    m_commandList = nullptr;
    m_computeCommandList = nullptr;
    m_uploadCommandList = nullptr;
    m_nvrhiDevice = nullptr;

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

    m_window = nullptr;
    m_hdrOutput = false;
    m_initialized = false;
    Msg("* [D3D12Backend] Shutdown complete");
}

bool D3D12Backend::CreateDXGIFactory(bool enableValidation) {
    UINT dxgiFlags = 0;
    if (enableValidation) {
        dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;

        ID3D12Debug* debugController = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();

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

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            adapter->Release();
            continue;
        }

        if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr))) {
            m_adapter = adapter;
            Msg("* [D3D12Backend] Using adapter: %S", desc.Description);

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
    swapChainDesc.Format = SwapChainFormat(m_hdrOutput);
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = BACK_BUFFER_COUNT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
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
        desc.format = m_hdrOutput ? nvrhi::Format::RGBA16_FLOAT : nvrhi::Format::RGBA8_UNORM;
        desc.isRenderTarget = true;
        desc.debugName = "BackBuffer";
        desc.keepInitialState = true;
        desc.initialState = nvrhi::ResourceStates::Present;

        m_backBuffers[i] = m_nvrhiDevice->createHandleForNativeTexture(
            nvrhi::ObjectTypes::D3D12_Resource,
            nvrhi::Object(backBuffer),
            desc
        );

        backBuffer->Release();
    }
}

void D3D12Backend::CreateBindlessResources() {
    if (!m_bindless.Initialize(m_nvrhiDevice, this, MAX_BINDLESS_TEXTURES))
        Msg("! [D3D12Backend] Failed to create the bindless texture table (SM6.6/ResourceBindingTier3 required)");
}

void D3D12Backend::QueryCapabilities() {
    m_capabilities.bindlessTextures = true;
    m_capabilities.maxBindlessResources = MAX_BINDLESS_TEXTURES;
    m_capabilities.shaderModel = 60;

    m_capabilities.meshShaders = false;
    m_capabilities.meshShaderMaxGroups = 0;
    D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
    if (SUCCEEDED(m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7)))) {
        m_capabilities.meshShaders = (options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED);
        if (m_capabilities.meshShaders)
            m_capabilities.meshShaderMaxGroups = 1u << 22;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    if (SUCCEEDED(m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)))) {
        m_capabilities.rayTracing = (options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED);
    }
    m_capabilities.rayTracingUpdates = m_capabilities.rayTracing;
    m_capabilities.rayTracingCompaction = m_capabilities.rayTracing &&
        m_nvrhiDevice->queryFeatureSupport(nvrhi::Feature::RayTracingCompaction);

    D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
    if (SUCCEEDED(m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6)))) {
        m_capabilities.variableRateShading = (options6.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED);
    }

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
    return m_bindless.Register(texture);
}

void D3D12Backend::UnregisterBindlessTexture(u32 index)
{
    ReleaseBindlessTextures(&index, 1);
}

nvrhi::ITexture* D3D12Backend::GetBindlessTexture(u32 index)
{
    return m_bindless.Get(index);
}

bool D3D12Backend::RetainBindlessTextures(const u32* indices, u32 count)
{
    return m_bindless.Retain(indices, count);
}

void D3D12Backend::ReleaseBindlessTextures(const u32* indices, u32 count)
{
    m_bindless.Release(indices, count, m_inFrame);
}

nvrhi::ITexture* D3D12Backend::GetBackBuffer() {
    return m_backBuffers[m_currentBackBufferIndex].Get();
}

void D3D12Backend::Present(bool vsync) {
    ZoneScopedN("D3D12Backend::Present");
    if (m_swapChain) {
        UINT syncInterval = vsync ? 1 : 0;
        UINT presentFlags = 0;

        if (!vsync && m_tearingSupported) {
            presentFlags = DXGI_PRESENT_ALLOW_TEARING;
        }

        m_swapChain->Present(syncInterval, presentFlags);
    }
}

void D3D12Backend::ResizeSwapChain(u32 width, u32 height)
{
    ResizeSwapChainBuffers(width, height);
}

bool D3D12Backend::ResizeSwapChainBuffers(u32 width, u32 height)
{
    WaitForIdle();

    for (auto& bb : m_backBuffers)
        bb = nullptr;

    const UINT swapChainFlags = m_tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    const HRESULT hr = m_swapChain->ResizeBuffers(BACK_BUFFER_COUNT, width, height, SwapChainFormat(m_hdrOutput), swapChainFlags);
    if (FAILED(hr))
    {
        Msg("! [D3D12Backend] Failed to resize swap chain");
        return false;
    }

    m_backBufferWidth = width;
    m_backBufferHeight = height;
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    const bool colorSpaceApplied = ApplySwapChainColorSpace();
    CreateBackBufferTextures();
    return colorSpaceApplied;
}

bool D3D12Backend::ApplySwapChainColorSpace()
{
    const DXGI_COLOR_SPACE_TYPE colorSpace = SwapChainColorSpace(m_hdrOutput);
    UINT support = 0;
    if (SUCCEEDED(m_swapChain->CheckColorSpaceSupport(colorSpace, &support))
        && (support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) != 0
        && SUCCEEDED(m_swapChain->SetColorSpace1(colorSpace)))
        return true;

    Msg("! [D3D12Backend] Swap chain color space %d is not supported", int(colorSpace));
    return !m_hdrOutput;
}

bool D3D12Backend::SetHDROutput(bool enabled)
{
    if (enabled == m_hdrOutput)
        return true;
    if (!m_initialized || m_inFrame || !m_swapChain)
        return false;
    if (enabled && !IsWindowHDREnabled(m_window))
        return false;

    m_hdrOutput = enabled;
    if (ResizeSwapChainBuffers(m_backBufferWidth, m_backBufferHeight))
        return true;

    m_hdrOutput = !enabled;
    ResizeSwapChainBuffers(m_backBufferWidth, m_backBufferHeight);
    return false;
}

IRenderBackend::DisplayOutput D3D12Backend::GetDisplayOutput() const
{
    return QueryWindowDisplayOutput(m_window, m_hdrOutput);
}

void D3D12Backend::BeginFrame() {
    ZoneScopedN("D3D12::BeginFrame");
    m_completion.DiscardOpenLeases();
    R_ASSERT2(!m_inFrame, "D3D12 frame recording was abandoned without EndFrame");

    if (m_gcTask) {
        ZoneScopedN("D3D12::WaitForGC");
        TaskScheduler->Wait(m_gcTask);
        m_gcTask.Reset();
    }
    m_bindless.BeginFrame();

    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    m_graphicsWaits.Clear();

    {
        ZoneScopedN("D3D12::CommandListOpen");
        m_commandList->open();
    }
    m_inFrame = true;
}

void D3D12Backend::EndFrame() {
    ZoneScopedN("D3D12::EndFrame");
    if (!m_inFrame)
    {
        m_completion.DiscardOpenLeases();
        return;
    }

    m_inFrame = false;

    {
        ZoneScopedN("D3D12::ExecuteCommandList");
        SubmitGraphics();
    }
    m_completion.CloseOpenLeases();


    nvrhi::IDevice* device = m_nvrhiDevice;
    m_gcTask = TaskScheduler->AddTask([device] {
        device->runGarbageCollection();
    });
}

void D3D12Backend::WaitForIdle() {
    if (m_gcTask) {
        TaskScheduler->Wait(m_gcTask);
        m_gcTask.Reset();
    }

    m_nvrhiDevice->waitForIdle();

    m_nvrhiDevice->runGarbageCollection();
}

void D3D12Backend::ExecuteCommandList(nvrhi::ICommandList* commandList) {
    if (m_nvrhiDevice && commandList) {
        const u64 ticket = m_completion.RegisterTicket(nvrhi::CommandQueue::Graphics);
        if (!m_nvrhiDevice->executeCommandList(commandList))
        {
            m_completion.CancelTicket(ticket);
            FATAL("D3D12 rejected a command list submission");
        }
        m_completion.ArmTicket(ticket);
    }
}

u64 D3D12Backend::SubmitGraphics() {
    {
        ZoneScopedN("D3D12::CommandListClose");
        m_commandList->close();
    }
    const u32 token = m_tokens.Issue(nvrhi::CommandQueue::Graphics);
    for (u32 i = 0; i < m_graphicsWaits.count; ++i)
        m_tokens.WaitFor(m_nvrhiDevice, nvrhi::CommandQueue::Graphics, m_graphicsWaits.tokens[i]);
    m_graphicsWaits.Clear();
    const u64 ticket = m_completion.RegisterTicket(nvrhi::CommandQueue::Graphics);
    const u64 instanceID = m_nvrhiDevice->executeCommandList(m_commandList);
    if (!instanceID)
    {
        m_completion.CancelTicket(ticket);
        FATAL("D3D12 rejected a graphics command list submission");
    }
    m_tokens.Resolve(token, nvrhi::CommandQueue::Graphics, instanceID);
    m_completion.ArmTicket(ticket);
    return instanceID;
}

u32 D3D12Backend::SplitGraphics() {
    R_ASSERT2(m_inFrame, "SplitGraphics requires an open frame");
    SubmitGraphics();
    const u32 token = m_tokens.last[u32(nvrhi::CommandQueue::Graphics)];
    {
        ZoneScopedN("D3D12::CommandListOpen");
        m_commandList->open();
    }
    return token;
}

nvrhi::ICommandList* D3D12Backend::AcquireComputeCommandList() {
    R_ASSERT2(m_inFrame && m_computeCommandList, "compute command lists are only available inside a frame with async compute");
    return m_computeCommandList;
}

u32 D3D12Backend::SubmitCompute(nvrhi::ICommandList* commandList, const u32* waitTokens, u32 numWaitTokens) {
    R_ASSERT2(m_inFrame && commandList && m_computeQueue, "SubmitCompute requires an in-frame compute command list");
    const u32 token = m_tokens.Issue(nvrhi::CommandQueue::Compute);
    for (u32 i = 0; i < numWaitTokens; ++i)
        m_tokens.WaitFor(m_nvrhiDevice, nvrhi::CommandQueue::Compute, waitTokens[i]);
    const u64 ticket = m_completion.RegisterTicket(nvrhi::CommandQueue::Compute);
    const u64 instanceID = m_nvrhiDevice->executeCommandList(commandList, nvrhi::CommandQueue::Compute);
    if (!instanceID)
    {
        m_completion.CancelTicket(ticket);
        FATAL("D3D12 rejected a compute command list submission");
    }
    m_tokens.Resolve(token, nvrhi::CommandQueue::Compute, instanceID);
    m_completion.ArmTicket(ticket);
    return token;
}


void D3D12Backend::ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) {
    if (!m_nvrhiDevice)
        return;

    for (u32 i = 0; i < count; ++i) {
        if (commandLists[i]) {
            const u64 ticket = m_completion.RegisterTicket(nvrhi::CommandQueue::Graphics);
            if (!m_nvrhiDevice->executeCommandList(commandLists[i]))
            {
                m_completion.CancelTicket(ticket);
                FATAL("D3D12 rejected an ordered command list submission");
            }
            m_completion.ArmTicket(ticket);
        }
    }
}

void D3D12Backend::UploadBufferData(nvrhi::IBuffer* buffer, const void* data, size_t size, u64 offset) {
    if (!buffer || !data || size == 0)
        return;

    if (m_inFrame) {
        m_commandList->writeBuffer(buffer, data, size, offset);
    } else {
        m_uploadCommandList->open();
        m_uploadCommandList->writeBuffer(buffer, data, size, offset);
        m_uploadCommandList->close();
        const u64 ticket = m_completion.RegisterTicket(nvrhi::CommandQueue::Graphics);
        if (!m_nvrhiDevice->executeCommandList(m_uploadCommandList))
        {
            m_completion.CancelTicket(ticket);
            FATAL("D3D12 rejected a buffer upload submission");
        }
        m_completion.ArmTicket(ticket);
    }
}

nvrhi::ICommandList* D3D12Backend::CreateCommandList() {
    return nullptr;
}

void D3D12Backend::BeginDebugEvent(pcstr name) {
}

void D3D12Backend::EndDebugEvent() {
}

void D3D12Backend::SetMarker(pcstr name) {
}

DeviceState D3D12Backend::GetDeviceState() const {
    if (!m_initialized || !m_d3d12Device)
        return DeviceState::Lost;

    HRESULT hr = m_d3d12Device->GetDeviceRemovedReason();
    if (FAILED(hr)) {
        Msg("! [D3D12Backend] Device removed: 0x%08X", hr);
        return DeviceState::Lost;
    }

    return DeviceState::Normal;
}

IRenderBackend::MemoryBudget D3D12Backend::GetMemoryBudget() const
{
    MemoryBudget result;
    result.bufferRangeBytes = 4ull << D3D12_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP;
    if (!m_adapter)
        return result;
    IDXGIAdapter3* adapter = nullptr;
    if (SUCCEEDED(m_adapter->QueryInterface(IID_PPV_ARGS(&adapter))))
    {
        DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
        if (SUCCEEDED(adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info)))
        {
            result.budgetBytes = info.Budget;
            result.usageBytes = info.CurrentUsage;
            result.usageKnown = true;
        }
        adapter->Release();
    }
    if (!result.budgetBytes)
    {
        DXGI_ADAPTER_DESC1 desc = {};
        if (SUCCEEDED(m_adapter->GetDesc1(&desc)))
            result.budgetBytes = desc.DedicatedVideoMemory ? desc.DedicatedVideoMemory : desc.SharedSystemMemory;
    }
    return result;
}
