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
#include "Layers/xrRenderDX11/ResourceManager.h"
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
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"   // Terrain material buffer (t9)
// SM6 bindless texture registration uses GEnv.Backend->RegisterBindlessTexture()
#include "xrEngine/IRenderBackend.h"                          // For IRenderBackend
#include "Layers/xrRenderDX11/Blender_CLSID.h"                    // For B_BmmD, B_LmBmmD CLASS_IDs
#include "Layers/xrRenderDX11/blenders/Blender_BmmD.h"            // For CBlender_BmmD detail texture accessors
#include "Layers/xrRender/r_constants.h"                      // For R_constant_setup
#include "Layers/xrRender/Materials/MaterialSystem.h"         // For MaterialSystem (D3D12)
#include "Layers/xrRender/ShaderVariant/ShaderVariantRegistry.h"
#include "Layers/xrRender/Bindless/VariantTextureBuffer.h"
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
    fg::RenderDevice* device,
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

    u32 shaderID = visual->shader_id;
    if (shaderID == UINT32_MAX)
        return nullptr;

    auto* compiled = RImplementation.getCompiledShader(shaderID);
    if (!compiled)
        return nullptr;

    u32 vertexFormatID = GetVertexFormatID(visual);
    u64 cacheKey = RImplementation.ComputePSOCacheKey(vertexFormatID, passType);

    auto it = compiled->precompiledPSOs.psoCache.find(cacheKey);
    if (it != compiled->precompiledPSOs.psoCache.end() && it->second) {
        m_stats.numCacheHits++;
        return it->second;
    }

    m_stats.numCacheMisses++;
    Msg("! [MaterialCache] PSO cache miss for shader %u (format %u, pass %u)",
        shaderID, vertexFormatID, (u32)passType);
    return nullptr;
}

// ══════════════════════════════════════════════════════════
//  GET OR CREATE DEPTH PSO (Phase 2.4)
// ══════════════════════════════════════════════════════════

MaterialPSO* MaterialCache::GetOrCreateDepthPSO(
    dxRender_Visual* visual,
    const framegraph::FrameGraph& fg)
{
    if (!visual)
        Msg("! [MaterialCache::GetOrCreateDepthPSO] Visual is NULL");
    return nullptr;
}


// ══════════════════════════════════════════════════════════
//  EXTRACT TEXTURES
// ══════════════════════════════════════════════════════════


// ══════════════════════════════════════════════════════════
//  EXTRACT SHADERS
// ══════════════════════════════════════════════════════════


// ══════════════════════════════════════════════════════════
//  EXTRACT SAMPLERS (Using Shader Reflection + X-Ray State)
// ══════════════════════════════════════════════════════════


// ══════════════════════════════════════════════════════════
//  CREATE BINDING LAYOUTS (Per-Stage)
// ══════════════════════════════════════════════════════════

