#pragma once

#include "xrEngine/Engine.h"

enum class DeviceState
{
    Normal = 0,
    Lost,
    NeedReset
};

namespace nvrhi {
    class IDevice;
    class ITexture;
    class IBuffer;
    class ICommandList;
    class IBindingLayout;
    class IDescriptorTable;
    struct CommandListParameters;
    template<class T> class RefCountPtr;
    using CommandListHandle = RefCountPtr<ICommandList>;
}

struct SDL_Window;

class ENGINE_API IRenderBackend
{
public:
    virtual ~IRenderBackend();

    enum class API : u8 {
        D3D11,
        D3D12,
        Vulkan
    };

    virtual API GetAPI() const = 0;
    virtual pcstr GetAPIName() const = 0;
    bool IsFrameGraph() const { return GetAPI() == API::D3D12 || GetAPI() == API::Vulkan; }

    virtual bool IsInitialized() const = 0;
    virtual void Shutdown() = 0;
    virtual void WaitForIdle() = 0;
    virtual DeviceState GetDeviceState() const { return DeviceState::Normal; }

    virtual nvrhi::IDevice* GetDevice() const = 0;
    virtual nvrhi::ICommandList* GetCommandList() const = 0;

    virtual nvrhi::ICommandList* CreateCommandList() { return nullptr; }

    virtual void ExecuteCommandList(nvrhi::ICommandList* commandList) {}
    virtual void ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) {}

    virtual nvrhi::ITexture* GetBackBuffer() = 0;
    virtual u32 GetCurrentBackBufferIndex() const { return 0; }
    virtual u32 GetBackBufferCount() const { return 1; }
    virtual void Present(bool vsync) = 0;
    virtual std::pair<u32, u32> GetBackBufferSize() const = 0;
    virtual void ResizeSwapChain(u32 width, u32 height) {}

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual bool IsInFrame() const { return false; }

    struct Capabilities {
        bool bindlessTextures = false;
        bool meshShaders = false;
        bool rayTracing = false;
        bool variableRateShading = false;
        u32 maxBindlessResources = 0;
        u32 shaderModel = 50;

        struct {
            u32 dwRegisters = 256;
            u32 dwInstructions = 65535;
            u32 dwClipPlanes = 6;
            u32 dwVertexCache = 24;
            bool bVTF = true;
        } geometry;

        struct {
            u32 dwRegisters = 256;
            u32 dwInstructions = 65535;
            u32 dwStages = 16;
            u32 dwMRT_count = 8;
            bool b_MRT_mixdepth = true;
        } raster;

        u16 raster_major = 5;
        u16 raster_minor = 0;
        pcstr raster_profile = "ps_5_0";
        u16 geometry_major = 5;
        u16 geometry_minor = 0;
        pcstr geometry_profile = "vs_5_0";
        u32 iGPUNum = 1;

        u32 id_vendor = 0;
        u32 id_device = 0;

        bool hasStencil = true;
        bool hasScissor = true;
        bool hasFixedPipeline = false;
        bool useCombinedSamplers = true;
        bool bForceGPU_REF = false;

        u32 SceneMode = 0;
    };
    virtual const Capabilities& GetCapabilities() const = 0;

    virtual Capabilities& GetMutableCapabilities() = 0;

    virtual void UpdateCapabilities() {}

    virtual u32 RegisterBindlessTexture(nvrhi::ITexture* texture) { return UINT32_MAX; }
    virtual void UnregisterBindlessTexture(u32 index) {}
    virtual nvrhi::IBindingLayout* GetBindlessLayout() const { return nullptr; }
    virtual nvrhi::IDescriptorTable* GetBindlessDescriptorTable() const { return nullptr; }

    virtual void UploadBufferData(nvrhi::IBuffer* buffer, const void* data, size_t size) {}

    virtual void BeginDebugEvent(pcstr name) {}
    virtual void EndDebugEvent() {}
    virtual void SetMarker(pcstr name) {}
};
