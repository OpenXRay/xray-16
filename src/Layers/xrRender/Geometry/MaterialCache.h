#pragma once

#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/PipelineState.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/ResourceManager/ResourceHandle.h"

namespace xray::render::fg {
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

namespace fg {
    class PipelineState;
    struct PipelineStateDesc;
}

namespace framegraph {
    struct DefaultOutputLayout;
}

using fg::Shader;
using fg::ShaderElement;
using fg::SPass;
using fg::dxRender_Visual;
using fg::SVS;
using fg::SPS;

enum class RenderPassType : u8 {
    DepthPrepass,
    ForwardColor,
    HUD,
    UI,
    Default
};


enum class PSOType : u8 {
    Material = 0,
    Depth = 1,
    UI = 2,
    Shadow = 3,
    PostProcess = 4
};


struct MaterialKey {
    PSOType psoType;
    Shader* shader;
    u64 textureHash;
    u64 stateHash;

    u32 element;
    nvrhi::IFramebuffer* framebuffer;

    MaterialKey()
        : psoType(PSOType::Material)
        , shader(nullptr)
        , textureHash(0)
        , stateHash(0)
        , element(0)
        , framebuffer(nullptr)
    {
    }

    MaterialKey(Shader* s, u64 texHash, u64 stHash, PSOType type = PSOType::Material)
        : psoType(type)
        , shader(s)
        , textureHash(texHash)
        , stateHash(stHash)
        , element(0)
        , framebuffer(nullptr)
    {
    }

    bool operator<(const MaterialKey& other) const {
        if (psoType != other.psoType) return psoType < other.psoType;

        if (psoType == PSOType::UI) {
            if (textureHash != other.textureHash) return textureHash < other.textureHash;
            return element < other.element;
        }

        if (psoType == PSOType::Depth) {
            if (shader != other.shader) return shader < other.shader;
            if (element != other.element) return element < other.element;
            return framebuffer < other.framebuffer;
        }

        if (shader != other.shader) return shader < other.shader;
        if (textureHash != other.textureHash) return textureHash < other.textureHash;
        return stateHash < other.stateHash;
    }

    bool operator==(const MaterialKey& other) const {
        if (psoType != other.psoType) return false;

        if (psoType == PSOType::UI) {
            return textureHash == other.textureHash &&
                   element == other.element;
        }

        if (psoType == PSOType::Depth) {
            return shader == other.shader &&
                   element == other.element &&
                   framebuffer == other.framebuffer;
        }

        return shader == other.shader &&
               textureHash == other.textureHash &&
               stateHash == other.stateHash;
    }
};


struct MaterialPSO {
    fg::PipelineState* pso = nullptr;

    nvrhi::BindingLayoutHandle vsBindingLayout;
    nvrhi::BindingLayoutHandle psBindingLayout;
    nvrhi::BindingSetHandle vsBindingSet;
    nvrhi::BindingSetHandle psBindingSet;
    bool needsBindingSetRebuild = false;

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
        shared_str name;
    };
    xr_vector<ConstantBufferInfo> constantBuffers;
    u32 perObjectCBSize = 0;

    struct VCBRequirement {
        u32 slot;
        u32 size;
        shared_str name;
        fg::BufferHandle vcbHandle;
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

    framegraph::ShaderConstantLayout constantLayout;

    const framegraph::ShaderConstant* FindConstant(const char* name) const {
        return constantLayout.FindConstant(name);
    }

    shared_str debugName;

    u32 bindlessMaterialID = UINT32_MAX;

    ~MaterialPSO() {
    }
};

class MaterialCache {
public:
    MaterialCache(
        fg::RenderDevice* device,
        resources::FGResourceManager* resourceManager,
        framegraph::VolatileConstantBufferPool* vcbPool = nullptr
    );
    ~MaterialCache();

    MaterialPSO* GetOrCreatePSO(
        dxRender_Visual* visual,
        const framegraph::DefaultOutputLayout& outputs,
        const framegraph::FrameGraph& fg,
        RenderPassType passType = RenderPassType::ForwardColor);

