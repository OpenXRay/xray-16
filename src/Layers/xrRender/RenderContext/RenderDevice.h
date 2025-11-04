#pragma once

#include "ResourceHandle.h"
#include "PipelineState.h"
#include <nvrhi/nvrhi.h>

// Forward declare ModernResourceManager (outside ng namespace!)
namespace xray::render::resources {
    class ModernResourceManager;
}

namespace xray::render::ng {

// Forward declarations
class RenderContext;
namespace nvrhi_wrapper = r4::nvrhi_wrapper;

// ═══════════════════════════════════════════════════
//  RENDER DEVICE (Resource Factory + Manager)
// ═══════════════════════════════════════════════════

class RenderDevice {
public:
    RenderDevice();
    ~RenderDevice();

    // ═══════════════════════════════════════════════════
    //  INITIALIZATION
    // ═══════════════════════════════════════════════════

    bool InitializeD3D11(ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    bool IsInitialized() const { return m_initialized; }

    // Get the underlying NVRHI device
    nvrhi::IDevice* GetNVRHIDevice() const;

    // ═══════════════════════════════════════════════════
    //  TEXTURE CREATION
    // ═══════════════════════════════════════════════════

    struct TextureDesc {
        u32 width = 0;
        u32 height = 0;
        u32 depth = 1;        // For 3D textures
        u32 arraySize = 1;    // For texture arrays/cubemaps
        u32 mipLevels = 1;

        nvrhi::Format format = nvrhi::Format::RGBA8_UNORM;

        enum Dimension {
            Texture1D,
            Texture2D,
            Texture3D,
            TextureCube,
            Texture2DArray,
        };
        Dimension dimension = Texture2D;

        // Usage flags
        bool isRenderTarget = false;
        bool isDepthStencil = false;
        bool isUAV = false;           // Unordered access (read/write)
        bool generateMips = false;

        // Initial state
        nvrhi::ResourceStates initialState = nvrhi::ResourceStates::Common;

        shared_str debugName;
    };

    TextureHandle CreateTexture(
        const TextureDesc& desc,
        const void* initialData = nullptr);

    // Wrap existing D3D11 texture (for backbuffer, etc.)
    TextureHandle CreateTextureFromD3D11(
        ID3D11Resource* d3d11Texture,
        const TextureDesc& desc);

    void DestroyTexture(TextureHandle handle);

    // Access
    nvrhi::ITexture* GetNativeTexture(TextureHandle handle);
    const TextureDesc* GetTextureDesc(TextureHandle handle);
    bool IsTextureValid(TextureHandle handle) const;

    // Upload texture data (for streaming/loading)
    // Uploads data to specific mip level and array slice
    void UploadTextureData(
        TextureHandle handle,
        u32 arraySlice,
        u32 mipLevel,
        const void* data,
        size_t dataSize
    );

    // Upload multiple mip levels at once (more efficient)
    struct TextureSliceData {
        u32 arraySlice;
        u32 mipLevel;
        const void* data;
        size_t dataSize;
    };

    void UploadTextureData(
        TextureHandle handle,
        const TextureSliceData* slices,
        u32 sliceCount
    );

    // ═══════════════════════════════════════════════════
    //  BUFFER CREATION
    // ═══════════════════════════════════════════════════

    struct BufferDesc {
        u64 byteSize = 0;
        u32 structStride = 0;  // For structured buffers (0 = not structured)

        // Usage flags
        bool isConstantBuffer = false;
        bool isVertexBuffer = false;
        bool isIndexBuffer = false;
        bool isDrawIndirectArgs = false;
        bool isUAV = false;

        // CPU access
        bool cpuRead = false;
        bool cpuWrite = false;

        // Update frequency hint
        bool isVolatile = false;  // Updated every frame

        shared_str debugName;
    };

    BufferHandle CreateBuffer(
        const BufferDesc& desc,
        const void* initialData = nullptr);

    void DestroyBuffer(BufferHandle handle);

    // Update buffer data (use only for dynamic buffers)
    void UpdateBuffer(
        BufferHandle handle,
        const void* data,
        u64 size,
        u64 offset = 0);

    // Access
    nvrhi::IBuffer* GetNativeBuffer(BufferHandle handle);
    const BufferDesc* GetBufferDesc(BufferHandle handle);
    bool IsBufferValid(BufferHandle handle) const;

    // ═══════════════════════════════════════════════════
    //  SAMPLER CREATION
    // ═══════════════════════════════════════════════════

    struct SamplerDesc {
        enum class AddressMode {
            Wrap,
            Mirror,
            Clamp,
            Border,
            MirrorOnce
        };

        AddressMode addressU = AddressMode::Wrap;
        AddressMode addressV = AddressMode::Wrap;
        AddressMode addressW = AddressMode::Wrap;

        enum class Filter {
            Point,
            Linear,
            Anisotropic
        };

        Filter minFilter = Filter::Linear;
        Filter magFilter = Filter::Linear;
        Filter mipFilter = Filter::Linear;

        bool anisotropyEnable = false;
        u32 maxAnisotropy = 1;

        float mipLodBias = 0.0f;
        float minLod = -FLT_MAX;
        float maxLod = FLT_MAX;

        // Comparison sampling (for shadow maps)
        bool comparisonEnable = false;
        ComparisonFunc comparisonFunc = ComparisonFunc::Never;

        // Border color (if addressMode = Border)
        float borderColor[4] = {0, 0, 0, 0};

        shared_str debugName;
    };

    SamplerHandle CreateSampler(const SamplerDesc& desc);
    void DestroySampler(SamplerHandle handle);

    nvrhi::ISampler* GetNativeSampler(SamplerHandle handle);
    const SamplerDesc* GetSamplerDesc(SamplerHandle handle);
    bool IsSamplerValid(SamplerHandle handle) const;

    // ═══════════════════════════════════════════════════
    //  BINDING LAYOUTS & SETS (for descriptor bindings)
    // ═══════════════════════════════════════════════════

    // Create binding layout (what resources a shader expects)
    nvrhi::BindingLayoutHandle CreateBindingLayout(const nvrhi::BindingLayoutDesc& desc);

    // Create binding set (actual resources to bind)
    nvrhi::BindingSetHandle CreateBindingSet(const nvrhi::BindingSetDesc& desc, nvrhi::IBindingLayout* layout);

    // Create framebuffer (render targets + depth)
    nvrhi::FramebufferHandle CreateFramebuffer(const nvrhi::FramebufferDesc& desc);

    // ═══════════════════════════════════════════════════
    //  SHADER CREATION
    // ═══════════════════════════════════════════════════

    ShaderHandle CreateShader(
        ShaderStage stage,
        const void* bytecode,
        size_t bytecodeSize,
        const char* debugName);

    // Note: NVRHI doesn't support wrapping native shader objects (only textures/buffers)
    // To use X-Ray shaders, we need access to the original bytecode

    void DestroyShader(ShaderHandle handle);

    RCShader* GetShader(ShaderHandle handle);
    bool IsShaderValid(ShaderHandle handle) const;

    // ═══════════════════════════════════════════════════
    //  CONTEXT CREATION
    // ═══════════════════════════════════════════════════

    RenderContext* CreateContext();
    void DestroyContext(RenderContext* context);

    // Execute a context's recorded commands
    void ExecuteContext(RenderContext* context);

    // ═══════════════════════════════════════════════════
    //  PIPELINE CACHE ACCESS
    // ═══════════════════════════════════════════════════

    PipelineStateCache* GetPipelineCache() { return m_pipelineCache.get(); }

    // ═══════════════════════════════════════════════════
    //  RESOURCE MANAGER ACCESS
    // ═══════════════════════════════════════════════════

    xray::render::resources::ModernResourceManager* GetModernResourceManager() { return m_modernResourceManager.get(); }

    // ═══════════════════════════════════════════════════
    //  DEVICE ACCESS (for low-level code)
    // ═══════════════════════════════════════════════════

    nvrhi::IDevice* GetNativeDevice() const;

    // ═══════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════

    struct Statistics {
        u32 texturesAlive = 0;
        u32 buffersAlive = 0;
        u32 samplersAlive = 0;
        u32 shadersAlive = 0;
        u32 contextsAlive = 0;

        u64 textureMemory = 0;   // Bytes
        u64 bufferMemory = 0;    // Bytes

        u32 texturesCreated = 0;
        u32 buffersCreated = 0;
        u32 samplersCreated = 0;
        u32 shadersCreated = 0;
    };

    const Statistics& GetStatistics() const { return m_stats; }
    void ResetStatistics();

private:
    // NVRHI device wrapper
    xr_unique_ptr<nvrhi_wrapper::NVRHIDevice> m_nvrhiDevice;

    // Pipeline state cache
    xr_unique_ptr<PipelineStateCache> m_pipelineCache;

    // Modern resource manager (Week 2-3)
    xr_unique_ptr<xray::render::resources::ModernResourceManager> m_modernResourceManager;

    // Persistent command list for upload operations (texture/buffer uploads)
    // Created once during Init(), reused for all uploads, released in destructor
    nvrhi::CommandListHandle m_uploadCommandList;

    // Initialization state
    bool m_initialized = false;

    // ═══════════════════════════════════════════════════
    //  RESOURCE STORAGE (Generational Sparse Arrays)
    // ═══════════════════════════════════════════════════

    struct TextureInfo {
        nvrhi::TextureHandle nvrhiHandle;
        TextureDesc desc;
        u32 generation = 0;
        bool isAlive = false;
    };

    struct BufferInfo {
        nvrhi::BufferHandle nvrhiHandle;
        BufferDesc desc;
        u32 generation = 0;
        bool isAlive = false;
    };

    struct SamplerInfo {
        nvrhi::SamplerHandle nvrhiHandle;
        SamplerDesc desc;
        u32 generation = 0;
        bool isAlive = false;
    };

    struct ShaderInfo {
        xr_unique_ptr<RCShader> shader;
        u32 generation = 0;
        bool isAlive = false;
    };

    // Resource arrays
    xr_vector<TextureInfo> m_textures;
    xr_vector<BufferInfo> m_buffers;
    xr_vector<SamplerInfo> m_samplers;
    xr_vector<ShaderInfo> m_shaders;

    // Free lists (indices of destroyed resources)
    xr_vector<u32> m_freeTextures;
    xr_vector<u32> m_freeBuffers;
    xr_vector<u32> m_freeSamplers;
    xr_vector<u32> m_freeShaders;

    // Statistics
    Statistics m_stats;

    // Internal helpers
    TextureHandle AllocateTextureHandle();
    BufferHandle AllocateBufferHandle();
    SamplerHandle AllocateSamplerHandle();
    ShaderHandle AllocateShaderHandle();

    void FreeTextureHandle(TextureHandle handle);
    void FreeBufferHandle(BufferHandle handle);
    void FreeSamplerHandle(SamplerHandle handle);
    void FreeShaderHandle(ShaderHandle handle);

    // Validation helpers
    bool ValidateTextureHandle(TextureHandle handle) const;
    bool ValidateBufferHandle(BufferHandle handle) const;
    bool ValidateSamplerHandle(SamplerHandle handle) const;
    bool ValidateShaderHandle(ShaderHandle handle) const;

    // Conversion helpers
    static nvrhi::TextureDesc ConvertTextureDesc(const TextureDesc& desc);
    static nvrhi::BufferDesc ConvertBufferDesc(const BufferDesc& desc);
    static nvrhi::SamplerDesc ConvertSamplerDesc(const SamplerDesc& desc);
    static nvrhi::ShaderDesc ConvertShaderDesc(ShaderStage stage, const char* debugName);

    // Prevent copying
    RenderDevice(const RenderDevice&) = delete;
    RenderDevice& operator=(const RenderDevice&) = delete;
};

} // namespace xray::render::ng
