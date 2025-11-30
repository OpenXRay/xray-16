// xrRender/Geometry/MaterialCache.cpp
#include "stdafx.h"
#include "MaterialCache.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/SH_Texture.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/FVisual.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/FProgressive.h"
#include "Layers/xrRender/FTreeVisual.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/SH_Atomic.h"
#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/RenderContext/PipelineState.h"
#include "Layers/xrRender/RenderContext/RCShader.h"
#include "Layers/xrRender/RenderContext/RenderStateConversion.h"  // State conversion helpers
#include "Layers/xrRender/RenderContext/RenderDevice.h"  // For RenderDevice definition
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"  // For ShaderConstant, ExtractedReflection
#include "Layers/xrRender/FrameGraph/ShaderCache.h"  // For ExtractedReflection definition
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"  // For VCB pool
#include "Layers/xrRender/FrameGraph/FrameGraph.h"  // For FrameGraph definition
#include "Layers/xrRender/FrameGraph/IPass.h"  // For DefaultOutputLayout
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"  // For PBR texture slot constants

#if defined(USE_DX11)
#include "Layers/xrRenderDX11/StateManager/dx11State.h"
#include "Layers/xrRenderDX11/StateManager/dx11SamplerStateCache.h"  // For sampler extraction
#include "Layers/xrRenderDX11/dx11ConstantBuffer.h"  // For CB size extraction
#include "../Externals/nvrhi/src/common/dxgi-format.h"  // For DXGI <-> NVRHI format conversion
#include "../Externals/nvrhi/src/d3d11/d3d11-backend.h"  // For D3D11 BindingSet access
#endif

