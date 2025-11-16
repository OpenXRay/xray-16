#pragma once

#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/FrameGraph/GlobalParamsMapper.h"
#include "Layers/xrRender/ResourceManager/ResourceHandle.h"

namespace xray::render::RENDER_NAMESPACE {
    struct Shader;
    struct ShaderElement;
    struct SPass;
    class dxRender_Visual;
    struct SVS;
    struct SPS;
    class CTexture;
}

namespace xray::render::resources {
    class FGResourceManager;
    class TextureManager;
}

namespace xray::render::framegraph {
    class FrameGraph;
    class VolatileConstantBufferPool;
}

namespace xray::render {

namespace ng {
    class PipelineState;
    struct PipelineStateDesc;
}

namespace framegraph {
    struct DefaultOutputLayout;
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

    u32 element;
    nvrhi::IFramebuffer* framebuffer;
    bool isUIPSO;

    MaterialKey()
        : shader(nullptr)
        , textureHash(0)
        , stateHash(0)
        , element(0)
        , framebuffer(nullptr)
        , isUIPSO(false)
    {
    }

    MaterialKey(Shader* s, u64 texHash, u64 stHash)
        : shader(s)
        , textureHash(texHash)
        , stateHash(stHash)
        , element(0)
        , framebuffer(nullptr)
        , isUIPSO(false)
    {
    }

    bool operator<(const MaterialKey& other) const {
        if (isUIPSO != other.isUIPSO) return isUIPSO < other.isUIPSO;
        if (shader != other.shader) return shader < other.shader;
        if (isUIPSO) {
            if (element != other.element) return element < other.element;
            return framebuffer < other.framebuffer;
        }
        if (textureHash != other.textureHash) return textureHash < other.textureHash;
        return stateHash < other.stateHash;
    }

    bool operator==(const MaterialKey& other) const {
        if (isUIPSO != other.isUIPSO) return false;
        if (shader != other.shader) return false;
        if (isUIPSO) {
            return element == other.element && framebuffer == other.framebuffer;
        }
        return textureHash == other.textureHash &&
               stateHash == other.stateHash;
    }
};

struct MaterialPSO {
    ng::PipelineState* pso = nullptr;

    nvrhi::BindingLayoutHandle vsBindingLayout;
    nvrhi::BindingLayoutHandle psBindingLayout;
    nvrhi::BindingSetHandle vsBindingSet;
    nvrhi::BindingSetHandle psBindingSet;

    struct TextureSlot {
        u32 slot;
        resources::TextureHandle handle;
    };
    xr_vector<TextureSlot> textures;
    u32 vertexStride = 0;
    nvrhi::Format indexFormat = nvrhi::Format::R16_UINT;

    float detail_scale = 1.0f;

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

    struct VCBRequirement {
        u32 slot;
        u32 size;
        shared_str name;
        ng::BufferHandle vcbHandle;
    };
    xr_vector<VCBRequirement> vcbRequirements;

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

    framegraph::VertexInputSignature vsInputSignature;

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

    xr_unique_ptr<GlobalParamsMapper> globalParamsMapper;

    framegraph::ShaderConstantLayout constantLayout;

    const framegraph::ShaderConstant* FindConstant(const char* name) const {
        return constantLayout.FindConstant(name);
    }

    shared_str debugName;

    ~MaterialPSO() {
    }
};

nvrhi::Format ConvertDxgiFormatToNvrhi(DXGI_FORMAT dxgiFormat);

class MaterialCache {
public:
    MaterialCache(
        ng::RenderDevice* device,
        resources::FGResourceManager* resourceManager,
        framegraph::VolatileConstantBufferPool* vcbPool = nullptr
    );
    ~MaterialCache();

    MaterialPSO* GetOrCreatePSO(
        dxRender_Visual* visual,
        const framegraph::DefaultOutputLayout& outputs,
        const framegraph::FrameGraph& fg);

    MaterialPSO* GetOrCreateUIPSO(
        Shader* shader,
        u32 elementIndex,
        nvrhi::IFramebuffer* framebuffer);

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
    resources::FGResourceManager* m_resourceManager;
    framegraph::VolatileConstantBufferPool* m_vcbPool;
    xr_map<MaterialKey, xr_unique_ptr<MaterialPSO>> m_cache;
    Stats m_stats;

    xr_map<xr_string, resources::TextureHandle> m_textureHandleCache;

    xr_map<xr_string, float> m_detailScaleCache;

    float GetDetailScale(const shared_str& textureName);

    MaterialPSO* CreatePSO(
        dxRender_Visual* visual,
        ShaderElement* elem,
        SPass* pass,
        const framegraph::DefaultOutputLayout& outputs,
        const framegraph::FrameGraph& fg);

    MaterialPSO* CreateUIPSO(
        Shader* shader,
        ShaderElement* elem,
        SPass* pass,
        nvrhi::IFramebuffer* framebuffer);

    void ExtractTextures(SPass* pass, MaterialPSO* matPSO);

    bool ExtractShaders(SPass* pass, MaterialPSO* matPSO);

    void ExtractSamplers(SPass* pass, MaterialPSO* matPSO);

public:
    void CreateBindingLayouts(MaterialPSO* matPSO);
    nvrhi::BindingLayoutHandle CreateStageBindingLayout(
        const MaterialPSO* matPSO,
        MaterialPSO::ShaderStage stage,
        nvrhi::ShaderType nvrhiStage);

    nvrhi::BindingSetHandle GetOrCreateBindingSet(
        MaterialPSO* matPSO,
        nvrhi::IBuffer* perObjectVCB,
        SPass* pass);

    xr_map<shared_str, nvrhi::ShaderHandle> m_shaderHandles;

    nvrhi::ShaderHandle GetOrCreateShaderVS(SVS* vs);
    nvrhi::ShaderHandle GetOrCreateShaderPS(SPS* ps);
    void SetupRenderStates(SPass* pass, ng::PipelineStateDesc& psoDesc);

private:

    static u64 ComputeTextureHash(SPass* pass);

    static u64 ComputeStateHash(SPass* pass);

    bool ValidateVertexLayoutCompatibility(dxRender_Visual* visual, MaterialPSO* matPSO);

    void SetupVertexAttributes(dxRender_Visual* visual, MaterialPSO* matPSO, ng::PipelineStateDesc& psoDesc);
    void SetupRenderTargets(
        MaterialPSO* matPSO,
        const framegraph::DefaultOutputLayout& outputs,
        const framegraph::FrameGraph& fg,
        ng::PipelineStateDesc& psoDesc);
};

}
