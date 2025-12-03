// RenderBackend.h
// API-agnostic backend abstraction for D3D11/D3D12/Vulkan
// Each backend creates its native device and wraps it with NVRHI
// This replaces the legacy CHW (Hardware) class entirely
#pragma once

#include <nvrhi/nvrhi.h>

// Forward declarations for native APIs (only used in backend implementations)
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D12Device;
struct ID3D12CommandQueue;
struct VkInstance_T;
struct VkPhysicalDevice_T;
struct VkDevice_T;
struct VkQueue_T;

typedef VkInstance_T* VkInstance;
typedef VkPhysicalDevice_T* VkPhysicalDevice;
typedef VkDevice_T* VkDevice;
typedef VkQueue_T* VkQueue;

struct SDL_Window;

namespace xray::render::ng {

// ═══════════════════════════════════════════════════════
//  GRAPHICS API SELECTION
// ═══════════════════════════════════════════════════════

enum class GraphicsAPI : u8 {
    D3D11,      // DirectX 11 (legacy, no true bindless)
    D3D12,      // DirectX 12 (bindless via descriptor heaps) - PRIMARY TARGET
    Vulkan,     // Vulkan (bindless via descriptor indexing) - FUTURE
    Auto        // Auto-select best available (prefers D3D12)
};

inline const char* GetGraphicsAPIName(GraphicsAPI api) {
    switch (api) {
        case GraphicsAPI::D3D11:  return "D3D11";
        case GraphicsAPI::D3D12:  return "D3D12";
        case GraphicsAPI::Vulkan: return "Vulkan";
        case GraphicsAPI::Auto:   return "Auto";
        default:                  return "Unknown";
    }
}

// ═══════════════════════════════════════════════════════
//  BACKEND CAPABILITIES (replaces CHWCaps)
// ═══════════════════════════════════════════════════════

// Shader model version (e.g., 50 = SM5.0, 66 = SM6.6)
enum class ShaderModel : u8 {
    SM_5_0 = 50,    // D3D11 baseline
    SM_5_1 = 51,    // Bindless-compatible (resource arrays)
    SM_6_0 = 60,    // D3D12 wave intrinsics
    SM_6_1 = 61,    // SV_ViewID, SV_Barycentrics
    SM_6_2 = 62,    // FP16
    SM_6_3 = 63,    // DXR 1.0
    SM_6_4 = 64,    // VRS
    SM_6_5 = 65,    // DXR 1.1, mesh shaders, sampler feedback
    SM_6_6 = 66     // Dynamic resources, derivatives in compute
};

struct BackendCapabilities {
    // ═══════ Modern Features ═══════
    bool bindlessTextures = false;      // True bindless texture arrays (D3D12/Vulkan)
    bool bindlessSamplers = false;      // Bindless sampler arrays
    bool meshShaders = false;           // Mesh/amplification shaders
    bool rayTracing = false;            // Hardware ray tracing (DXR/VK_KHR_ray_tracing)
    bool variableRateShading = false;   // VRS support
    bool computeShaders = true;         // Compute shader support

    u32 maxBindlessTextures = 0;        // Max textures in bindless heap (65536 for D3D12)
    u32 maxBindlessSamplers = 0;        // Max samplers in bindless heap

    // ═══════ Shader Model ═══════
    ShaderModel shaderModel = ShaderModel::SM_5_0;

    // ═══════ Legacy Caps (for gradual migration) ═══════
    // Geometry/Vertex capabilities
    u32 maxVertexShaderRegisters = 256;     // Constant buffer registers
    u32 maxVertexShaderInstructions = 65535;
    u32 maxClipPlanes = 6;
    u32 vertexCacheSize = 24;
    bool vertexTextureFetch = true;         // VTF support

    // Pixel/Raster capabilities
    u32 maxPixelShaderRegisters = 256;
    u32 maxPixelShaderInstructions = 65535;
    u32 maxTextureStages = 16;              // Texture stages (legacy FFP concept)
    u32 maxRenderTargets = 8;               // MRT count
    bool mixedDepthMRT = true;              // Different depth per MRT
    bool nonPowerOf2Textures = true;
    bool cubemapSupport = true;

