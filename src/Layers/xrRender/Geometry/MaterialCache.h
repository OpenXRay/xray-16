// xrRender/Geometry/MaterialCache.h
#pragma once

#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/FrameGraph/GlobalParamsMapper.h"
#include "Layers/xrRender/ResourceManager/ResourceHandle.h"  // For TextureHandle definition

// Forward declarations - must be in RENDER_NAMESPACE
// Note: RENDER_NAMESPACE is defined as render_r4 in preprocessor
namespace xray::render::RENDER_NAMESPACE {
    struct Shader;
    struct ShaderElement;
    struct SPass;
    class dxRender_Visual;
    struct SVS;
    struct SPS;
    class CTexture;  // Legacy - will be replaced by FGResourceManager
    class dxFontRender;  // Font rendering system
}

// Forward declare FGResourceManager
namespace xray::render::resources {
    class FGResourceManager;
    class TextureManager;
}

namespace xray::render::framegraph {
    class FrameGraph;
    class VolatileConstantBufferPool;
}

namespace xray::render {

// Forward declarations
namespace fg {
    class PipelineState;
    struct PipelineStateDesc;
}

namespace framegraph {
    struct DefaultOutputLayout;  // Shared geometry pass output layout (defined in IPass.h)
}

// Bring RENDER_NAMESPACE types into scope for easier usage
using RENDER_NAMESPACE::Shader;
using RENDER_NAMESPACE::ShaderElement;
using RENDER_NAMESPACE::SPass;
using RENDER_NAMESPACE::dxRender_Visual;
using RENDER_NAMESPACE::SVS;
using RENDER_NAMESPACE::SPS;
using RENDER_NAMESPACE::dxFontRender;

// Pass types for different depth state requirements
enum class RenderPassType : u8 {
    DepthPrepass,      // depthFunc=Less, depthWrite=true
    ForwardColor,      // depthFunc=Equal, depthWrite=false (early-Z optimization)
    HUD,               // depthFunc=LessEqual, depthWrite=true (renders in front)
    UI,                // depthFunc disabled
    Default            // Use material's original depth state
};

// ══════════════════════════════════════════════════════════
//  PSO TYPE ENUM (Phase 2.4)
// ══════════════════════════════════════════════════════════

enum class PSOType : u8 {
    Material = 0,   // Regular material rendering (forward/deferred)
    Depth = 1,      // Depth-only prepass (Phase 2.4)
    UI = 2,         // UI rendering (HUD, menus)
    Shadow = 3,     // Shadow map rendering (Phase 4)
    PostProcess = 4 // Post-processing effects
};

// ══════════════════════════════════════════════════════════
//  MATERIAL KEY (FOR PSO CACHE LOOKUP)
// ══════════════════════════════════════════════════════════

struct MaterialKey {
    PSOType psoType;             // PSO type (material/depth/UI/shadow/postprocess)
    Shader* shader;              // Pointer to shader object
    u64 textureHash;             // Hash of texture combination
    u64 stateHash;               // Hash of render state

    // For specialized PSOs (UI, depth-only, etc.)
    u32 element;                 // Shader element index
    nvrhi::IFramebuffer* framebuffer;  // Framebuffer (needed for PSO creation)

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
        // Sort by PSO type first
        if (psoType != other.psoType) return psoType < other.psoType;

        // UI PSOs: Use textureHash (shader+texture combined) + element
        // NOTE: Don't include framebuffer for UI - it's recreated every frame but format is always the same
        if (psoType == PSOType::UI) {
            if (textureHash != other.textureHash) return textureHash < other.textureHash;
            return element < other.element;
        }

        // Depth PSOs: Use shader pointer + element + framebuffer
        if (psoType == PSOType::Depth) {
            if (shader != other.shader) return shader < other.shader;
            if (element != other.element) return element < other.element;
            return framebuffer < other.framebuffer;
        }

        // Regular material PSOs: Use shader pointer + texture/state hashes
        if (shader != other.shader) return shader < other.shader;
        if (textureHash != other.textureHash) return textureHash < other.textureHash;
        return stateHash < other.stateHash;
    }