void MaterialCache::CreateBindingLayouts(MaterialPSO* matPSO)
{
    VERIFY(matPSO);

    if (GEnv.Backend->GetAPI() == IRenderBackend::API::Vulkan)
    {
        matPSO->vsBindingLayout = CreateCombinedBindingLayout(matPSO);
        matPSO->psBindingLayout = nullptr;
    }
    else
    {
        matPSO->vsBindingLayout = CreateStageBindingLayout(
            matPSO, MaterialPSO::ShaderStage::Vertex, nvrhi::ShaderType::Vertex);
        matPSO->psBindingLayout = CreateStageBindingLayout(
            matPSO, MaterialPSO::ShaderStage::Pixel, nvrhi::ShaderType::Pixel);
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

    //if (GEnv.Backend->GetAPI() == IRenderBackend::API::Vulkan)
    //    layoutDesc.bindingOffsets = { 0, 0, 0, 0 };

    nvrhi::BindingLayoutHandle layout = m_device->CreateBindingLayout(layoutDesc);
    if (!layout) {
        return nullptr;
    }

    return layout;
}

nvrhi::BindingLayoutHandle MaterialCache::CreateCombinedBindingLayout(const MaterialPSO* matPSO)
{
    VERIFY(matPSO);

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;

    struct CBBinding {
        u32 slot;
        bool isVCB;
    };
    xr_vector<CBBinding> allCBs;

    for (const auto& vcbReq : matPSO->vcbRequirements) {
        allCBs.push_back({vcbReq.slot, true});
    }

    for (const auto& cbInfo : matPSO->constantBuffers) {
        bool duplicate = false;
        for (const auto& existing : allCBs) {
            if (existing.slot == cbInfo.slot) { duplicate = true; break; }
        }
        if (!duplicate)
            allCBs.push_back({cbInfo.slot, false});
    }

    std::sort(allCBs.begin(), allCBs.end(), [](const CBBinding& a, const CBBinding& b) {
        return a.slot < b.slot;
    });

    for (const auto& cb : allCBs) {
        if (cb.isVCB)
            layoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::VolatileConstantBuffer(cb.slot));
        else
            layoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::ConstantBuffer(cb.slot));
    }

    xr_vector<MaterialPSO::TextureSlot> sortedTextures(matPSO->textures);
    std::sort(sortedTextures.begin(), sortedTextures.end(),
        [](const MaterialPSO::TextureSlot& a, const MaterialPSO::TextureSlot& b) {
            return a.slot < b.slot;
        });
    for (const auto& texSlot : sortedTextures) {
        layoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::Texture_SRV(texSlot.slot));
    }

    xr_vector<MaterialPSO::SamplerInfo> allSamplers;
    xr_set<u32> addedSamplerSlots;
    for (const auto& samplerInfo : matPSO->samplers) {
        if (addedSamplerSlots.count(samplerInfo.slot))
            continue;
        allSamplers.push_back(samplerInfo);
        addedSamplerSlots.insert(samplerInfo.slot);
    }
    std::sort(allSamplers.begin(), allSamplers.end(),
        [](const MaterialPSO::SamplerInfo& a, const MaterialPSO::SamplerInfo& b) {
            return a.slot < b.slot;
        });
    for (const auto& samplerInfo : allSamplers) {
        layoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::Sampler(samplerInfo.slot));
    }

    //if (GEnv.Backend->GetAPI() == IRenderBackend::API::Vulkan)
    //    layoutDesc.bindingOffsets = { 0, 0, 0, 0 };

    return m_device->CreateBindingLayout(layoutDesc);
}

// ══════════════════════════════════════════════════════════
//  GET OR CREATE CACHED BINDING SET (with per-object VCB)
// ══════════════════════════════════════════════════════════

