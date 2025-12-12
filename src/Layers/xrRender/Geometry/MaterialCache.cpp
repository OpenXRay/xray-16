// xrRender/Geometry/MaterialCache.cpp
#include "stdafx.h"
#include "MaterialCache.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/SH_Texture.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/dxUIShader.h"  // For dxUIShader NVRHI handles
#include "Layers/xrRender/dxFontRender.h"  // For dxFontRender NVRHI handles
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
#include "Layers/xrRender/Bindless/MaterialBuffer.h"          // Bindless material buffer
// SM6 bindless texture registration uses GEnv.Backend->RegisterBindlessTexture()
#include "xrEngine/IRenderBackend.h"                          // For IRenderBackend
#include "Layers/xrRender/r_constants.h"                      // For R_constant_setup
#include "Layers/xrRender/Materials/MaterialSystem.h"         // For MaterialSystem (D3D12)
#include "xrEngine/xr_object.h"                               // For GEnv

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

    // Create 1x1 RGBA8 default PBR texture with appropriate values:
    // R = Metallic:  0   (dielectric)
    // G = Roughness: 255 (fully rough, safer default)
    // B = AO:        255 (no occlusion)
    // A = Parallax:  128 (neutral height, no displacement)

    resources::TextureDesc desc;
    desc.type = resources::TextureDesc::Texture2D;
    desc.width = 1;
    desc.height = 1;
    desc.mipLevels = 1;
    desc.format = nvrhi::Format::RGBA8_UNORM;  // Packed PBR format
    desc.debugName = "$default_pbr";

    // RGBA8 pixel: R=0 (metallic), G=255 (rough), B=255 (ao), A=128 (parallax)
    u8 pbrPixel[4] = { 0, 255, 255, 128 };
    m_defaultPBR = texManager->CreateTexture(desc, pbrPixel);
    if (m_defaultPBR.IsValid()) {
        m_textureHandleCache["$default_pbr"] = m_defaultPBR;
    }

    Msg("* [MaterialCache] Created default PBR texture");
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

    // ═══════════════════════════════════════════════════
    //  D3D12: FAST PATH - Precompiled PSO Lookup
    // ═══════════════════════════════════════════════════
    if (GEnv.Backend && GEnv.Backend->GetAPI() == IRenderBackend::API::D3D12) {
        // Check if this is level geometry with precompiled PSOs
        u32 shaderID = visual->shader_id;
        if (shaderID != UINT32_MAX) {
            auto* compiled = RImplementation.getCompiledShader(shaderID);
            if (compiled) {
                // Extract vertex format from visual
                u32 vertexFormatID = GetVertexFormatID(visual);

                // Compute cache key
                u64 cacheKey = RImplementation.ComputePSOCacheKey(vertexFormatID, passType);

                // Look up precompiled PSO
                auto it = compiled->precompiledPSOs.psoCache.find(cacheKey);
                if (it != compiled->precompiledPSOs.psoCache.end() && it->second) {
                    // ✅ CACHE HIT! Return precompiled PSO
                    m_stats.numCacheHits++;
                    return it->second;
                }

                // Cache miss - PSO not precompiled for this format combination
                m_stats.numCacheMisses++;
                Msg("! [MaterialCache] PSO cache miss for shader %u (format %u, pass %u)",
                    shaderID, vertexFormatID, (u32)passType);
            }
        }

        // Fallback: use bindless pipeline (dynamic objects, or cache miss)
        return nullptr;
    }

    // Legacy D3D11 path below
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

    // D3D12/FrameGraph mode: Return null - uses bindless depth prepass instead
    if (GEnv.Backend && GEnv.Backend->GetAPI() == IRenderBackend::API::D3D12) {
        return nullptr;
    }

    // Legacy D3D11 path below
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

    // CRITICAL FIX: NVRHI creates NEW layout objects internally when creating a pipeline.
    // We must use the layouts FROM the pipeline, not the ones we created!
    nvrhi::IGraphicsPipeline* nativePipeline = nvrhiPSO->GetNativePipeline();
    if (nativePipeline) {
        const nvrhi::GraphicsPipelineDesc& actualDesc = nativePipeline->getDesc();
        if (actualDesc.bindingLayouts.size() >= 1) {
            pso->vsBindingLayout = actualDesc.bindingLayouts[0];
            if (actualDesc.bindingLayouts.size() >= 2) {
                pso->psBindingLayout = actualDesc.bindingLayouts[1];
            }
        }
    }

    // Register with bindless system for GPU-driven rendering
    // This assigns a material ID that will be used during geometry upload
    RegisterBindlessMaterial(pso.get());

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

            // Load consolidated PBR texture (R=metallic, G=roughness, B=ao, A=parallax)
            resources::TextureHandle pbrHandle;
            shared_str pbrTexName = texDescMgr.GetPBRName(baseTexName);

            if (!pbrTexName.empty()) {
                // Check cache
                auto cacheIt = m_textureHandleCache.find(pbrTexName.c_str());
                if (cacheIt != m_textureHandleCache.end()) {
                    pbrHandle = cacheIt->second;
                } else {
                    // Load texture
                    pbrHandle = texManager->LoadTexture(
                        pbrTexName.c_str(),
                        resources::TexturePriority::High
                    );
                    if (pbrHandle.IsValid()) {
                        m_textureHandleCache[pbrTexName.c_str()] = pbrHandle;
                    }
                }
            }

            // Use default PBR texture if no explicit texture available
            if (!pbrHandle.IsValid()) {
                pbrHandle = m_defaultPBR;
            }

            if (pbrHandle.IsValid()) {
                MaterialPSO::TextureSlot texSlot;
                texSlot.slot = RENDER_NAMESPACE::passes::TEX_SLOT_PBR;
                texSlot.handle = pbrHandle;
                matPSO->textures.push_back(texSlot);
            }
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
        bufferDesc.keepInitialState = true;  // D3D12 requires state tracking
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
    // CRITICAL: Must sort by slot to match binding set order in GetOrCreateBindingSet
    u32 texCount = 0;
    if (stage == MaterialPSO::ShaderStage::Pixel) {
        // Sort textures by slot before adding to layout
        xr_vector<MaterialPSO::TextureSlot> sortedTextures(matPSO->textures);
        std::sort(sortedTextures.begin(), sortedTextures.end(),
            [](const MaterialPSO::TextureSlot& a, const MaterialPSO::TextureSlot& b) {
                return a.slot < b.slot;
            });

        for (const auto& texSlot : sortedTextures) {
            layoutDesc.bindings.push_back(
                nvrhi::BindingLayoutItem::Texture_SRV(texSlot.slot));
            texCount++;
        }
    }

    // Samplers (s0, s1, s2, ...) - currently only in PS
    // CRITICAL: Must sort by slot to match binding set order in GetOrCreateBindingSet
    u32 samplerCount = 0;
    if (stage == MaterialPSO::ShaderStage::Pixel) {
        // Collect samplers for this stage
        xr_vector<MaterialPSO::SamplerInfo> stageSamplers;
        for (const auto& samplerInfo : matPSO->samplers) {
            if (samplerInfo.stage == stage) {
                stageSamplers.push_back(samplerInfo);
            }
        }

        // Sort samplers by slot before adding to layout
        std::sort(stageSamplers.begin(), stageSamplers.end(),
            [](const MaterialPSO::SamplerInfo& a, const MaterialPSO::SamplerInfo& b) {
                return a.slot < b.slot;
            });

        for (const auto& samplerInfo : stageSamplers) {
            layoutDesc.bindings.push_back(
                nvrhi::BindingLayoutItem::Sampler(samplerInfo.slot));
            samplerCount++;
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
            // Texture not ready - binding set creation will fail, so return early
            Msg("! [MaterialCache::GetOrCreateBindingSet] Texture not loaded yet (slot t%u), cannot create binding set", texSlot.slot);
            return nullptr;
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

    // CRITICAL: SORT by slot number AND type to match layout order!
    // CreateStageBindingLayout sorts ALL CBs by slot, then adds textures, then samplers.
    // NVRHI matches bindings by INDEX, so binding set order MUST match layout order (b0, b1, ..., t0, t1, ..., s0, s1, ...)
    std::sort(psBindings.begin(), psBindings.end(), [](const PSBinding& a, const PSBinding& b) {
        // First sort by type (CB < Texture < Sampler) to match CreateStageBindingLayout order
        if (a.type != b.type) return a.type < b.type;
        // Then sort by slot within each type
        return a.slot < b.slot;
    });

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
        Msg("! [MaterialCache::GetOrCreateBindingSet] Failed to create VS binding set");
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  CREATE PS BINDING SET
    // ═══════════════════════════════════════════════════════

    nvrhi::BindingSetHandle psBindingSet = m_device->CreateBindingSet(
        psBindingDesc,
        matPSO->psBindingLayout);

    if (!psBindingSet) {
        Msg("! [MaterialCache::GetOrCreateBindingSet] Failed to create PS binding set");
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

// D3D12: Extract vertex format ID from visual's geometry
u32 MaterialCache::GetVertexFormatID(dxRender_Visual* visual)
{
    if (!visual)
        return 0;

    // For level geometry, the VB index is stored in the visual
    // The vertex format ID corresponds to the nDC/xDC index
    // For now, return 0 (most common format) - will be refined when we add proper VB tracking
    // TODO: Extract actual VB declaration ID from visual's geometry
    return 0;  // Default to first format
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
    IUIShader* uiShader,
    u32 elementIndex,
    nvrhi::IFramebuffer* framebuffer)
{
    if (!uiShader || !framebuffer)
        return nullptr;

    // Cast to dxUIShader to access NVRHI handles
    dxUIShader* dxShader = static_cast<dxUIShader*>(uiShader);
    if (!dxShader)
        return nullptr;

    // Create cache key using VS/PS handle pointers + texture name
    // ShaderLoader caches shaders internally, so same shader bytecode = same handle pointer
    // Example: All UI elements using "hud\default" share the same VS/PS handles
    // Only the texture differs, so we hash: (VS ptr, PS ptr, texture name)

    u64 shaderHash = 0;
    if (dxShader->m_vsHandle && dxShader->m_psHandle) {
        // Combine VS pointer + PS pointer into hash
        shaderHash = reinterpret_cast<uintptr_t>(dxShader->m_vsHandle.Get()) ^
                     (reinterpret_cast<uintptr_t>(dxShader->m_psHandle.Get()) << 1);
    }

    // Hash texture name (what actually differs between UI shader instances)
    u64 textureHash = 0;
    if (dxShader->m_baseTexture && dxShader->m_baseTexture->cName.size() > 0) {
        textureHash = std::hash<xr_string>{}(xr_string(dxShader->m_baseTexture->cName.c_str()));
    }

    // Combine shader hash + texture hash
    u64 combinedHash = shaderHash ^ (textureHash << 2);

    MaterialKey key;
    key.psoType = PSOType::UI;
    key.shader = nullptr;  // Not used for UI
    key.textureHash = combinedHash;  // VS/PS handles + texture name
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
    MaterialPSO* pso = CreateUIPSO(uiShader, nullptr, nullptr, framebuffer);
    if (!pso)
        return nullptr;

    m_cache[key] = xr_unique_ptr<MaterialPSO>(pso);
    m_stats.numCachedPSOs = static_cast<u32>(m_cache.size());

    return pso;
}

// ══════════════════════════════════════════════════════════
//  GET OR CREATE FONT PSO
// ══════════════════════════════════════════════════════════

MaterialPSO* MaterialCache::GetOrCreateFontPSO(
    dxFontRender* fontRender,
    nvrhi::IFramebuffer* framebuffer)
{
    if (!fontRender || !framebuffer)
        return nullptr;

    // Create cache key using VS/PS handle pointers + texture name
    // Fonts use stub_notransform_t.vs + hud_font.ps, with different textures

    u64 shaderHash = 0;
    if (fontRender->m_vsHandle && fontRender->m_psHandle) {
        // Combine VS pointer + PS pointer into hash
        shaderHash = reinterpret_cast<uintptr_t>(fontRender->m_vsHandle.Get()) ^
                     (reinterpret_cast<uintptr_t>(fontRender->m_psHandle.Get()) << 1);
    }

    // Hash texture name (what actually differs between font instances)
    u64 textureHash = 0;
    if (fontRender->m_baseTexture && fontRender->m_baseTexture->cName.size() > 0) {
        textureHash = std::hash<xr_string>{}(xr_string(fontRender->m_baseTexture->cName.c_str()));
    }

    // Combine shader hash + texture hash
    u64 combinedHash = shaderHash ^ (textureHash << 2);

    MaterialKey key;
    key.psoType = PSOType::UI;  // Fonts use same PSO type as UI (same vertex layout)
    key.shader = nullptr;  // Not used for fonts
    key.textureHash = combinedHash;  // VS/PS handles + texture name
    key.element = 0;  // Fonts always use element 0
    key.framebuffer = framebuffer;

    // Check cache
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_stats.numCacheHits++;
        return it->second.get();
    }

    // Create new font PSO (reuse CreateUIPSO implementation)
    m_stats.numCacheMisses++;
    m_stats.totalPSOCreations++;
    MaterialPSO* pso = CreateFontPSO(fontRender, framebuffer);
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

    // CRITICAL FIX: Get layouts FROM the pipeline, not by creating new ones
    nvrhi::IGraphicsPipeline* nativePipeline = nvrhiPSO->GetNativePipeline();
    if (nativePipeline) {
        const nvrhi::GraphicsPipelineDesc& actualDesc = nativePipeline->getDesc();
        if (actualDesc.bindingLayouts.size() >= 1) {
            pso->vsBindingLayout = actualDesc.bindingLayouts[0];
            if (actualDesc.bindingLayouts.size() >= 2) {
                pso->psBindingLayout = actualDesc.bindingLayouts[1];
            }
        }
    }

    pso->debugName = shared_str(pass->vs._get() ? pass->vs._get()->cName.c_str() : "depth_pso");

    return pso.release();
}

MaterialPSO* MaterialCache::CreateUIPSO(
    IUIShader* uiShader,
    ShaderElement* elem,
    SPass* pass,
    nvrhi::IFramebuffer* framebuffer)
{
    auto pso = xr_make_unique<MaterialPSO>();
    // Note: pso->pass is nullptr for DX12 (no legacy shader system)

    // Cast to dxUIShader to access NVRHI handles
    dxUIShader* dxShader = static_cast<dxUIShader*>(uiShader);
    if (!dxShader) {
        Msg("! [MaterialCache::CreateUIPSO] Invalid uiShader pointer");
        return nullptr;
    }

    // Get NVRHI shader handles directly from dxUIShader (compiled in dxUIShader::create)
    nvrhi::ShaderHandle nvrhiVS = dxShader->m_vsHandle;
    nvrhi::ShaderHandle nvrhiPS = dxShader->m_psHandle;

    if (!nvrhiVS || !nvrhiPS) {
        Msg("! [MaterialCache::CreateUIPSO] Missing NVRHI shader handles in dxUIShader (VS=%p PS=%p)",
            nvrhiVS.Get(), nvrhiPS.Get());
        return nullptr;
    }

    // Extract reflection from dxUIShader
    if (dxShader->m_vsReflection) {
        pso->vsInputSignature = framegraph::ShaderReflector::GetVertexInputSignature(dxShader->m_vsReflection);
        pso->constantLayout = dxShader->m_vsReflection->constantLayout;

        // Extract VS constant buffers from constantLayout
        for (const auto& cb : dxShader->m_vsReflection->constantLayout.constantBuffers.buffers) {
            MaterialPSO::ConstantBufferInfo cbInfo;
            cbInfo.name = cb.name.c_str();
            cbInfo.slot = cb.slot;
            cbInfo.size = cb.size;
            cbInfo.stage = MaterialPSO::ShaderStage::Vertex;
            pso->constantBuffers.push_back(cbInfo);
        }
    }
    else {
        Msg("! [MaterialCache::CreateUIPSO] No VS reflection data available");
    }

    // Extract PS reflection
    if (dxShader->m_psReflection) {
        // Extract PS constant buffers from constantLayout
        for (const auto& cb : dxShader->m_psReflection->constantLayout.constantBuffers.buffers) {
            MaterialPSO::ConstantBufferInfo cbInfo;
            cbInfo.name = cb.name.c_str();
            cbInfo.slot = cb.slot;
            cbInfo.size = cb.size;
            cbInfo.stage = MaterialPSO::ShaderStage::Pixel;
            pso->constantBuffers.push_back(cbInfo);
        }

        // Extract PS textures from rtBindings and load them
        for (const auto& tex : dxShader->m_psReflection->rtBindings.inputTextures) {
            // Load texture from UI shader
            // For UI shaders, s_base is the main texture (from the shader's "tex" parameter)
            if (xr_strcmp(tex.name.c_str(), "s_base") != 0)
                break;

            CTexture* baseTexture = dxShader->GetBaseTexture();
            if (baseTexture) {
                // Get NVRHI texture handle from texture manager (it will load the texture if needed)
                resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
                resources::TextureHandle texHandle = texManager->LoadTexture(baseTexture->cName.c_str());

                if (!texHandle.IsValid()) {
                    Msg("! [MaterialCache::CreateUIPSO] Failed to load texture: %s", baseTexture->cName.c_str());
                } else {
                    // Only add to textures list if we successfully loaded it
                    MaterialPSO::TextureSlot texSlot;
                    texSlot.slot = tex.slot;
                    texSlot.handle = texHandle;
                    pso->textures.push_back(texSlot);
                }
            }
        }

        // Extract PS samplers from rtBindings and create NVRHI sampler objects
        for (const auto& samp : dxShader->m_psReflection->rtBindings.samplers) {
            MaterialPSO::SamplerInfo sampInfo;
            sampInfo.name = samp.name.c_str();
            sampInfo.slot = samp.slot;
            sampInfo.stage = MaterialPSO::ShaderStage::Pixel;

            // Create NVRHI sampler based on X-Ray sampler naming convention
            // smp_base -> anisotropic filter, wrap
            // smp_rtlinear -> linear filter, clamp
            // smp_nofilter -> point filter, clamp
            nvrhi::SamplerDesc samplerDesc;
            if (strstr(samp.name.c_str(), "smp_base")) {
                samplerDesc.minFilter = true;
                samplerDesc.magFilter = true;
                samplerDesc.mipFilter = true;
                samplerDesc.maxAnisotropy = 16;
                samplerDesc.addressU = nvrhi::SamplerAddressMode::Wrap;
                samplerDesc.addressV = nvrhi::SamplerAddressMode::Wrap;
                samplerDesc.addressW = nvrhi::SamplerAddressMode::Wrap;
            } else if (strstr(samp.name.c_str(), "smp_linear") || strstr(samp.name.c_str(), "smp_rtlinear")) {
                samplerDesc.minFilter = true;
                samplerDesc.magFilter = true;
                samplerDesc.mipFilter = true;
                samplerDesc.addressU = nvrhi::SamplerAddressMode::Clamp;
                samplerDesc.addressV = nvrhi::SamplerAddressMode::Clamp;
                samplerDesc.addressW = nvrhi::SamplerAddressMode::Clamp;
            } else if (strstr(samp.name.c_str(), "smp_nofilter")) {
                samplerDesc.minFilter = false;
                samplerDesc.magFilter = false;
                samplerDesc.mipFilter = false;
                samplerDesc.addressU = nvrhi::SamplerAddressMode::Clamp;
                samplerDesc.addressV = nvrhi::SamplerAddressMode::Clamp;
                samplerDesc.addressW = nvrhi::SamplerAddressMode::Clamp;
            } else {
                // Default: linear filter, wrap
                samplerDesc.minFilter = true;
                samplerDesc.magFilter = true;
                samplerDesc.mipFilter = true;
                samplerDesc.addressU = nvrhi::SamplerAddressMode::Wrap;
                samplerDesc.addressV = nvrhi::SamplerAddressMode::Wrap;
                samplerDesc.addressW = nvrhi::SamplerAddressMode::Wrap;
            }

            sampInfo.nvrhiSampler = m_device->GetNVRHIDevice()->createSampler(samplerDesc);
            if (!sampInfo.nvrhiSampler) {
                Msg("! [MaterialCache::CreateUIPSO] Failed to create sampler: %s", samp.name.c_str());
            }

            pso->samplers.push_back(sampInfo);
        }
    }

    // Create NVRHI buffers for all constant buffers
    // Group constant buffers by name to create shared buffers for VS+PS
    xr_map<shared_str, nvrhi::BufferHandle> createdBuffers;

    for (auto& cbInfo : pso->constantBuffers) {
        // Check if we already created a buffer for this CB name
        auto it = createdBuffers.find(cbInfo.name);
        if (it != createdBuffers.end()) {
            // Reuse existing buffer
            cbInfo.nvrhiBuffer = it->second;
            continue;
        }

        // Create new buffer
        nvrhi::BufferDesc bufferDesc;
        bufferDesc.byteSize = cbInfo.size;
        bufferDesc.isConstantBuffer = true;
        bufferDesc.debugName = make_string("UI_CB_%s", cbInfo.name.c_str()).c_str();
        bufferDesc.keepInitialState = true;
        bufferDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

        nvrhi::BufferHandle buffer = m_device->GetNVRHIDevice()->createBuffer(bufferDesc);
        if (!buffer) {
            Msg("! [MaterialCache::CreateUIPSO] Failed to create constant buffer: %s", cbInfo.name.c_str());
            continue;
        }

        cbInfo.nvrhiBuffer = buffer;
        createdBuffers[cbInfo.name] = buffer;
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
    uiVertexLayout["POSITION"]  = {nvrhi::Format::RGBA32_FLOAT, 0};   // float4 at offset 0 (16 bytes)
    uiVertexLayout["POSITIONT"] = {nvrhi::Format::RGBA32_FLOAT, 0};   // Alias for POSITION
    uiVertexLayout["COLOR"]     = {nvrhi::Format::RGBA8_UNORM, 16};   // u32 at offset 16 (4 bytes)
    uiVertexLayout["TEXCOORD"]  = {nvrhi::Format::RG32_FLOAT, 20};    // float2 at offset 20 (8 bytes)

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

    // Set render target formats from framebuffer FIRST
    const nvrhi::FramebufferDesc& fbDesc = framebuffer->getDesc();
    psoDesc.renderTargetCount = static_cast<u32>(fbDesc.colorAttachments.size());
    for (u32 i = 0; i < fbDesc.colorAttachments.size() && i < 8; ++i) {
        if (fbDesc.colorAttachments[i].texture) {
            psoDesc.renderTargetFormats[i] = fbDesc.colorAttachments[i].texture->getDesc().format;
        }
    }

    // UI render state:
    // Depth/Stencil: Only enable if framebuffer has depth
    // Blend: standard alpha blending
    bool hasDepth = (fbDesc.depthAttachment.texture != nullptr);
    if (hasDepth) {
        psoDesc.depthStencilFormat = fbDesc.depthAttachment.texture->getDesc().format;
        psoDesc.depthStencilState.depthTestEnable = true;
        psoDesc.depthStencilState.depthFunc = ng::ComparisonFunc::Always;  // Always pass
        psoDesc.depthStencilState.depthWriteEnable = false;  // Don't write depth
        psoDesc.depthStencilState.stencilEnable = true;      // Enable stencil for UI effects
    } else {
        psoDesc.depthStencilState.depthTestEnable = false;
        psoDesc.depthStencilState.depthWriteEnable = false;
        psoDesc.depthStencilState.stencilEnable = false;
    }

    // Standard premultiplied alpha blending (matches vanilla)
    psoDesc.blendState.renderTargets[0].blendEnable = true;
    psoDesc.blendState.renderTargets[0].srcBlend = ng::BlendFactor::SrcAlpha;
    psoDesc.blendState.renderTargets[0].dstBlend = ng::BlendFactor::InvSrcAlpha;
    psoDesc.blendState.renderTargets[0].srcBlendAlpha = ng::BlendFactor::SrcAlpha;
    psoDesc.blendState.renderTargets[0].dstBlendAlpha = ng::BlendFactor::InvSrcAlpha;

    psoDesc.rasterizerState.cullMode = ng::CullMode::None;
    psoDesc.rasterizerState.frontCounterClockwise = false;
    psoDesc.rasterizerState.scissorEnable = true;  // Enable scissor for UI clipping

    // Set primitive topology (UI uses triangle lists) - already defaults to TriangleList but being explicit
    psoDesc.primitiveTopology = ng::PrimitiveTopology::TriangleList;

    // Use binding layouts created by CreateBindingLayouts() from shader reflection
    // These layouts were built from the shader's actual resource declarations
    if (pso->vsBindingLayout) {
        psoDesc.bindingLayouts.push_back(pso->vsBindingLayout);
    }
    if (pso->psBindingLayout) {
        psoDesc.bindingLayouts.push_back(pso->psBindingLayout);
    }

    if (psoDesc.bindingLayouts.empty()) {
        Msg("! [MaterialCache::CreateUIPSO] No binding layouts created from shader reflection!");
        return nullptr;
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

    // CRITICAL FIX: NVRHI creates NEW layout objects internally when creating a pipeline.
    // We must use the layouts FROM the pipeline, not the ones we created!
    nvrhi::IGraphicsPipeline* nativePipeline = nvrhiPSO->GetNativePipeline();
    if (nativePipeline) {
        const nvrhi::GraphicsPipelineDesc& actualDesc = nativePipeline->getDesc();
        if (actualDesc.bindingLayouts.size() >= 2) {
            pso->vsBindingLayout = actualDesc.bindingLayouts[0];
            pso->psBindingLayout = actualDesc.bindingLayouts[1];
        }
    }

    return pso.release();
}

// ══════════════════════════════════════════════════════════
//  CREATE FONT PSO
// ══════════════════════════════════════════════════════════

MaterialPSO* MaterialCache::CreateFontPSO(
    dxFontRender* fontRender,
    nvrhi::IFramebuffer* framebuffer)
{
    // Fonts use the same structure as UI - just call CreateUIPSO with a wrapper
    // We don't have an IUIShader*, but we can pass dxFontRender as the first param since
    // CreateUIPSO casts it to dxUIShader* anyway (and both share NVRHI shader handles)

    // However, this won't work because dxFontRender != dxUIShader hierarchy
    // So we need to implement font PSO creation inline here

    auto pso = xr_make_unique<MaterialPSO>();

    if (!fontRender) {
        Msg("! [MaterialCache::CreateFontPSO] Invalid fontRender pointer");
        return nullptr;
    }

    // Get NVRHI shader handles directly from dxFontRender
    nvrhi::ShaderHandle nvrhiVS = fontRender->m_vsHandle;
    nvrhi::ShaderHandle nvrhiPS = fontRender->m_psHandle;

    if (!nvrhiVS || !nvrhiPS) {
        Msg("! [MaterialCache::CreateFontPSO] Missing NVRHI shader handles in dxFontRender (VS=%p PS=%p)",
            nvrhiVS.Get(), nvrhiPS.Get());
        return nullptr;
    }

    // Extract reflection from dxFontRender (same structure as dxUIShader)
    if (fontRender->m_vsReflection) {
        pso->vsInputSignature = framegraph::ShaderReflector::GetVertexInputSignature(fontRender->m_vsReflection);
        pso->constantLayout = fontRender->m_vsReflection->constantLayout;

        // Extract VS constant buffers
        for (const auto& cb : fontRender->m_vsReflection->constantLayout.constantBuffers.buffers) {
            MaterialPSO::ConstantBufferInfo cbInfo;
            cbInfo.name = cb.name.c_str();
            cbInfo.slot = cb.slot;
            cbInfo.size = cb.size;
            cbInfo.stage = MaterialPSO::ShaderStage::Vertex;
            pso->constantBuffers.push_back(cbInfo);
        }
    }

    // Extract PS reflection
    if (fontRender->m_psReflection) {
        // Extract PS constant buffers
        for (const auto& cb : fontRender->m_psReflection->constantLayout.constantBuffers.buffers) {
            MaterialPSO::ConstantBufferInfo cbInfo;
            cbInfo.name = cb.name.c_str();
            cbInfo.slot = cb.slot;
            cbInfo.size = cb.size;
            cbInfo.stage = MaterialPSO::ShaderStage::Pixel;
            pso->constantBuffers.push_back(cbInfo);
        }

        // Extract PS textures - load font texture
        for (const auto& tex : fontRender->m_psReflection->rtBindings.inputTextures) {
            if (xr_strcmp(tex.name.c_str(), "s_base") != 0)
                continue;

            CTexture* baseTexture = fontRender->m_baseTexture;
            if (baseTexture) {
                // Get NVRHI texture handle from texture manager
                resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
                resources::TextureHandle texHandle = texManager->LoadTexture(baseTexture->cName.c_str());

                if (texHandle.IsValid()) {
                    MaterialPSO::TextureSlot texSlot;
                    texSlot.slot = tex.slot;
                    texSlot.handle = texHandle;
                    pso->textures.push_back(texSlot);
                }
            }
        }

        // Extract PS samplers (same as UI)
        for (const auto& samp : fontRender->m_psReflection->rtBindings.samplers) {
            MaterialPSO::SamplerInfo sampInfo;
            sampInfo.name = samp.name.c_str();
            sampInfo.slot = samp.slot;
            sampInfo.stage = MaterialPSO::ShaderStage::Pixel;

            nvrhi::SamplerDesc samplerDesc;
            samplerDesc.minFilter = true;
            samplerDesc.magFilter = true;
            samplerDesc.mipFilter = true;
            samplerDesc.addressU = nvrhi::SamplerAddressMode::Wrap;
            samplerDesc.addressV = nvrhi::SamplerAddressMode::Wrap;
            samplerDesc.addressW = nvrhi::SamplerAddressMode::Wrap;

            sampInfo.nvrhiSampler = m_device->GetNVRHIDevice()->createSampler(samplerDesc);
            pso->samplers.push_back(sampInfo);
        }
    }

    // Create NVRHI buffers for constant buffers
    xr_map<shared_str, nvrhi::BufferHandle> createdBuffers;

    for (auto& cbInfo : pso->constantBuffers) {
        auto it = createdBuffers.find(cbInfo.name);
        if (it != createdBuffers.end()) {
            cbInfo.nvrhiBuffer = it->second;
            continue;
        }

        nvrhi::BufferDesc cbDesc;
        cbDesc.byteSize = cbInfo.size;
        cbDesc.isConstantBuffer = true;
        cbDesc.debugName = cbInfo.name.c_str();
        cbDesc.isVolatile = false;
        cbDesc.keepInitialState = true;
        cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

        cbInfo.nvrhiBuffer = m_device->GetNVRHIDevice()->createBuffer(cbDesc);
        createdBuffers[cbInfo.name] = cbInfo.nvrhiBuffer;
    }

    // Create binding layouts
    CreateBindingLayouts(pso.get());

    // Create graphics PSO descriptor
    ng::PipelineStateDesc psoDesc;
    psoDesc.vertexShader = nvrhiVS.Get();
    psoDesc.pixelShader = nvrhiPS.Get();

    // Set vertex input layout (same as UI - FVF::TL format)
    // Map semantic -> (format, offset) for FVF::TL (same as UIVertex structure)
    struct VertexElement {
        nvrhi::Format format;
        u32 offset;
    };

    std::map<std::string, VertexElement> fontVertexLayout;
    fontVertexLayout["POSITION"]  = {nvrhi::Format::RGBA32_FLOAT, 0};   // float4 at offset 0 (16 bytes)
    fontVertexLayout["POSITIONT"] = {nvrhi::Format::RGBA32_FLOAT, 0};   // Alias for POSITION
    fontVertexLayout["COLOR"]     = {nvrhi::Format::RGBA8_UNORM, 16};   // u32 at offset 16 (4 bytes)
    fontVertexLayout["TEXCOORD"]  = {nvrhi::Format::RG32_FLOAT, 20};    // float2 at offset 20 (8 bytes)

    // Build attributes in shader-expected order
    for (const auto& shaderElem : pso->vsInputSignature.elements) {
        std::string semantic = shaderElem.semanticName.c_str();

        auto it = fontVertexLayout.find(semantic);
        if (it == fontVertexLayout.end()) {
            Msg("! [MaterialCache::CreateFontPSO] Unknown font vertex semantic: %s", semantic.c_str());
            continue;
        }

        ng::VertexAttribute attr;
        attr.semanticName = shaderElem.semanticName.c_str();
        attr.semanticIndex = shaderElem.semanticIndex;
        attr.format = it->second.format;
        attr.offset = it->second.offset;
        attr.bufferIndex = 0;
        attr.elementStride = 0;  // Will be set below

        psoDesc.vertexAttributes.push_back(attr);
    }

    // Calculate vertex stride
    u32 calculatedStride = 0;
    for (const auto& attr : psoDesc.vertexAttributes) {
        if (attr.bufferIndex == 0) {
            const nvrhi::FormatInfo& formatInfo = nvrhi::getFormatInfo(attr.format);
            u32 formatSize = formatInfo.bytesPerBlock;
            u32 endOffset = attr.offset + formatSize;
            calculatedStride = std::max(calculatedStride, endOffset);
        }
    }

    for (auto& attr : psoDesc.vertexAttributes) {
        attr.elementStride = calculatedStride;
    }

    pso->vertexStride = calculatedStride;

    // Set render target formats from framebuffer
    const nvrhi::FramebufferDesc& fbDesc = framebuffer->getDesc();
    psoDesc.renderTargetCount = static_cast<u32>(fbDesc.colorAttachments.size());
    for (u32 i = 0; i < fbDesc.colorAttachments.size() && i < 8; ++i) {
        if (fbDesc.colorAttachments[i].texture) {
            psoDesc.renderTargetFormats[i] = fbDesc.colorAttachments[i].texture->getDesc().format;
        }
    }

    // Font render state (same as UI)
    psoDesc.depthStencilState.depthTestEnable = false;
    psoDesc.depthStencilState.depthWriteEnable = false;
    psoDesc.depthStencilState.stencilEnable = false;

    psoDesc.blendState.alphaToCoverageEnable = false;
    psoDesc.blendState.renderTargets[0].blendEnable = true;
    psoDesc.blendState.renderTargets[0].srcBlend = ng::BlendFactor::SrcAlpha;
    psoDesc.blendState.renderTargets[0].dstBlend = ng::BlendFactor::InvSrcAlpha;
    psoDesc.blendState.renderTargets[0].blendOp = ng::BlendOp::Add;
    psoDesc.blendState.renderTargets[0].srcBlendAlpha = ng::BlendFactor::SrcAlpha;
    psoDesc.blendState.renderTargets[0].dstBlendAlpha = ng::BlendFactor::InvSrcAlpha;

    psoDesc.rasterizerState.cullMode = ng::CullMode::None;
    psoDesc.rasterizerState.frontCounterClockwise = false;
    psoDesc.rasterizerState.scissorEnable = true;

    psoDesc.primitiveTopology = ng::PrimitiveTopology::TriangleList;

    if (pso->vsBindingLayout) {
        psoDesc.bindingLayouts.push_back(pso->vsBindingLayout);
    }
    if (pso->psBindingLayout) {
        psoDesc.bindingLayouts.push_back(pso->psBindingLayout);
    }

    if (psoDesc.bindingLayouts.empty()) {
        Msg("! [MaterialCache::CreateFontPSO] No binding layouts created!");
        return nullptr;
    }

    psoDesc.debugName = "Font_PSO";

    ng::PipelineStateCache* psoCache = m_device->GetPipelineCache();
    if (!psoCache) {
        Msg("! [MaterialCache::CreateFontPSO] No PSO cache");
        return nullptr;
    }

    ng::PipelineState* nvrhiPSO = psoCache->GetOrCreate(psoDesc);
    if (!nvrhiPSO) {
        Msg("! [MaterialCache::CreateFontPSO] Failed to create pipeline state");
        return nullptr;
    }

    pso->pso = nvrhiPSO;

    // CRITICAL FIX: NVRHI creates NEW layout objects internally when creating a pipeline.
    // We must use the layouts FROM the pipeline, not the ones we created!
    nvrhi::IGraphicsPipeline* nativePipeline = nvrhiPSO->GetNativePipeline();
    if (nativePipeline) {
        const nvrhi::GraphicsPipelineDesc& actualDesc = nativePipeline->getDesc();
        if (actualDesc.bindingLayouts.size() >= 2) {
            pso->vsBindingLayout = actualDesc.bindingLayouts[0];
            pso->psBindingLayout = actualDesc.bindingLayouts[1];
        }
    }

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
    m_visualToMaterialID.clear();  // Clear bindless material cache
    m_pendingMaterials.clear();    // Clear pending materials
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

// ══════════════════════════════════════════════════════════
//  REGISTER MATERIAL WITH BINDLESS SYSTEM
// ══════════════════════════════════════════════════════════

u32 MaterialCache::RegisterBindlessMaterial(MaterialPSO* matPSO)
{
    using namespace RENDER_NAMESPACE::bindless;

    if (!matPSO)
        return UINT32_MAX;

    // Already registered?
    if (matPSO->bindlessMaterialID != UINT32_MAX)
        return matPSO->bindlessMaterialID;

    // Check if bindless system is initialized
    auto& materialBuffer = MaterialBuffer::Instance();
    if (!materialBuffer.IsInitialized())
        return UINT32_MAX;

    // Get texture manager for NVRHI texture access
    resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
    if (!texManager)
        return UINT32_MAX;

    // Build MaterialData from PSO textures (SM6 bindless: simple u32 descriptor indices)
    MaterialData matData = {};
    // Initialize with invalid texture indices
    matData.diffuseIndex = INVALID_TEXTURE_INDEX;
    matData.normalIndex = INVALID_TEXTURE_INDEX;
    matData.detailIndex = INVALID_TEXTURE_INDEX;
    matData.pbrIndex = INVALID_TEXTURE_INDEX;
    matData.detailScale = matPSO->detail_scale;
    matData.alphaRef = 0.5f;  // Default alpha ref
    matData.flags = 0;
    matData.padding = 0.0f;

    // Extract material properties from shader/pass
    if (matPSO->pass) {
        // Check texture list for material flags
        RENDER_NAMESPACE::STextureList* texList = matPSO->pass->T._get();
        if (texList && !texList->empty()) {
            // Check for normal map (usually slot 1 or named with _bump)
            for (size_t i = 0; i < texList->size(); i++) {
                const auto& texPair = (*texList)[i];
                if (texPair.first == 1) {  // Normal map slot
                    matData.flags |= MAT_FLAG_HAS_NORMAL;
                }
            }
        }
    }

    // Note: Actual texture registration to bindless descriptor heap happens lazily during rendering
    // when RenderContext is available. For now, we just set up the material flags
    // and register with the buffer. The descriptor indices will be updated later.

    // Register with material buffer
    u32 materialID = materialBuffer.RegisterMaterial(matData);
    matPSO->bindlessMaterialID = materialID;

    return materialID;
}

// ══════════════════════════════════════════════════════════
//  PRE-REGISTER BINDLESS MATERIAL BY VISUAL
// ══════════════════════════════════════════════════════════
// Called during geometry collection (before PSO exists)
// Creates bindless material entry based on visual's shader/textures
// Returns material ID for batch.bindlessMaterialID

u32 MaterialCache::PreRegisterBindlessMaterial(dxRender_Visual* visual)
{
    using namespace RENDER_NAMESPACE::bindless;

    if (!visual)
        return UINT32_MAX;

    // Check cache first
    auto it = m_visualToMaterialID.find(visual);
    if (it != m_visualToMaterialID.end())
        return it->second;

    // Check if bindless system is initialized
    auto& materialBuffer = MaterialBuffer::Instance();
    if (!materialBuffer.IsInitialized())
        return UINT32_MAX;

    // Build MaterialData (SM6 bindless: simple u32 descriptor indices)
    MaterialData matData = {};
    matData.diffuseIndex = INVALID_TEXTURE_INDEX;
    matData.normalIndex = INVALID_TEXTURE_INDEX;
    matData.detailIndex = INVALID_TEXTURE_INDEX;
    matData.pbrIndex = INVALID_TEXTURE_INDEX;
    matData.detailScale = 1.0f;
    matData.alphaRef = 0.5f;  // Default, will be overwritten if blender data available
    matData.flags = 0;
    matData.padding = 0.0f;

    // Get material info from MaterialSystem using shader/texture names
    if (visual->textureName.size() > 0) {
        matData.detailScale = GetDetailScale(visual->textureName);
    }

    // Get material flags from MaterialSystem (queries blender properties)
    if (visual->shaderName.size() > 0) {
        const auto& matInfo = MaterialSystem::Instance().GetMaterialInfo(visual->shaderName.c_str(), visual->textureName.c_str());
        if (matInfo.alphaTest) {
            matData.flags |= MAT_FLAG_ALPHA_TEST;
            // Use actual alphaRef from blender, normalized to 0.0-1.0
            matData.alphaRef = matInfo.alphaRef / 255.0f;
        }
        // Note: Normal map detection would need texture probing or metadata
        // For now, assume materials have normals (common case)
        matData.flags |= MAT_FLAG_HAS_NORMAL;
    }

    // Register with material buffer
    u32 materialID = materialBuffer.RegisterMaterial(matData);

    // Cache for future lookups
    m_visualToMaterialID[visual] = materialID;

    // Add to pending list for descriptor registration
    // This will be finalized when RenderContext is available
    PendingMaterial pending;
    pending.materialID = materialID;
    pending.visual = visual;
    m_pendingMaterials.push_back(pending);

    // Debug: log first few materials added
    static u32 logCount = 0;
    if (++logCount <= 10) {
        Msg("* [MaterialCache] PreRegister: matID=%u visual=%p type=%u tex='%s' shader='%s' pending=%u",
            materialID, visual, visual->getType(), visual->textureName.c_str(), visual->shaderName.c_str(),
            static_cast<u32>(m_pendingMaterials.size()));
    }

    return materialID;
}

// Pre-register bindless material for particle effects
// Particles have their texture name in CPEDef, not in visual
u32 MaterialCache::PreRegisterParticleMaterial(const shared_str& textureName)
{
    using namespace RENDER_NAMESPACE::bindless;

    if (!textureName.size() || !textureName[0])
        return UINT32_MAX;

    // Check cache first
    auto it = m_particleTextureToMaterialID.find(textureName);
    if (it != m_particleTextureToMaterialID.end())
        return it->second;

    // Check if bindless system is initialized
    auto& materialBuffer = MaterialBuffer::Instance();
    if (!materialBuffer.IsInitialized())
        return UINT32_MAX;

    // Build MaterialData for particle (just needs diffuse texture)
    MaterialData matData = {};
    matData.diffuseIndex = INVALID_TEXTURE_INDEX;
    matData.normalIndex = INVALID_TEXTURE_INDEX;
    matData.detailIndex = INVALID_TEXTURE_INDEX;
    matData.pbrIndex = INVALID_TEXTURE_INDEX;
    matData.detailScale = 1.0f;
    matData.alphaRef = 0.01f / 255.0f;  // Minimal alpha test for particles
    matData.flags = 0;  // No alpha test, no normal map for particles
    matData.padding = 0.0f;

    // Register with material buffer
    u32 materialID = materialBuffer.RegisterMaterial(matData);

    // Cache for future lookups
    m_particleTextureToMaterialID[textureName] = materialID;

    // Add to pending list for descriptor registration
    PendingMaterial pending;
    pending.materialID = materialID;
    pending.visual = nullptr;  // No visual for particles
    pending.textureName = textureName;  // Store texture name directly
    m_pendingMaterials.push_back(pending);

    Msg("* [MaterialCache] PreRegisterParticle: matID=%u tex='%s' pending=%u",
        materialID, textureName.c_str(), static_cast<u32>(m_pendingMaterials.size()));

    return materialID;
}

// ══════════════════════════════════════════════════════════
//  FINALIZE PENDING MATERIALS (Register Textures to Bindless Descriptor Heap)
// ══════════════════════════════════════════════════════════
// Called once per frame when RenderContext is available
// Registers textures to D3D12 descriptor heap and updates material buffer

void MaterialCache::FinalizePendingMaterials(ng::RenderContext* ctx)
{
    using namespace RENDER_NAMESPACE::bindless;

    if (m_pendingMaterials.empty()) {
        return;
    }

    auto& materialBuffer = MaterialBuffer::Instance();
    if (!materialBuffer.IsInitialized())
        return;

    resources::TextureManager* texManager = m_resourceManager ? m_resourceManager->GetTextureManager() : nullptr;
    if (!texManager)
        return;

    // Get backend for bindless texture registration (uses IRenderBackend virtual method)
    IRenderBackend* backend = GEnv.Backend;
    if (!backend) {
        Msg("! [MaterialCache] Backend not available - cannot register bindless textures");
        return;
    }

    u32 processedCount = 0;

    for (const auto& pending : m_pendingMaterials) {
        dxRender_Visual* visual = pending.visual;
        u32 materialID = pending.materialID;

        if (materialID == UINT32_MAX)
            continue;

        // Get material data to update
        const MaterialData* existingMat = materialBuffer.GetMaterial(materialID);
        if (!existingMat)
            continue;

        MaterialData matData = *existingMat;
        bool updated = false;

        // Get diffuse texture name from visual or pending (particle case)
        shared_str diffuseName;
        if (visual) {
            diffuseName = visual->textureName;
        } else if (pending.textureName.size()) {
            // Particle material - texture name stored directly
            diffuseName = pending.textureName;
        }

        // Skip if no diffuse texture name
        if (!diffuseName.size() || !diffuseName[0])
            continue;

        // Load diffuse texture through modern resource manager
        {
            resources::TextureHandle handle = texManager->LoadTexture(diffuseName.c_str());
            if (handle.IsValid()) {
                nvrhi::ITexture* nvrhiTex = texManager->GetNVRHITexture(handle);
                if (nvrhiTex) {
                    u32 descriptorIndex = backend->RegisterBindlessTexture(nvrhiTex);
                    if (descriptorIndex != INVALID_TEXTURE_INDEX) {
                        matData.diffuseIndex = descriptorIndex;
                        updated = true;
                    }
                }
            }
        }

        // Get normal/bump map from texture description (proper X-Ray way)
        auto& texDescMgr = RImplementation.Resources->m_textures_description;
        shared_str bumpName = texDescMgr.GetBumpName(diffuseName);
        if (bumpName.size() && bumpName[0]) {
            resources::TextureHandle handle = texManager->LoadTexture(bumpName.c_str());
            if (handle.IsValid()) {
                nvrhi::ITexture* nvrhiTex = texManager->GetNVRHITexture(handle);
                if (nvrhiTex) {
                    u32 descriptorIndex = backend->RegisterBindlessTexture(nvrhiTex);
                    if (descriptorIndex != INVALID_TEXTURE_INDEX) {
                        matData.normalIndex = descriptorIndex;
                        matData.flags |= MAT_FLAG_HAS_NORMAL;
                        updated = true;
                    }
                }
            }
        }

        // Get detail texture from texture description
        LPCSTR detailTexName = nullptr;
        R_constant_setup* detailCS = nullptr;
        if (texDescMgr.GetDetailTexture(diffuseName, detailTexName, detailCS)) {
            if (detailTexName && detailTexName[0]) {
                resources::TextureHandle handle = texManager->LoadTexture(detailTexName);
                if (handle.IsValid()) {
                    nvrhi::ITexture* nvrhiTex = texManager->GetNVRHITexture(handle);
                    if (nvrhiTex) {
                        u32 descriptorIndex = backend->RegisterBindlessTexture(nvrhiTex);
                        if (descriptorIndex != INVALID_TEXTURE_INDEX) {
                            matData.detailIndex = descriptorIndex;
                            matData.detailScale = texDescMgr.GetDetailScale(diffuseName);
                            matData.flags |= MAT_FLAG_HAS_DETAIL;
                            updated = true;
                        }
                    }
                }
            }
        }

        // Get PBR texture from texture description metadata
        // Prefer consolidated _pbr texture, fallback to legacy _metallic
        if (diffuseName.c_str() && diffuseName[0]) {
            // Try consolidated _pbr texture first (R=metallic, G=roughness, B=ao, A=parallax)
            shared_str pbrName = texDescMgr.GetPBRName(diffuseName);
            if (!pbrName.empty()) {
                resources::TextureHandle handle = texManager->LoadTexture(pbrName.c_str());
                if (handle.IsValid()) {
                    nvrhi::ITexture* nvrhiTex = texManager->GetNVRHITexture(handle);
                    if (nvrhiTex) {
                        u32 descriptorIndex = backend->RegisterBindlessTexture(nvrhiTex);
                        if (descriptorIndex != INVALID_TEXTURE_INDEX) {
                            matData.pbrIndex = descriptorIndex;
                            matData.flags |= MAT_FLAG_HAS_PBR;
                            updated = true;
                        }
                    }
                }
            }
        }

        // Update material buffer if any textures were registered
        if (updated) {
            materialBuffer.UpdateMaterial(materialID, matData);
            processedCount++;
        }
    }

    // Clear pending list
    m_pendingMaterials.clear();

    // Upload updated materials to GPU
    if (processedCount > 0) {
        materialBuffer.Upload(ctx);
    }
}

} // namespace xray::render