    bool operator==(const MaterialKey& other) const {
        if (psoType != other.psoType) return false;

        // UI PSOs: Compare textureHash (shader+texture combined) + element
        // NOTE: Don't include framebuffer for UI - it's recreated every frame but format is always the same
        if (psoType == PSOType::UI) {
            return textureHash == other.textureHash &&
                   element == other.element;
        }

        // Depth PSOs: Compare shader pointer + element + framebuffer
        if (psoType == PSOType::Depth) {
            return shader == other.shader &&
                   element == other.element &&
                   framebuffer == other.framebuffer;
        }

        // Regular material PSOs: Compare shader pointer + texture/state hashes
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
    fg::PipelineState* pso = nullptr;

    // Binding layouts and sets (PER-STAGE to handle VS/PS having different slots)
    // VS: b0=$Globals, b1=dynamic_transforms, b2=static_globals
    // PS: b0=dynamic_transforms, b1=static_globals
    // NVRHI requires separate layouts per stage when slots differ!
    nvrhi::BindingLayoutHandle vsBindingLayout;   // Vertex shader resources
    nvrhi::BindingLayoutHandle psBindingLayout;   // Pixel shader resources
    nvrhi::BindingSetHandle vsBindingSet;         // Cached VS binding set
    nvrhi::BindingSetHandle psBindingSet;         // Cached PS binding set
    bool needsBindingSetRebuild = false;          // True if textures weren't ready when binding set was created

    // Extracted data for quick access
    struct TextureSlot {
        u32 slot;                          // Binding slot (t0, t1, t2, etc.)
        resources::TextureHandle handle;   // Native resource handle (from FGResourceManager)
    };
    xr_vector<TextureSlot> textures;  // Textures with their binding slots
    u32 vertexStride = 0;
    nvrhi::Format indexFormat = nvrhi::Format::R16_UINT;

    // Detail texture scale (from .thm metadata via CTextureDescrMngr)
    // Used to update dt_params in DynamicTransforms CB per-material
    float detail_scale = 1.0f;  // Default 1.0 if no detail texture

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
        shared_str name;             // CB name from reflection
        // NOTE: Only contains GLOBAL CBs (shared across draws)
        // Per-draw CBs are in vcbRequirements and managed by VCB pool
    };
    xr_vector<ConstantBufferInfo> constantBuffers;
    u32 perObjectCBSize = 0;  // Size of $Globals CB (for convenience)

    // VCB requirements (from shader reflection - for dynamic VCB pool selection)
    struct VCBRequirement {
        u32 slot;                     // CB slot (b0, b1, etc.)
        u32 size;                     // Required size in bytes
        shared_str name;              // CB name (e.g. "dynamic_transforms", "$Globals")
        fg::BufferHandle vcbHandle;   // VCB from pool (filled after registration)
    };
    xr_vector<VCBRequirement> vcbRequirements;

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

    // ─── GlobalParams_0 Mapper (Slang-wrapped loose uniforms) ───
    // Automatically populated from Slang reflection if shader has globalParams_0
    xr_unique_ptr<GlobalParamsMapper> globalParamsMapper;

    // ─── Constant Layout (Full per-constant metadata) ───
    framegraph::ShaderConstantLayout constantLayout;

    // Helper: Get constant metadata by name
    const framegraph::ShaderConstant* FindConstant(const char* name) const {
        return constantLayout.FindConstant(name);
    }

    // Debug name
    shared_str debugName;

    // ─── Bindless Material ID ───
    // Index into g_Materials buffer for GPU-driven rendering
    // UINT32_MAX = not registered with bindless system
    u32 bindlessMaterialID = UINT32_MAX;

    ~MaterialPSO() {
        // pso is managed by RenderDevice, don't delete
    }
};

// ══════════════════════════════════════════════════════════
//  MATERIAL CACHE (PSO MANAGER)
// ══════════════════════════════════════════════════════════
nvrhi::Format ConvertDxgiFormatToNvrhi(DXGI_FORMAT dxgiFormat);

class MaterialCache {
public:
    MaterialCache(
        fg::RenderDevice* device,
        resources::FGResourceManager* resourceManager,
        framegraph::VolatileConstantBufferPool* vcbPool = nullptr
    );
    ~MaterialCache();

    // Get or create PSO for a visual (geometry passes with DefaultOutputLayout)
    // Future: Add overloads for LightingOutputLayout, PostProcessOutputLayout, etc.
    MaterialPSO* GetOrCreatePSO(
        dxRender_Visual* visual,
        const framegraph::DefaultOutputLayout& outputs,
        const framegraph::FrameGraph& fg,
        RenderPassType passType = RenderPassType::ForwardColor);

