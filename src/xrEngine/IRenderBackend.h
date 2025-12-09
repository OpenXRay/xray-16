#pragma once

#include "xrEngine/Engine.h"

// Device state enum - shared between IRenderBackend and IRender
enum class DeviceState
{
    Normal = 0,
    Lost,
    NeedReset
};

// Forward declarations - avoid exposing NVRHI to engine code
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

// ═══════════════════════════════════════════════════════
//  IRENDERBACKEND INTERFACE
// ═══════════════════════════════════════════════════════
// Abstract hardware backend interface exposed through GEnv.Backend
// Implementations: D3D11BackendWrapper, D3D12Backend, VulkanBackend (future)
//
// This replaces the legacy CHW (HW global) with an API-agnostic interface.

class ENGINE_API IRenderBackend
{
public:
    virtual ~IRenderBackend();

    // ═══════ Graphics API ═══════
    enum class API : u8 {
        D3D11,      // Legacy, no true bindless
        D3D12,      // Primary target, bindless via descriptor heaps
        Vulkan      // Future, bindless via descriptor indexing
    };

    virtual API GetAPI() const = 0;
    virtual pcstr GetAPIName() const = 0;

    // ═══════ Lifecycle ═══════
    virtual bool IsInitialized() const = 0;
    virtual void Shutdown() = 0;
    virtual void WaitForIdle() = 0;
    virtual DeviceState GetDeviceState() const { return DeviceState::Normal; }

    // ═══════ NVRHI Access ═══════
    // All modern rendering goes through NVRHI
    virtual nvrhi::IDevice* GetDevice() const = 0;
    virtual nvrhi::ICommandList* GetCommandList() const = 0;

    // Create additional command lists for parallel recording
    // Note: Returns raw pointer, caller must manage lifetime via RefCountPtr
    virtual nvrhi::ICommandList* CreateCommandList() { return nullptr; }

    // ═══════ Command Execution ═══════
    virtual void ExecuteCommandList(nvrhi::ICommandList* commandList) {}
    virtual void ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) {}

    // ═══════ Swap Chain ═══════
    virtual nvrhi::ITexture* GetBackBuffer() = 0;
    virtual u32 GetCurrentBackBufferIndex() const { return 0; }
    virtual u32 GetBackBufferCount() const { return 1; }
    virtual void Present(bool vsync) = 0;
    virtual std::pair<u32, u32> GetBackBufferSize() const = 0;
    virtual void ResizeSwapChain(u32 width, u32 height) {}

    // ═══════ Frame Sync ═══════
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual bool IsInFrame() const { return false; }  // True between BeginFrame/EndFrame

    // ═══════ Capabilities ═══════
    struct Capabilities {
        // Modern features
        bool bindlessTextures = false;
        bool meshShaders = false;
        bool rayTracing = false;
        bool variableRateShading = false;
        u32 maxBindlessResources = 0;
        u32 shaderModel = 50;  // 50 = SM5.0, 60 = SM6.0, etc.

        // Legacy caps (mapped from CHWCaps for migration)
        struct {
            u32 dwRegisters = 256;
            u32 dwInstructions = 65535;
            u32 dwClipPlanes = 6;
            u32 dwVertexCache = 24;
            bool bVTF = true;  // Vertex texture fetch
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

        // GPU identification (for vendor-specific workarounds)
        u32 id_vendor = 0;   // 0x10DE = NVIDIA, 0x1002 = AMD, 0x8086 = Intel
        u32 id_device = 0;

        bool hasStencil = true;
        bool hasScissor = true;
        bool hasFixedPipeline = false;
        bool useCombinedSamplers = true;  // D3D11/D3D12 use combined
        bool bForceGPU_REF = false;

        // Debug mode (0=normal, 1/2=debug wireframe modes)
        u32 SceneMode = 0;
    };
    virtual const Capabilities& GetCapabilities() const = 0;

    // Mutable access for debug mode toggling (SceneMode)
    virtual Capabilities& GetMutableCapabilities() = 0;

    // Update capabilities (called on device reset, etc.)
    virtual void UpdateCapabilities() {}

    // ═══════ Bindless (D3D12/Vulkan only) ═══════
    // Returns UINT32_MAX if not supported
    virtual u32 RegisterBindlessTexture(nvrhi::ITexture* texture) { return UINT32_MAX; }
    virtual void UnregisterBindlessTexture(u32 index) {}
    virtual nvrhi::IBindingLayout* GetBindlessLayout() const { return nullptr; }
    virtual nvrhi::IDescriptorTable* GetBindlessDescriptorTable() const { return nullptr; }

    // ═══════ Buffer Upload (for resource creation) ═══════
    // Uploads data to GPU buffer using a dedicated upload command list
    // This avoids creating a new command list for every buffer upload
    virtual void UploadBufferData(nvrhi::IBuffer* buffer, const void* data, size_t size) {}

    // ═══════ Debug ═══════
    virtual void BeginDebugEvent(pcstr name) {}
    virtual void EndDebugEvent() {}
    virtual void SetMarker(pcstr name) {}
};
