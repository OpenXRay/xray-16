#pragma once

#include "ResourceHandle.h"
#include "PipelineState.h"
#include <nvrhi/nvrhi.h>

class IRenderBackend;

namespace xray::render::resources {
    class FGResourceManager;
}

namespace xray::render {
    class SlangCompiler;
}

namespace xray::render::ng {

class RenderContext;

class RenderDevice {
public:
    RenderDevice();
    ~RenderDevice();

    bool InitializeD3D11(ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    bool IsInitialized() const { return m_initialized; }

    nvrhi::IDevice* GetNVRHIDevice() const;

    IRenderBackend* GetBackend() const { return m_backend.get(); }

    nvrhi::ICommandList* GetImmediateCommandList() const;

    xray::render::SlangCompiler* GetSlangCompiler() const;

    void UploadTextureDataToNVRHI(
        nvrhi::ITexture* texture,
        u32 arraySlice,
        u32 mipLevel,
        const void* data,
        size_t dataSize
    );

    void UploadTextureDataToNVRHI(
        nvrhi::ITexture* texture,
        u32 arraySlice,
        u32 mipLevel,
        const void* data,
        size_t dataSize,
        u32 rowPitch,
        u32 slicePitch
    );

    struct TextureDesc {
        u32 width = 0;
        u32 height = 0;
        u32 depth = 1;
        u32 arraySize = 1;
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

        bool isRenderTarget = false;
        bool isDepthStencil = false;
        bool isUAV = false;
        bool generateMips = false;

        nvrhi::ResourceStates initialState = nvrhi::ResourceStates::Common;

        shared_str debugName;
    };

    TextureHandle CreateTexture(
        const TextureDesc& desc,
        const void* initialData = nullptr);

    TextureHandle CreateTextureFromD3D11(
        ID3D11Resource* d3d11Texture,
        const TextureDesc& desc);

    void DestroyTexture(TextureHandle handle);

    nvrhi::ITexture* GetNativeTexture(TextureHandle handle);
    const TextureDesc* GetTextureDesc(TextureHandle handle);
    bool IsTextureValid(TextureHandle handle) const;

    void UploadTextureData(
        TextureHandle handle,
        u32 arraySlice,
        u32 mipLevel,
        const void* data,
        size_t dataSize
    );

    struct TextureSliceData {
        u32 arraySlice;
        u32 mipLevel;
        const void* data;
        size_t dataSize;
        u32 rowPitch;
        u32 slicePitch;
    };

    void UploadTextureData(
        TextureHandle handle,
        const TextureSliceData* slices,
        u32 sliceCount
    );

    struct BufferDesc {
        u64 byteSize = 0;
        u32 structStride = 0;

        bool isConstantBuffer = false;
        bool isVertexBuffer = false;
        bool isIndexBuffer = false;
        bool isDrawIndirectArgs = false;
        bool isUAV = false;

        bool cpuRead = false;
        bool cpuWrite = false;

        bool isVolatile = false;
        u32 maxVersions = 0;

        shared_str debugName;
    };

    BufferHandle CreateBuffer(
        const BufferDesc& desc,
        const void* initialData = nullptr);

    void DestroyBuffer(BufferHandle handle);

    void UpdateBuffer(
        BufferHandle handle,
        const void* data,
        u64 size,
        u64 offset = 0);

    nvrhi::IBuffer* GetNativeBuffer(BufferHandle handle);
    const BufferDesc* GetBufferDesc(BufferHandle handle);
    bool IsBufferValid(BufferHandle handle) const;

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

        bool comparisonEnable = false;
        ComparisonFunc comparisonFunc = ComparisonFunc::Never;

        float borderColor[4] = {0, 0, 0, 0};

        shared_str debugName;
    };

    SamplerHandle CreateSampler(const SamplerDesc& desc);
    void DestroySampler(SamplerHandle handle);

    nvrhi::ISampler* GetNativeSampler(SamplerHandle handle);
    const SamplerDesc* GetSamplerDesc(SamplerHandle handle);
    bool IsSamplerValid(SamplerHandle handle) const;