namespace xray::render {

using namespace xray::render::RENDER_NAMESPACE;  // For Shader types (STextureList, etc.)


// ══════════════════════════════════════════════════════════
//  FORMAT CONVERSION HELPER
// ══════════════════════════════════════════════════════════

// Convert DXGI_FORMAT to nvrhi::Format for textures
// IMPORTANT: Do NOT use static_cast! DXGI_FORMAT and nvrhi::Format have different enum values!
// We need to search NVRHI's format mapping table to find the correct conversion.
nvrhi::Format ConvertDxgiFormatToNvrhi(DXGI_FORMAT dxgiFormat) {
    // Search through all NVRHI formats to find one that matches this DXGI format
    for (uint32_t i = 0; i < uint32_t(nvrhi::Format::COUNT); i++) {
        nvrhi::Format nvrhiFormat = static_cast<nvrhi::Format>(i);
        const nvrhi::DxgiFormatMapping& mapping = nvrhi::getDxgiFormatMapping(nvrhiFormat);

        // Check if any of the DXGI formats in the mapping match our input
        if (mapping.resourceFormat == dxgiFormat ||
            mapping.srvFormat == dxgiFormat ||
            mapping.rtvFormat == dxgiFormat) {
            return nvrhiFormat;
        }
    }

    // Not found - return UNKNOWN
    return nvrhi::Format::UNKNOWN;
}

// Deleted ConvertVertexFormat - use ConvertDxgiFormatToNvrhi instead

// ══════════════════════════════════════════════════════════
//  DEBUG LOGGING FOR CONSTANT LAYOUT
// ══════════════════════════════════════════════════════════

#ifdef DEBUG
static void LogConstantLayout(const framegraph::ShaderConstantLayout& layout, const char* shaderName) {
    using namespace framegraph;
    return;

    for (const auto& constant : layout.constants) {
        if (constant.cbIndex >= layout.constantBuffers.buffers.size()) {
            Msg("    - %s: ERROR - invalid cbIndex %u", constant.name.c_str(), constant.cbIndex);
            continue;
        }
    }
}
#endif


framegraph::ShaderConstantLayout MergeConstantLayouts(
    const framegraph::ShaderConstantLayout& vsLayout,
    const framegraph::ShaderConstantLayout& psLayout)
{
    using namespace framegraph;

    ShaderConstantLayout merged;

    // ═══════════════════════════════════════════════════════
    // STEP 1: Deduplicate Constant Buffers by Name
    // ═══════════════════════════════════════════════════════

    struct CBDeduplicationInfo {
        shared_str name;
        u32 vsSlot = UINT32_MAX;  // UINT32_MAX = not used in this stage
        u32 psSlot = UINT32_MAX;
        u32 size = 0;  // Max size across stages
        u16 mergedIndex = 0;  // Index in merged.constantBuffers array
    };

    xr_map<shared_str, CBDeduplicationInfo> uniqueCBs;

    // Add VS constant buffers
    for (const auto& cb : vsLayout.constantBuffers.buffers) {
        auto& unique = uniqueCBs[cb.name];
        unique.name = cb.name;
        unique.vsSlot = cb.slot;
        unique.size = std::max(unique.size, cb.size);
    }

    // Add PS constant buffers (merge with VS if name matches)
    for (const auto& cb : psLayout.constantBuffers.buffers) {
        auto& unique = uniqueCBs[cb.name];
        unique.name = cb.name;
        unique.psSlot = cb.slot;
        unique.size = std::max(unique.size, cb.size);  // Take max size
    }

    // Build merged CB list and assign indices
    u16 mergedIndex = 0;
    for (auto& [cbName, cbInfo] : uniqueCBs) {
        cbInfo.mergedIndex = mergedIndex++;

        ConstantBufferInfo mergedCB;
        mergedCB.name = cbInfo.name;
        mergedCB.size = cbInfo.size;
        // Use VS slot if available, otherwise PS slot
        mergedCB.slot = (cbInfo.vsSlot != UINT32_MAX) ? cbInfo.vsSlot : cbInfo.psSlot;

        merged.constantBuffers.buffers.push_back(mergedCB);
    }

    // ═══════════════════════════════════════════════════════
    // STEP 2: Build VS CB Name -> Merged Index Mapping
    // ═══════════════════════════════════════════════════════

    xr_map<u16, u16> vsIndexToMerged;  // Old VS cbIndex -> New merged cbIndex

    for (u16 vsIdx = 0; vsIdx < vsLayout.constantBuffers.buffers.size(); ++vsIdx) {
        const auto& vsCB = vsLayout.constantBuffers.buffers[vsIdx];

        // Find this CB in uniqueCBs to get merged index
        auto it = uniqueCBs.find(vsCB.name);
        if (it != uniqueCBs.end()) {
            vsIndexToMerged[vsIdx] = it->second.mergedIndex;
        }
    }

    // ═══════════════════════════════════════════════════════
    // STEP 3: Build PS CB Name -> Merged Index Mapping
    // ═══════════════════════════════════════════════════════

    xr_map<u16, u16> psIndexToMerged;  // Old PS cbIndex -> New merged cbIndex

    for (u16 psIdx = 0; psIdx < psLayout.constantBuffers.buffers.size(); ++psIdx) {
        const auto& psCB = psLayout.constantBuffers.buffers[psIdx];

        // Find this CB in uniqueCBs to get merged index
        auto it = uniqueCBs.find(psCB.name);
        if (it != uniqueCBs.end()) {
            psIndexToMerged[psIdx] = it->second.mergedIndex;
        }
    }

    // ═══════════════════════════════════════════════════════
    // STEP 4: Add VS Constants (with remapped cbIndex)
    // ═══════════════════════════════════════════════════════

    for (const auto& vsConstant : vsLayout.constants) {
        ShaderConstant mergedConstant = vsConstant;

        // Remap cbIndex to merged index
        auto it = vsIndexToMerged.find(vsConstant.cbIndex);
        if (it != vsIndexToMerged.end()) {
            mergedConstant.cbIndex = it->second;
        }
        else {
            Msg("! [MergeConstantLayouts] WARNING: VS constant '%s' has unmapped cbIndex %u",
                vsConstant.name.c_str(), vsConstant.cbIndex);
        }

        merged.constants.push_back(mergedConstant);
    }

    // ═══════════════════════════════════════════════════════
    // STEP 5: Add PS Constants (with remapped cbIndex, skip duplicates)
    // ═══════════════════════════════════════════════════════

    for (const auto& psConstant : psLayout.constants) {
        // Check if constant already exists from VS
        bool isDuplicate = false;
        for (const auto& existingConstant : merged.constants) {
            if (existingConstant.name == psConstant.name) {
                isDuplicate = true;

                // Validate that duplicate has same metadata
                if (existingConstant.offset != psConstant.offset ||
                    existingConstant.size != psConstant.size) {
                    Msg("! [MergeConstantLayouts] WARNING: Constant '%s' differs between VS and PS",
                        psConstant.name.c_str());
                    Msg("    VS: offset=%u, size=%u", existingConstant.offset, existingConstant.size);
                    Msg("    PS: offset=%u, size=%u", psConstant.offset, psConstant.size);
                }

                break;
            }
        }

        if (isDuplicate) {
            continue;  // Skip - already added from VS
        }

        // Add PS-only constant with remapped cbIndex
        ShaderConstant mergedConstant = psConstant;

        auto it = psIndexToMerged.find(psConstant.cbIndex);
        if (it != psIndexToMerged.end()) {
            mergedConstant.cbIndex = it->second;
        }
        else {
            Msg("! [MergeConstantLayouts] WARNING: PS constant '%s' has unmapped cbIndex %u",
                psConstant.name.c_str(), psConstant.cbIndex);
        }

        merged.constants.push_back(mergedConstant);
    }

    return merged;
}

// ══════════════════════════════════════════════════════════
//  CONSTRUCTOR / DESTRUCTOR
// ══════════════════════════════════════════════════════════

MaterialCache::MaterialCache(
    ng::RenderDevice* device,
    resources::FGResourceManager* resourceManager,
    framegraph::VolatileConstantBufferPool* vcbPool)
    : m_device(device)
    , m_resourceManager(resourceManager)
    , m_vcbPool(vcbPool)
{
    VERIFY(m_device);
    VERIFY(m_resourceManager);
    // VCB pool is optional - can be null for legacy code

    // Create default PBR textures for materials without explicit PBR maps
    CreateDefaultPBRTextures();
}

void MaterialCache::CreateDefaultPBRTextures()
{
    resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
    if (!texManager) {
        Msg("! [MaterialCache] TextureManager not available - skipping default PBR textures");
        return;
    }

    // Create 1x1 default textures with appropriate values:
    // - Metallic: 0 (black) = non-metallic/dielectric
    // - Roughness: 1 (white) = fully rough (safer default)
    // - AO: 1 (white) = no occlusion

    resources::TextureDesc desc;
    desc.type = resources::TextureDesc::Texture2D;
    desc.width = 1;
    desc.height = 1;
    desc.mipLevels = 1;
    desc.format = nvrhi::Format::R8_UNORM;  // Single channel for PBR maps

    // Metallic: black (0)
    desc.debugName = "$default_metallic";
    u8 blackPixel = 0;
    m_defaultMetallic = texManager->CreateTexture(desc, &blackPixel);
    if (m_defaultMetallic.IsValid()) {
        m_textureHandleCache["$default_metallic"] = m_defaultMetallic;
    }

    // Roughness: white (255 = 1.0)
    desc.debugName = "$default_roughness";
    u8 whitePixel = 255;
    m_defaultRoughness = texManager->CreateTexture(desc, &whitePixel);
    if (m_defaultRoughness.IsValid()) {
        m_textureHandleCache["$default_roughness"] = m_defaultRoughness;
    }

    // AO: white (255 = 1.0)
    desc.debugName = "$default_ao";
    m_defaultAO = texManager->CreateTexture(desc, &whitePixel);
    if (m_defaultAO.IsValid()) {
        m_textureHandleCache["$default_ao"] = m_defaultAO;
    }

    // Parallax: gray (128 = 0.5 = neutral height, no displacement)
    desc.debugName = "$default_parallax";
    u8 grayPixel = 128;
    m_defaultParallax = texManager->CreateTexture(desc, &grayPixel);
    if (m_defaultParallax.IsValid()) {
        m_textureHandleCache["$default_parallax"] = m_defaultParallax;
    }

    Msg("* [MaterialCache] Created default PBR textures");
}

MaterialCache::~MaterialCache() {
    Clear();
}

// ══════════════════════════════════════════════════════════
//  GET OR CREATE PSO
// ══════════════════════════════════════════════════════════

MaterialPSO* MaterialCache::GetOrCreatePSO(
    dxRender_Visual* visual,
    const framegraph::DefaultOutputLayout& outputs,
    const framegraph::FrameGraph& fg,
    RenderPassType passType)
{
    if (!visual) {
        Msg("! [MaterialCache::GetOrCreatePSO] Visual is NULL");
        return nullptr;
    }

    // Get or compile shader
    Shader* shader = nullptr;

    // Check if visual has pre-compiled shader (legacy path or header-based shaders)
    if (visual->shader && visual->shader._get()) {
        shader = visual->shader._get();
    }
    // Otherwise, compile from stored names (FrameGraph deferred compilation)
    else if (visual->shaderName.c_str() && visual->shaderName.size() > 0 &&
             visual->textureName.c_str() && visual->textureName.size() > 0) {

        // Compile shader now (on first use)
        // Note: This may fail if blender tries to compile broken tessellation shaders
        visual->shader.create(visual->shaderName.c_str(), visual->textureName.c_str());
        shader = visual->shader._get();

        if (!shader) {
            return nullptr;
        }
    } else {
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  EXTRACT SHADER ELEMENT (E[0] = DEFERRED RENDERING)
    // ═══════════════════════════════════════════════════════

    // E[0] = SE_R2_NORMAL_HQ (deferred rendering mode)
    ShaderElement* elem = shader->E[0]._get();
    if (!elem) {
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  EXTRACT FIRST PASS (GBUFFER PASS)
    // ═══════════════════════════════════════════════════════

    if (elem->passes.empty()) {
        return nullptr;
    }

    SPass* pass = elem->passes[0]._get();
    if (!pass) {
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  COMPUTE MATERIAL KEY (includes pass type for different depth states)
    // ═══════════════════════════════════════════════════════

    // Compute hash based on X-Ray texture pointers (stable across frames)
    u64 textureHash = ComputeTextureHash(pass);
    u64 stateHash = ComputeStateHash(pass);

    // Include pass type in state hash so different passes get different PSOs
    stateHash ^= (static_cast<u64>(passType) << 56);

    MaterialKey key(shader, textureHash, stateHash);

    // Get shader name for logging
    const char* shaderName = "unknown";
    if (pass->vs._get() && pass->vs._get()->cName.c_str()) {
        shaderName = pass->vs._get()->cName.c_str();
    }

    // ═══════════════════════════════════════════════════════
    //  CHECK CACHE
    // ═══════════════════════════════════════════════════════

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_stats.numCacheHits++;
        return it->second.get();
    }

    // ═══════════════════════════════════════════════════════
    //  CACHE MISS - CREATE NEW PSO
    // ═══════════════════════════════════════════════════════

    m_stats.numCacheMisses++;


    MaterialPSO* pso = CreatePSO(visual, elem, pass, outputs, fg, passType);
    if (!pso) {
        Msg("! [MaterialCache::GetOrCreatePSO] CreatePSO failed for shader '%s'", shaderName);
        return nullptr;
    }

    // Store in cache
    m_cache[key] = xr_unique_ptr<MaterialPSO>(pso);
    m_stats.numCachedPSOs = static_cast<u32>(m_cache.size());

    return pso;
}

// ══════════════════════════════════════════════════════════
//  GET OR CREATE DEPTH PSO (Phase 2.4)
// ══════════════════════════════════════════════════════════

MaterialPSO* MaterialCache::GetOrCreateDepthPSO(
    dxRender_Visual* visual,
    const framegraph::FrameGraph& fg)
{
    if (!visual) {
        Msg("! [MaterialCache::GetOrCreateDepthPSO] Visual is NULL");
        return nullptr;
    }

    // Get or compile shader
    Shader* shader = nullptr;

    if (visual->shader && visual->shader._get()) {
        shader = visual->shader._get();
    }
    else if (visual->shaderName.c_str() && visual->shaderName.size() > 0 &&
             visual->textureName.c_str() && visual->textureName.size() > 0) {
        visual->shader.create(visual->shaderName.c_str(), visual->textureName.c_str());
        shader = visual->shader._get();

        if (!shader) {
            return nullptr;
        }
    } else {
        return nullptr;
    }

    // Extract shader element
    ShaderElement* elem = shader->E[0]._get();
    if (!elem || elem->passes.empty()) {
        return nullptr;
    }

    SPass* pass = elem->passes[0]._get();
    if (!pass) {
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  COMPUTE DEPTH PSO KEY
    // ═══════════════════════════════════════════════════════
    // Depth PSOs are keyed by shader + texture hash only
    // (state hash doesn't matter as we override render state)

    u64 textureHash = ComputeTextureHash(pass);
    MaterialKey key(shader, textureHash, 0, PSOType::Depth);

    // Check cache
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_stats.numCacheHits++;
        return it->second.get();
    }

    // ═══════════════════════════════════════════════════════
    //  CACHE MISS - CREATE DEPTH PSO
    // ═══════════════════════════════════════════════════════

    m_stats.numCacheMisses++;

    MaterialPSO* pso = CreateDepthPSO(visual, elem, pass, fg);
    if (!pso) {
        const char* shaderName = (pass->vs._get() && pass->vs._get()->cName.c_str())
            ? pass->vs._get()->cName.c_str() : "unknown";
        Msg("! [MaterialCache::GetOrCreateDepthPSO] CreateDepthPSO failed for shader '%s'", shaderName);
        return nullptr;
    }

    // Store in cache
    m_cache[key] = xr_unique_ptr<MaterialPSO>(pso);
    m_stats.numCachedPSOs = static_cast<u32>(m_cache.size());

    return pso;
}

// ══════════════════════════════════════════════════════════
//  CREATE PSO
// ══════════════════════════════════════════════════════════

MaterialPSO* MaterialCache::CreatePSO(
    dxRender_Visual* visual,
    ShaderElement* elem,
    SPass* pass,
    const framegraph::DefaultOutputLayout& outputs,
    const framegraph::FrameGraph& fg,
    RenderPassType passType)
{
    auto pso = xr_make_unique<MaterialPSO>();

    // Store the pass for later use (SRV extraction during binding)
    pso->pass = pass;

    // ═══════════════════════════════════════════════════════
    //  EXTRACT TEXTURES
    // ═══════════════════════════════════════════════════════

    ExtractTextures(pass, pso.get());

    // ═══════════════════════════════════════════════════════
    //  GET DETAIL TEXTURE SCALE FROM METADATA
    // ═══════════════════════════════════════════════════════
    // Legacy X-Ray: C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
    // We query the base texture (slot 0) to get its detail scale from .thm metadata

    if (pass && pass->T && !pass->T->empty()) {
        // Get base texture name (usually slot 0)
        const shared_str& baseTexName = (*pass->T)[0].second;
        if (baseTexName.c_str() && baseTexName[0]) {
            // Query our cache first
            pso->detail_scale = GetDetailScale(baseTexName);
        }
    }

    // ═══════════════════════════════════════════════════════
    //  EXTRACT SHADERS
    // ═══════════════════════════════════════════════════════

    if (!ExtractShaders(pass, pso.get())) {
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  EXTRACT SAMPLERS
    // ═══════════════════════════════════════════════════════

    ExtractSamplers(pass, pso.get());

    // ═══════════════════════════════════════════════════════
    //  CREATE BINDING LAYOUTS (Per-Stage: VS and PS)
    // ═══════════════════════════════════════════════════════

    CreateBindingLayouts(pso.get());
    if (!pso->vsBindingLayout || !pso->psBindingLayout) {
        return nullptr;
    }

    // Note: We don't create the binding sets here because they include the per-object CB
    // which changes every draw. We'll create them on-demand in GBufferPass with the CB.

    // ═══════════════════════════════════════════════════════
    //  GET OR CREATE CACHED SHADERS
    // ═══════════════════════════════════════════════════════
    //  GET NVRHI SHADER HANDLES (Direct - no wrapper layer!)
    // ═══════════════════════════════════════════════════════

    nvrhi::ShaderHandle nvrhiVS = GetOrCreateShaderVS(pso->vertexShader);
    if (!nvrhiVS) {
        return nullptr;
    }

    nvrhi::ShaderHandle nvrhiPS = GetOrCreateShaderPS(pso->pixelShader);
    if (!nvrhiPS) {
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  EXTRACT CONSTANT LAYOUT (CB + PER-CONSTANT METADATA)
    // ═══════════════════════════════════════════════════════

    framegraph::ShaderConstantLayout vsLayout;
    framegraph::ShaderConstantLayout psLayout;

    // Extract VS constant layout
    if (pso->vertexShader && pso->vertexShader->reflection) {
        vsLayout = pso->vertexShader->reflection->constantLayout;
    }

    // Extract PS constant layout
    if (pso->pixelShader && pso->pixelShader->reflection) {
        psLayout = pso->pixelShader->reflection->constantLayout;
    }

    // ✅ CRITICAL FIX: Merge layouts with proper CB deduplication
    pso->constantLayout = MergeConstantLayouts(vsLayout, psLayout);

#ifdef DEBUG
    // Log detailed constant layout for debugging
    const char* shaderName = pso->vertexShader ? pso->vertexShader->cName.c_str() : "Unknown";
    LogConstantLayout(pso->constantLayout, shaderName);
#endif

    // ═══════════════════════════════════════════════════════
    //  BUILD PIPELINE STATE DESCRIPTOR
    // ═══════════════════════════════════════════════════════

    ng::PipelineStateDesc psoDesc;
    psoDesc.vertexShader = nvrhiVS.Get();  // Direct NVRHI shader pointer
    psoDesc.pixelShader = nvrhiPS.Get();   // No wrapper layer!

    // ═══════════════════════════════════════════════════════
    //  VALIDATE VERTEX LAYOUT COMPATIBILITY
    // ═══════════════════════════════════════════════════════
    // Check if geometry can satisfy shader's vertex input requirements
    // BEFORE calling D3D11 CreateInputLayout (which would crash on mismatch)

    if (!ValidateVertexLayoutCompatibility(visual, pso.get())) {
        Msg("! [MaterialCache::CreatePSO] Vertex layout mismatch for shader '%s' - geometry doesn't provide required attributes",
            pso->debugName.c_str());
        return nullptr;
    }

    // Extract vertex attributes from visual's geometry declaration
    // CRITICAL: Use shader's input signature to determine correct order!
    SetupVertexAttributes(visual, pso.get(), psoDesc);

    // Set up render states from X-Ray pass
    SetupRenderStates(pass, psoDesc);

    // Override depth state based on pass type
    switch (passType) {
    case RenderPassType::ForwardColor:
        // Forward pass with depth prepass optimization:
        // - Use LessEqual (not Equal) so alpha-tested geometry can pass
        // - Enable depth write so alpha-tested geometry can write depth for clip() holes
        // - Opaque geometry benefits from early-Z since prepass filled depth buffer
        // - Alpha-tested geometry wasn't in prepass, needs LessEqual to render
        psoDesc.depthStencilState.depthTestEnable = true;
        psoDesc.depthStencilState.depthWriteEnable = true;
        psoDesc.depthStencilState.depthFunc = ng::ComparisonFunc::LessEqual;
        psoDesc.depthStencilState.stencilEnable = false;
        break;

    case RenderPassType::HUD:
        // HUD renders in front using viewport depth compression [0.0, 0.1]
        psoDesc.depthStencilState.depthTestEnable = true;
        psoDesc.depthStencilState.depthWriteEnable = true;
        psoDesc.depthStencilState.depthFunc = ng::ComparisonFunc::LessEqual;
        psoDesc.depthStencilState.stencilEnable = false;
        break;

    case RenderPassType::DepthPrepass:
        // Depth prepass: write depth, normal Less test
        psoDesc.depthStencilState.depthTestEnable = true;
        psoDesc.depthStencilState.depthWriteEnable = true;
        psoDesc.depthStencilState.depthFunc = ng::ComparisonFunc::Less;
        psoDesc.depthStencilState.stencilEnable = false;
        break;

    case RenderPassType::UI:
        // UI: no depth testing
        psoDesc.depthStencilState.depthTestEnable = false;
        psoDesc.depthStencilState.depthWriteEnable = false;
        psoDesc.depthStencilState.stencilEnable = false;
        break;

    case RenderPassType::Default:
    default:
        // Use material's original depth state (already set by SetupRenderStates)
        break;
    }

    // Set up render target formats from DefaultOutputLayout (using shader reflection)
    SetupRenderTargets(pso.get(), outputs, fg, psoDesc);

    // Set debug name
    psoDesc.debugName = pso->debugName;

    // ═══════════════════════════════════════════════════════
    //  CREATE PIPELINE STATE
    // ═══════════════════════════════════════════════════════

    ng::PipelineStateCache* psoCache = m_device->GetPipelineCache();
    if (!psoCache) {
        return nullptr;
    }

    ng::PipelineState* nvrhiPSO = nullptr;

    try {
        nvrhiPSO = psoCache->GetOrCreate(psoDesc);
    }
    catch (const std::exception& e) {
        // Catch D3D11 validation errors (e.g., vertex layout mismatches)
        // This happens when shader expects attributes that geometry doesn't provide
        Msg("! [MaterialCache::CreatePSO] PSO creation failed for shader '%s': %s",
            pso->debugName.c_str(), e.what());
        return nullptr;
    }
    catch (...) {
        Msg("! [MaterialCache::CreatePSO] PSO creation failed for shader '%s': Unknown exception",
            pso->debugName.c_str());
        return nullptr;
    }

    if (!nvrhiPSO) {
        Msg("! [MaterialCache::CreatePSO] PSO creation returned NULL for shader '%s'",
            pso->debugName.c_str());
        return nullptr;
    }

    pso->pso = nvrhiPSO;

    m_stats.totalPSOCreations++;

    return pso.release();
}

// ══════════════════════════════════════════════════════════
//  EXTRACT TEXTURES
// ══════════════════════════════════════════════════════════

void MaterialCache::ExtractTextures(SPass* pass, MaterialPSO* matPSO)
{
    VERIFY(pass);
    VERIFY(matPSO);

    // Clear existing textures
    matPSO->textures.clear();

    // Get TextureManager from FGResourceManager
    resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
    VERIFY(texManager);

    // Get texture list populated by r_dx11Texture() from script shaders
    STextureList* texList = pass->T._get();

    if (!texList || texList->empty()) {
        return;
    }

    // STextureList is now a vector of (stage, shared_str) pairs - directly stores texture names!
    for (size_t i = 0; i < texList->size(); i++) {
        const auto& texPair = (*texList)[i];
        u32 stage = texPair.first;
        const shared_str& textureName = texPair.second;

        // Skip empty texture names
        if (!textureName || !textureName[0]) {
            continue;
        }

        // Skip $user$ render target textures (handled separately)
        if (xr_strlen(textureName.c_str()) > 6 && 0 == strncmp(textureName.c_str(), "$user$", 6))
        {
            continue;
        }

        // Check cache first
        auto cacheIt = m_textureHandleCache.find(textureName.c_str());
        if (cacheIt != m_textureHandleCache.end()) {
            // Cache HIT - reuse existing handle
            MaterialPSO::TextureSlot texSlot;
            texSlot.slot = stage;
            texSlot.handle = cacheIt->second;
            matPSO->textures.push_back(texSlot);
            continue;
        }

        // Cache MISS - load via TextureManager
        resources::TextureHandle resourceHandle = texManager->LoadTexture(
            textureName.c_str(),
            resources::TexturePriority::High  // UI/material textures are high priority
        );

        if (!resourceHandle.IsValid()) {
            Msg("! [MaterialCache] Failed to load texture: %s", textureName.c_str());
            continue;
        }

        // Cache the handle for reuse
        m_textureHandleCache[textureName.c_str()] = resourceHandle;

        // Add to material PSO
        MaterialPSO::TextureSlot texSlot;
        texSlot.slot = stage;
        texSlot.handle = resourceHandle;
        matPSO->textures.push_back(texSlot);
    }

    // ═══════════════════════════════════════════════════════
    //  LOAD PBR TEXTURES FROM BASE TEXTURE METADATA
    // ═══════════════════════════════════════════════════════
    // PBR textures are associated with the base diffuse texture (slot 0)
    // Load them into reserved slots for Forward+ PBR rendering

    if (!texList->empty()) {
        const shared_str& baseTexName = (*texList)[0].second;

        if (!baseTexName.empty()) {
            auto& texDescMgr = RImplementation.Resources->m_textures_description;

            // PBR texture slot assignments (from ShaderConstants.h)
            using namespace passes;

            // Helper lambda to load PBR texture with fallback to default
            auto loadPBRTexture = [&](const shared_str& pbrTexName, u32 slot, const char* type, resources::TextureHandle defaultTex) {
                resources::TextureHandle handle;

                if (!pbrTexName.empty()) {
                    // Check cache
                    auto cacheIt = m_textureHandleCache.find(pbrTexName.c_str());
                    if (cacheIt != m_textureHandleCache.end()) {
                        handle = cacheIt->second;
                    } else {
                        // Load texture
                        handle = texManager->LoadTexture(
                            pbrTexName.c_str(),
                            resources::TexturePriority::High
                        );
                        if (handle.IsValid()) {
                            m_textureHandleCache[pbrTexName.c_str()] = handle;
                        }
                    }
                }

                // Use default texture if no explicit texture available
                if (!handle.IsValid()) {
                    handle = defaultTex;
                }

                if (handle.IsValid()) {
                    MaterialPSO::TextureSlot texSlot;
                    texSlot.slot = slot;
                    texSlot.handle = handle;
                    matPSO->textures.push_back(texSlot);
                }
            };

            // Load PBR textures from metadata (with fallback to defaults)
            loadPBRTexture(texDescMgr.GetMetallicName(baseTexName),  TEX_SLOT_METALLIC,  "metallic",  m_defaultMetallic);
            loadPBRTexture(texDescMgr.GetRoughnessName(baseTexName), TEX_SLOT_ROUGHNESS, "roughness", m_defaultRoughness);
            loadPBRTexture(texDescMgr.GetAOName(baseTexName),        TEX_SLOT_AO,        "ao",        m_defaultAO);
            loadPBRTexture(texDescMgr.GetParallaxName(baseTexName),  TEX_SLOT_PARALLAX,  "parallax",  m_defaultParallax);
        }
    }
}

// ══════════════════════════════════════════════════════════
//  EXTRACT SHADERS
// ══════════════════════════════════════════════════════════

bool MaterialCache::ExtractShaders(SPass* pass, MaterialPSO* matPSO)
{
    VERIFY(pass);
    VERIFY(matPSO);

    // ═══════════════════════════════════════════════════════
    //  EXTRACT VERTEX SHADER
    // ═══════════════════════════════════════════════════════

    SVS* vs = pass->vs._get();
    if (!vs) {
        return false;
    }
    matPSO->vertexShader = vs;

    // ═══════════════════════════════════════════════════════
    //  EXTRACT PIXEL SHADER
    // ═══════════════════════════════════════════════════════

    SPS* ps = pass->ps._get();
    if (!ps) {
        return false;
    }
    matPSO->pixelShader = ps;

    // Store debug names
    matPSO->debugName = vs->cName;

    // ═══════════════════════════════════════════════════
    //  SHADER REFLECTION (Week 15)
    // ═══════════════════════════════════════════════════

    // Get VERTEX shader input signature from extracted reflection (CRITICAL FOR INPUT LAYOUT!)
    if (vs->reflection) {
        matPSO->vsInputSignature = framegraph::ShaderReflector::GetVertexInputSignature(
            vs->reflection
        );
    } else {
        Msg("! [MaterialCache] VS '%s' has NULL reflection!", vs->cName.c_str());
    }

    // Get PIXEL shader RT bindings from extracted reflection
    if (ps->reflection) {
        matPSO->rtBindings = framegraph::ShaderReflector::GetRTBindings(
            ps->reflection
        );

        matPSO->rtBindings.shaderName = ps->cName;
    }

    // ═══════════════════════════════════════════════════════
    //  EXTRACT ALL CONSTANT BUFFERS from VS and PS
    // ═══════════════════════════════════════════════════════

    matPSO->constantBuffers.clear();

    // ═══════════════════════════════════════════════════════
    //  EXTRACT CONSTANT BUFFERS WITH DEDUPLICATION (FIX: VS/PS SHARED BUFFERS)
    // ═══════════════════════════════════════════════════════
    // CRITICAL FIX: Create ONE shared buffer per unique CB name instead of separate
    // buffers for VS and PS. This ensures both shader stages see the same data.

    struct CBRequirement {
        shared_str name;
        u32 vsSlot = UINT32_MAX;  // b-register in VS (UINT32_MAX = not used)
        u32 psSlot = UINT32_MAX;  // b-register in PS (UINT32_MAX = not used)
        u32 size = 0;              // Max size across stages
        bool usedInVS = false;
        bool usedInPS = false;
    };

    xr_map<shared_str, CBRequirement> uniqueCBs;  // Keyed by CB name

    // ═══════════════════════════════════════════════════════
    // STEP 1: Extract VS Constant Buffers
    // ═══════════════════════════════════════════════════════

    if (vs && vs->reflection) {
        const auto& vsCBs = vs->reflection->constantLayout.constantBuffers.buffers;
        for (const auto& cb : vsCBs) {
            auto& unique = uniqueCBs[cb.name];
            unique.name = cb.name;
            unique.vsSlot = cb.slot;
            unique.size = std::max(unique.size, cb.size);
            unique.usedInVS = true;
        }
    }

    // ═══════════════════════════════════════════════════════
    // STEP 2: Extract PS Constant Buffers
    // ═══════════════════════════════════════════════════════

    if (ps && ps->reflection) {
        const auto& psCBs = ps->reflection->constantLayout.constantBuffers.buffers;
        for (const auto& cb : psCBs) {
            auto& unique = uniqueCBs[cb.name];
            unique.name = cb.name;
            unique.psSlot = cb.slot;
            unique.size = std::max(unique.size, cb.size);  // Take max size
            unique.usedInPS = true;
        }
    }

    // ═══════════════════════════════════════════════════════
    // STEP 3: Create Shared Buffers (ONLY for Global CBs)
    // ═══════════════════════════════════════════════════════
    // Volatile CBs are handled by VCB pool (determined by reflection metadata)

    for (const auto& [cbName, cbReq] : uniqueCBs) {
        // ─────────────────────────────────────────────────────
        //  REFLECTION-DRIVEN CB CLASSIFICATION
        // ─────────────────────────────────────────────────────
        // Check if ANY constant in this CB has ConstantPersistence::Volatile
        // If so, the entire CB should be allocated from VCB pool

        bool isVolatileCB = false;

        // Check VS reflection for this CB
        if (vs && vs->reflection) {
            const auto& constantLayout = vs->reflection->constantLayout;
            for (const auto& constant : constantLayout.constants) {
                // Find which CB this constant belongs to
                if (constant.cbIndex < constantLayout.constantBuffers.buffers.size()) {
                    const auto& cbInfo = constantLayout.constantBuffers.buffers[constant.cbIndex];
                    if (cbInfo.name == cbReq.name) {
                        if (constant.persistence == framegraph::ConstantPersistence::Volatile) {
                            isVolatileCB = true;
                            break;
                        }
                    }
                }
            }
        }

        // Also check PS reflection
        if (!isVolatileCB && ps && ps->reflection) {
            const auto& constantLayout = ps->reflection->constantLayout;
            for (const auto& constant : constantLayout.constants) {
                if (constant.cbIndex < constantLayout.constantBuffers.buffers.size()) {
                    const auto& cbInfo = constantLayout.constantBuffers.buffers[constant.cbIndex];
                    if (cbInfo.name == cbReq.name) {
                        if (constant.persistence == framegraph::ConstantPersistence::Volatile) {
                            isVolatileCB = true;
                            break;
                        }
                    }
                }
            }
        }

        if (isVolatileCB) {
            if (cbReq.usedInVS) {
                framegraph::VolatileConstantBufferPool::CBLayout layout(
                    cbReq.name.c_str(),
                    cbReq.vsSlot,
                    cbReq.size
                );

                ng::BufferHandle vcbHandle = m_vcbPool->GetOrCreateVCB(layout);

                MaterialPSO::VCBRequirement req;
                req.slot = cbReq.vsSlot;
                req.size = cbReq.size;
                req.name = cbReq.name;
                req.vcbHandle = vcbHandle;
                matPSO->vcbRequirements.push_back(req);
            }
            continue;  // Don't create shared buffer for volatile CBs
        }

        // Create single shared buffer for global CBs (used by both VS and PS)
        nvrhi::BufferDesc bufferDesc;
        bufferDesc.byteSize = cbReq.size;
        bufferDesc.isConstantBuffer = true;
        bufferDesc.debugName = make_string("CB_%s", cbReq.name.c_str()).c_str();
        bufferDesc.keepInitialState = false;
        bufferDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

        nvrhi::BufferHandle sharedBuffer = m_device->GetNativeDevice()->createBuffer(bufferDesc);

        if (!sharedBuffer) {
            Msg("! [MaterialCache] FAILED to create shared CB: %s", cbReq.name.c_str());
            continue;
        }

        // CRITICAL FIX: Create TWO entries (VS and PS) both pointing to same buffer
        // This ensures binding set creation finds the buffer for both stages

        if (cbReq.usedInVS) {
            MaterialPSO::ConstantBufferInfo vsInfo;
            vsInfo.name = cbReq.name;
            vsInfo.size = cbReq.size;
            vsInfo.nvrhiBuffer = sharedBuffer;
            vsInfo.stage = MaterialPSO::ShaderStage::Vertex;
            vsInfo.slot = cbReq.vsSlot;
            matPSO->constantBuffers.push_back(vsInfo);
        }

        if (cbReq.usedInPS) {
            MaterialPSO::ConstantBufferInfo psInfo;
            psInfo.name = cbReq.name;
            psInfo.size = cbReq.size;
            psInfo.nvrhiBuffer = sharedBuffer;  // ← SAME BUFFER as VS!
            psInfo.stage = MaterialPSO::ShaderStage::Pixel;
            psInfo.slot = cbReq.psSlot;
            matPSO->constantBuffers.push_back(psInfo);
        }
    }

    // Store per-object CB size for convenience (from VCB requirements)
    if (!matPSO->vcbRequirements.empty()) {
        matPSO->perObjectCBSize = matPSO->vcbRequirements[0].size;
    }

    // Fallback if no slot 0 CB found
    if (matPSO->perObjectCBSize == 0) {
        matPSO->perObjectCBSize = 256;
    }

    return true;
}

// ══════════════════════════════════════════════════════════
//  EXTRACT SAMPLERS (Using Shader Reflection + X-Ray State)
// ══════════════════════════════════════════════════════════

void MaterialCache::ExtractSamplers(SPass* pass, MaterialPSO* matPSO)
{
    VERIFY(pass);
    VERIFY(matPSO);

#if defined(USE_DX11)
    // Extract samplers based on Slang shader reflection
    // Sampler state is inferred from X-Ray naming conventions (see ShaderReflection.h)
    for (const auto& samplerDecl : matPSO->rtBindings.samplers) {
        MaterialPSO::SamplerInfo samplerInfo;
        samplerInfo.slot = samplerDecl.slot;
        samplerInfo.stage = MaterialPSO::ShaderStage::Pixel;  // Currently only PS samplers
        samplerInfo.name = samplerDecl.name.c_str();

        // Create NVRHI sampler directly from reflection metadata
        samplerInfo.nvrhiSampler = samplerDecl.CreateNVRHISampler(m_device->GetNativeDevice());

        if (samplerInfo.nvrhiSampler) {
            matPSO->samplers.push_back(samplerInfo);
        } else {
            Msg("! [ExtractSamplers] Failed to create sampler '%s'", samplerInfo.name.c_str());
        }
    }

#endif
}

// ══════════════════════════════════════════════════════════
//  CREATE BINDING LAYOUTS (Per-Stage)
// ══════════════════════════════════════════════════════════

void MaterialCache::CreateBindingLayouts(MaterialPSO* matPSO)
{
    VERIFY(matPSO);

    // Create VS layout with visibility = ShaderType::Vertex
    matPSO->vsBindingLayout = CreateStageBindingLayout(
        matPSO, MaterialPSO::ShaderStage::Vertex, nvrhi::ShaderType::Vertex);

    // Create PS layout with visibility = ShaderType::Pixel
    matPSO->psBindingLayout = CreateStageBindingLayout(
        matPSO, MaterialPSO::ShaderStage::Pixel, nvrhi::ShaderType::Pixel);

    if (matPSO->vsBindingLayout && matPSO->psBindingLayout) {
    }
}

// ══════════════════════════════════════════════════════════
//  CREATE STAGE BINDING LAYOUT (Helper)
// ══════════════════════════════════════════════════════════

nvrhi::BindingLayoutHandle MaterialCache::CreateStageBindingLayout(
    const MaterialPSO* matPSO,
    MaterialPSO::ShaderStage stage,
    nvrhi::ShaderType nvrhiStage)
{
    VERIFY(matPSO);

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhiStage;  // ← KEY: Per-stage visibility!

    const char* stageName = (stage == MaterialPSO::ShaderStage::Vertex) ? "VS" : "PS";

    // CRITICAL: Collect ALL CBs and sort by slot number!
    // NVRHI binding sets match by INDEX, so layout order must match shader slot order (b0, b1, b2, ...)
    struct CBBinding {
        u32 slot;
        bool isVCB;
        shared_str name;
        u32 size;
    };
    xr_vector<CBBinding> allCBs;

    // Collect VCBs from vcbRequirements
    if (stage == MaterialPSO::ShaderStage::Vertex) {
        for (const auto& vcbReq : matPSO->vcbRequirements) {
            allCBs.push_back({vcbReq.slot, true, vcbReq.name, vcbReq.size});
        }
    }

    // Collect global CBs from constantBuffers
    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.stage == stage) {
            allCBs.push_back({cbInfo.slot, false, cbInfo.name, cbInfo.size});
        }
    }

    // SORT by slot number (b0, b1, b2, b3, ...)
    std::sort(allCBs.begin(), allCBs.end(), [](const CBBinding& a, const CBBinding& b) {
        return a.slot < b.slot;
    });

    // Add to layout in SORTED order
    u32 cbCount = 0;
    for (const auto& cb : allCBs) {
        if (cb.isVCB) {
            layoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::VolatileConstantBuffer(cb.slot));
        } else {
            layoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::ConstantBuffer(cb.slot));
        }
        cbCount++;
    }

    // Textures (t0, t1, t2, ...) - currently only in PS
    u32 texCount = 0;
    if (stage == MaterialPSO::ShaderStage::Pixel) {
        for (const auto& texSlot : matPSO->textures) {
            layoutDesc.bindings.push_back(
                nvrhi::BindingLayoutItem::Texture_SRV(texSlot.slot));
            texCount++;
        }
    }

    // Samplers (s0, s1, s2, ...) - currently only in PS
    u32 samplerCount = 0;
    if (stage == MaterialPSO::ShaderStage::Pixel) {
        for (const auto& samplerInfo : matPSO->samplers) {
            if (samplerInfo.stage == stage) {
                layoutDesc.bindings.push_back(
                    nvrhi::BindingLayoutItem::Sampler(samplerInfo.slot));
                samplerCount++;
            }
        }
    }


    nvrhi::BindingLayoutHandle layout = m_device->CreateBindingLayout(layoutDesc);
    if (!layout) {
        return nullptr;
    }

    return layout;
}

// ══════════════════════════════════════════════════════════
//  GET OR CREATE CACHED BINDING SET (with per-object VCB)
// ══════════════════════════════════════════════════════════

nvrhi::BindingSetHandle MaterialCache::GetOrCreateBindingSet(MaterialPSO* matPSO)
{
    VERIFY(matPSO);
    VERIFY(matPSO->vsBindingLayout);
    VERIFY(matPSO->psBindingLayout);

    // ═══════════════════════════════════════════════════════
    //  CHECK CACHE - Return existing binding sets if already created
    // ═══════════════════════════════════════════════════════
    // With proper NVRHI VCB support (isVolatile=true, maxVersions set),
    // binding sets can be cached even with VCBs - NVRHI handles versioning.
    if (matPSO->vsBindingSet && matPSO->psBindingSet && !matPSO->needsBindingSetRebuild) {
        return matPSO->vsBindingSet;
    }

    // Clear the rebuild flag - we're rebuilding now
    matPSO->needsBindingSetRebuild = false;

    // ═══════════════════════════════════════════════════════
    //  BUILD VS BINDING SET DESCRIPTOR
    // ═══════════════════════════════════════════════════════
    // CRITICAL: Binding order must match layout order (VCBs first, then global CBs).
    // NVRHI matches bindings by INDEX, not slot number!

    struct TempBinding {
        u32 slot;
        nvrhi::IBuffer* buffer;
        shared_str name;  // For logging
        bool isVCB;
    };
    xr_vector<TempBinding> vsBindings;

    // Collect ALL VCBs from vcbRequirements (per-draw data)
    for (const auto& vcbReq : matPSO->vcbRequirements) {
        // Query VCB pool for latest handle (FGConstantSystem might have updated it)
        ng::BufferHandle latestHandle = m_vcbPool->GetOrCreateVCB(
            framegraph::VolatileConstantBufferPool::CBLayout(
                vcbReq.name.c_str(), vcbReq.slot, vcbReq.size
            )
        );

        if (!latestHandle.IsValid()) {
            continue;
        }

        nvrhi::IBuffer* vcbBuffer = m_device->GetNativeBuffer(latestHandle);
        if (!vcbBuffer) {
            continue;
        }

        vsBindings.push_back({vcbReq.slot, vcbBuffer, vcbReq.name, true});
    }

    // Collect global CBs from constantBuffers
    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.stage == MaterialPSO::ShaderStage::Vertex) {
            if (cbInfo.nvrhiBuffer) {
                vsBindings.push_back({cbInfo.slot, cbInfo.nvrhiBuffer.Get(), cbInfo.name, false});
            }
        }
    }

    // CRITICAL: SORT by slot number to match layout order!
    // NVRHI matches bindings by INDEX, so binding set order must match layout order (b0, b1, b2, ...)
    std::sort(vsBindings.begin(), vsBindings.end(), [](const TempBinding& a, const TempBinding& b) {
        return a.slot < b.slot;
    });

    nvrhi::BindingSetDesc vsBindingDesc;
    for (const auto& binding : vsBindings) {
        // ConstantBuffer() auto-detects volatile buffers (checks buffer->getDesc().isVolatile)
        vsBindingDesc.bindings.push_back(
            nvrhi::BindingSetItem::ConstantBuffer(binding.slot, binding.buffer));
    }

    // ═══════════════════════════════════════════════════════
    //  BUILD PS BINDING SET DESCRIPTOR
    // ═══════════════════════════════════════════════════════
    // CRITICAL: Binding order must match layout order.
    // Collect bindings in the same order as CreateStageBindingLayout

    struct PSBinding {
        u32 slot;
        nvrhi::BindingSetItem item;
        shared_str name;  // For logging
        enum Type { CB, Texture, Sampler } type;
    };
    xr_vector<PSBinding> psBindings;

    // Collect global CBs from constantBuffers
    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.stage == MaterialPSO::ShaderStage::Pixel) {
            if (cbInfo.nvrhiBuffer) {
                psBindings.push_back({cbInfo.slot,
                    nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, cbInfo.nvrhiBuffer.Get()),
                    cbInfo.name, PSBinding::CB});
            }
        }
    }

    // Collect textures
    // Track if all textures are valid - if not, we won't cache the binding set
    bool allTexturesValid = true;
    resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
    for (const auto& texSlot : matPSO->textures) {
        nvrhi::ITexture* nativeTex = texManager->GetNVRHITexture(texSlot.handle);

        if (nativeTex) {
            const nvrhi::TextureDesc& texDesc = nativeTex->getDesc();
            psBindings.push_back({texSlot.slot,
                nvrhi::BindingSetItem::Texture_SRV(texSlot.slot, nativeTex,
                    nvrhi::Format::UNKNOWN, nvrhi::AllSubresources, texDesc.dimension),
                "texture", PSBinding::Texture});
        } else {
            // Texture not ready - mark as invalid so we don't cache this binding set
            allTexturesValid = false;
            psBindings.push_back({texSlot.slot,
                nvrhi::BindingSetItem::Texture_SRV(texSlot.slot, nullptr),
                "texture_null", PSBinding::Texture});
        }
    }

    // Collect samplers
    for (const auto& samplerInfo : matPSO->samplers) {
        if (samplerInfo.stage == MaterialPSO::ShaderStage::Pixel) {
            if (samplerInfo.nvrhiSampler) {
                psBindings.push_back({samplerInfo.slot,
                    nvrhi::BindingSetItem::Sampler(samplerInfo.slot, samplerInfo.nvrhiSampler),
                    samplerInfo.name, PSBinding::Sampler});
            } else {
                psBindings.push_back({samplerInfo.slot,
                    nvrhi::BindingSetItem::Sampler(samplerInfo.slot, nullptr),
                    "sampler_null", PSBinding::Sampler});
            }
        }
    }

    // NOTE: Do NOT sort! NVRHI matches bindings by INDEX, not slot number.
    // The binding set order MUST match the layout order.

    nvrhi::BindingSetDesc psBindingDesc;
    for (const auto& binding : psBindings) {
        psBindingDesc.bindings.push_back(binding.item);
    }

    // ═══════════════════════════════════════════════════════
    //  CREATE VS BINDING SET
    // ═══════════════════════════════════════════════════════

    nvrhi::BindingSetHandle vsBindingSet = m_device->CreateBindingSet(
        vsBindingDesc,
        matPSO->vsBindingLayout);

    if (!vsBindingSet) {
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  CREATE PS BINDING SET
    // ═══════════════════════════════════════════════════════

    nvrhi::BindingSetHandle psBindingSet = m_device->CreateBindingSet(
        psBindingDesc,
        matPSO->psBindingLayout);

    if (!psBindingSet) {
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  STORE BINDING SETS
    // ═══════════════════════════════════════════════════════
    // Cache binding sets for reuse. VCBs use NVRHI's internal versioning
    // (isVolatile=true, maxVersions set) so caching is safe.
    // Only rebuild if textures weren't valid.

    matPSO->vsBindingSet = vsBindingSet;
    matPSO->psBindingSet = psBindingSet;

    if (!allTexturesValid) {
        // Textures not ready - mark for recreation next frame
        matPSO->needsBindingSetRebuild = true;
    }

    return matPSO->vsBindingSet;
}

// ══════════════════════════════════════════════════════════
//  COMPUTE TEXTURE HASH
// ══════════════════════════════════════════════════════════

u64 MaterialCache::ComputeTextureHash(SPass* pass)
{
    if (!pass)
        return 0;

    // Get texture list from pass
    STextureList* texList = pass->T._get();
    if (!texList || texList->empty())
        return 0;

    // Hash texture NAMES (stable across frames)
    // Now we hash the texture name string instead of CTexture pointer
    u32 hash = 0;
    for (const auto& texPair : *texList) {
        const shared_str& textureName = texPair.second;
        if (textureName.c_str() && textureName[0]) {
            // Hash the texture name string (identifies the texture uniquely)
            hash = crc32(textureName.c_str(), xr_strlen(textureName.c_str()), hash);
        }
    }

    return static_cast<u64>(hash);
}

// ══════════════════════════════════════════════════════════
//  COMPUTE STATE HASH
// ══════════════════════════════════════════════════════════

u64 MaterialCache::ComputeStateHash(SPass* pass)
{
    if (!pass)
        return 0;

    // Hash render state pointer
    // In a more sophisticated implementation, we'd hash the actual state values
    void* statePtr = pass->state._get();
    u32 hash = crc32(&statePtr, sizeof(statePtr), 0);

    return static_cast<u64>(hash);
}

// Helper: Get size in bytes of a DXGI format
static u32 GetFormatSize(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return 16;
        case DXGI_FORMAT_R32G32B32_FLOAT: return 12;
        case DXGI_FORMAT_R32G32_FLOAT: return 8;
        case DXGI_FORMAT_R32_FLOAT: return 4;
        case DXGI_FORMAT_R16G16B16A16_FLOAT: return 8;
        case DXGI_FORMAT_R16G16_FLOAT: return 4;
        case DXGI_FORMAT_R16_FLOAT: return 2;
        case DXGI_FORMAT_R8G8B8A8_UNORM: return 4;
        case DXGI_FORMAT_R8G8_UNORM: return 2;
        case DXGI_FORMAT_R8_UNORM: return 1;
        case DXGI_FORMAT_R16G16B16A16_SNORM: return 8;
        case DXGI_FORMAT_R16G16_SNORM: return 4;
        case DXGI_FORMAT_R16_SNORM: return 2;
        case DXGI_FORMAT_R8G8B8A8_SNORM: return 4;
        case DXGI_FORMAT_R8G8_SNORM: return 2;
        case DXGI_FORMAT_R8_SNORM: return 1;
        case DXGI_FORMAT_R16G16B16A16_UINT: return 8;
        case DXGI_FORMAT_R16G16_UINT: return 4;
        case DXGI_FORMAT_R16_UINT: return 2;
        case DXGI_FORMAT_R8G8B8A8_UINT: return 4;
        case DXGI_FORMAT_R8G8_UINT: return 2;
        case DXGI_FORMAT_R8_UINT: return 1;
        case DXGI_FORMAT_R32G32B32A32_UINT: return 16;
        case DXGI_FORMAT_R32G32_UINT: return 8;
        case DXGI_FORMAT_R32_UINT: return 4;
        // SINT formats (signed integers)
        case DXGI_FORMAT_R16G16B16A16_SINT: return 8;
        case DXGI_FORMAT_R16G16_SINT: return 4;
        case DXGI_FORMAT_R16_SINT: return 2;
        case DXGI_FORMAT_R8G8B8A8_SINT: return 4;
        case DXGI_FORMAT_R8G8_SINT: return 2;
        case DXGI_FORMAT_R8_SINT: return 1;
        case DXGI_FORMAT_R32G32B32A32_SINT: return 16;
        case DXGI_FORMAT_R32G32_SINT: return 8;
        case DXGI_FORMAT_R32_SINT: return 4;
        default:
            return 4;  // Default fallback
    }
}