nvrhi::BindingSetHandle MaterialCache::GetOrCreateBindingSet(MaterialPSO* matPSO)
{
    VERIFY(matPSO);
    VERIFY(matPSO->vsBindingLayout);

    bool combinedMode = (matPSO->psBindingLayout == nullptr);

    if (!combinedMode)
        VERIFY(matPSO->psBindingLayout);

    // ═══════════════════════════════════════════════════════
    //  CHECK CACHE - Return existing binding sets if already created
    // ═══════════════════════════════════════════════════════
    // With proper NVRHI VCB support (isVolatile=true, maxVersions set),
    // binding sets can be cached even with VCBs - NVRHI handles versioning.
    bool cacheValid = combinedMode
        ? (matPSO->vsBindingSet && !matPSO->needsBindingSetRebuild)
        : (matPSO->vsBindingSet && matPSO->psBindingSet && !matPSO->needsBindingSetRebuild);
    if (cacheValid)
        return matPSO->vsBindingSet;

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
        fg::BufferHandle latestHandle = m_vcbPool->GetOrCreateVCB(
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

    if (combinedMode)
    {
        nvrhi::BindingSetDesc combinedDesc;

        xr_set<u32> addedCBSlots;
        for (const auto& binding : vsBindings) {
            combinedDesc.bindings.push_back(
                nvrhi::BindingSetItem::ConstantBuffer(binding.slot, binding.buffer));
            addedCBSlots.insert(binding.slot);
        }

        for (const auto& binding : psBindings) {
            if (binding.type == PSBinding::CB && addedCBSlots.count(binding.slot))
                continue;
            combinedDesc.bindings.push_back(binding.item);
        }

        nvrhi::BindingSetHandle combinedSet = m_device->CreateBindingSet(
            combinedDesc, matPSO->vsBindingLayout);

        if (!combinedSet) {
            Msg("! [MaterialCache::GetOrCreateBindingSet] Failed to create combined binding set");
            return nullptr;
        }

        matPSO->vsBindingSet = combinedSet;
        matPSO->psBindingSet = nullptr;
    }
    else
    {
        nvrhi::BindingSetHandle vsBindingSet = m_device->CreateBindingSet(
            vsBindingDesc, matPSO->vsBindingLayout);

        if (!vsBindingSet) {
            Msg("! [MaterialCache::GetOrCreateBindingSet] Failed to create VS binding set");
            return nullptr;
        }

        nvrhi::BindingSetHandle psBindingSet = m_device->CreateBindingSet(
            psBindingDesc, matPSO->psBindingLayout);

        if (!psBindingSet) {
            Msg("! [MaterialCache::GetOrCreateBindingSet] Failed to create PS binding set");
            return nullptr;
        }

        matPSO->vsBindingSet = vsBindingSet;
        matPSO->psBindingSet = psBindingSet;
    }

    if (!allTexturesValid) {
        // Textures not ready - mark for recreation next frame
        matPSO->needsBindingSetRebuild = true;
    }

    return matPSO->vsBindingSet;
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



// ══════════════════════════════════════════════════════════
//  STATE CONVERSION HELPERS
// ══════════════════════════════════════════════════════════
// Now located in RenderStateConversion.h (shared with ParticlePass)

// ══════════════════════════════════════════════════════════
//  SETUP RENDER STATES
// ══════════════════════════════════════════════════════════


// ══════════════════════════════════════════════════════════
//  SETUP RENDER TARGETS
// ══════════════════════════════════════════════════════════


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
    fg::PipelineStateDesc psoDesc;
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

    Msg("  [CreateUIPSO] Building vertex attributes from %u signature elements:", pso->vsInputSignature.elements.size());
    for (const auto& shaderElem : pso->vsInputSignature.elements) {
        std::string semantic = shaderElem.semanticName.c_str();

        auto it = uiVertexLayout.find(semantic);
        if (it == uiVertexLayout.end()) {
            Msg("! [MaterialCache::CreateUIPSO] Unknown UI vertex semantic: %s", semantic.c_str());
            continue;
        }

        fg::VertexAttribute attr;
        attr.semanticName = shaderElem.semanticName.c_str();
        attr.semanticIndex = shaderElem.semanticIndex;
        attr.format = it->second.format;
        attr.offset = it->second.offset;
        attr.bufferIndex = 0;
        attr.elementStride = 0;

        Msg("    attr[%u]: semantic='%s' format=%d offset=%u", (u32)psoDesc.vertexAttributes.size(), semantic.c_str(), (int)attr.format, attr.offset);
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
        psoDesc.depthStencilState.depthFunc = fg::ComparisonFunc::Always;  // Always pass
        psoDesc.depthStencilState.depthWriteEnable = false;  // Don't write depth
        psoDesc.depthStencilState.stencilEnable = true;      // Enable stencil for UI effects
    } else {
        psoDesc.depthStencilState.depthTestEnable = false;
        psoDesc.depthStencilState.depthWriteEnable = false;
        psoDesc.depthStencilState.stencilEnable = false;
    }

    // Standard premultiplied alpha blending (matches vanilla)
    psoDesc.blendState.renderTargets[0].blendEnable = true;
    psoDesc.blendState.renderTargets[0].srcBlend = fg::BlendFactor::SrcAlpha;
    psoDesc.blendState.renderTargets[0].dstBlend = fg::BlendFactor::InvSrcAlpha;
    psoDesc.blendState.renderTargets[0].srcBlendAlpha = fg::BlendFactor::SrcAlpha;
    psoDesc.blendState.renderTargets[0].dstBlendAlpha = fg::BlendFactor::InvSrcAlpha;

    psoDesc.rasterizerState.cullMode = fg::CullMode::None;
    psoDesc.rasterizerState.frontCounterClockwise = false;
    psoDesc.rasterizerState.scissorEnable = true;  // Enable scissor for UI clipping

    // Set primitive topology (UI uses triangle lists) - already defaults to TriangleList but being explicit
    psoDesc.primitiveTopology = fg::PrimitiveTopology::TriangleList;

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
    fg::PipelineStateCache* psoCache = m_device->GetPipelineCache();
    if (!psoCache) {
        Msg("! [MaterialCache::CreateUIPSO] No PSO cache");
        return nullptr;
    }

    fg::PipelineState* nvrhiPSO = psoCache->GetOrCreate(psoDesc);
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
        if (actualDesc.bindingLayouts.size() >= 1) {
            pso->vsBindingLayout = actualDesc.bindingLayouts[0];
            if (actualDesc.bindingLayouts.size() >= 2)
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
    fg::PipelineStateDesc psoDesc;
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

        fg::VertexAttribute attr;
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
    psoDesc.blendState.renderTargets[0].srcBlend = fg::BlendFactor::SrcAlpha;
    psoDesc.blendState.renderTargets[0].dstBlend = fg::BlendFactor::InvSrcAlpha;
    psoDesc.blendState.renderTargets[0].blendOp = fg::BlendOp::Add;
    psoDesc.blendState.renderTargets[0].srcBlendAlpha = fg::BlendFactor::SrcAlpha;
    psoDesc.blendState.renderTargets[0].dstBlendAlpha = fg::BlendFactor::InvSrcAlpha;

    psoDesc.rasterizerState.cullMode = fg::CullMode::None;
    psoDesc.rasterizerState.frontCounterClockwise = false;
    psoDesc.rasterizerState.scissorEnable = true;

    psoDesc.primitiveTopology = fg::PrimitiveTopology::TriangleList;

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

    fg::PipelineStateCache* psoCache = m_device->GetPipelineCache();
    if (!psoCache) {
        Msg("! [MaterialCache::CreateFontPSO] No PSO cache");
        return nullptr;
    }

    fg::PipelineState* nvrhiPSO = psoCache->GetOrCreate(psoDesc);
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
        if (actualDesc.bindingLayouts.size() >= 1) {
            pso->vsBindingLayout = actualDesc.bindingLayouts[0];
            if (actualDesc.bindingLayouts.size() >= 2)
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
    matData.shaderVariant = 0;

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
//  TERRAIN MATERIAL DETECTION
// ══════════════════════════════════════════════════════════
// Uses Blender CLASS_ID to detect terrain materials (B_BmmD, B_LmBmmD)
// These use 4-layer detail blending with RGBA mask

bool MaterialCache::IsTerrainMaterial(dxRender_Visual* visual)
{
    if (!visual || !visual->shaderName.size())
        return false;

    // Get blender from shader name
    IBlender* blender = RImplementation.Resources->_FindBlender(visual->shaderName.c_str());
    if (!blender)
        return false;

    // Check CLASS_ID for terrain blenders
    CLASS_ID cls = blender->getDescription().CLS;
    return (cls == B_BmmD || cls == B_LmBmmD);
}

// ══════════════════════════════════════════════════════════
//  PRE-REGISTER TERRAIN MATERIAL
// ══════════════════════════════════════════════════════════
// Creates TerrainMaterialData entry for 4-layer detail blending
// Returns terrain material ID for batch.terrainMaterialID

u32 MaterialCache::PreRegisterTerrainMaterial(dxRender_Visual* visual)
{
    using namespace RENDER_NAMESPACE::bindless;

    if (!visual)
        return UINT32_MAX;

    // Cache by shader name - terrain materials are determined by shader
    // (e.g., "levels\zaton_earth_2" defines which detail textures to use)
    // Multiple visuals with the same shader share one terrain material
    shared_str shaderName = visual->shaderName;

    // Check cache first
    auto it = m_shaderToTerrainMaterialID.find(shaderName);
    if (it != m_shaderToTerrainMaterialID.end())
        return it->second;

    // Check if terrain material buffer is initialized
    auto& terrainBuffer = TerrainMaterialBuffer::Instance();
    if (!terrainBuffer.IsInitialized()) {
        static bool s_warnOnce = false;
        if (!s_warnOnce) {
            Msg("! [MaterialCache] TerrainMaterialBuffer not initialized - terrain will render BLACK!");
            Msg("!   This usually means level loaded before FrameGraphRenderer::Initialize()");
            s_warnOnce = true;
        }
        return UINT32_MAX;
    }

    // Build TerrainMaterialData (texture indices will be filled during finalization)
    TerrainMaterialData matData = {};

    // Initialize all texture indices to invalid
    matData.baseAlbedoIndex = INVALID_TEXTURE_INDEX;
    matData.blendMaskIndex = INVALID_TEXTURE_INDEX;
    matData.detailR_Index = INVALID_TEXTURE_INDEX;
    matData.detailG_Index = INVALID_TEXTURE_INDEX;
    matData.detailB_Index = INVALID_TEXTURE_INDEX;
    matData.detailA_Index = INVALID_TEXTURE_INDEX;
    matData.normalR_Index = INVALID_TEXTURE_INDEX;
    matData.normalG_Index = INVALID_TEXTURE_INDEX;
    matData.normalB_Index = INVALID_TEXTURE_INDEX;
    matData.normalA_Index = INVALID_TEXTURE_INDEX;
    matData.pbrR_Index = INVALID_TEXTURE_INDEX;
    matData.pbrG_Index = INVALID_TEXTURE_INDEX;
    matData.pbrB_Index = INVALID_TEXTURE_INDEX;
    matData.pbrA_Index = INVALID_TEXTURE_INDEX;

    // Set terrain flag
    matData.flags = MAT_FLAG_TERRAIN;

    // Get detail scale from texture description
    if (visual->textureName.size() > 0) {
        matData.detailScale = GetDetailScale(visual->textureName);
    } else {
        matData.detailScale = 4.0f;  // Default terrain detail scale
    }

    // Register with terrain material buffer
    u32 terrainMaterialID = terrainBuffer.RegisterMaterial(matData);

    // Cache for future lookups (by shader name, not visual pointer)
    m_shaderToTerrainMaterialID[shaderName] = terrainMaterialID;

    // Add to pending list for texture registration
    PendingTerrainMaterial pending;
    pending.terrainMaterialID = terrainMaterialID;
    pending.visual = visual;
    m_pendingTerrainMaterials.push_back(pending);

    // Debug: log terrain materials
    static u32 logCount = 0;
    if (++logCount <= 5) {
        Msg("* [MaterialCache] PreRegisterTerrain: matID=%u visual=%p tex='%s' shader='%s'",
            terrainMaterialID, visual, visual->textureName.c_str(), visual->shaderName.c_str());
    }

    return terrainMaterialID;
}

// ══════════════════════════════════════════════════════════
//  FINALIZE PENDING TERRAIN MATERIALS
// ══════════════════════════════════════════════════════════
// Registers all 14 terrain textures (base, mask, 4x detail, 4x normal, 4x pbr)

void MaterialCache::FinalizePendingTerrainMaterials(fg::RenderContext* ctx)
{
    using namespace RENDER_NAMESPACE::bindless;

    if (m_pendingTerrainMaterials.empty())
        return;

    auto& terrainBuffer = TerrainMaterialBuffer::Instance();
    if (!terrainBuffer.IsInitialized())
        return;

    resources::TextureManager* texManager = m_resourceManager ? m_resourceManager->GetTextureManager() : nullptr;
    if (!texManager)
        return;

    IRenderBackend* backend = GEnv.Backend;
    if (!backend)
        return;

    u32 processedCount = 0;

    for (const auto& pending : m_pendingTerrainMaterials) {
        dxRender_Visual* visual = pending.visual;
        u32 terrainMaterialID = pending.terrainMaterialID;

        if (!visual || terrainMaterialID == UINT32_MAX)
            continue;

        // Get existing material data
        const TerrainMaterialData* existingMat = terrainBuffer.GetMaterial(terrainMaterialID);
        if (!existingMat)
            continue;

        TerrainMaterialData matData = *existingMat;
        bool updated = false;
        xr_vector<xr_string> missingTextures;  // Track missing textures for logging

        // Get blender to access detail texture names
        IBlender* blender = RImplementation.Resources->_FindBlender(visual->shaderName.c_str());
        CBlender_BmmD* terrainBlender = nullptr;
        if (blender) {
            CLASS_ID cls = blender->getDescription().CLS;
            if (cls == B_BmmD || cls == B_LmBmmD) {
                terrainBlender = static_cast<CBlender_BmmD*>(blender);
            }
        }

        // Helper lambda to register a texture with failure tracking
        auto RegisterTexture = [&](const char* texName, const char* slotName) -> u32 {
            if (!texName || !texName[0]) {
                missingTextures.push_back(xr_string(slotName) + ": (empty name)");
                return INVALID_TEXTURE_INDEX;
            }

            resources::TextureHandle handle = texManager->LoadTexture(texName);
            if (!handle.IsValid()) {
                missingTextures.push_back(xr_string(slotName) + ": " + texName + " (load failed)");
                return INVALID_TEXTURE_INDEX;
            }

            nvrhi::ITexture* nvrhiTex = texManager->GetNVRHITexture(handle);
            if (!nvrhiTex) {
                missingTextures.push_back(xr_string(slotName) + ": " + texName + " (no NVRHI tex)");
                return INVALID_TEXTURE_INDEX;
            }

            u32 idx = backend->RegisterBindlessTexture(nvrhiTex);
            if (idx == INVALID_TEXTURE_INDEX) {
                missingTextures.push_back(xr_string(slotName) + ": " + texName + " (register failed)");
            }
            return idx;
        };

        // 1. Base albedo texture (s_base) - from visual texture name
        if (visual->textureName.size()) {
            u32 idx = RegisterTexture(visual->textureName.c_str(), "base");
            if (idx != INVALID_TEXTURE_INDEX) {
                matData.baseAlbedoIndex = idx;
                updated = true;
            }
        }

        // 2. Blend mask texture (s_mask) - derived from base texture name
        // Convention: baseTexture + "_mask" (e.g., "terrain\\terrain_zaton_mask")
        if (visual->textureName.size()) {
            xr_string maskName(visual->textureName.c_str());
            maskName += "_mask";
            u32 idx = RegisterTexture(maskName.c_str(), "mask");
            if (idx != INVALID_TEXTURE_INDEX) {
                matData.blendMaskIndex = idx;
                updated = true;
            }
        }

        // Get detail texture names from blender (oR_Name, oG_Name, oB_Name, oA_Name)
        const char* detailR = terrainBlender ? terrainBlender->GetDetailR() : nullptr;
        const char* detailG = terrainBlender ? terrainBlender->GetDetailG() : nullptr;
        const char* detailB = terrainBlender ? terrainBlender->GetDetailB() : nullptr;
        const char* detailA = terrainBlender ? terrainBlender->GetDetailA() : nullptr;

        // 3-6. Detail color textures (s_dt_r/g/b/a)
        {
            u32 idx = RegisterTexture(detailR, "detailR");
            if (idx != INVALID_TEXTURE_INDEX) { matData.detailR_Index = idx; updated = true; }
        }
        {
            u32 idx = RegisterTexture(detailG, "detailG");
            if (idx != INVALID_TEXTURE_INDEX) { matData.detailG_Index = idx; updated = true; }
        }
        {
            u32 idx = RegisterTexture(detailB, "detailB");
            if (idx != INVALID_TEXTURE_INDEX) { matData.detailB_Index = idx; updated = true; }
        }
        {
            u32 idx = RegisterTexture(detailA, "detailA");
            if (idx != INVALID_TEXTURE_INDEX) { matData.detailA_Index = idx; updated = true; }
        }

        // 7-10. Detail normal textures (s_dn_r/g/b/a)
        // Convention: detail texture name + "_bump"
        auto& texDescMgr = RImplementation.Resources->m_textures_description;
        if (detailR && detailR[0]) {
            shared_str bumpR = texDescMgr.GetBumpName(detailR);
            if (bumpR.size()) {
                u32 idx = RegisterTexture(bumpR.c_str(), "normalR");
                if (idx != INVALID_TEXTURE_INDEX) { matData.normalR_Index = idx; updated = true; }
            }
        }
        if (detailG && detailG[0]) {
            shared_str bumpG = texDescMgr.GetBumpName(detailG);
            if (bumpG.size()) {
                u32 idx = RegisterTexture(bumpG.c_str(), "normalG");
                if (idx != INVALID_TEXTURE_INDEX) { matData.normalG_Index = idx; updated = true; }
            }
        }
        if (detailB && detailB[0]) {
            shared_str bumpB = texDescMgr.GetBumpName(detailB);
            if (bumpB.size()) {
                u32 idx = RegisterTexture(bumpB.c_str(), "normalB");
                if (idx != INVALID_TEXTURE_INDEX) { matData.normalB_Index = idx; updated = true; }
            }
        }
        if (detailA && detailA[0]) {
            shared_str bumpA = texDescMgr.GetBumpName(detailA);
            if (bumpA.size()) {
                u32 idx = RegisterTexture(bumpA.c_str(), "normalA");
                if (idx != INVALID_TEXTURE_INDEX) { matData.normalA_Index = idx; updated = true; }
            }
        }

        // 11-14. Detail PBR textures (s_pbr_r/g/b/a)
        // Convention: detail texture name + "_pbr"
        if (detailR && detailR[0]) {
            shared_str pbrR = texDescMgr.GetPBRName(detailR);
            if (pbrR.size()) {
                u32 idx = RegisterTexture(pbrR.c_str(), "pbrR");
                if (idx != INVALID_TEXTURE_INDEX) {
                    matData.pbrR_Index = idx;
                    matData.flags |= MAT_FLAG_HAS_PBR_LAYER;
                    updated = true;
                }
            }
        }
        if (detailG && detailG[0]) {
            shared_str pbrG = texDescMgr.GetPBRName(detailG);
            if (pbrG.size()) {
                u32 idx = RegisterTexture(pbrG.c_str(), "pbrG");
                if (idx != INVALID_TEXTURE_INDEX) { matData.pbrG_Index = idx; updated = true; }
            }
        }
        if (detailB && detailB[0]) {
            shared_str pbrB = texDescMgr.GetPBRName(detailB);
            if (pbrB.size()) {
                u32 idx = RegisterTexture(pbrB.c_str(), "pbrB");
                if (idx != INVALID_TEXTURE_INDEX) { matData.pbrB_Index = idx; updated = true; }
            }
        }
        if (detailA && detailA[0]) {
            shared_str pbrA = texDescMgr.GetPBRName(detailA);
            if (pbrA.size()) {
                u32 idx = RegisterTexture(pbrA.c_str(), "pbrA");
                if (idx != INVALID_TEXTURE_INDEX) { matData.pbrA_Index = idx; updated = true; }
            }
        }

        // Update terrain material buffer
        if (updated) {
            terrainBuffer.UpdateMaterial(terrainMaterialID, matData);
            processedCount++;

            // Debug: Log first 5 materials with ALL indices (including normals/PBR)
            static u32 s_debugLogCount = 0;
            bool hasZeroIndex = (matData.baseAlbedoIndex == 0 || matData.blendMaskIndex == 0 ||
                                 matData.detailR_Index == 0 || matData.detailG_Index == 0 ||
                                 matData.normalR_Index == 0 || matData.pbrR_Index == 0);
            Msg("* [TerrainMat] id=%u base=%u mask=%u dR=%u dG=%u dB=%u dA=%u nR=%u nG=%u pR=%u%s",
                terrainMaterialID, matData.baseAlbedoIndex, matData.blendMaskIndex,
                matData.detailR_Index, matData.detailG_Index, matData.detailB_Index, matData.detailA_Index,
                matData.normalR_Index, matData.normalG_Index, matData.pbrR_Index,
                hasZeroIndex ? " [IDX 0!]" : "");
            s_debugLogCount++;
        }

        // Log materials with missing textures (only if there are failures)
        if (!missingTextures.empty()) {
            Msg("! [TerrainMaterial] matID=%u shader='%s' tex='%s' - missing %zu textures:",
                terrainMaterialID, visual->shaderName.c_str(), visual->textureName.c_str(),
                missingTextures.size());
            for (const auto& missing : missingTextures) {
                Msg("!   - %s", missing.c_str());
            }
        }
    }

    // Clear pending list
    m_pendingTerrainMaterials.clear();

    // Track finalize call count for debugging
    static u32 s_finalizeCallCount = 0;
    s_finalizeCallCount++;

    // Upload to GPU
    if (processedCount > 0) {
        terrainBuffer.Upload(ctx);
        Msg("* [MaterialCache] Finalized %u terrain materials (call #%u, total registered: %u)",
            processedCount, s_finalizeCallCount, terrainBuffer.GetMaterialCount());
    }
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
    matData.shaderVariant = 0;

    // Get material info from MaterialSystem using shader/texture names
    if (visual->textureName.size() > 0) {
        matData.detailScale = GetDetailScale(visual->textureName);
    }

    // Get material flags from MaterialSystem (queries blender properties)
    if (visual->shaderName.size() > 0) {
        const auto& matInfo = MaterialSystem::Instance().GetMaterialInfo(visual->shaderName.c_str(), visual->textureName.c_str());
        if (matInfo.alphaTest) {
            matData.flags |= MAT_FLAG_ALPHA_TEST;
            matData.alphaRef = matInfo.alphaRef / 255.0f;
        }
        if (matInfo.transparent) {
            matData.flags |= MAT_FLAG_ALPHA_BLEND;
        }
        if (strstr(visual->shaderName.c_str(), "water") != nullptr)
            matData.flags |= MAT_FLAG_WATER;
        matData.shaderVariant = matInfo.shaderVariant;
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
    matData.shaderVariant = 0;

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

void MaterialCache::FinalizePendingMaterials(fg::RenderContext* ctx)
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

        if (matData.shaderVariant > 0) {
            auto& registry = ShaderVariantRegistry::Instance();
            const auto* variant = registry.GetVariantByIndex(matData.shaderVariant);
            if (variant && !variant->textures.empty()) {
                auto& vtb = bindless::VariantTextureBuffer::Instance();
                bindless::VariantTextureData vtData;
                for (u32 i = 0; i < bindless::MAX_VARIANT_TEXTURE_SLOTS; i++)
                    vtData.tex[i] = INVALID_TEXTURE_INDEX;

                u32 slotIdx = 0;
                for (const auto& [slotName, texPath] : variant->textures) {
                    if (slotIdx >= bindless::MAX_VARIANT_TEXTURE_SLOTS) break;
                    if (texPath.c_str()[0] == '$') { slotIdx++; continue; }

                    resources::TextureHandle handle = texManager->LoadTexture(texPath.c_str());
                    if (handle.IsValid()) {
                        nvrhi::ITexture* nvrhiTex = texManager->GetNVRHITexture(handle);
                        if (nvrhiTex) {
                            u32 idx = backend->RegisterBindlessTexture(nvrhiTex);
                            if (idx != INVALID_TEXTURE_INDEX)
                                vtData.tex[slotIdx] = idx;
                        }
                    }
                    slotIdx++;
                }
                vtb.SetVariantTextures(materialID, vtData);
            }
        }
    }

    // Clear pending list
    m_pendingMaterials.clear();

    // Upload updated materials to GPU
    if (processedCount > 0) {
        materialBuffer.Upload(ctx);
    }

    auto& vtb = bindless::VariantTextureBuffer::Instance();
    if (vtb.IsInitialized())
        vtb.Upload(ctx);
}

nvrhi::ITexture* MaterialCache::GetNVRHITextureByName(const char* textureName)
{
    if (!textureName || !textureName[0])
        return nullptr;

    auto cacheIt = m_textureHandleCache.find(textureName);
    if (cacheIt != m_textureHandleCache.end())
    {
        auto* texManager = m_resourceManager ? m_resourceManager->GetTextureManager() : nullptr;
        return texManager ? texManager->GetNVRHITexture(cacheIt->second) : nullptr;
    }

    auto* texManager = m_resourceManager ? m_resourceManager->GetTextureManager() : nullptr;
    if (!texManager) return nullptr;

    auto handle = texManager->LoadTexture(textureName);
    return handle.IsValid() ? texManager->GetNVRHITexture(handle) : nullptr;
}

} // namespace xray::render
