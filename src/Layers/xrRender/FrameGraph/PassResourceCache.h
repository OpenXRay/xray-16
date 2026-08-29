#pragma once

#include <nvrhi/nvrhi.h>
#include "Common/Common.hpp"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::framegraph {

struct ExtractedReflection;

class PassResourceCache {
public:
    static PassResourceCache& Instance();

    nvrhi::SamplerHandle GetOrCreateSampler(
        const char* passName,
        const nvrhi::SamplerDesc& desc,
        nvrhi::IDevice* device);

    nvrhi::ISampler* GetAnisoWrapSampler(nvrhi::IDevice* device);
    nvrhi::ISampler* GetLinearWrapSampler(nvrhi::IDevice* device);
    nvrhi::ISampler* GetLinearClampSampler(nvrhi::IDevice* device);
    nvrhi::ISampler* GetPointClampSampler(nvrhi::IDevice* device);
    nvrhi::ISampler* GetShadowCmpSampler(nvrhi::IDevice* device);
    nvrhi::ISampler* GetSunShadowCmpSampler(nvrhi::IDevice* device);
    nvrhi::ITexture* GetDummyShadowMap(nvrhi::IDevice* device);
    nvrhi::ITexture* GetDummyShadowMap2D(nvrhi::IDevice* device);

    nvrhi::ISampler* GetSamplerByName(const char* smpName, nvrhi::IDevice* device);

    nvrhi::BindingLayoutHandle GetOrCreateBindingLayout(
        const char* passName,
        const nvrhi::BindingLayoutDesc& desc,
        nvrhi::IDevice* device);

    nvrhi::BindingLayoutHandle GetOrCreateBindingLayoutFromReflection(
        const char* passName,
        const ExtractedReflection& csReflection,
        nvrhi::IDevice* device);

    nvrhi::BindingLayoutHandle GetOrCreateBindingLayoutFromReflection(
        const char* passName,
        const ExtractedReflection& vsReflection,
        const ExtractedReflection& psReflection,
        nvrhi::IDevice* device);

    nvrhi::GraphicsPipelineHandle GetOrCreatePipeline(
        const char* passName,
        const nvrhi::GraphicsPipelineDesc& desc,
        const nvrhi::FramebufferInfo& fbInfo,
        nvrhi::IDevice* device);

    nvrhi::ComputePipelineHandle GetOrCreateComputePipeline(
        const char* passName,
        const nvrhi::ComputePipelineDesc& desc,
        nvrhi::IDevice* device);

    nvrhi::FramebufferHandle GetOrCreateFramebuffer(
        const char* passName,
        const nvrhi::FramebufferDesc& desc,
        nvrhi::IDevice* device);

    nvrhi::InputLayoutHandle GetOrCreateInputLayout(
        const char* passName,
        const nvrhi::VertexAttributeDesc* attrs,
        u32 attrCount,
        nvrhi::IShader* vertexShader,
        nvrhi::IDevice* device);

    nvrhi::BufferHandle GetOrCreateStaticBuffer(
        const char* passName,
        const char* bufferName,
        const nvrhi::BufferDesc& desc,
        nvrhi::IDevice* device);

    nvrhi::IBuffer* GetOrCreateVolatileCB(
        const char* passName,
        const char* bufferName,
        u32 byteSize,
        fg::RenderDevice* device,
        u32 maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS);

    bool HasStaticBuffer(const char* passName, const char* bufferName) const;

    nvrhi::BindingSetHandle GetOrCreateBindingSet(
        const nvrhi::BindingSetDesc& desc,
        nvrhi::IBindingLayout* layout,
        nvrhi::IDevice* device);

    void Clear();
    void ClearFramebufferDependent();

    struct Stats {
        u32 samplerHits = 0;
        u32 samplerMisses = 0;
        u32 layoutHits = 0;
        u32 layoutMisses = 0;
        u32 pipelineHits = 0;
        u32 pipelineMisses = 0;
        u32 computePipelineHits = 0;
        u32 computePipelineMisses = 0;
        u32 framebufferHits = 0;
        u32 framebufferMisses = 0;
        u32 inputLayoutHits = 0;
        u32 inputLayoutMisses = 0;
        u32 bufferHits = 0;
        u32 bufferMisses = 0;
        u32 bindingSetHits = 0;
        u32 bindingSetMisses = 0;
    };
    const Stats& GetStats() const { return m_stats; }
    void ResetStats();

private:
    PassResourceCache() = default;
    ~PassResourceCache() = default;

    PassResourceCache(const PassResourceCache&) = delete;
    PassResourceCache& operator=(const PassResourceCache&) = delete;

    xr_map<u64, nvrhi::SamplerHandle> m_samplers;
    xr_map<u64, nvrhi::BindingLayoutHandle> m_bindingLayouts;
    xr_map<u64, nvrhi::GraphicsPipelineHandle> m_graphicsPipelines;
    xr_map<u64, nvrhi::ComputePipelineHandle> m_computePipelines;
    xr_map<u64, nvrhi::FramebufferHandle> m_framebuffers;
    xr_map<u64, nvrhi::InputLayoutHandle> m_inputLayouts;
    xr_map<u64, nvrhi::BufferHandle> m_staticBuffers;
    xr_map<u64, fg::BufferHandle> m_volatileCBs;
    xr_map<u64, nvrhi::BindingSetHandle> m_bindingSets;

    nvrhi::SamplerHandle m_commonAnisoWrap;
    nvrhi::SamplerHandle m_commonLinearWrap;
    nvrhi::SamplerHandle m_commonLinearClamp;
    nvrhi::SamplerHandle m_commonPointClamp;
    nvrhi::SamplerHandle m_commonShadowCmp;
    nvrhi::SamplerHandle m_sunShadowCmp;
    nvrhi::TextureHandle m_dummyShadowMap;
    nvrhi::TextureHandle m_dummyShadowMap2D;

    Stats m_stats;

public:
    static u64 HashString(const char* str);
    static u64 HashCombine(u64 a, u64 b);
    static u64 HashPointer(const void* ptr);
};

PassResourceCache& GetPassResourceCache();

}