    // Format support
    nvrhi::Format preferredColorFormat = nvrhi::Format::RGBA8_UNORM;
    nvrhi::Format preferredDepthFormat = nvrhi::Format::D24S8;

    // GPU info
    u32 vendorId = 0;
    u32 deviceId = 0;
    u32 gpuCount = 1;

    // Stencil (for shadow volumes, etc)
    bool hasStencil = true;
    u32 maxStencilValue = 255;

    // Feature flags
    bool hasScissor = true;
    bool hasTableFog = false;               // Legacy FFP fog
    bool hasFixedPipeline = false;          // No FFP in D3D12/Vulkan
    bool useCombinedSamplers = false;       // Vulkan uses separate; D3D uses combined
};

// ═══════════════════════════════════════════════════════
//  BACKEND INITIALIZATION PARAMS
// ═══════════════════════════════════════════════════════

struct BackendInitParams {
    SDL_Window* window = nullptr;
    bool enableValidation = false;
    bool enableGPUValidation = false;   // More thorough but slower
    bool vsync = true;
    u32 backBufferWidth = 0;
    u32 backBufferHeight = 0;
    nvrhi::Format backBufferFormat = nvrhi::Format::RGBA8_UNORM;

    // For wrapping existing D3D11 device (migration path)
    ID3D11Device* existingD3D11Device = nullptr;
    ID3D11DeviceContext* existingD3D11Context = nullptr;
};

// ═══════════════════════════════════════════════════════
//  RENDER BACKEND INTERFACE
// ═══════════════════════════════════════════════════════

class ECORE_API IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    // ═══════ Lifecycle ═══════
    virtual bool Initialize(const BackendInitParams& params) = 0;
    virtual void Shutdown() = 0;
    virtual bool IsInitialized() const = 0;

    // ═══════ API Info ═══════
    virtual GraphicsAPI GetAPI() const = 0;
    virtual const BackendCapabilities& GetCapabilities() const = 0;

    // ═══════ NVRHI Access ═══════
    // All rendering should go through NVRHI for API abstraction
    virtual nvrhi::IDevice* GetDevice() const = 0;
    virtual nvrhi::ICommandList* GetImmediateCommandList() const = 0;

    // Create additional command lists for parallel recording
    virtual nvrhi::CommandListHandle CreateCommandList(
        nvrhi::CommandListParameters params = nvrhi::CommandListParameters()) = 0;

    // ═══════ Swap Chain ═══════
    virtual nvrhi::ITexture* GetCurrentBackBuffer() = 0;
    virtual u32 GetCurrentBackBufferIndex() const = 0;
    virtual u32 GetBackBufferCount() const = 0;
    virtual std::pair<u32, u32> GetBackBufferSize() const = 0;
    virtual void Present(bool vsync) = 0;
    virtual void ResizeSwapChain(u32 width, u32 height) = 0;

    // ═══════ Frame Synchronization ═══════
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void WaitForIdle() = 0;

    // ═══════ Command Execution ═══════
    virtual void ExecuteCommandList(nvrhi::ICommandList* commandList) = 0;
    virtual void ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) = 0;

    // ═══════ Bindless Resources (D3D12/Vulkan only) ═══════
    // Returns UINT32_MAX if bindless not supported or failed
    virtual u32 RegisterBindlessTexture(nvrhi::ITexture* texture) { return UINT32_MAX; }
    virtual void UnregisterBindlessTexture(u32 index) {}
    virtual nvrhi::IBindingLayout* GetBindlessLayout() const { return nullptr; }
    virtual nvrhi::IDescriptorTable* GetBindlessDescriptorTable() const { return nullptr; }

    // ═══════ Debug/Profiling ═══════
    virtual void BeginEvent(const char* name) {}
    virtual void EndEvent() {}
    virtual void SetMarker(const char* name) {}
};

// ═══════════════════════════════════════════════════
//  BACKEND FACTORY
// ═══════════════════════════════════════════════════

// Create backend for specified API
xr_unique_ptr<IRenderBackend> CreateRenderBackend(GraphicsAPI api);

// Check if API is available on this system
bool IsGraphicsAPIAvailable(GraphicsAPI api);

// Get best available API (prefers D3D12, falls back to D3D11)
GraphicsAPI GetBestAvailableAPI();

} // namespace xray::render::ng
