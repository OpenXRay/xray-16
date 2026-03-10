// PassResourceCache.h
// Centralized cache for FrameGraph pass resources (samplers, layouts, pipelines, etc.)
// Prevents descriptor heap exhaustion by reusing immutable resources across frames
#pragma once

#include <nvrhi/nvrhi.h>
#include "Common/Common.hpp"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::framegraph {

struct ExtractedReflection;

// ═══════════════════════════════════════════════════════════════════════════════
//  PASS RESOURCE CACHE
// ═══════════════════════════════════════════════════════════════════════════════
// Singleton cache for all FrameGraph pass resources. Caches:
// - Samplers (immutable, keyed by pass name)
// - Binding layouts (immutable, keyed by pass name)
// - Graphics pipelines (keyed by pass name + framebuffer format)
// - Compute pipelines (keyed by pass name)
// - Framebuffers (keyed by render target pointers)
// - Input layouts (keyed by pass name + vertex shader)
// - Static buffers (keyed by pass name + buffer name)
//
// Usage:
//   auto& cache = GetPassResourceCache();
//   auto sampler = cache.GetOrCreateSampler("SkyPass", samplerDesc, device);

class PassResourceCache {
public:
    static PassResourceCache& Instance();

    // ═══════════════════════════════════════════════════════
    //  SAMPLER CACHE
    // ═══════════════════════════════════════════════════════
    // Samplers are immutable - cache by pass name (one sampler per pass)
    // If you need multiple samplers per pass, append a suffix to passName
    nvrhi::SamplerHandle GetOrCreateSampler(
        const char* passName,
        const nvrhi::SamplerDesc& desc,
        nvrhi::IDevice* device);

    // ═══════════════════════════════════════════════════════
    //  COMMON SAMPLERS (shared across all passes)
    // ═══════════════════════════════════════════════════════
    nvrhi::ISampler* GetAnisoWrapSampler(nvrhi::IDevice* device);
    nvrhi::ISampler* GetLinearWrapSampler(nvrhi::IDevice* device);
    nvrhi::ISampler* GetLinearClampSampler(nvrhi::IDevice* device);
    nvrhi::ISampler* GetPointClampSampler(nvrhi::IDevice* device);
    nvrhi::ISampler* GetShadowCmpSampler(nvrhi::IDevice* device);
    nvrhi::ITexture* GetDummyShadowMap(nvrhi::IDevice* device);
    nvrhi::ITexture* GetDummyShadowMap2D(nvrhi::IDevice* device);

    nvrhi::ISampler* GetSamplerByName(const char* smpName, nvrhi::IDevice* device);

    // ═══════════════════════════════════════════════════════
    //  BINDING LAYOUT CACHE
    // ═══════════════════════════════════════════════════════
    // Layouts are immutable - cache by pass name
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

    // ═══════════════════════════════════════════════════════
    //  GRAPHICS PIPELINE CACHE
    // ═══════════════════════════════════════════════════════
    // Pipelines depend on framebuffer format
    nvrhi::GraphicsPipelineHandle GetOrCreatePipeline(
        const char* passName,
        const nvrhi::GraphicsPipelineDesc& desc,
        const nvrhi::FramebufferInfo& fbInfo,
        nvrhi::IDevice* device);

    // ═══════════════════════════════════════════════════════
    //  COMPUTE PIPELINE CACHE
    // ═══════════════════════════════════════════════════════
    nvrhi::ComputePipelineHandle GetOrCreateComputePipeline(
        const char* passName,
        const nvrhi::ComputePipelineDesc& desc,
        nvrhi::IDevice* device);

    // ═══════════════════════════════════════════════════════
    //  FRAMEBUFFER CACHE
    // ═══════════════════════════════════════════════════════
    // Framebuffers keyed by render target pointer combination
    // This allows reuse when same RTs are used in different frames
    nvrhi::FramebufferHandle GetOrCreateFramebuffer(
        const char* passName,
        const nvrhi::FramebufferDesc& desc,
        nvrhi::IDevice* device);

    // ═══════════════════════════════════════════════════════
    //  INPUT LAYOUT CACHE
    // ═══════════════════════════════════════════════════════
    nvrhi::InputLayoutHandle GetOrCreateInputLayout(
        const char* passName,
        const nvrhi::VertexAttributeDesc* attrs,
        u32 attrCount,
        nvrhi::IShader* vertexShader,
        nvrhi::IDevice* device);

    // ═══════════════════════════════════════════════════════
    //  BUFFER CACHE
    // ═══════════════════════════════════════════════════════
    // For index buffers, vertex buffers, structured buffers
    // bufferName distinguishes multiple buffers within same pass
    nvrhi::BufferHandle GetOrCreateStaticBuffer(
        const char* passName,
        const char* bufferName,
        const nvrhi::BufferDesc& desc,
        nvrhi::IDevice* device);

    nvrhi::IBuffer* GetOrCreateVolatileCB(
        const char* passName,
        const char* bufferName,
        u32 byteSize,
        ng::RenderDevice* device,
        u32 maxVersions = ng::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS);

    bool HasStaticBuffer(const char* passName, const char* bufferName) const;

    // ═══════════════════════════════════════════════════════
    //  BINDING SET CACHE
    // ═══════════════════════════════════════════════════════
    // Binding sets are immutable in NVRHI - cache by hashing resource pointers + layout.
    // VCB handles are stable (NVRHI resolves versions at bind-time), so binding sets
    // referencing VCBs are cacheable. Only transient FG textures invalidate per-frame.
    nvrhi::BindingSetHandle GetOrCreateBindingSet(
        const nvrhi::BindingSetDesc& desc,
        nvrhi::IBindingLayout* layout,
        nvrhi::IDevice* device);

    // ═══════════════════════════════════════════════════════
    //  LIFECYCLE
    // ═══════════════════════════════════════════════════════
    void Clear();  // Clear all caches (on shutdown or device reset)

    // Statistics
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

    // Prevent copying
    PassResourceCache(const PassResourceCache&) = delete;
    PassResourceCache& operator=(const PassResourceCache&) = delete;

    // Cache storage - keyed by u64 hash
    xr_map<u64, nvrhi::SamplerHandle> m_samplers;
    xr_map<u64, nvrhi::BindingLayoutHandle> m_bindingLayouts;
    xr_map<u64, nvrhi::GraphicsPipelineHandle> m_graphicsPipelines;
    xr_map<u64, nvrhi::ComputePipelineHandle> m_computePipelines;
    xr_map<u64, nvrhi::FramebufferHandle> m_framebuffers;
    xr_map<u64, nvrhi::InputLayoutHandle> m_inputLayouts;
    xr_map<u64, nvrhi::BufferHandle> m_staticBuffers;
    xr_map<u64, ng::BufferHandle> m_volatileCBs;
    xr_map<u64, nvrhi::BindingSetHandle> m_bindingSets;

    Stats m_stats;

public:
    static u64 HashString(const char* str);
    static u64 HashCombine(u64 a, u64 b);
    static u64 HashPointer(const void* ptr);
};

// Global accessor
PassResourceCache& GetPassResourceCache();

} // namespace xray::render::framegraph