    // Get or create depth-only PSO for depth prepass (Phase 2.4)
    // Optimized PSO with no color writes, minimal pixel shader
    MaterialPSO* GetOrCreateDepthPSO(
        dxRender_Visual* visual,
        const framegraph::FrameGraph& fg);

    // Get or create PSO for UI rendering (simplified - no visual required)
    // Uses fixed UI vertex layout (position, color, texcoord)
    // Takes IUIShader* (backend-agnostic) instead of Shader*
    MaterialPSO* GetOrCreateUIPSO(
        IUIShader* uiShader,
        u32 elementIndex,
        nvrhi::IFramebuffer* framebuffer);

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

    // Get VCB pool (for FGConstantSystem)
    framegraph::VolatileConstantBufferPool* GetVCBPool() const { return m_vcbPool; }

private:
    fg::RenderDevice* m_device;
    resources::FGResourceManager* m_resourceManager;  // NEW: Direct access to resource management
    framegraph::VolatileConstantBufferPool* m_vcbPool;  // VCB pool for dynamic CB management
    xr_map<MaterialKey, xr_unique_ptr<MaterialPSO>> m_cache;
    Stats m_stats;

    // Texture handle cache: Maps texture NAME to resources::TextureHandle
    // Uses FGResourceManager's TextureManager for native NVRHI textures (no wrapping!)
    xr_map<xr_string, resources::TextureHandle> m_textureHandleCache;

    // Detail scale cache: Maps texture name -> detail scale value (from .thm metadata)
    // Mirrors TextureDescrManager's m_detail_scalers for clean access
    // Cached to avoid repeated queries and avoid casting R_constant_setup*
    xr_map<xr_string, float> m_detailScaleCache;

    // Bindless material ID cache: Maps visual pointer -> material ID
    // Used for pre-registration during geometry collection
    xr_unordered_map<dxRender_Visual*, u32> m_visualToMaterialID;

    // Pending materials that need texture registration
    // Stores (materialID, visual) pairs for deferred bindless descriptor registration
    struct PendingMaterial {
        u32 materialID;
        dxRender_Visual* visual;
        shared_str textureName;  // For particles (visual is null)
    };
    xr_vector<PendingMaterial> m_pendingMaterials;

    // Cache for particle materials (texture name -> material ID)
    xr_map<shared_str, u32> m_particleTextureToMaterialID;

    // ═══════════════════════════════════════════════════════
    //  TERRAIN MATERIAL TRACKING
    // ═══════════════════════════════════════════════════════
    // Terrain uses separate TerrainMaterialBuffer (register t9)
    // with 4-layer detail blending via RGBA mask

    // Cache: shader name -> terrain material ID
    // Terrain materials are determined by shader (e.g., "levels\zaton_earth_2")
    // which defines the detail textures. Multiple visuals with same shader share one material.
    xr_unordered_map<shared_str, u32> m_shaderToTerrainMaterialID;

    // Pending terrain materials needing texture registration
    struct PendingTerrainMaterial {
        u32 terrainMaterialID;
        dxRender_Visual* visual;
    };
    xr_vector<PendingTerrainMaterial> m_pendingTerrainMaterials;

    // Default PBR texture (1x1 RGBA8 packed texture for fallback)
    // Used when no PBR texture is specified in .thm metadata
    // Format: R=metallic(0), G=roughness(255), B=ao(255), A=parallax(128)
    resources::TextureHandle m_defaultPBR;

    // Create default PBR textures
    void CreateDefaultPBRTextures();

    // Helper: Get detail scale for texture (queries TextureDescrManager and caches result)
    float GetDetailScale(const shared_str& textureName);

    // Create new PSO from shader element
    MaterialPSO* CreatePSO(
        dxRender_Visual* visual,
        ShaderElement* elem,
        SPass* pass,
        const framegraph::DefaultOutputLayout& outputs,
        const framegraph::FrameGraph& fg,
        RenderPassType passType);

    // Create depth-only PSO (Phase 2.4)
    MaterialPSO* CreateDepthPSO(
        dxRender_Visual* visual,
        ShaderElement* elem,
        SPass* pass,
        const framegraph::FrameGraph& fg);

    // Create UI PSO (no visual, fixed vertex layout)
    // Takes IUIShader* to access NVRHI handles directly
    MaterialPSO* CreateUIPSO(
        IUIShader* uiShader,
        ShaderElement* elem,
        SPass* pass,
        nvrhi::IFramebuffer* framebuffer);

