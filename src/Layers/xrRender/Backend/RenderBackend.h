#pragma once

#include <nvrhi/nvrhi.h>

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

enum class GraphicsAPI : u8 {
    D3D11,
    D3D12,
    Vulkan,
    Auto
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

enum class ShaderModel : u8 {
    SM_5_0 = 50,
    SM_5_1 = 51,
    SM_6_0 = 60,
    SM_6_1 = 61,
    SM_6_2 = 62,
    SM_6_3 = 63,
    SM_6_4 = 64,
    SM_6_5 = 65,
    SM_6_6 = 66
};

struct BackendCapabilities {
    bool bindlessTextures = false;
    bool bindlessSamplers = false;
    bool meshShaders = false;
    bool rayTracing = false;
    bool variableRateShading = false;
    bool computeShaders = true;

    u32 maxBindlessTextures = 0;
    u32 maxBindlessSamplers = 0;

    ShaderModel shaderModel = ShaderModel::SM_5_0;

    u32 maxVertexShaderRegisters = 256;
    u32 maxVertexShaderInstructions = 65535;
    u32 maxClipPlanes = 6;
    u32 vertexCacheSize = 24;
    bool vertexTextureFetch = true;

    u32 maxPixelShaderRegisters = 256;
    u32 maxPixelShaderInstructions = 65535;
    u32 maxTextureStages = 16;
    u32 maxRenderTargets = 8;
    bool mixedDepthMRT = true;
    bool nonPowerOf2Textures = true;
    bool cubemapSupport = true;

    nvrhi::Format preferredColorFormat = nvrhi::Format::RGBA8_UNORM;
    nvrhi::Format preferredDepthFormat = nvrhi::Format::D24S8;

    u32 vendorId = 0;
    u32 deviceId = 0;
    u32 gpuCount = 1;

    bool hasStencil = true;
    u32 maxStencilValue = 255;

    bool hasScissor = true;
    bool hasTableFog = false;
    bool hasFixedPipeline = false;
    bool useCombinedSamplers = false;
};

struct BackendInitParams {
    SDL_Window* window = nullptr;
    bool enableValidation = false;
    bool enableGPUValidation = false;
    bool vsync = true;
    u32 backBufferWidth = 0;
    u32 backBufferHeight = 0;
    nvrhi::Format backBufferFormat = nvrhi::Format::RGBA8_UNORM;

    ID3D11Device* existingD3D11Device = nullptr;
    ID3D11DeviceContext* existingD3D11Context = nullptr;
};

class ECORE_API IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    virtual bool Initialize(const BackendInitParams& params) = 0;
    virtual void Shutdown() = 0;
    virtual bool IsInitialized() const = 0;

    virtual GraphicsAPI GetAPI() const = 0;
    virtual const BackendCapabilities& GetCapabilities() const = 0;

    virtual nvrhi::IDevice* GetDevice() const = 0;
    virtual nvrhi::ICommandList* GetImmediateCommandList() const = 0;

    virtual nvrhi::CommandListHandle CreateCommandList(
        nvrhi::CommandListParameters params = nvrhi::CommandListParameters()) = 0;

    virtual nvrhi::ITexture* GetCurrentBackBuffer() = 0;
    virtual u32 GetCurrentBackBufferIndex() const = 0;
    virtual u32 GetBackBufferCount() const = 0;
    virtual std::pair<u32, u32> GetBackBufferSize() const = 0;
    virtual void Present(bool vsync) = 0;
    virtual void ResizeSwapChain(u32 width, u32 height) = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void WaitForIdle() = 0;

    virtual void ExecuteCommandList(nvrhi::ICommandList* commandList) = 0;
    virtual void ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) = 0;

    virtual u32 RegisterBindlessTexture(nvrhi::ITexture* texture) { return UINT32_MAX; }
    virtual void UnregisterBindlessTexture(u32 index) {}
    virtual nvrhi::IBindingLayout* GetBindlessLayout() const { return nullptr; }
    virtual nvrhi::IDescriptorTable* GetBindlessDescriptorTable() const { return nullptr; }

    virtual void BeginEvent(const char* name) {}
    virtual void EndEvent() {}
    virtual void SetMarker(const char* name) {}
};

xr_unique_ptr<IRenderBackend> CreateRenderBackend(GraphicsAPI api);

bool IsGraphicsAPIAvailable(GraphicsAPI api);

GraphicsAPI GetBestAvailableAPI();

}