// ══════════════════════════════════════════════════════════
//  SETUP VERTEX ATTRIBUTES
// ══════════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════════
//  VALIDATE VERTEX LAYOUT COMPATIBILITY
// ══════════════════════════════════════════════════════════
// Returns true if the visual's geometry provides all attributes the shader expects

bool MaterialCache::ValidateVertexLayoutCompatibility(dxRender_Visual* visual, MaterialPSO* matPSO)
{
    if (!visual || !matPSO) return false;

    // Get geometry from visual
    IRender_Mesh* meshVisual = nullptr;
    switch (visual->getType()) {
        case MT_NORMAL:
            meshVisual = static_cast<Fvisual*>(visual);
            break;
        case MT_PROGRESSIVE:
            meshVisual = static_cast<FProgressive*>(visual);
            break;
        case MT_TREE_ST:
        case MT_TREE_PM:
            meshVisual = static_cast<FTreeVisual*>(visual);
            break;
        case MT_SKELETON_GEOMDEF_ST:
            meshVisual = static_cast<CSkeletonX_ST*>(visual);
            break;
        case MT_SKELETON_GEOMDEF_PM:
            meshVisual = static_cast<CSkeletonX_PM*>(visual);
            break;
        default:
            // Unknown visual type - assume compatible
            return true;
    }

    if (!meshVisual || !meshVisual->rm_geom || !meshVisual->rm_geom._get())
        return true;  // No geometry, assume compatible

    SGeometry* geom = meshVisual->rm_geom._get();
    if (!geom->dcl || !geom->dcl._get())
        return true;  // No declaration, assume compatible

    SDeclaration* decl = geom->dcl._get();

    // Check if shader input signature has requirements
    if (!matPSO->vsInputSignature.elements.empty()) {
        // Check each shader input requirement
        for (const auto& shaderElem : matPSO->vsInputSignature.elements) {
            // Look for matching element in geometry declaration
            bool found = false;
            for (const auto& d3dElem : decl->dx11_dcl_code) {
                if (!d3dElem.SemanticName)
                    continue;

                if (xr_strcmp(d3dElem.SemanticName, shaderElem.semanticName.c_str()) == 0 &&
                    d3dElem.SemanticIndex == shaderElem.semanticIndex) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                // Shader expects this attribute, but geometry doesn't provide it
                Msg("! [MaterialCache] Vertex layout validation FAILED: shader expects '%s%d' but geometry doesn't provide it",
                    shaderElem.semanticName.c_str(), shaderElem.semanticIndex);
                return false;
            }
        }
    }

    return true;  // All shader requirements satisfied
}