    MaterialPSO* GetOrCreateDepthPSO(
        dxRender_Visual* visual,
        const framegraph::FrameGraph& fg);

    MaterialPSO* GetOrCreateUIPSO(
        IUIShader* uiShader,
        u32 elementIndex,
        nvrhi::IFramebuffer* framebuffer,
        fg::PrimitiveTopology topology = fg::PrimitiveTopology::TriangleList);

    void Clear();

    struct Stats {
        u32 numCachedPSOs = 0;
        u32 numCacheHits = 0;
        u32 numCacheMisses = 0;
        u32 totalPSOCreations = 0;
    };

    const Stats& GetStats() const { return m_stats; }
    void ResetStats() { m_stats.numCacheHits = 0; m_stats.numCacheMisses = 0; }

    framegraph::VolatileConstantBufferPool* GetVCBPool() const { return m_vcbPool; }

private:
    fg::RenderDevice* m_device;
    resources::FGResourceManager* m_resourceManager;
    framegraph::VolatileConstantBufferPool* m_vcbPool;
    xr_map<MaterialKey, xr_unique_ptr<MaterialPSO>> m_cache;
    Stats m_stats;

    xr_map<xr_string, resources::TextureHandle> m_textureHandleCache;

    xr_map<xr_string, float> m_detailScaleCache;

    // Epoch for the intrusive visual material-ID stamps (FBasicVisual.h);
    // bumping invalidates every visual's cached bindless material ID in O(1)
    u32 m_visualMaterialEpoch = 1;

    // One material slot per distinct (shader, texture) identity; inserts only
    // for never-before-seen content, so it amortizes to zero per frame
    xr_map<std::pair<shared_str, shared_str>, u32> m_materialIDByNames;

    struct PendingMaterial {
        u32 materialID;
        dxRender_Visual* visual;
        shared_str textureName;
    };
    xr_vector<PendingMaterial> m_pendingMaterials;

    xr_map<shared_str, u32> m_particleTextureToMaterialID;


    xr_unordered_map<shared_str, u32> m_shaderToTerrainMaterialID;

    struct PendingTerrainMaterial {
        u32 terrainMaterialID;
        dxRender_Visual* visual;
    };
    xr_vector<PendingTerrainMaterial> m_pendingTerrainMaterials;

    resources::TextureHandle m_defaultPBR;

    void CreateDefaultPBRTextures();

    float GetDetailScale(const shared_str& textureName);

    MaterialPSO* CreateUIPSO(
        IUIShader* uiShader,
        ShaderElement* elem,
        SPass* pass,
        nvrhi::IFramebuffer* framebuffer,
        fg::PrimitiveTopology topology);

public:
    void CreateBindingLayouts(MaterialPSO* matPSO);
    nvrhi::BindingLayoutHandle CreateStageBindingLayout(
        const MaterialPSO* matPSO,
        MaterialPSO::ShaderStage stage,
        nvrhi::ShaderType nvrhiStage);

    nvrhi::BindingLayoutHandle CreateCombinedBindingLayout(const MaterialPSO* matPSO);

    nvrhi::BindingSetHandle GetOrCreateBindingSet(MaterialPSO* matPSO);

    u32 RegisterBindlessMaterial(MaterialPSO* matPSO);

    nvrhi::ITexture* GetNVRHITextureByName(const char* textureName);

    u32 PreRegisterBindlessMaterial(dxRender_Visual* visual);

    u32 PreRegisterParticleMaterial(const shared_str& textureName);


    bool IsTerrainMaterial(dxRender_Visual* visual);

    u32 PreRegisterTerrainMaterial(dxRender_Visual* visual);

    void FinalizePendingTerrainMaterials(fg::RenderContext* ctx);

    void FinalizePendingMaterials(fg::RenderContext* ctx);

    xr_map<shared_str, nvrhi::ShaderHandle> m_shaderHandles;

private:

    static u32 GetVertexFormatID(dxRender_Visual* visual);
};

}