    // Create Font PSO (similar to UI PSO but uses dxFontRender)
    MaterialPSO* CreateFontPSO(
        dxFontRender* fontRender,
        nvrhi::IFramebuffer* framebuffer);

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

    nvrhi::BindingLayoutHandle CreateCombinedBindingLayout(const MaterialPSO* matPSO);

    // Get or create cached binding set for material
    // Cached per MaterialPSO for reuse across draws - only creates once!
    // VCBs are queried directly from the pool
    nvrhi::BindingSetHandle GetOrCreateBindingSet(MaterialPSO* matPSO);

    // Register material with bindless system
    // Registers textures to D3D12 descriptor heap and creates material buffer entry
    // Returns material ID (stored in matPSO->bindlessMaterialID)
    u32 RegisterBindlessMaterial(MaterialPSO* matPSO);

    // Get NVRHI texture by name (for debug visualization)
    nvrhi::ITexture* GetNVRHITextureByName(const char* textureName);

    // Pre-register bindless material by visual (for geometry collection)
    // Creates material entry before PSO exists, returns material ID
    // Used to populate batch.bindlessMaterialID before GPU culling upload
    u32 PreRegisterBindlessMaterial(dxRender_Visual* visual);

    // Pre-register bindless material for particle effects (texture name from CPEDef)
    // Returns material ID for particle batch
    u32 PreRegisterParticleMaterial(const shared_str& textureName);

    // ═══════════════════════════════════════════════════════
    //  TERRAIN MATERIAL REGISTRATION (4-layer detail blending)
    // ═══════════════════════════════════════════════════════

    // Check if visual uses terrain blender (B_BmmD or B_LmBmmD)
    // Uses Blender CLASS_ID detection for reliability
    bool IsTerrainMaterial(dxRender_Visual* visual);

    // Pre-register terrain material - creates TerrainMaterialData entry
    // Returns terrain material ID for batch.terrainMaterialID
    u32 PreRegisterTerrainMaterial(dxRender_Visual* visual);

    // Finalize pending terrain materials (register textures to descriptor heap)
    // Call once per frame when RenderContext is available
    void FinalizePendingTerrainMaterials(fg::RenderContext* ctx);

    // Finalize pending materials (register textures to bindless descriptor heap)
    // Call once per frame when RenderContext is available
    // This registers textures to D3D12 descriptor heap for any materials registered this frame
    void FinalizePendingMaterials(fg::RenderContext* ctx);

    // Shader handle cache (shared across all materials to avoid recreating identical shaders)
    // Key format: "VS_<shadername>" or "PS_<shadername>" to distinguish stages
    xr_map<shared_str, nvrhi::ShaderHandle> m_shaderHandles;  // Direct NVRHI handles (no wrapper needed)

    // Get or create cached shader handle (stage-aware caching)
    // Returns direct NVRHI handles (no wrapper layer!)
    nvrhi::ShaderHandle GetOrCreateShaderVS(SVS* vs);
    nvrhi::ShaderHandle GetOrCreateShaderPS(SPS* ps);
    void SetupRenderStates(SPass* pass, fg::PipelineStateDesc& psoDesc);

    MaterialPSO* GetOrCreateFontPSO(
        dxFontRender* fontRender,
        nvrhi::IFramebuffer* framebuffer);

private:

    // Compute texture hash from SPass
    static u64 ComputeTextureHash(SPass* pass);

    // Compute state hash
    static u64 ComputeStateHash(SPass* pass);

    // D3D12: Extract vertex format ID from visual's geometry
    static u32 GetVertexFormatID(dxRender_Visual* visual);

    // Setup PSO descriptor helpers
    // Validate that geometry provides all vertex attributes the shader expects
    bool ValidateVertexLayoutCompatibility(dxRender_Visual* visual, MaterialPSO* matPSO);

    void SetupVertexAttributes(dxRender_Visual* visual, MaterialPSO* matPSO, fg::PipelineStateDesc& psoDesc);
    void SetupRenderTargets(
        MaterialPSO* matPSO,  // Pass MaterialPSO for shader reflection data
        const framegraph::DefaultOutputLayout& outputs,
        const framegraph::FrameGraph& fg,
        fg::PipelineStateDesc& psoDesc);
};

} // namespace xray::render