void MaterialCache::SetupVertexAttributes(dxRender_Visual* visual, MaterialPSO* matPSO, ng::PipelineStateDesc& psoDesc)
{
    psoDesc.vertexAttributes.clear();

    // Get geometry from visual
    IRender_Mesh* meshVisual = nullptr;
    switch (visual->getType()) {
        case MT_NORMAL:
            meshVisual = static_cast<Fvisual*>(visual);
            break;
        case MT_PROGRESSIVE:
            meshVisual = static_cast<FProgressive*>(visual);
            break;
        case MT_TREE_ST:
        case MT_TREE_PM:
            meshVisual = static_cast<FTreeVisual*>(visual);
            break;
        case MT_SKELETON_GEOMDEF_ST:
            meshVisual = static_cast<CSkeletonX_ST*>(visual);
            break;
        case MT_SKELETON_GEOMDEF_PM:
            meshVisual = static_cast<CSkeletonX_PM*>(visual);
            break;
        default:
            // Fallback to hardcoded layout
            return;
    }

    if (!meshVisual || !meshVisual->rm_geom || !meshVisual->rm_geom._get())
        return;

    SGeometry* geom = meshVisual->rm_geom._get();
    if (!geom->dcl || !geom->dcl._get())
        return;

    SDeclaration* decl = geom->dcl._get();

    // ═══════════════════════════════════════════════════════
    //  USE SHADER INPUT SIGNATURE FOR CORRECT ORDERING
    // ═══════════════════════════════════════════════════════
    //
    // CRITICAL: We MUST create the input layout in the EXACT order the shader
    // expects! D3D11 CreateInputLayout requires the elements to match the
    // shader's input signature order, otherwise vertex data gets mismatched.
    //
    // The shader's input signature is extracted from VS bytecode reflection
    // and stored in matPSO->vsInputSignature in shader-expected order.

    if (!matPSO || matPSO->vsInputSignature.elements.empty()) {
        // Fallback to old behavior (will likely be wrong order)
    } else {
    }

    // CRITICAL: Compute stride for each buffer slot!
    // D3D11_INPUT_ELEMENT_DESC doesn't have stride - we must calculate it
    // Stride = total size of all elements in this buffer slot
    std::map<u32, u32> bufferStrides;  // slot -> stride in bytes


    for (const auto& d3dElem : decl->dx11_dcl_code) {
        if (!d3dElem.SemanticName)
            continue;

        u32 slot = d3dElem.InputSlot;
        u32 elemSize = GetFormatSize(d3dElem.Format);

        // Calculate end offset of this element
        u32 endOffset = d3dElem.AlignedByteOffset + elemSize;

        // Format name for debugging
        const char* formatName = "UNKNOWN";
        if (d3dElem.Format == DXGI_FORMAT_R8G8B8A8_UNORM) formatName = "R8G8B8A8_UNORM";
        else if (d3dElem.Format == DXGI_FORMAT_R32G32_FLOAT) formatName = "R32G32_FLOAT";
        else if (d3dElem.Format == DXGI_FORMAT_R32G32B32_FLOAT) formatName = "R32G32B32_FLOAT";
        else if (d3dElem.Format == DXGI_FORMAT_R32G32B32A32_FLOAT) formatName = "R32G32B32A32_FLOAT";
        else if (d3dElem.Format == DXGI_FORMAT_R16G16_SINT) formatName = "R16G16_SINT";

            d3dElem.SemanticName, d3dElem.SemanticIndex,

        // Update stride to be at least this large
        bufferStrides[slot] = std::max(bufferStrides[slot], endOffset);
    }

    // ═══════════════════════════════════════════════════════
    //  BUILD INPUT LAYOUT IN SHADER-EXPECTED ORDER
    // ═══════════════════════════════════════════════════════
    //
    // Iterate through the shader's input signature (in order!), and for each
    // element, find the matching element in the vertex declaration to get the
    // actual format and byte offset.

    if (matPSO && !matPSO->vsInputSignature.elements.empty()) {
        // NEW CODE PATH: Use shader input signature order (CORRECT!)

        for (u32 shaderIdx = 0; shaderIdx < matPSO->vsInputSignature.elements.size(); ++shaderIdx) {
            const auto& shaderElem = matPSO->vsInputSignature.elements[shaderIdx];


            // Find matching element in vertex declaration
            const D3D11_INPUT_ELEMENT_DESC* matchingDeclElem = nullptr;
            for (const auto& d3dElem : decl->dx11_dcl_code) {
                if (!d3dElem.SemanticName)
                    continue;

                if (xr_strcmp(d3dElem.SemanticName, shaderElem.semanticName.c_str()) == 0 &&
                    d3dElem.SemanticIndex == shaderElem.semanticIndex) {
                    matchingDeclElem = &d3dElem;
                    break;
                }
            }

            if (!matchingDeclElem) {
                continue;
            }

            // Create vertex attribute using:
            // - Shader element ORDER (iteration order determines final order)
            // - Vertex decl's semantic name, index, format, and offset (actual data layout)
            ng::VertexAttribute attr;
            attr.semanticName = matchingDeclElem->SemanticName;
            attr.semanticIndex = matchingDeclElem->SemanticIndex;  // Use decl's index (TEXCOORD0 vs TEXCOORD1)
            attr.format = ConvertDxgiFormatToNvrhi(matchingDeclElem->Format);
            attr.offset = matchingDeclElem->AlignedByteOffset;
            attr.bufferIndex = matchingDeclElem->InputSlot;
            attr.isInstanced = (matchingDeclElem->InputSlotClass == D3D11_INPUT_PER_INSTANCE_DATA);
            attr.elementStride = bufferStrides[matchingDeclElem->InputSlot];

            psoDesc.vertexAttributes.push_back(attr);
        }
    }
}