    nvrhi::BindingLayoutHandle CreateBindingLayout(const nvrhi::BindingLayoutDesc& desc);

    nvrhi::BindingSetHandle CreateBindingSet(const nvrhi::BindingSetDesc& desc, nvrhi::IBindingLayout* layout);

    nvrhi::FramebufferHandle CreateFramebuffer(const nvrhi::FramebufferDesc& desc);

    nvrhi::ComputePipelineHandle CreateComputePipeline(
        nvrhi::IShader* computeShader,
        nvrhi::IBindingLayout* bindingLayout);

    nvrhi::ComputePipelineHandle CreateComputePipeline(const nvrhi::ComputePipelineDesc& desc);

    ShaderHandle CreateShader(
        ShaderStage stage,
        const void* bytecode,
        size_t bytecodeSize,
        const char* debugName);

    void DestroyShader(ShaderHandle handle);

    RCShader* GetShader(ShaderHandle handle);
    bool IsShaderValid(ShaderHandle handle) const;

    RenderContext* CreateContext();
    void DestroyContext(RenderContext* context);

    void ExecuteContext(RenderContext* context);

    PipelineStateCache* GetPipelineCache() { return m_pipelineCache.get(); }

    xray::render::resources::FGResourceManager* GetFGResourceManager() { return m_modernResourceManager.get(); }

    nvrhi::IDevice* GetNativeDevice() const;

    struct Statistics {
        u32 texturesAlive = 0;
        u32 buffersAlive = 0;
        u32 samplersAlive = 0;
        u32 shadersAlive = 0;
        u32 contextsAlive = 0;

        u64 textureMemory = 0;
        u64 bufferMemory = 0;

        u32 texturesCreated = 0;
        u32 buffersCreated = 0;
        u32 samplersCreated = 0;
        u32 shadersCreated = 0;
    };

    const Statistics& GetStatistics() const { return m_stats; }
    void ResetStatistics();

private:
    xr_unique_ptr<IRenderBackend> m_backend;

    xr_unique_ptr<PipelineStateCache> m_pipelineCache;

    xr_unique_ptr<xray::render::resources::FGResourceManager> m_modernResourceManager;

    xr_unique_ptr<xray::render::SlangCompiler> m_slangCompiler;

    nvrhi::CommandListHandle m_uploadCommandList;

    std::mutex m_uploadMutex;

    bool m_initialized = false;

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

    xr_vector<TextureInfo> m_textures;
    xr_vector<BufferInfo> m_buffers;
    xr_vector<SamplerInfo> m_samplers;
    xr_vector<ShaderInfo> m_shaders;

    xr_vector<u32> m_freeTextures;
    xr_vector<u32> m_freeBuffers;
    xr_vector<u32> m_freeSamplers;
    xr_vector<u32> m_freeShaders;

    Statistics m_stats;

    TextureHandle AllocateTextureHandle();
    BufferHandle AllocateBufferHandle();
    SamplerHandle AllocateSamplerHandle();
    ShaderHandle AllocateShaderHandle();

    void FreeTextureHandle(TextureHandle handle);
    void FreeBufferHandle(BufferHandle handle);
    void FreeSamplerHandle(SamplerHandle handle);
    void FreeShaderHandle(ShaderHandle handle);

    bool ValidateTextureHandle(TextureHandle handle) const;
    bool ValidateBufferHandle(BufferHandle handle) const;
    bool ValidateSamplerHandle(SamplerHandle handle) const;
    bool ValidateShaderHandle(ShaderHandle handle) const;

    static nvrhi::TextureDesc ConvertTextureDesc(const TextureDesc& desc);
    static nvrhi::BufferDesc ConvertBufferDesc(const BufferDesc& desc);
    static nvrhi::SamplerDesc ConvertSamplerDesc(const SamplerDesc& desc);
    static nvrhi::ShaderDesc ConvertShaderDesc(ShaderStage stage, const char* debugName);

    RenderDevice(const RenderDevice&) = delete;
    RenderDevice& operator=(const RenderDevice&) = delete;
};

}
