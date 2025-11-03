#pragma once

#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"

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

    struct TextureSlot {
        u32 slot;
        ng::TextureHandle handle;
    };
    xr_vector<TextureSlot> textures;
    u32 vertexStride = 0;
    nvrhi::Format indexFormat = nvrhi::Format::R16_UINT;

    enum class ShaderStage : u8 {
        Vertex = 0,
        Pixel = 1,
        Geometry = 2,
        Hull = 3,
        Domain = 4,
        Compute = 5
    };

    struct ConstantBufferInfo {
        u32 slot;
        ShaderStage stage;
        nvrhi::BufferHandle nvrhiBuffer;
        u32 size;
        bool isPerObject;
        shared_str name;
    };
    xr_vector<ConstantBufferInfo> constantBuffers;
    u32 perObjectCBSize = 0;

    struct SamplerInfo {
        u32 slot;
        ShaderStage stage;
        shared_str name;
        nvrhi::SamplerHandle nvrhiSampler;
    };
    xr_vector<SamplerInfo> samplers;

    SVS* vertexShader = nullptr;
    SPS* pixelShader = nullptr;
    SPass* pass = nullptr;

    framegraph::ShaderRTBindings rtBindings;

    framegraph::RenderPhase GetPhase() const {
        return rtBindings.phase;
    }

    const xr_vector<framegraph::ShaderRTBindings::InputTexture>& GetInputTextures() const {
        return rtBindings.inputTextures;
    }

    u32 GetOutputRTCount() const {
        return (u32)rtBindings.outputRTs.size();
    }

    u32 GetSlotForSemantic(framegraph::ShaderRTBindings::RTSemantic semantic) const {
        for (const auto& output : rtBindings.outputRTs) {
            if (output.semantic == semantic) {
                return output.slot;
            }
        }
        return ~0u;
    }

    bool WritesSemantic(framegraph::ShaderRTBindings::RTSemantic semantic) const {
        return GetSlotForSemantic(semantic) != ~0u;
    }

    xr_vector<u32> GetOutputSlots() const {
        xr_vector<u32> slots;
        for (const auto& output : rtBindings.outputRTs) {
            slots.push_back(output.slot);
        }
        std::sort(slots.begin(), slots.end());
        return slots;
    }

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

    void ExtractSamplers(SPass* pass, MaterialPSO* matPSO);

    nvrhi::BindingLayoutHandle CreateBindingLayout(const MaterialPSO* matPSO);

public:
    nvrhi::BindingSetHandle CreateMaterialBindingSet(const MaterialPSO* matPSO);
    nvrhi::BindingSetHandle GetOrCreateBindingSet(
        MaterialPSO* matPSO,
        nvrhi::IBuffer* perObjectVCB,
        SPass* pass);
private:

    static u64 ComputeTextureHash(SPass* pass);

    static u64 ComputeStateHash(SPass* pass);

    void SetupVertexAttributes(dxRender_Visual* visual, ng::PipelineStateDesc& psoDesc);
    void SetupRenderStates(SPass* pass, ng::PipelineStateDesc& psoDesc);
    void SetupRenderTargets(
        MaterialPSO* matPSO,
        const passes::GBufferOutputs& outputs,
        const xray::render::framegraph::FrameGraph& fg,
        ng::PipelineStateDesc& psoDesc);
};

}