// ══════════════════════════════════════════════════════════
//  STATE CONVERSION HELPERS
// ══════════════════════════════════════════════════════════
// Now located in RenderStateConversion.h (shared with ParticlePass)

// ══════════════════════════════════════════════════════════
//  SETUP RENDER STATES
// ══════════════════════════════════════════════════════════

void MaterialCache::SetupRenderStates(SPass* pass, ng::PipelineStateDesc& psoDesc)
{
    VERIFY(pass);

    // Get X-Ray state object
    SState* xrState = pass->state._get();
    if (!xrState || !xrState->state) {
        // Fallback to safe defaults if no state
        psoDesc.rasterizerState.cullMode = ng::CullMode::Back;
        psoDesc.rasterizerState.fillMode = ng::FillMode::Solid;
        psoDesc.depthStencilState.depthTestEnable = true;
        psoDesc.depthStencilState.depthWriteEnable = true;
        psoDesc.depthStencilState.depthFunc = ng::ComparisonFunc::Less;
        psoDesc.blendState.renderTargets[0].blendEnable = false;
        return;
    }


#if defined(USE_DX11)
    dx11State* d3dState = static_cast<dx11State*>(xrState->state);

    // ═══════════════════════════════════════════════════════
    //  EXTRACT RASTERIZER STATE
    // ═══════════════════════════════════════════════════════

    ID3DRasterizerState* rasterizerState = d3dState->GetRasterizerState();
    if (rasterizerState) {
        D3D11_RASTERIZER_DESC rsDesc;
        rasterizerState->GetDesc(&rsDesc);

        psoDesc.rasterizerState.cullMode = ConvertCullMode(rsDesc.CullMode);
        psoDesc.rasterizerState.fillMode = ConvertFillMode(rsDesc.FillMode);
        psoDesc.rasterizerState.frontCounterClockwise = rsDesc.FrontCounterClockwise;
        psoDesc.rasterizerState.depthClipEnable = rsDesc.DepthClipEnable;
        psoDesc.rasterizerState.depthBias = rsDesc.DepthBias;
        psoDesc.rasterizerState.slopeScaledDepthBias = rsDesc.SlopeScaledDepthBias;
        psoDesc.rasterizerState.scissorEnable = rsDesc.ScissorEnable;
    }

    // ═══════════════════════════════════════════════════════
    //  EXTRACT DEPTH/STENCIL STATE
    // ═══════════════════════════════════════════════════════

    ID3DDepthStencilState* depthStencilState = d3dState->GetDepthStencilState();
    if (depthStencilState) {
        D3D11_DEPTH_STENCIL_DESC dsDesc;
        depthStencilState->GetDesc(&dsDesc);

        psoDesc.depthStencilState.depthTestEnable = dsDesc.DepthEnable;
        psoDesc.depthStencilState.depthWriteEnable = (dsDesc.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL);
        psoDesc.depthStencilState.depthFunc = ConvertComparisonFunc(dsDesc.DepthFunc);
        psoDesc.depthStencilState.stencilEnable = dsDesc.StencilEnable;

        if (dsDesc.StencilEnable) {
            // Front face stencil
            psoDesc.depthStencilState.stencilReadMask = dsDesc.StencilReadMask;
            psoDesc.depthStencilState.stencilWriteMask = dsDesc.StencilWriteMask;

            // Front face operations
            psoDesc.depthStencilState.frontFace.failOp = ConvertStencilOp(dsDesc.FrontFace.StencilFailOp);
            psoDesc.depthStencilState.frontFace.depthFailOp = ConvertStencilOp(dsDesc.FrontFace.StencilDepthFailOp);
            psoDesc.depthStencilState.frontFace.passOp = ConvertStencilOp(dsDesc.FrontFace.StencilPassOp);
            psoDesc.depthStencilState.frontFace.compareFunc = ConvertComparisonFunc(dsDesc.FrontFace.StencilFunc);

            // Back face operations
            psoDesc.depthStencilState.backFace.failOp = ConvertStencilOp(dsDesc.BackFace.StencilFailOp);
            psoDesc.depthStencilState.backFace.depthFailOp = ConvertStencilOp(dsDesc.BackFace.StencilDepthFailOp);
            psoDesc.depthStencilState.backFace.passOp = ConvertStencilOp(dsDesc.BackFace.StencilPassOp);
            psoDesc.depthStencilState.backFace.compareFunc = ConvertComparisonFunc(dsDesc.BackFace.StencilFunc);
        }
    }

    // ═══════════════════════════════════════════════════════
    //  EXTRACT BLEND STATE
    // ═══════════════════════════════════════════════════════

    ID3DBlendState* blendState = d3dState->GetBlendState();
    if (blendState) {
        D3D11_BLEND_DESC blendDesc;
        blendState->GetDesc(&blendDesc);


        // D3D11 can have independent blend per RT or same for all
        psoDesc.blendState.alphaToCoverageEnable = blendDesc.AlphaToCoverageEnable;

        if (blendDesc.IndependentBlendEnable) {
            // Independent blend for each RT (MRT support)
            for (int i = 0; i < 3; ++i) {  // GBuffer has 3 RTs
                const D3D11_RENDER_TARGET_BLEND_DESC& rtBlend = blendDesc.RenderTarget[i];

                psoDesc.blendState.renderTargets[i].blendEnable = rtBlend.BlendEnable;
                psoDesc.blendState.renderTargets[i].srcBlend = ConvertBlendFactor(rtBlend.SrcBlend);
                psoDesc.blendState.renderTargets[i].dstBlend = ConvertBlendFactor(rtBlend.DestBlend);
                psoDesc.blendState.renderTargets[i].blendOp = ConvertBlendOp(rtBlend.BlendOp);
                psoDesc.blendState.renderTargets[i].srcBlendAlpha = ConvertBlendFactor(rtBlend.SrcBlendAlpha);
                psoDesc.blendState.renderTargets[i].dstBlendAlpha = ConvertBlendFactor(rtBlend.DestBlendAlpha);
                psoDesc.blendState.renderTargets[i].blendOpAlpha = ConvertBlendOp(rtBlend.BlendOpAlpha);
                psoDesc.blendState.renderTargets[i].writeMask = ConvertColorWriteMask(rtBlend.RenderTargetWriteMask);
            }
        } else {
            // Same blend state for all RTs
            const D3D11_RENDER_TARGET_BLEND_DESC& rtBlend = blendDesc.RenderTarget[0];

            for (int i = 0; i < 3; ++i) {
                psoDesc.blendState.renderTargets[i].blendEnable = rtBlend.BlendEnable;
                psoDesc.blendState.renderTargets[i].srcBlend = ConvertBlendFactor(rtBlend.SrcBlend);
                psoDesc.blendState.renderTargets[i].dstBlend = ConvertBlendFactor(rtBlend.DestBlend);
                psoDesc.blendState.renderTargets[i].blendOp = ConvertBlendOp(rtBlend.BlendOp);
                psoDesc.blendState.renderTargets[i].srcBlendAlpha = ConvertBlendFactor(rtBlend.SrcBlendAlpha);
                psoDesc.blendState.renderTargets[i].dstBlendAlpha = ConvertBlendFactor(rtBlend.DestBlendAlpha);
                psoDesc.blendState.renderTargets[i].blendOpAlpha = ConvertBlendOp(rtBlend.BlendOpAlpha);
                psoDesc.blendState.renderTargets[i].writeMask = ConvertColorWriteMask(rtBlend.RenderTargetWriteMask);
            }
        }
    } else {
    }
#endif // USE_DX11
}

