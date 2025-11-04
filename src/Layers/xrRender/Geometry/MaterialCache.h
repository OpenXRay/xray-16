// xrRender/Geometry/MaterialCache.h
#pragma once

#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"

// Forward declarations - must be in RENDER_NAMESPACE
// Note: RENDER_NAMESPACE is defined as render_r4 in preprocessor
namespace xray::render::RENDER_NAMESPACE {
    struct Shader;
    struct ShaderElement;
    struct SPass;
    class dxRender_Visual;
    struct SVS;
    struct SPS;
    class CTexture;  // For texture wrapper cache
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

    // Binding layouts and sets (PER-STAGE to handle VS/PS having different slots)
    // VS: b0=$Globals, b1=dynamic_transforms, b2=static_globals
    // PS: b0=dynamic_transforms, b1=static_globals
    // NVRHI requires separate layouts per stage when slots differ!
    nvrhi::BindingLayoutHandle vsBindingLayout;   // Vertex shader resources
    nvrhi::BindingLayoutHandle psBindingLayout;   // Pixel shader resources
    nvrhi::BindingSetHandle vsBindingSet;         // Cached VS binding set
    nvrhi::BindingSetHandle psBindingSet;         // Cached PS binding set

    // Extracted data for quick access
    struct TextureSlot {
        u32 slot;                    // Binding slot (t0, t1, t2, etc.)
        ng::TextureHandle handle;     // Wrapped texture
    };
    xr_vector<TextureSlot> textures;  // Textures with their binding slots
    u32 vertexStride = 0;
    nvrhi::Format indexFormat = nvrhi::Format::R16_UINT;

    // Constant buffers (extracted from shader reflection)
    // PER-STAGE to handle VS/PS having different CBs at same slot
    enum class ShaderStage : u8 {
        Vertex = 0,
        Pixel = 1,
        Geometry = 2,
        Hull = 3,
        Domain = 4,
        Compute = 5
    };

    struct ConstantBufferInfo {
        u32 slot;                    // Binding slot (b0, b1, b2, etc.)
        ShaderStage stage;           // Which shader stage (VS/PS/etc)
        nvrhi::BufferHandle nvrhiBuffer;  // NVRHI wrapped buffer
        u32 size;                    // Size in bytes
        bool isPerObject;            // True if $Globals CB
        shared_str name;             // CB name from reflection
        // NOTE: initialData removed - causes memory corruption
    };
    xr_vector<ConstantBufferInfo> constantBuffers;
    u32 perObjectCBSize = 0;  // Size of $Globals CB (for convenience)

    // Samplers (extracted from shader reflection + X-Ray state)
    // PER-STAGE to handle VS/PS having different samplers
    struct SamplerInfo {
        u32 slot;                    // Binding slot (s0, s1, s2, etc.)
        ShaderStage stage;           // Which shader stage (VS/PS/etc)
        shared_str name;             // Sampler name from reflection (e.g. "smp_base")
        nvrhi::SamplerHandle nvrhiSampler;  // NVRHI wrapped sampler
    };
    xr_vector<SamplerInfo> samplers;

    // Shader references (for debugging)
    SVS* vertexShader = nullptr;
    SPS* pixelShader = nullptr;
    SPass* pass = nullptr;  // Store pass for SRV extraction

    // ─── RT Bindings (Extracted from Shader - Week 15) ───
    framegraph::ShaderRTBindings rtBindings;

    // ─── Vertex Input Signature (Extracted from VS - shader-expected order!) ───
    framegraph::VertexInputSignature vsInputSignature;

    // ─── Phase (Determines which pass to use) ───
    framegraph::RenderPhase GetPhase() const {
        return rtBindings.phase;
    }

    // ─── Required Input RTs ───
    const xr_vector<framegraph::ShaderRTBindings::InputTexture>& GetInputTextures() const {
        return rtBindings.inputTextures;
    }

    // ─── Output RT Count ───
    u32 GetOutputRTCount() const {
        return (u32)rtBindings.outputRTs.size();
    }

    // ─── Dynamic RT Slot Mapping ───
    // Maps GBuffer RT semantics to shader output slots dynamically
    // based on shader reflection data

    // Get the SV_Target slot for a given RT semantic
    // Returns ~0u if this semantic is not written by the shader
    u32 GetSlotForSemantic(framegraph::ShaderRTBindings::RTSemantic semantic) const {
        for (const auto& output : rtBindings.outputRTs) {
            if (output.semantic == semantic) {
                return output.slot;
            }
        }
        return ~0u;  // Not written by this shader
    }

    // Check if shader writes to a specific semantic
    bool WritesSemantic(framegraph::ShaderRTBindings::RTSemantic semantic) const {
        return GetSlotForSemantic(semantic) != ~0u;
    }

    // Get all output slots sorted by slot index
    xr_vector<u32> GetOutputSlots() const {
        xr_vector<u32> slots;
        for (const auto& output : rtBindings.outputRTs) {
            slots.push_back(output.slot);
        }
        std::sort(slots.begin(), slots.end());
        return slots;
    }

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

    // Texture wrapper cache: Maps texture NAME to NVRHI TextureHandle
    // Prevents wrapping the same texture multiple times (massive leak!)
    // We use texture name (string) instead of CTexture* because X-Ray may recreate
    // CTexture objects at different addresses for the same logical texture
    xr_map<xr_string, ng::TextureHandle> m_textureWrapperCache;

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

public:
    // Create binding layouts for material (separate VS and PS)
    void CreateBindingLayouts(MaterialPSO* matPSO);
    nvrhi::BindingLayoutHandle CreateStageBindingLayout(
        const MaterialPSO* matPSO,
        MaterialPSO::ShaderStage stage,
        nvrhi::ShaderType nvrhiStage);

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
    void SetupVertexAttributes(dxRender_Visual* visual, MaterialPSO* matPSO, ng::PipelineStateDesc& psoDesc);
    void SetupRenderStates(SPass* pass, ng::PipelineStateDesc& psoDesc);
    void SetupRenderTargets(
        MaterialPSO* matPSO,  // Pass MaterialPSO for shader reflection data
        const passes::GBufferOutputs& outputs,
        const xray::render::framegraph::FrameGraph& fg,
        ng::PipelineStateDesc& psoDesc);
};

} // namespace xray::render
