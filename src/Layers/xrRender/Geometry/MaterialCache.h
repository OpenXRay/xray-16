// xrRender/Geometry/MaterialCache.h
#pragma once

#include "Layers/xrRender/RenderContext/RenderContext.h"

// Forward declarations - must be in RENDER_NAMESPACE
// Note: RENDER_NAMESPACE is defined as render_r4 in preprocessor
namespace xray::render::RENDER_NAMESPACE {
    struct Shader;
    struct ShaderElement;
    struct SPass;
    class dxRender_Visual;
    struct SVS;
    struct SPS;
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render {

// Forward declarations
namespace ng {
    class PipelineState;
    struct PipelineStateDesc;
}

namespace passes {
    struct GBufferOutputs;
}

// Bring RENDER_NAMESPACE types into scope for easier usage
using RENDER_NAMESPACE::Shader;
using RENDER_NAMESPACE::ShaderElement;
using RENDER_NAMESPACE::SPass;
using RENDER_NAMESPACE::dxRender_Visual;
using RENDER_NAMESPACE::SVS;
using RENDER_NAMESPACE::SPS;

// ══════════════════════════════════════════════════════════
//  MATERIAL KEY (FOR PSO CACHE LOOKUP)
// ══════════════════════════════════════════════════════════

struct MaterialKey {
    Shader* shader;              // Pointer to shader object
    u64 textureHash;             // Hash of texture combination
    u64 stateHash;               // Hash of render state

    MaterialKey()
        : shader(nullptr)
        , textureHash(0)
        , stateHash(0)
    {
    }

    MaterialKey(Shader* s, u64 texHash, u64 stHash)
        : shader(s)
        , textureHash(texHash)
        , stateHash(stHash)
    {
    }

    bool operator<(const MaterialKey& other) const {
        if (shader != other.shader) return shader < other.shader;
        if (textureHash != other.textureHash) return textureHash < other.textureHash;
        return stateHash < other.stateHash;
    }

    bool operator==(const MaterialKey& other) const {
        return shader == other.shader &&
               textureHash == other.textureHash &&
               stateHash == other.stateHash;
    }
};

// ══════════════════════════════════════════════════════════
//  MATERIAL PSO (CACHED PSO + BINDINGS)
// ══════════════════════════════════════════════════════════

struct MaterialPSO {
    // Graphics pipeline
    ng::PipelineState* pso = nullptr;

    // Binding layout and set
    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::BindingSetHandle bindingSet;

    // Extracted data for quick access
    struct TextureSlot {
        u32 slot;                    // Binding slot (t0, t1, t2, etc.)
        ng::TextureHandle handle;     // Wrapped texture
    };
    xr_vector<TextureSlot> textures;  // Textures with their binding slots
    u32 vertexStride = 0;
    nvrhi::Format indexFormat = nvrhi::Format::R16_UINT;

    // Constant buffers (extracted from shader reflection)
    struct ConstantBufferInfo {
        u32 slot;                    // Binding slot (b0, b1, b2, etc.)
        nvrhi::BufferHandle nvrhiBuffer;  // NVRHI wrapped buffer
        u32 size;                    // Size in bytes
        bool isPerObject;            // True if slot 0 (per-object CB)
        // NOTE: initialData removed - causes memory corruption
    };
    xr_vector<ConstantBufferInfo> constantBuffers;
    u32 perObjectCBSize = 0;  // Size of slot 0 CB (for convenience)

    // Samplers (extracted from X-Ray state)
    struct SamplerInfo {
        u32 slot;                    // Binding slot (s0, s1, s2, etc.)
        nvrhi::SamplerHandle nvrhiSampler;  // NVRHI wrapped sampler
    };
    xr_vector<SamplerInfo> samplers;

    // Shader references (for debugging)
    SVS* vertexShader = nullptr;
    SPS* pixelShader = nullptr;
    SPass* pass = nullptr;  // Store pass for SRV extraction

    // Debug name
    shared_str debugName;

    ~MaterialPSO() {
        // pso is managed by RenderDevice, don't delete
    }
};

// ══════════════════════════════════════════════════════════
//  MATERIAL CACHE (PSO MANAGER)
// ══════════════════════════════════════════════════════════

class MaterialCache {
public:
    MaterialCache(ng::RenderDevice* device);
    ~MaterialCache();

    // Get or create PSO for a visual
    MaterialPSO* GetOrCreatePSO(
        dxRender_Visual* visual,
        const passes::GBufferOutputs& outputs,
        const xray::render::framegraph::FrameGraph& fg);

    // Clear cache
    void Clear();

    // Statistics
    struct Stats {
        u32 numCachedPSOs = 0;
        u32 numCacheHits = 0;
        u32 numCacheMisses = 0;
        u32 totalPSOCreations = 0;
    };

    const Stats& GetStats() const { return m_stats; }
    void ResetStats() { m_stats.numCacheHits = 0; m_stats.numCacheMisses = 0; }

private:
    ng::RenderDevice* m_device;
    xr_map<MaterialKey, xr_unique_ptr<MaterialPSO>> m_cache;
    Stats m_stats;

    // Create new PSO from shader element
    MaterialPSO* CreatePSO(
        dxRender_Visual* visual,
        ShaderElement* elem,
        SPass* pass,
        const passes::GBufferOutputs& outputs,
        const xray::render::framegraph::FrameGraph& fg);

    // Extract textures from SPass
    void ExtractTextures(SPass* pass, MaterialPSO* matPSO);

    // Extract shader bytecode from SPass
    bool ExtractShaders(SPass* pass, MaterialPSO* matPSO);

    // Extract samplers from SPass state
    void ExtractSamplers(SPass* pass, MaterialPSO* matPSO);

    // Create binding layout for material
    nvrhi::BindingLayoutHandle CreateBindingLayout(const MaterialPSO* matPSO);

    // Create material binding set (textures only, no CB)
public:
    nvrhi::BindingSetHandle CreateMaterialBindingSet(const MaterialPSO* matPSO);
    // Get or create cached binding set for material (with per-object VCB)
    // Cached per MaterialPSO for reuse across draws - only creates once!
    nvrhi::BindingSetHandle GetOrCreateBindingSet(
        MaterialPSO* matPSO,  // Non-const to allow caching
        nvrhi::IBuffer* perObjectVCB,
        SPass* pass);  // For extracting X-Ray's SRVs
private:

    // Compute texture hash from SPass
    static u64 ComputeTextureHash(SPass* pass);

    // Compute state hash
    static u64 ComputeStateHash(SPass* pass);

    // Setup PSO descriptor helpers
    void SetupVertexAttributes(dxRender_Visual* visual, ng::PipelineStateDesc& psoDesc);
    void SetupRenderStates(SPass* pass, ng::PipelineStateDesc& psoDesc);
    void SetupRenderTargets(
        const passes::GBufferOutputs& outputs,
        const xray::render::framegraph::FrameGraph& fg,
        ng::PipelineStateDesc& psoDesc);
};

} // namespace xray::render
