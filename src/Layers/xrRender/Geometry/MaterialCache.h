#pragma once

#include "Layers/xrRender/RenderContext/RenderContext.h"

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

namespace ng {
    class PipelineState;
    struct PipelineStateDesc;
}

namespace passes {
    struct GBufferOutputs;
}

using RENDER_NAMESPACE::Shader;
using RENDER_NAMESPACE::ShaderElement;
using RENDER_NAMESPACE::SPass;
using RENDER_NAMESPACE::dxRender_Visual;
using RENDER_NAMESPACE::SVS;
using RENDER_NAMESPACE::SPS;

struct MaterialKey {
    Shader* shader;
    u64 textureHash;
    u64 stateHash;

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

struct MaterialPSO {
    ng::PipelineState* pso = nullptr;

    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::BindingSetHandle bindingSet;

    xr_vector<ng::TextureHandle> textures;
    u32 vertexStride = 0;
    nvrhi::Format indexFormat = nvrhi::Format::R16_UINT;

    SVS* vertexShader = nullptr;
    SPS* pixelShader = nullptr;

    shared_str debugName;

    ~MaterialPSO() {
    }
};

class MaterialCache {
public:
    MaterialCache(ng::RenderDevice* device);
    ~MaterialCache();

    MaterialPSO* GetOrCreatePSO(
        dxRender_Visual* visual,
        const passes::GBufferOutputs& outputs,
        const xray::render::framegraph::FrameGraph& fg);

    void Clear();

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

    MaterialPSO* CreatePSO(
        dxRender_Visual* visual,
        ShaderElement* elem,
        SPass* pass,
        const passes::GBufferOutputs& outputs,
        const xray::render::framegraph::FrameGraph& fg);

    void ExtractTextures(SPass* pass, MaterialPSO* matPSO);

    bool ExtractShaders(SPass* pass, MaterialPSO* matPSO);

    nvrhi::BindingLayoutHandle CreateBindingLayout(const MaterialPSO* matPSO);

    nvrhi::BindingSetHandle CreateBindingSet(
        const MaterialPSO* matPSO,
        nvrhi::IBuffer* perObjectCB);

    static u64 ComputeTextureHash(const xr_vector<ng::TextureHandle>& textures);

    static u64 ComputeStateHash(SPass* pass);

    void SetupVertexAttributes(dxRender_Visual* visual, ng::PipelineStateDesc& psoDesc);
    void SetupRenderStates(SPass* pass, ng::PipelineStateDesc& psoDesc);
    void SetupRenderTargets(
        const passes::GBufferOutputs& outputs,
        const xray::render::framegraph::FrameGraph& fg,
        ng::PipelineStateDesc& psoDesc);
};

}