// ══════════════════════════════════════════════════════════
//  SETUP RENDER TARGETS
// ══════════════════════════════════════════════════════════

void MaterialCache::SetupRenderTargets(
    MaterialPSO* matPSO,
    const framegraph::DefaultOutputLayout& outputs,
    const framegraph::FrameGraph& fg,
    ng::PipelineStateDesc& psoDesc)
{
    // Extract actual formats from FrameGraph resources
    // Forward+ only uses albedo (color) and depth - no separate normal/material RTs
    nvrhi::ITexture* albedoTex = fg.GetPhysicalTexture(outputs.albedo);
    nvrhi::ITexture* depthTex = fg.GetPhysicalTexture(outputs.depth);

    if (!albedoTex || !depthTex) {
        // Fallback to hardcoded format for Forward+ (single color RT)
        psoDesc.renderTargetCount = 1;
        psoDesc.renderTargetFormats[0] = nvrhi::Format::RGBA16_FLOAT;  // HDR color
        psoDesc.depthStencilFormat = nvrhi::Format::D32;  // Match createDepthBuffer format
        return;
    }

    // ═══════════════════════════════════════════════════════
    //  FORWARD+ RENDERING: SINGLE RT ONLY
    // ═══════════════════════════════════════════════════════
    // Forward+ always uses exactly 1 render target (color/albedo)
    // Any shader outputting multiple RTs is incompatible and will fail

    psoDesc.renderTargetCount = 1;
    psoDesc.renderTargetFormats[0] = albedoTex->getDesc().format;
    psoDesc.depthStencilFormat = depthTex->getDesc().format;

    // Clear unused slots
    for (u32 i = 1; i < 8; i++) {
        psoDesc.renderTargetFormats[i] = nvrhi::Format::UNKNOWN;
    }
}

