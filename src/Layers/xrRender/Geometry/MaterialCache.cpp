// xrRender/Geometry/MaterialCache.cpp
#include "stdafx.h"
#include "MaterialCache.h"
#include "Layers/xrRender/FrameGraphPasses/GBufferPass.h"
#include "Layers/xrRender/SH_Texture.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/FVisual.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/FTreeVisual.h"
#include "Layers/xrRender/SH_Atomic.h"
#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/RenderContext/PipelineState.h"
#include "Layers/xrRender/RenderContext/RCShader.h"

namespace xray::render {

using namespace passes;
using namespace xray::render::RENDER_NAMESPACE;  // For Shader types (STextureList, etc.)

// ══════════════════════════════════════════════════════════
//  CONSTRUCTOR / DESTRUCTOR
// ══════════════════════════════════════════════════════════

MaterialCache::MaterialCache(ng::RenderDevice* device)
    : m_device(device)
{
    VERIFY(m_device);
    Msg("* [MaterialCache] Created");
}

MaterialCache::~MaterialCache() {
    Clear();
    Msg("* [MaterialCache] Destroyed");
}

// ══════════════════════════════════════════════════════════
//  GET OR CREATE PSO
// ══════════════════════════════════════════════════════════

MaterialPSO* MaterialCache::GetOrCreatePSO(
    dxRender_Visual* visual,
    const passes::GBufferOutputs& outputs,
    const xray::render::framegraph::FrameGraph& fg)
{
    if (!visual) {
        Msg("! [MaterialCache] NULL visual passed to GetOrCreatePSO");
        return nullptr;
    }

    // Check if visual has shader
    if (!visual->shader || !visual->shader._get()) {
        Msg("! [MaterialCache] Visual '%s' has no shader", visual->dbg_name.c_str());
        return nullptr;
    }

    Shader* shader = visual->shader._get();

    // ═══════════════════════════════════════════════════════
    //  EXTRACT SHADER ELEMENT (E[0] = DEFERRED RENDERING)
    // ═══════════════════════════════════════════════════════

    // E[0] = SE_R2_NORMAL_HQ (deferred rendering mode)
    ShaderElement* elem = shader->E[0]._get();
    if (!elem) {
        Msg("! [MaterialCache] Shader has no element E[0] (deferred mode)");
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  EXTRACT FIRST PASS (GBUFFER PASS)
    // ═══════════════════════════════════════════════════════

    if (elem->passes.empty()) {
        Msg("! [MaterialCache] ShaderElement has no passes");
        return nullptr;
    }

    SPass* pass = elem->passes[0]._get();
    if (!pass) {
        Msg("! [MaterialCache] First pass is NULL");
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  COMPUTE MATERIAL KEY
    // ═══════════════════════════════════════════════════════

    // For now, create temporary MaterialPSO to extract textures for hashing
    auto tempPSO = xr_make_unique<MaterialPSO>();
    ExtractTextures(pass, tempPSO.get());

    u64 textureHash = ComputeTextureHash(tempPSO->textures);
    u64 stateHash = ComputeStateHash(pass);

    MaterialKey key(shader, textureHash, stateHash);

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

    Msg("~ [MaterialCache] Creating PSO (hash: texture=0x%llX, state=0x%llX)", textureHash, stateHash);

    MaterialPSO* pso = CreatePSO(visual, elem, pass, outputs, fg);
    if (!pso) {
        Msg("! [MaterialCache] Failed to create PSO");
        return nullptr;
    }

    // Store in cache
    m_cache[key] = xr_unique_ptr<MaterialPSO>(pso);
    m_stats.numCachedPSOs = static_cast<u32>(m_cache.size());

    Msg("  ✓ PSO created and cached (%u total PSOs)", m_stats.numCachedPSOs);

    return pso;
}

// ══════════════════════════════════════════════════════════
//  CREATE PSO
// ══════════════════════════════════════════════════════════

MaterialPSO* MaterialCache::CreatePSO(
    dxRender_Visual* visual,
    ShaderElement* elem,
    SPass* pass,
    const passes::GBufferOutputs& outputs,
    const xray::render::framegraph::FrameGraph& fg)
{
    auto pso = xr_make_unique<MaterialPSO>();

    // ═══════════════════════════════════════════════════════
    //  EXTRACT TEXTURES
    // ═══════════════════════════════════════════════════════

    ExtractTextures(pass, pso.get());
    Msg("  Extracted %u textures", pso->textures.size());

    // ═══════════════════════════════════════════════════════
    //  EXTRACT SHADERS
    // ═══════════════════════════════════════════════════════

    if (!ExtractShaders(pass, pso.get())) {
        Msg("! [MaterialCache] Failed to extract shaders");
        return nullptr;
    }

    Msg("  Extracted VS/PS shaders");

    // ═══════════════════════════════════════════════════════
    //  CREATE BINDING LAYOUT
    // ═══════════════════════════════════════════════════════

    pso->bindingLayout = CreateBindingLayout(pso.get());
    if (!pso->bindingLayout) {
        Msg("! [MaterialCache] Failed to create binding layout");
        return nullptr;
    }

    Msg("  Created binding layout");

    // ═══════════════════════════════════════════════════════
    //  CREATE SHADERS FROM BYTECODE
    // ═══════════════════════════════════════════════════════

    if (!pso->vertexShader->bytecode) {
        Msg("! [MaterialCache] Vertex shader has no bytecode");
        return nullptr;
    }

    ng::ShaderHandle vsHandle = m_device->CreateShader(
        ng::ShaderStage::Vertex,
        pso->vertexShader->bytecode->GetBufferPointer(),
        pso->vertexShader->bytecode->GetBufferSize(),
        pso->vertexShader->cName.c_str());

    if (!vsHandle.IsValid()) {
        Msg("! [MaterialCache] Failed to create vertex shader");
        return nullptr;
    }

    if (!pso->pixelShader->bytecode) {
        Msg("! [MaterialCache] Pixel shader has no bytecode");
        m_device->DestroyShader(vsHandle);
        return nullptr;
    }

    ng::ShaderHandle psHandle = m_device->CreateShader(
        ng::ShaderStage::Pixel,
        pso->pixelShader->bytecode->GetBufferPointer(),
        pso->pixelShader->bytecode->GetBufferSize(),
        pso->pixelShader->cName.c_str());

    if (!psHandle.IsValid()) {
        Msg("! [MaterialCache] Failed to create pixel shader");
        m_device->DestroyShader(vsHandle);
        return nullptr;
    }

    Msg("  Created VS/PS shaders from bytecode");

    // ═══════════════════════════════════════════════════════
    //  GET RCSHADER OBJECTS FROM HANDLES
    // ═══════════════════════════════════════════════════════

    ng::RCShader* rcVS = m_device->GetShader(vsHandle);
    if (!rcVS) {
        Msg("! [MaterialCache] Failed to get VS from handle");
        m_device->DestroyShader(vsHandle);
        m_device->DestroyShader(psHandle);
        return nullptr;
    }

    ng::RCShader* rcPS = m_device->GetShader(psHandle);
    if (!rcPS) {
        Msg("! [MaterialCache] Failed to get PS from handle");
        m_device->DestroyShader(vsHandle);
        m_device->DestroyShader(psHandle);
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  BUILD PIPELINE STATE DESCRIPTOR
    // ═══════════════════════════════════════════════════════

    ng::PipelineStateDesc psoDesc;
    psoDesc.vertexShader = rcVS;
    psoDesc.pixelShader = rcPS;

    // Extract vertex attributes from visual's geometry declaration
    SetupVertexAttributes(visual, psoDesc);

    // Set up render states from X-Ray pass
    SetupRenderStates(pass, psoDesc);

    // Set up render target formats from GBufferOutputs
    SetupRenderTargets(outputs, fg, psoDesc);

    // Set debug name
    psoDesc.debugName = pso->debugName;

    Msg("  Built PSO descriptor");

    // ═══════════════════════════════════════════════════════
    //  CREATE PIPELINE STATE
    // ═══════════════════════════════════════════════════════

    ng::PipelineStateCache* psoCache = m_device->GetPipelineCache();
    if (!psoCache) {
        Msg("! [MaterialCache] Pipeline cache is NULL");
        return nullptr;
    }

    ng::PipelineState* nvrhiPSO = psoCache->GetOrCreate(psoDesc);
    if (!nvrhiPSO) {
        Msg("! [MaterialCache] Failed to create pipeline state");
        return nullptr;
    }

    pso->pso = nvrhiPSO;

    Msg("  ✓ Pipeline state created");

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

    // Get texture list from pass
    STextureList* texList = pass->T._get();
    if (!texList) {
        // No textures in this pass
        return;
    }

    // STextureList is a vector of (stage, ref_texture) pairs
    for (size_t i = 0; i < texList->size(); i++) {
        const auto& texPair = (*texList)[i];
        u32 stage = texPair.first;
        const ref_texture& texRef = texPair.second;

        CTexture* tex = texRef._get();
        if (!tex) {
            Msg("! [MaterialCache] Texture at stage %u is NULL", stage);
            continue;
        }

        // Get D3D11 shader resource view
        ID3DShaderResourceView* srv = tex->get_SRView();
        if (!srv) {
            Msg("! [MaterialCache] Texture '%s' has NULL SRV", tex->cName.c_str());
            continue;
        }

        // Get underlying D3D11 resource
        ID3D11Resource* d3dResource = nullptr;
        srv->GetResource(&d3dResource);
        if (!d3dResource) {
            Msg("! [MaterialCache] SRV has NULL resource");
            continue;
        }

        // Query texture properties from D3D11
        ID3D11Texture2D* d3dTex2D = nullptr;
        HRESULT hr = d3dResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&d3dTex2D);

        if (FAILED(hr) || !d3dTex2D) {
            Msg("! [MaterialCache] Texture '%s' is not a 2D texture", tex->cName.c_str());
            d3dResource->Release();
            continue;
        }

        D3D11_TEXTURE2D_DESC d3dDesc;
        d3dTex2D->GetDesc(&d3dDesc);
        d3dTex2D->Release();

        // Build TextureDesc
        ng::RenderDevice::TextureDesc texDesc;
        texDesc.width = d3dDesc.Width;
        texDesc.height = d3dDesc.Height;
        texDesc.mipLevels = d3dDesc.MipLevels;
        texDesc.arraySize = d3dDesc.ArraySize;
        texDesc.dimension = ng::RenderDevice::TextureDesc::Texture2D;
        // Format conversion would go here - for now assume RGBA8
        texDesc.format = nvrhi::Format::RGBA8_UNORM;  // TODO: Convert DXGI_FORMAT
        texDesc.isRenderTarget = false;
        texDesc.isUAV = false;
        texDesc.debugName = tex->cName.c_str();

        // Wrap in NVRHI handle using device abstraction
        ng::TextureHandle nvrhiTex = m_device->CreateTextureFromD3D11(d3dResource, texDesc);
        d3dResource->Release();  // Release our ref, NVRHI holds its own

        if (!nvrhiTex.IsValid()) {
            Msg("! [MaterialCache] Failed to wrap texture '%s'", tex->cName.c_str());
            continue;
        }

        matPSO->textures.push_back(nvrhiTex);

        Msg("    Texture[%u]: '%s' (wrapped)", stage, tex->cName.c_str());
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
        Msg("! [MaterialCache] Pass has NULL vertex shader");
        return false;
    }
    matPSO->vertexShader = vs;

    // ═══════════════════════════════════════════════════════
    //  EXTRACT PIXEL SHADER
    // ═══════════════════════════════════════════════════════

    SPS* ps = pass->ps._get();
    if (!ps) {
        Msg("! [MaterialCache] Pass has NULL pixel shader");
        return false;
    }
    matPSO->pixelShader = ps;

    // Store debug names
    matPSO->debugName = vs->cName;

    Msg("    VS: '%s', PS: '%s'", vs->cName.c_str(), ps->cName.c_str());

    // TODO: Extract actual bytecode from SVS/SPS
    // The bytecode is stored in vs->sh/ps->sh (ID3D11VertexShader/ID3D11PixelShader)
    // We may need to reflect on these to get input layout and other metadata

    return true;
}

// ══════════════════════════════════════════════════════════
//  CREATE BINDING LAYOUT
// ══════════════════════════════════════════════════════════

nvrhi::BindingLayoutHandle MaterialCache::CreateBindingLayout(const MaterialPSO* matPSO)
{
    VERIFY(matPSO);

    // ═══════════════════════════════════════════════════════
    //  BUILD BINDING LAYOUT DESCRIPTOR
    // ═══════════════════════════════════════════════════════

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;

    // Slot 0: Per-object constant buffer (b0)
    layoutDesc.bindings.push_back(
        nvrhi::BindingLayoutItem::ConstantBuffer(0));

    // Slots 1+: Textures (t0, t1, t2, ...)
    for (u32 i = 0; i < matPSO->textures.size(); i++) {
        layoutDesc.bindings.push_back(
            nvrhi::BindingLayoutItem::Texture_SRV(i));  // t0, t1, t2...
    }

    // Slot N: Sampler (s0)
    // TODO: Add sampler binding
    // For now, we assume a default linear sampler

    Msg("    Creating binding layout: 1 CB + %u textures", matPSO->textures.size());

    // ═══════════════════════════════════════════════════════
    //  CREATE BINDING LAYOUT
    // ═══════════════════════════════════════════════════════

    nvrhi::BindingLayoutHandle layout = m_device->CreateBindingLayout(layoutDesc);
    if (!layout) {
        Msg("! [MaterialCache] Failed to create binding layout");
        return nullptr;
    }

    return layout;
}

// ══════════════════════════════════════════════════════════
//  CREATE BINDING SET
// ══════════════════════════════════════════════════════════

nvrhi::BindingSetHandle MaterialCache::CreateBindingSet(
    const MaterialPSO* matPSO,
    nvrhi::IBuffer* perObjectCB)
{
    VERIFY(matPSO);
    VERIFY(perObjectCB);
    VERIFY(matPSO->bindingLayout);

    // ═══════════════════════════════════════════════════════
    //  BUILD BINDING SET DESCRIPTOR
    // ═══════════════════════════════════════════════════════

    nvrhi::BindingSetDesc bindingDesc;

    // Bind constant buffer at slot 0
    bindingDesc.bindings.push_back(
        nvrhi::BindingSetItem::ConstantBuffer(0, perObjectCB));

    // Bind textures at slots 1+
    for (u32 i = 0; i < matPSO->textures.size(); i++) {
        nvrhi::ITexture* nativeTex = m_device->GetNativeTexture(matPSO->textures[i]);
        if (nativeTex) {
            bindingDesc.bindings.push_back(
                nvrhi::BindingSetItem::Texture_SRV(i, nativeTex));
        }
    }

    // ═══════════════════════════════════════════════════════
    //  CREATE BINDING SET
    // ═══════════════════════════════════════════════════════

    nvrhi::BindingSetHandle bindingSet = m_device->CreateBindingSet(
        bindingDesc,
        matPSO->bindingLayout);

    if (!bindingSet) {
        Msg("! [MaterialCache] Failed to create binding set");
        return nullptr;
    }

    return bindingSet;
}

// ══════════════════════════════════════════════════════════
//  COMPUTE TEXTURE HASH
// ══════════════════════════════════════════════════════════

u64 MaterialCache::ComputeTextureHash(const xr_vector<ng::TextureHandle>& textures)
{
    if (textures.empty())
        return 0;

    // Hash all texture handles using CRC32
    u32 hash = 0;
    for (const auto& tex : textures) {
        // Hash the handle index
        hash = crc32(&tex.index, sizeof(tex.index), hash);
    }

    // Extend to 64-bit (for consistency with hash type)
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

// ══════════════════════════════════════════════════════════
//  FORMAT CONVERSION HELPER
// ══════════════════════════════════════════════════════════

namespace {
    // Convert DXGI_FORMAT to NVRHI format, handling IA-incompatible formats
    nvrhi::Format ConvertVertexFormat(DXGI_FORMAT dxgiFormat) {
        switch (dxgiFormat) {
            // Direct mappings (IA-compatible)
            case DXGI_FORMAT_R32G32B32A32_FLOAT: return nvrhi::Format::RGBA32_FLOAT;
            case DXGI_FORMAT_R32G32B32_FLOAT:    return nvrhi::Format::RGB32_FLOAT;
            case DXGI_FORMAT_R32G32_FLOAT:       return nvrhi::Format::RG32_FLOAT;
            case DXGI_FORMAT_R32_FLOAT:          return nvrhi::Format::R32_FLOAT;

            case DXGI_FORMAT_R16G16B16A16_FLOAT: return nvrhi::Format::RGBA16_FLOAT;
            case DXGI_FORMAT_R16G16_FLOAT:       return nvrhi::Format::RG16_FLOAT;
            case DXGI_FORMAT_R16_FLOAT:          return nvrhi::Format::R16_FLOAT;

            case DXGI_FORMAT_R8G8B8A8_UNORM:     return nvrhi::Format::RGBA8_UNORM;
            case DXGI_FORMAT_R8G8B8A8_SNORM:     return nvrhi::Format::RGBA8_SNORM;
            case DXGI_FORMAT_R8G8B8A8_UINT:      return nvrhi::Format::RGBA8_UINT;
            case DXGI_FORMAT_R8G8B8A8_SINT:      return nvrhi::Format::RGBA8_SINT;

            case DXGI_FORMAT_R16G16B16A16_UNORM: return nvrhi::Format::RGBA16_UNORM;
            case DXGI_FORMAT_R16G16B16A16_SNORM: return nvrhi::Format::RGBA16_SNORM;
            case DXGI_FORMAT_R16G16B16A16_UINT:  return nvrhi::Format::RGBA16_UINT;
            case DXGI_FORMAT_R16G16B16A16_SINT:  return nvrhi::Format::RGBA16_SINT;

            case DXGI_FORMAT_R32G32B32A32_UINT:  return nvrhi::Format::RGBA32_UINT;
            case DXGI_FORMAT_R32G32B32A32_SINT:  return nvrhi::Format::RGBA32_SINT;
            case DXGI_FORMAT_R32G32_UINT:        return nvrhi::Format::RG32_UINT;
            case DXGI_FORMAT_R32G32_SINT:        return nvrhi::Format::RG32_SINT;

            case DXGI_FORMAT_R10G10B10A2_UNORM:  return nvrhi::Format::R10G10B10A2_UNORM;
            case DXGI_FORMAT_R10G10B10A2_UINT:
                Msg("  [MaterialCache] Converting R10G10B10A2_UINT → RGBA16_UINT for compatibility");
                return nvrhi::Format::RGBA16_UINT;
            case DXGI_FORMAT_R11G11B10_FLOAT:    return nvrhi::Format::R11G11B10_FLOAT;

            // IA-incompatible formats - convert to compatible equivalents
            case DXGI_FORMAT_B4G4R4A4_UNORM:     // 4-bit per channel → 8-bit per channel
                Msg("  [MaterialCache] Converting B4G4R4A4_UNORM → RGBA8_UNORM for IA compatibility");
                return nvrhi::Format::RGBA8_UNORM;

            case DXGI_FORMAT_B5G6R5_UNORM:       // 16-bit RGB → RGBA8
                Msg("  [MaterialCache] Converting B5G6R5_UNORM → RGBA8_UNORM for IA compatibility");
                return nvrhi::Format::RGBA8_UNORM;

            case DXGI_FORMAT_B5G5R5A1_UNORM:     // 16-bit RGBA → RGBA8
                Msg("  [MaterialCache] Converting B5G5R5A1_UNORM → RGBA8_UNORM for IA compatibility");
                return nvrhi::Format::RGBA8_UNORM;

            case DXGI_FORMAT_B8G8R8A8_UNORM:     // BGRA → RGBA
                return nvrhi::Format::BGRA8_UNORM;

            case DXGI_FORMAT_B8G8R8X8_UNORM:     // BGRX → BGRA
                return nvrhi::Format::BGRA8_UNORM;

            default:
                Msg("! [MaterialCache] Unknown DXGI format %d, defaulting to RGBA32_FLOAT", dxgiFormat);
                return nvrhi::Format::RGBA32_FLOAT;
        }
    }
}

// ══════════════════════════════════════════════════════════
//  SETUP VERTEX ATTRIBUTES
// ══════════════════════════════════════════════════════════

void MaterialCache::SetupVertexAttributes(dxRender_Visual* visual, ng::PipelineStateDesc& psoDesc)
{
    psoDesc.vertexAttributes.clear();

    // Get geometry from visual
    IRender_Mesh* meshVisual = nullptr;
    switch (visual->getType()) {
        case MT_NORMAL:
            meshVisual = static_cast<Fvisual*>(visual);
            break;
        case MT_TREE_ST:
        case MT_TREE_PM:
            meshVisual = static_cast<FTreeVisual*>(visual);
            break;
        default:
            // Fallback to hardcoded layout
            Msg("! [MaterialCache] Unknown visual type for vertex layout extraction");
            return;
    }

    if (!meshVisual || !meshVisual->rm_geom || !meshVisual->rm_geom._get())
        return;

    SGeometry* geom = meshVisual->rm_geom._get();
    if (!geom->dcl || !geom->dcl._get())
        return;

    SDeclaration* decl = geom->dcl._get();

    // Track seen semantics to avoid duplicates (which cause CreateInputLayout to fail)
    struct SemanticKey {
        xr_string name;
        u32 index;
        bool operator<(const SemanticKey& other) const {
            if (name != other.name) return name < other.name;
            return index < other.index;
        }
    };
    std::set<SemanticKey> seenSemantics;

    // Convert X-Ray's D3D11 input elements to NVRHI format
    for (const auto& d3dElem : decl->dx11_dcl_code) {
        // Skip invalid elements
        if (!d3dElem.SemanticName) {
            Msg("! [MaterialCache] Skipping vertex element with NULL semantic name");
            continue;
        }

        // Check for duplicates
        SemanticKey key;
        key.name = d3dElem.SemanticName;
        key.index = d3dElem.SemanticIndex;

        if (seenSemantics.find(key) != seenSemantics.end()) {
            Msg("! [MaterialCache] Skipping duplicate semantic: %s%d",
                d3dElem.SemanticName, d3dElem.SemanticIndex);
            continue;
        }
        seenSemantics.insert(key);

        ng::VertexAttribute attr;

        // Map semantic name to persistent string literal
        // D3D11 semantic names might be temporary, so we need persistent pointers
        xr_string semanticStr = d3dElem.SemanticName;
        if (semanticStr == "POSITION") {
            attr.semanticName = "POSITION";
        } else if (semanticStr == "NORMAL") {
            attr.semanticName = "NORMAL";
        } else if (semanticStr == "TEXCOORD") {
            attr.semanticName = "TEXCOORD";
        } else if (semanticStr == "TANGENT") {
            attr.semanticName = "TANGENT";
        } else if (semanticStr == "BINORMAL") {
            attr.semanticName = "BINORMAL";
        } else if (semanticStr == "COLOR") {
            attr.semanticName = "COLOR";
        } else if (semanticStr == "BLENDWEIGHT") {
            attr.semanticName = "BLENDWEIGHT";
        } else if (semanticStr == "BLENDINDICES") {
            attr.semanticName = "BLENDINDICES";
        } else {
            // Unknown semantic - use the original but warn
            Msg("! [MaterialCache] Unknown semantic: %s", d3dElem.SemanticName);
            attr.semanticName = d3dElem.SemanticName;
        }

        attr.semanticIndex = d3dElem.SemanticIndex;

        // Convert DXGI format to NVRHI format with proper IA compatibility handling
        attr.format = ConvertVertexFormat(d3dElem.Format);

        attr.offset = d3dElem.AlignedByteOffset;
        attr.bufferIndex = d3dElem.InputSlot;
        attr.isInstanced = (d3dElem.InputSlotClass == D3D11_INPUT_PER_INSTANCE_DATA);

        psoDesc.vertexAttributes.push_back(attr);
    }

    Msg("  Extracted %u vertex attributes from geometry", psoDesc.vertexAttributes.size());
}

// ══════════════════════════════════════════════════════════
//  SETUP RENDER STATES
// ══════════════════════════════════════════════════════════

void MaterialCache::SetupRenderStates(SPass* pass, ng::PipelineStateDesc& psoDesc)
{
    // TODO: Extract actual render states from X-Ray SPass->state
    // For now, use default GBuffer rendering states

    // Rasterizer state
    psoDesc.rasterizerState.cullMode = ng::CullMode::Back;
    psoDesc.rasterizerState.fillMode = ng::FillMode::Solid;
    psoDesc.rasterizerState.frontCounterClockwise = false;
    psoDesc.rasterizerState.depthClipEnable = true;

    // Depth/Stencil state
    psoDesc.depthStencilState.depthTestEnable = true;
    psoDesc.depthStencilState.depthWriteEnable = true;
    psoDesc.depthStencilState.depthFunc = ng::ComparisonFunc::Less;
    psoDesc.depthStencilState.stencilEnable = false;

    // Blend state (opaque rendering for GBuffer)
    psoDesc.blendState.renderTargets[0].blendEnable = false;
    psoDesc.blendState.renderTargets[0].writeMask = ng::ColorWriteMask::All;
}

// ══════════════════════════════════════════════════════════
//  SETUP RENDER TARGETS
// ══════════════════════════════════════════════════════════

void MaterialCache::SetupRenderTargets(
    const passes::GBufferOutputs& outputs,
    const xray::render::framegraph::FrameGraph& fg,
    ng::PipelineStateDesc& psoDesc)
{
    // GBuffer has 3 render targets + depth
    psoDesc.renderTargetCount = 3;

    // RT0: Albedo.rgb + Metallic.a (RGBA8)
    psoDesc.renderTargetFormats[0] = nvrhi::Format::RGBA8_UNORM;

    // RT1: Normal.xyz + Roughness.a (RGBA8_SNORM for normals)
    psoDesc.renderTargetFormats[1] = nvrhi::Format::RGBA8_SNORM;

    // RT2: Material ID (R16_UINT)
    psoDesc.renderTargetFormats[2] = nvrhi::Format::R16_UINT;

    // Depth/Stencil: D24S8
    psoDesc.depthStencilFormat = nvrhi::Format::D24S8;
}

// ══════════════════════════════════════════════════════════
//  CLEAR CACHE
// ══════════════════════════════════════════════════════════

void MaterialCache::Clear()
{
    Msg("~ [MaterialCache] Clearing cache (%u PSOs)", m_stats.numCachedPSOs);
    m_cache.clear();
    m_stats = Stats{};
}

} // namespace xray::render
