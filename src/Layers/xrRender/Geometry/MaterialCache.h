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
    xr_vector<ng::TextureHandle> textures;  // Wrapped textures (using our abstraction)
    u32 vertexStride = 0;
    nvrhi::Format indexFormat = nvrhi::Format::R16_UINT;

    // Shader references (for debugging)
    SVS* vertexShader = nullptr;
    SPS* pixelShader = nullptr;

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

    // Create binding layout for material
    nvrhi::BindingLayoutHandle CreateBindingLayout(const MaterialPSO* matPSO);

    // Create binding set for material
    nvrhi::BindingSetHandle CreateBindingSet(
        const MaterialPSO* matPSO,
        nvrhi::IBuffer* perObjectCB);

    // Compute texture hash
    static u64 ComputeTextureHash(const xr_vector<ng::TextureHandle>& textures);

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