// ══════════════════════════════════════════════════════════
//  UI PSO CREATION (Simplified - no visual required)
// ══════════════════════════════════════════════════════════

MaterialPSO* MaterialCache::GetOrCreateUIPSO(
    Shader* shader,
    u32 elementIndex,
    nvrhi::IFramebuffer* framebuffer)
{
    if (!shader || !framebuffer)
        return nullptr;

    // Extract shader element and pass
    ShaderElement* elem = shader->E[elementIndex]._get();
    if (!elem || elem->passes.empty())
        return nullptr;

    SPass* pass = elem->passes[0]._get();
    if (!pass)
        return nullptr;

    // Create cache key (shader + element + framebuffer)
    MaterialKey key;
    key.psoType = PSOType::UI;
    key.shader = shader;
    key.element = elementIndex;
    key.framebuffer = framebuffer;

    // Check cache
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_stats.numCacheHits++;
        return it->second.get();
    }

    // Create new UI PSO
    m_stats.numCacheMisses++;
    m_stats.totalPSOCreations++;

    MaterialPSO* pso = CreateUIPSO(shader, elem, pass, framebuffer);
    if (!pso)
        return nullptr;

    m_cache[key] = xr_unique_ptr<MaterialPSO>(pso);
    m_stats.numCachedPSOs = static_cast<u32>(m_cache.size());

    return pso;
}

// ══════════════════════════════════════════════════════════
//  CREATE DEPTH PSO (Phase 2.4)
// ══════════════════════════════════════════════════════════
// Creates optimized depth-only PSO with:
// - No color writes (color mask = 0)
// - Depth write enabled
// - Minimal pixel shader (alpha test only for vegetation)
// - Same vertex processing as material PSO

MaterialPSO* MaterialCache::CreateDepthPSO(
    dxRender_Visual* visual,
    ShaderElement* elem,
    SPass* pass,
    const framegraph::FrameGraph& fg)
{
    auto pso = xr_make_unique<MaterialPSO>();
    pso->pass = pass;

    // Reuse material's vertex shader + NULL pixel shader for depth-only rendering
    if (!ExtractShaders(pass, pso.get())) {
        return nullptr;
    }

    nvrhi::ShaderHandle materialVS = GetOrCreateShaderVS(pso->vertexShader);
    if (!materialVS) {
        return nullptr;
    }

    pso->pixelShader = nullptr;  // NULL PS for depth-only

    // Extract constant layout from VS (needed for FGConstantSystem)
    if (pso->vertexShader && pso->vertexShader->reflection) {
        pso->constantLayout = pso->vertexShader->reflection->constantLayout;
    }

    ExtractTextures(pass, pso.get());
    ExtractSamplers(pass, pso.get());

    // ═══════════════════════════════════════════════════════
    //  EXTRACT DETAIL SCALE (before PSO creation)
    // ═══════════════════════════════════════════════════════
    if (pass && pass->T && !pass->T->empty()) {
        // Get base texture name (usually slot 0)
        const shared_str& baseTexName = (*pass->T)[0].second;
        if (baseTexName.c_str() && baseTexName[0]) {
            pso->detail_scale = GetDetailScale(baseTexName);
        }
    }

    // ═══════════════════════════════════════════════════════
    //  BUILD DEPTH-ONLY PSO DESCRIPTOR
    // ═══════════════════════════════════════════════════════

    ng::PipelineStateDesc psoDesc;
    psoDesc.vertexShader = materialVS.Get();       // Material's VS (has correct constant layout)
    psoDesc.pixelShader = nullptr;                 // NULL PS for depth-only (D3D11 standard)

    // Vertex layout (same as material PSO)
    if (!ValidateVertexLayoutCompatibility(visual, pso.get())) {
        return nullptr;
    }
    SetupVertexAttributes(visual, pso.get(), psoDesc);

    // Depth-only render state
    psoDesc.renderTargetCount = 0;
    psoDesc.depthStencilFormat = nvrhi::Format::D32;
    SetupRenderStates(pass, psoDesc);

    // Override depth state (must be after SetupRenderStates)
    psoDesc.depthStencilState.depthTestEnable = true;
    psoDesc.depthStencilState.depthWriteEnable = true;
    psoDesc.depthStencilState.depthFunc = ng::ComparisonFunc::Less;
    psoDesc.depthStencilState.stencilEnable = false;

    // ═══════════════════════════════════════════════════════
    //  CREATE PIPELINE STATE VIA CACHE
    // ═══════════════════════════════════════════════════════

    ng::PipelineStateCache* psoCache = m_device->GetPipelineCache();
    if (!psoCache) {
        return nullptr;
    }

    ng::PipelineState* nvrhiPSO = psoCache->GetOrCreate(psoDesc);
    if (!nvrhiPSO) {
        Msg("! [MaterialCache::CreateDepthPSO] PSO creation failed");
        return nullptr;
    }

    pso->pso = nvrhiPSO;

    CreateBindingLayouts(pso.get());
    pso->debugName = shared_str(pass->vs._get() ? pass->vs._get()->cName.c_str() : "depth_pso");

    return pso.release();
}

MaterialPSO* MaterialCache::CreateUIPSO(
    Shader* shader,
    ShaderElement* elem,
    SPass* pass,
    nvrhi::IFramebuffer* framebuffer)
{
    auto pso = xr_make_unique<MaterialPSO>();
    pso->pass = pass;

    // Extract textures and samplers
    ExtractTextures(pass, pso.get());

    // Extract shaders (THIS MUST COME FIRST - populates rtBindings!)
    if (!ExtractShaders(pass, pso.get())) {
        Msg("! [MaterialCache::CreateUIPSO] Failed to extract shaders");
        return nullptr;
    }

    // ExtractSamplers MUST come after ExtractShaders (relies on rtBindings.samplers)
    ExtractSamplers(pass, pso.get());

    // ═══════════════════════════════════════════════════════
    //  EXTRACT CONSTANT LAYOUT (CB + PER-CONSTANT METADATA)
    // ═══════════════════════════════════════════════════════

    framegraph::ShaderConstantLayout vsLayout;
    framegraph::ShaderConstantLayout psLayout;

    // Extract VS constant layout
    if (pso->vertexShader && pso->vertexShader->reflection) {
        vsLayout = pso->vertexShader->reflection->constantLayout;
    }

    // Extract PS constant layout
    if (pso->pixelShader && pso->pixelShader->reflection) {
        psLayout = pso->pixelShader->reflection->constantLayout;
    }

    // ✅ CRITICAL FIX: Merge layouts with proper CB deduplication
    pso->constantLayout = MergeConstantLayouts(vsLayout, psLayout);

#ifdef DEBUG
    // Log detailed constant layout for debugging
    const char* shaderName = pso->vertexShader ? pso->vertexShader->cName.c_str() : "Unknown";
    LogConstantLayout(pso->constantLayout, shaderName);
#endif

    // Get native NVRHI shaders directly
    nvrhi::ShaderHandle nvrhiVS = GetOrCreateShaderVS(pso->vertexShader);
    nvrhi::ShaderHandle nvrhiPS = GetOrCreateShaderPS(pso->pixelShader);

    if (!nvrhiVS || !nvrhiPS) {
        Msg("! [MaterialCache::CreateUIPSO] Failed to get shader handles");
        return nullptr;
    }

    // Create binding layouts
    CreateBindingLayouts(pso.get());

    // Build pipeline state descriptor
    ng::PipelineStateDesc psoDesc;
    psoDesc.vertexShader = nvrhiVS.Get();  // Direct NVRHI shader pointer
    psoDesc.pixelShader = nvrhiPS.Get();   // No wrapper layer!

    // ═══════════════════════════════════════════════════════
    //  BUILD VERTEX ATTRIBUTES FROM SHADER REFLECTION
    // ═══════════════════════════════════════════════════════
    // Use shader's input signature to determine correct order and formats
    // UIVertex in-memory layout: float x,y,z,w (16 bytes), u32 color (4 bytes), float u,v (8 bytes)
    // - POSITIONT: offset 0, RGBA32_FLOAT (float4 - 16 bytes) - shader expects float4!
    // - COLOR: offset 16, RGBA8_UNORM (u32 - 4 bytes)
    // - TEXCOORD: offset 20, RG32_FLOAT (float2 - 8 bytes)

    if (pso->vsInputSignature.elements.empty()) {
        Msg("! [MaterialCache::CreateUIPSO] No vertex input signature from shader reflection!");
        return nullptr;
    }

    // Map semantic -> (format, offset) for UIVertex structure
    struct VertexElement {
        nvrhi::Format format;
        u32 offset;
    };

    std::map<std::string, VertexElement> uiVertexLayout;
    uiVertexLayout["POSITIONT"] = {nvrhi::Format::RGBA32_FLOAT, 0};   // float4 at offset 0 (16 bytes) - vanilla uses RGBA32!
    uiVertexLayout["COLOR"]      = {nvrhi::Format::RGBA8_UNORM, 16};  // u32 at offset 16 (4 bytes)
    uiVertexLayout["TEXCOORD"]   = {nvrhi::Format::RG32_FLOAT, 20};   // float2 at offset 20 (8 bytes)

    // Build attributes in shader-expected order (WITHOUT stride first)
    for (const auto& shaderElem : pso->vsInputSignature.elements) {
        std::string semantic = shaderElem.semanticName.c_str();

        auto it = uiVertexLayout.find(semantic);
        if (it == uiVertexLayout.end()) {
            Msg("! [MaterialCache::CreateUIPSO] Unknown UI vertex semantic: %s", semantic.c_str());
            continue;
        }

        ng::VertexAttribute attr;
        attr.semanticName = shaderElem.semanticName.c_str();
        attr.semanticIndex = shaderElem.semanticIndex;
        attr.format = it->second.format;
        attr.offset = it->second.offset;
        attr.bufferIndex = 0;
        attr.elementStride = 0;  // Will be set below after calculating stride

        psoDesc.vertexAttributes.push_back(attr);
    }

    // Calculate vertex stride from vertex attributes (CRITICAL for D3D11!)
    // Stride = max(offset + size) for all attributes in buffer slot 0
    u32 calculatedStride = 0;
    for (const auto& attr : psoDesc.vertexAttributes) {
        if (attr.bufferIndex == 0) {
            const nvrhi::FormatInfo& formatInfo = nvrhi::getFormatInfo(attr.format);
            u32 formatSize = formatInfo.bytesPerBlock;
            u32 endOffset = attr.offset + formatSize;
            calculatedStride = std::max(calculatedStride, endOffset);
        }
    }

    // Now set elementStride for ALL attributes
    for (auto& attr : psoDesc.vertexAttributes) {
        attr.elementStride = calculatedStride;
    }

    pso->vertexStride = calculatedStride;

    // UI render state - matches vanilla X-Ray:
    // Depth: enabled, always pass, no write (for stencil support)
    // Stencil: enabled (used by some UI shaders)
    // Blend: standard alpha blending for both color and alpha channels
    psoDesc.depthStencilState.depthTestEnable = true;
    psoDesc.depthStencilState.depthFunc = ng::ComparisonFunc::Always;  // Always pass depth test
    psoDesc.depthStencilState.depthWriteEnable = false;  // Don't write depth
    psoDesc.depthStencilState.stencilEnable = true;      // Enable stencil for UI effects

    // Standard premultiplied alpha blending (matches vanilla)
    psoDesc.blendState.renderTargets[0].blendEnable = true;
    psoDesc.blendState.renderTargets[0].srcBlend = ng::BlendFactor::SrcAlpha;
    psoDesc.blendState.renderTargets[0].dstBlend = ng::BlendFactor::InvSrcAlpha;
    psoDesc.blendState.renderTargets[0].srcBlendAlpha = ng::BlendFactor::SrcAlpha;  // FIXED: was One, should be SrcAlpha!
    psoDesc.blendState.renderTargets[0].dstBlendAlpha = ng::BlendFactor::InvSrcAlpha;

    psoDesc.rasterizerState.cullMode = ng::CullMode::None;
    psoDesc.rasterizerState.frontCounterClockwise = false;
    psoDesc.rasterizerState.scissorEnable = true;  // Enable scissor for UI clipping (maps, scrollviews, etc.)

    // Set render target formats from framebuffer
    const nvrhi::FramebufferDesc& fbDesc = framebuffer->getDesc();
    for (u32 i = 0; i < fbDesc.colorAttachments.size() && i < 8; ++i) {
        if (fbDesc.colorAttachments[i].texture) {
            psoDesc.renderTargetFormats[i] = fbDesc.colorAttachments[i].texture->getDesc().format;
        }
    }
    if (fbDesc.depthAttachment.texture) {
        psoDesc.depthStencilFormat = fbDesc.depthAttachment.texture->getDesc().format;
    }

    psoDesc.debugName = "UI_PSO";

    // Create pipeline state via cache
    ng::PipelineStateCache* psoCache = m_device->GetPipelineCache();
    if (!psoCache) {
        Msg("! [MaterialCache::CreateUIPSO] No PSO cache");
        return nullptr;
    }

    ng::PipelineState* nvrhiPSO = psoCache->GetOrCreate(psoDesc);
    if (!nvrhiPSO) {
        Msg("! [MaterialCache::CreateUIPSO] Failed to create pipeline state");
        return nullptr;
    }

    pso->pso = nvrhiPSO;

    return pso.release();
}

// ══════════════════════════════════════════════════════════
//  CLEAR CACHE
// ══════════════════════════════════════════════════════════

void MaterialCache::Clear()
{
    m_cache.clear();
    m_textureHandleCache.clear();  // Updated: uses resource handles now
    m_detailScaleCache.clear();
    m_shaderHandles.clear();  // Clear shader handle cache
    m_stats = Stats{};
}

// ══════════════════════════════════════════════════════════
//  SHADER HANDLE CACHING (STAGE-AWARE)
// ══════════════════════════════════════════════════════════

nvrhi::ShaderHandle MaterialCache::GetOrCreateShaderVS(SVS* vs)
{
    if (!vs) {
        return nullptr;  // Invalid handle
    }

    // CRITICAL: Include stage in cache key! VS and PS can have same name!
    xr_string vsKeyStr = xr_string("VS_") + vs->cName.c_str();
    shared_str vsKey = vsKeyStr.c_str();

    // Check cache
    auto it = m_shaderHandles.find(vsKey);
    if (it != m_shaderHandles.end()) {
        return it->second;  // Return cached handle
    }

    // Shaders must be compiled upfront now - no lazy compilation
    if (!vs->nvrhiShader)
    {
        Msg("! [MaterialCache] ERROR: VS '%s' has no nvrhiShader - shader not compiled upfront!", vs->cName.c_str());
        R_ASSERT2(false, "All shaders must be compiled during material loading");
        return nullptr;
    }

    // Use native NVRHI shader directly
    nvrhi::ShaderHandle vsHandle = vs->nvrhiShader;

    if (!vsHandle) {
        Msg("! [MaterialCache] ERROR: Invalid nvrhiShader for VS '%s'", vs->cName.c_str());
        return nullptr;
    }

    // Cache and return
    m_shaderHandles[vsKey] = vsHandle;
    return vsHandle;
}

nvrhi::ShaderHandle MaterialCache::GetOrCreateShaderPS(SPS* ps)
{
    if (!ps) {
        return nullptr;  // Invalid handle
    }

    // CRITICAL: Include stage in cache key! VS and PS can have same name!
    xr_string psKeyStr = xr_string("PS_") + ps->cName.c_str();
    shared_str psKey = psKeyStr.c_str();

    // Check cache
    auto it = m_shaderHandles.find(psKey);
    if (it != m_shaderHandles.end()) {
        return it->second;  // Return cached handle
    }

    // Shaders must be compiled upfront now - no lazy compilation
    if (!ps->nvrhiShader)
    {
        Msg("! [MaterialCache] ERROR: PS '%s' has no nvrhiShader - shader not compiled upfront!", ps->cName.c_str());
        R_ASSERT2(false, "All shaders must be compiled during material loading");
        return nullptr;
    }

    // Use native NVRHI shader directly
    nvrhi::ShaderHandle psHandle = ps->nvrhiShader;

    if (!psHandle) {
        Msg("! [MaterialCache] ERROR: Invalid nvrhiShader for PS '%s'", ps->cName.c_str());
        return nullptr;
    }

    // Cache and return
    m_shaderHandles[psKey] = psHandle;
    return psHandle;
}

// ══════════════════════════════════════════════════════════
//  GET DETAIL SCALE FROM TEXTURE METADATA
// ══════════════════════════════════════════════════════════

float MaterialCache::GetDetailScale(const shared_str& textureName)
{
    // Check cache first
    auto cacheIt = m_detailScaleCache.find(textureName.c_str());
    if (cacheIt != m_detailScaleCache.end()) {
        return cacheIt->second;
    }

    // Query TextureDescrManager via clean public API
    // This internally queries m_detail_scalers map (populated from .ltx files)
    float scale = RImplementation.Resources->m_textures_description.GetDetailScale(textureName);

    // Cache for next time
    m_detailScaleCache[textureName.c_str()] = scale;

    return scale;
}

} // namespace xray::render
