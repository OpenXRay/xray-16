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

#if defined(USE_DX11)
#include "Layers/xrRenderDX11/StateManager/dx11State.h"
#include "Layers/xrRenderDX11/StateManager/dx11SamplerStateCache.h"  // For sampler extraction
#include "Layers/xrRenderDX11/dx11ConstantBuffer.h"  // For CB size extraction
#include "../Externals/nvrhi/src/common/dxgi-format.h"  // For DXGI <-> NVRHI format conversion
#include "../Externals/nvrhi/src/d3d11/d3d11-backend.h"  // For D3D11 BindingSet access
#endif

namespace xray::render {

using namespace passes;
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
    Msg("! [MaterialCache] Failed to convert DXGI_FORMAT %u to nvrhi::Format", dxgiFormat);
    return nvrhi::Format::UNKNOWN;
}

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

    case DXGI_FORMAT_R16G16B16A16_UNORM: return nvrhi::Format::RGBA16_UNORM;
    case DXGI_FORMAT_R16G16B16A16_SNORM: return nvrhi::Format::RGBA16_SNORM;
    case DXGI_FORMAT_R16G16B16A16_UINT:  return nvrhi::Format::RGBA16_UINT;
    case DXGI_FORMAT_R16G16B16A16_SINT:  return nvrhi::Format::RGBA16_SINT;

    case DXGI_FORMAT_R16G16_UNORM:       return nvrhi::Format::RG16_UNORM;
    case DXGI_FORMAT_R16G16_SNORM:       return nvrhi::Format::RG16_SNORM;
    case DXGI_FORMAT_R16G16_UINT:        return nvrhi::Format::RG16_UINT;
    case DXGI_FORMAT_R16G16_SINT:        return nvrhi::Format::RG16_SINT;

    case DXGI_FORMAT_R8G8B8A8_UNORM:     return nvrhi::Format::RGBA8_UNORM;
    case DXGI_FORMAT_R8G8B8A8_SNORM:     return nvrhi::Format::RGBA8_SNORM;
    case DXGI_FORMAT_R8G8B8A8_UINT:      return nvrhi::Format::RGBA8_UINT;
    case DXGI_FORMAT_R8G8B8A8_SINT:      return nvrhi::Format::RGBA8_SINT;

    case DXGI_FORMAT_R32G32B32A32_UINT:  return nvrhi::Format::RGBA32_UINT;
    case DXGI_FORMAT_R32G32B32A32_SINT:  return nvrhi::Format::RGBA32_SINT;
    case DXGI_FORMAT_R32G32_UINT:        return nvrhi::Format::RG32_UINT;
    case DXGI_FORMAT_R32G32_SINT:        return nvrhi::Format::RG32_SINT;

    case DXGI_FORMAT_R10G10B10A2_UNORM:  return nvrhi::Format::R10G10B10A2_UNORM;
    case DXGI_FORMAT_R10G10B10A2_UINT:   return nvrhi::Format::RGBA16_UINT;
    case DXGI_FORMAT_R11G11B10_FLOAT:    return nvrhi::Format::R11G11B10_FLOAT;

        // IA-incompatible formats - convert to compatible equivalents
    case DXGI_FORMAT_B4G4R4A4_UNORM:     return nvrhi::Format::RGBA8_UNORM;
    case DXGI_FORMAT_B5G6R5_UNORM:       return nvrhi::Format::RGBA8_UNORM;
    case DXGI_FORMAT_B5G5R5A1_UNORM:     return nvrhi::Format::RGBA8_UNORM;

    case DXGI_FORMAT_B8G8R8A8_UNORM:     // BGRA → RGBA
        return nvrhi::Format::BGRA8_UNORM;

    case DXGI_FORMAT_B8G8R8X8_UNORM:     // BGRX → BGRA
        return nvrhi::Format::BGRA8_UNORM;

    default:
        Msg("! [MaterialCache] Unknown DXGI format %d, defaulting to RGBA32_FLOAT", dxgiFormat);
        return nvrhi::Format::RGBA32_FLOAT;
    }
}

// ══════════════════════════════════════════════════════════
//  CONSTRUCTOR / DESTRUCTOR
// ══════════════════════════════════════════════════════════

MaterialCache::MaterialCache(ng::RenderDevice* device)
    : m_device(device)
{
    VERIFY(m_device);
}

MaterialCache::~MaterialCache() {
    Clear();
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

    // Compute hash based on X-Ray texture pointers (stable across frames)
    u64 textureHash = ComputeTextureHash(pass);
    u64 stateHash = ComputeStateHash(pass);

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
        // Msg("  [MaterialCache] Cache HIT for '%s' (hits=%u, misses=%u)",
        //     shaderName, m_stats.numCacheHits, m_stats.numCacheMisses);
        return it->second.get();
    }

    // ═══════════════════════════════════════════════════════
    //  CACHE MISS - CREATE NEW PSO
    // ═══════════════════════════════════════════════════════

    m_stats.numCacheMisses++;
    Msg("! [MaterialCache] Cache MISS - creating PSO for '%s' (hash: tex=0x%llX, state=0x%llX) [hits=%u, misses=%u]",
        shaderName, textureHash, stateHash, m_stats.numCacheHits, m_stats.numCacheMisses);

    MaterialPSO* pso = CreatePSO(visual, elem, pass, outputs, fg);
    if (!pso) {
        Msg("! [MaterialCache] Failed to create PSO");
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
    const passes::GBufferOutputs& outputs,
    const xray::render::framegraph::FrameGraph& fg)
{
    auto pso = xr_make_unique<MaterialPSO>();

    // Store the pass for later use (SRV extraction during binding)
    pso->pass = pass;

    // ═══════════════════════════════════════════════════════
    //  EXTRACT TEXTURES
    // ═══════════════════════════════════════════════════════

    ExtractTextures(pass, pso.get());

    // ═══════════════════════════════════════════════════════
    //  EXTRACT SHADERS
    // ═══════════════════════════════════════════════════════

    if (!ExtractShaders(pass, pso.get())) {
        Msg("! [MaterialCache] Failed to extract shaders");
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  EXTRACT SAMPLERS
    // ═══════════════════════════════════════════════════════

    ExtractSamplers(pass, pso.get());

    // ═══════════════════════════════════════════════════════
    //  CREATE BINDING LAYOUT
    // ═══════════════════════════════════════════════════════

    pso->bindingLayout = CreateBindingLayout(pso.get());
    if (!pso->bindingLayout) {
        Msg("! [MaterialCache] Failed to create binding layout");
        return nullptr;
    }

    // Note: We don't create the binding set here because it includes the per-object CB
    // which changes every draw. We'll create it on-demand in GBufferPass with the CB.

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

    // Set up render target formats from GBufferOutputs (using shader reflection)
    SetupRenderTargets(pso.get(), outputs, fg, psoDesc);

    // Set debug name
    psoDesc.debugName = pso->debugName;

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

        // Debug: Check SRV format vs resource format
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srv->GetDesc(&srvDesc);

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

        // Debug: Check if texture has BIND_SHADER_RESOURCE flag
        bool hasBindSRV = (d3dDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0;
        if (!hasBindSRV) {
            Msg("! [MaterialCache] WARNING: Texture '%s' does NOT have D3D11_BIND_SHADER_RESOURCE flag! BindFlags=0x%x",
                tex->cName.c_str(), d3dDesc.BindFlags);
        }

        // Debug logging for format mismatches
        if (srvDesc.Format != d3dDesc.Format) {
            Msg("! [MaterialCache] Format mismatch for texture '%s': Resource=0x%x (%u), SRV=0x%x (%u)",
                tex->cName.c_str(),
                d3dDesc.Format, d3dDesc.Format,
                srvDesc.Format, srvDesc.Format);
        }

        // Build TextureDesc - use resource format and let NVRHI handle SRV creation
        ng::RenderDevice::TextureDesc texDesc;
        texDesc.width = d3dDesc.Width;
        texDesc.height = d3dDesc.Height;
        texDesc.mipLevels = d3dDesc.MipLevels;
        texDesc.arraySize = d3dDesc.ArraySize;

        // Detect texture dimension (2D vs Cube vs Array)
        if (d3dDesc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) {
            texDesc.dimension = ng::RenderDevice::TextureDesc::TextureCube;
        } else if (d3dDesc.ArraySize > 1) {
            texDesc.dimension = ng::RenderDevice::TextureDesc::Texture2DArray;
        } else {
            texDesc.dimension = ng::RenderDevice::TextureDesc::Texture2D;
        }

        // Convert DXGI format to NVRHI format using proper lookup
        // DO NOT use static_cast - the enum values are completely different!
        texDesc.format = ConvertDxgiFormatToNvrhi(d3dDesc.Format);
        texDesc.isRenderTarget = false;
        texDesc.isUAV = false;
        texDesc.debugName = tex->cName.c_str();

        // Wrap in NVRHI handle using device abstraction
        Msg("  [MaterialCache] Wrapping texture '%s' with format %u", tex->cName.c_str(), texDesc.format);
        ng::TextureHandle nvrhiTex = m_device->CreateTextureFromD3D11(d3dResource, texDesc);
        d3dResource->Release();  // Release our ref, NVRHI holds its own

        if (!nvrhiTex.IsValid()) {
            Msg("! [MaterialCache] Failed to wrap texture '%s'", tex->cName.c_str());
            continue;
        }

        Msg("  [MaterialCache] Successfully wrapped texture '%s' at slot %u", tex->cName.c_str(), stage);
        MaterialPSO::TextureSlot texSlot;
        texSlot.slot = stage;
        texSlot.handle = nvrhiTex;
        matPSO->textures.push_back(texSlot);
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

    // ═══════════════════════════════════════════════════
    //  SHADER REFLECTION (Week 15)
    // ═══════════════════════════════════════════════════

    // Try to analyze pixel shader via D3D reflection
    // X-Ray's SPS structure has: sh (ID3D11PixelShader*), bytecode (ID3DBlob*)
    if (ps->sh && ps->bytecode) {
        Msg("! [MaterialCache] Performing shader reflection on PS '%s'", ps->cName.c_str());

        // Analyze pixel shader using SPS's bytecode directly
        matPSO->rtBindings = framegraph::ShaderReflector::AnalyzePixelShader(
            ps->sh,
            ps->bytecode
        );

        matPSO->rtBindings.shaderName = ps->cName;

        Msg("! [MaterialCache] Shader reflection complete:");
        Msg("!   Phase: %d", (int)matPSO->rtBindings.phase);
        Msg("!   Input textures: %u", matPSO->rtBindings.inputTextures.size());
        Msg("!   Output RTs: %u", matPSO->rtBindings.outputRTs.size());
    } else {
        Msg("! [MaterialCache] Cannot perform shader reflection - no bytecode available");
    }

    // ═══════════════════════════════════════════════════════
    //  EXTRACT ALL CONSTANT BUFFERS from VS and PS
    // ═══════════════════════════════════════════════════════

    matPSO->constantBuffers.clear();

    // Helper lambda to extract CBs from a shader's constant table
    auto extractCBsFromShader = [&](R_constant_table& constTable, const char* shaderName) {
        // X-Ray has multiple rendering contexts (R__NUM_CONTEXTS = 5)
        // Check all contexts to find constant buffers
        for (u32 contextIdx = 0; contextIdx < R__NUM_CONTEXTS; contextIdx++) {
            const auto& cbTable = constTable.m_CBTable[contextIdx];

            if (cbTable.empty())
                continue;

            Msg("  [MaterialCache] Shader '%s' context %u has %u CBs",
                shaderName, contextIdx, cbTable.size());

            for (const auto& cbRecord : cbTable) {
                u32 encodedSlot = cbRecord.first;
                dx11ConstantBuffer* cb = cbRecord.second._get();

                // Decode the slot to get shader type and binding slot
                u32 shaderType = dx11ConstantBuffer::DecodeShaderType(encodedSlot);
                u32 bindingSlot = dx11ConstantBuffer::DecodeBindingSlot(encodedSlot);
                const char* shaderTypeName = dx11ConstantBuffer::GetShaderTypeName(shaderType);

                Msg("  [MaterialCache]   Checking encoded slot 0x%02X (%s b%u) in context %u",
                    encodedSlot, shaderTypeName, bindingSlot, contextIdx);

                if (cb) {
                    ID3DBuffer* d3dBuffer = cb->GetBuffer();
                    if (d3dBuffer) {
                        D3D11_BUFFER_DESC bufDesc;
                        d3dBuffer->GetDesc(&bufDesc);

                        Msg("  [MaterialCache]   CB at %s b%u: %u bytes",
                            shaderTypeName, bindingSlot, bufDesc.ByteWidth);

                    // Check if we already have this binding slot (could be in both VS and PS)
                    bool found = false;
                    for (const auto& existing : matPSO->constantBuffers) {
                        if (existing.slot == bindingSlot) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        nvrhi::BufferHandle bufferHandle;

                        if (bindingSlot == 0) {
                            // Slot 0 (per-object): Wrap X-Ray's buffer (it's managed per-draw)
                            nvrhi::BufferDesc nvrhiDesc;
                            nvrhiDesc.byteSize = bufDesc.ByteWidth;
                            nvrhiDesc.isConstantBuffer = true;
                            nvrhiDesc.debugName = "XRay_PerObject_CB";
                            nvrhiDesc.keepInitialState = true;
                            nvrhiDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

                            bufferHandle = m_device->GetNativeDevice()->createHandleForNativeBuffer(
                                nvrhi::ObjectTypes::D3D11_Buffer,
                                nvrhi::Object(d3dBuffer),
                                nvrhiDesc);
                        } else {
                            // Slot 1+ (global): Create NEW buffer that WE control
                            // Don't wrap X-Ray's buffer because RCache might clear/reset it
                            // Use DEFAULT usage to support UpdateSubresource (no CPU access needed)
                            nvrhi::BufferDesc nvrhiDesc;
                            nvrhiDesc.byteSize = bufDesc.ByteWidth;
                            nvrhiDesc.isConstantBuffer = true;
                            nvrhiDesc.debugName = "FrameGraph_Global_CB";
                            nvrhiDesc.keepInitialState = false;
                            nvrhiDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
                            // Don't set cpuAccess - we'll use UpdateSubresource which works with DEFAULT usage

                            bufferHandle = m_device->GetNativeDevice()->createBuffer(nvrhiDesc);
                            Msg("  [MaterialCache]   Created NEW global CB for slot %u (%u bytes)",
                                bindingSlot, bufDesc.ByteWidth);
                        }

                        if (bufferHandle) {
                            MaterialPSO::ConstantBufferInfo cbInfo;
                            cbInfo.slot = bindingSlot;  // Use DECODED slot for NVRHI binding!
                            cbInfo.nvrhiBuffer = bufferHandle;
                            cbInfo.size = bufDesc.ByteWidth;
                            cbInfo.isPerObject = (bindingSlot == 0);  // Check decoded slot 0

                            // DISABLED: CB data extraction causes memory corruption
                            // TODO: Debug why this corrupts the heap
                            #if 0
                            if (bindingSlot > 0) {
                                void* xrayData = cb->GetBufferData();
                                u32 xraySize = cb->GetBufferSize();

                                if (xrayData && xraySize > 0 && xraySize == bufDesc.ByteWidth) {
                                    cbInfo.initialData.resize(xraySize);
                                    memcpy(cbInfo.initialData.data(), xrayData, xraySize);
                                    Msg("  [MaterialCache]   Extracted %u bytes of global CB data", xraySize);
                                }
                            }
                            #endif

                            matPSO->constantBuffers.push_back(cbInfo);

                            Msg("  [MaterialCache]   SUCCESS: Added CB to PSO at %s b%u: %u bytes (isPerObject=%d)",
                                shaderTypeName, bindingSlot, cbInfo.size, cbInfo.isPerObject);
                        } else {
                            Msg("! [MaterialCache]   FAILED: Could not wrap D3D11 buffer at %s b%u (encoded=0x%02X)",
                                shaderTypeName, bindingSlot, encodedSlot);
                        }
                    }
                }
            }
            }  // end for cbRecord
        }  // end for contextIdx
    };

    // Extract from vertex shader
    extractCBsFromShader(vs->constants, vs->cName.c_str());

    // Extract from pixel shader
    extractCBsFromShader(ps->constants, ps->cName.c_str());

    // Store per-object CB size for convenience
    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.isPerObject) {
            matPSO->perObjectCBSize = cbInfo.size;
            break;
        }
    }

    // Fallback if no slot 0 CB found
    if (matPSO->perObjectCBSize == 0) {
        Msg("! [MaterialCache] No slot 0 CB found, using default 256 bytes");
        matPSO->perObjectCBSize = 256;
    }

    return true;
}

// ══════════════════════════════════════════════════════════
//  EXTRACT SAMPLERS
// ══════════════════════════════════════════════════════════

void MaterialCache::ExtractSamplers(SPass* pass, MaterialPSO* matPSO)
{
    VERIFY(pass);
    VERIFY(matPSO);

#if defined(USE_DX11)
    // Get X-Ray's render state from the pass
    void* statePtr = pass->state._get();
    if (!statePtr) {
        Msg("! [MaterialCache] Pass has NULL state");
        return;
    }

    dx11State* xrState = static_cast<dx11State*>(statePtr);

    // Extract pixel shader samplers (most common)
    // NOTE: X-Ray's sampler arrays are sparse - most slots are invalid
    // D3D11 has max 16 sampler slots per stage, so only check those
    const auto& psSamplers = xrState->GetPSSamplers();
    u32 maxSamplerSlots = std::min<u32>(psSamplers.size(), D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

    for (u32 slot = 0; slot < maxSamplerSlots; ++slot) {
        auto samplerHandle = psSamplers[slot];
        if (samplerHandle == dx11SamplerStateCache::hInvalidHandle)
            continue;

        // Get the D3D11 sampler state from SSManager
        ID3DSamplerState* d3dSampler = SSManager.GetSamplerState(samplerHandle);
        if (!d3dSampler) {
            Msg("! [MaterialCache] Failed to get D3D11 sampler for PS slot %u", slot);
            continue;
        }

        // Get sampler desc for debugging
        D3D11_SAMPLER_DESC samplerDesc;
        d3dSampler->GetDesc(&samplerDesc);

        // Wrap X-Ray's D3D11 sampler in NVRHI
        nvrhi::SamplerDesc nvrhiDesc;
        nvrhiDesc.setMinFilter(samplerDesc.Filter != D3D11_FILTER_MIN_MAG_MIP_POINT);
        nvrhiDesc.setMagFilter(samplerDesc.Filter != D3D11_FILTER_MIN_MAG_MIP_POINT);
        nvrhiDesc.setMipFilter(samplerDesc.Filter != D3D11_FILTER_MIN_MAG_MIP_POINT);

        // Convert address modes
        auto convertAddressMode = [](D3D11_TEXTURE_ADDRESS_MODE mode) {
            switch (mode) {
                case D3D11_TEXTURE_ADDRESS_WRAP: return nvrhi::SamplerAddressMode::Wrap;
                case D3D11_TEXTURE_ADDRESS_CLAMP: return nvrhi::SamplerAddressMode::Clamp;
                case D3D11_TEXTURE_ADDRESS_MIRROR: return nvrhi::SamplerAddressMode::Mirror;
                case D3D11_TEXTURE_ADDRESS_BORDER: return nvrhi::SamplerAddressMode::Border;
                default: return nvrhi::SamplerAddressMode::Wrap;
            }
        };

        nvrhiDesc.setAddressU(convertAddressMode(samplerDesc.AddressU));
        nvrhiDesc.setAddressV(convertAddressMode(samplerDesc.AddressV));
        nvrhiDesc.setAddressW(convertAddressMode(samplerDesc.AddressW));
        nvrhiDesc.setMaxAnisotropy(static_cast<float>(samplerDesc.MaxAnisotropy));
        nvrhiDesc.setMipBias(samplerDesc.MipLODBias);

        nvrhi::SamplerHandle nvrhiSampler = m_device->GetNativeDevice()->createSampler(nvrhiDesc);
        if (nvrhiSampler) {
            MaterialPSO::SamplerInfo samplerInfo;
            samplerInfo.slot = slot;
            samplerInfo.nvrhiSampler = nvrhiSampler;
            matPSO->samplers.push_back(samplerInfo);

            Msg("  [MaterialCache] Extracted PS sampler at slot %u (aniso=%.1f)",
                slot, nvrhiDesc.maxAnisotropy);
        } else {
            Msg("! [MaterialCache] Failed to create NVRHI sampler for PS slot %u", slot);
        }
    }

    // TODO: Extract VS, GS, CS, HS, DS samplers if needed

    Msg("  [MaterialCache] Extracted %u samplers", matPSO->samplers.size());
#endif
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

    // Add ALL constant buffers from shader reflection
    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.isPerObject) {
            // Slot 0: Per-object VOLATILE constant buffer (updated per-draw)
            layoutDesc.bindings.push_back(
                nvrhi::BindingLayoutItem::VolatileConstantBuffer(cbInfo.slot));
        } else {
            // Other slots: Regular constant buffers (global state, updated per-frame)
            layoutDesc.bindings.push_back(
                nvrhi::BindingLayoutItem::ConstantBuffer(cbInfo.slot));
        }
    }

    // Textures (t0, t1, t2, ...) - use actual texture slots from X-Ray
    for (const auto& texSlot : matPSO->textures) {
        layoutDesc.bindings.push_back(
            nvrhi::BindingLayoutItem::Texture_SRV(texSlot.slot));
    }

    // Samplers (s0, s1, s2, ...)
    for (const auto& samplerInfo : matPSO->samplers) {
        layoutDesc.bindings.push_back(
            nvrhi::BindingLayoutItem::Sampler(samplerInfo.slot));
    }

    nvrhi::BindingLayoutHandle layout = m_device->CreateBindingLayout(layoutDesc);
    if (!layout) {
        Msg("! [MaterialCache] Failed to create binding layout");
        return nullptr;
    }

    return layout;
}

// ══════════════════════════════════════════════════════════
//  CREATE MATERIAL BINDING SET (textures only)
// ══════════════════════════════════════════════════════════

nvrhi::BindingSetHandle MaterialCache::CreateMaterialBindingSet(const MaterialPSO* matPSO)
{
    VERIFY(matPSO);
    VERIFY(matPSO->bindingLayout);

    // ═══════════════════════════════════════════════════════
    //  BUILD BINDING SET DESCRIPTOR (textures only)
    // ═══════════════════════════════════════════════════════

    nvrhi::BindingSetDesc bindingDesc;

    // Bind textures at their actual slots from X-Ray
    for (const auto& texSlot : matPSO->textures) {
        nvrhi::ITexture* nativeTex = m_device->GetNativeTexture(texSlot.handle);
        if (nativeTex) {
            bindingDesc.bindings.push_back(
                nvrhi::BindingSetItem::Texture_SRV(texSlot.slot, nativeTex));
        }
    }

    // TODO: Add sampler bindings (s0, s1, etc.)
    // For now, rely on default samplers

    // ═══════════════════════════════════════════════════════
    //  CREATE BINDING SET
    // ═══════════════════════════════════════════════════════

    nvrhi::BindingSetHandle bindingSet = m_device->CreateBindingSet(
        bindingDesc,
        matPSO->bindingLayout);

    if (!bindingSet) {
        Msg("! [MaterialCache] Failed to create material binding set");
        return nullptr;
    }

    return bindingSet;
}

// ══════════════════════════════════════════════════════════
//  GET OR CREATE CACHED BINDING SET (with per-object VCB)
// ══════════════════════════════════════════════════════════

nvrhi::BindingSetHandle MaterialCache::GetOrCreateBindingSet(
    MaterialPSO* matPSO,
    nvrhi::IBuffer* perObjectVCB,
    SPass* pass)
{
    VERIFY(matPSO);
    VERIFY(perObjectVCB);
    VERIFY(matPSO->bindingLayout);

    // ═══════════════════════════════════════════════════════
    //  CHECK CACHE - Return existing binding set if already created
    // ═══════════════════════════════════════════════════════

    if (matPSO->bindingSet) {
        // Already cached - reuse it!
        return matPSO->bindingSet;
    }

    // ═══════════════════════════════════════════════════════
    //  BUILD BINDING SET DESCRIPTOR
    // ═══════════════════════════════════════════════════════

    nvrhi::BindingSetDesc bindingDesc;

    // Bind ALL CBs to bindingDesc so NVRHI keeps them alive
    // We'll also manually set them in D3D11 binding set to ensure correct binding
    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.isPerObject) {
            // Slot 0: Use the per-object VCB passed in (updated per-draw via WriteBuffer)
            bindingDesc.bindings.push_back(
                nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, perObjectVCB));
            Msg("  [MaterialCache] Adding per-object CB to bindingDesc at slot %u (%u bytes)",
                cbInfo.slot, cbInfo.size);
        } else {
            // Slots 1+: Add global CBs to keep them alive
            if (cbInfo.nvrhiBuffer) {
                bindingDesc.bindings.push_back(
                    nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, cbInfo.nvrhiBuffer.Get()));
                Msg("  [MaterialCache] Adding global CB to bindingDesc at slot %u (%u bytes)",
                    cbInfo.slot, cbInfo.size);
            }
        }
    }

    // Bind textures at their actual slots from X-Ray
    Msg("  [MaterialCache] Creating binding set with %u textures", matPSO->textures.size());
    for (const auto& texSlot : matPSO->textures) {
        nvrhi::ITexture* nativeTex = m_device->GetNativeTexture(texSlot.handle);
        if (nativeTex) {
            // Get texture descriptor
            const nvrhi::TextureDesc& texDesc = nativeTex->getDesc();
            Msg("  [MaterialCache] Binding texture at slot %u: format=%u (0x%x), dimension=%u, arraySize=%u, mipLevels=%u",
                texSlot.slot, texDesc.format, texDesc.format, texDesc.dimension, texDesc.arraySize, texDesc.mipLevels);

            // Create binding item with proper dimension
            nvrhi::BindingSetItem item = {};  // Zero-initialize to avoid garbage
            item.resourceHandle = nativeTex;
            item.slot = texSlot.slot;  // Use actual slot from X-Ray!
            item.type = nvrhi::ResourceType::Texture_SRV;
            item.format = texDesc.format;

            Msg("  [MaterialCache] Set item.format = %u (0x%x)", item.format, item.format);

            // Set dimension based on texture type
            if (texDesc.dimension == nvrhi::TextureDimension::TextureCube) {
                item.dimension = nvrhi::TextureDimension::TextureCube;
            } else if (texDesc.dimension == nvrhi::TextureDimension::Texture2DArray) {
                item.dimension = nvrhi::TextureDimension::Texture2DArray;
            } else if (texDesc.dimension == nvrhi::TextureDimension::Texture3D) {
                item.dimension = nvrhi::TextureDimension::Texture3D;
            } else {
                item.dimension = nvrhi::TextureDimension::Texture2D;
            }

            // Set subresource range (all mips and array slices)
            item.subresources = nvrhi::AllSubresources;

            bindingDesc.bindings.push_back(item);
        } else {
            Msg("! [MaterialCache] Texture at slot %u is NULL when creating binding set", texSlot.slot);
        }
    }

    // Bind samplers at slots s0, s1, s2, etc.
    for (const auto& samplerInfo : matPSO->samplers) {
        if (samplerInfo.nvrhiSampler) {
            bindingDesc.bindings.push_back(
                nvrhi::BindingSetItem::Sampler(samplerInfo.slot, samplerInfo.nvrhiSampler));
            Msg("  [MaterialCache] Binding sampler at slot %u", samplerInfo.slot);
        }
    }

    // ═══════════════════════════════════════════════════════
    //  CREATE BINDING SET
    // ═══════════════════════════════════════════════════════

    Msg("  [MaterialCache] Creating NEW binding set (will be cached)");
    nvrhi::BindingSetHandle bindingSet = m_device->CreateBindingSet(
        bindingDesc,
        matPSO->bindingLayout);

    if (!bindingSet) {
        Msg("! [MaterialCache] Failed to create binding set");
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  WORKAROUND: Manually set SRVs from X-Ray textures
    // ═══════════════════════════════════════════════════════
    // NVRHI's createBindingSet tries to create new SRVs from wrapped resources,
    // but this fails because we only wrapped the resource, not the SRV.
    // X-Ray already has SRVs, so we'll use those directly!

#if defined(USE_DX11)
    nvrhi::d3d11::BindingSet* d3d11Set = static_cast<nvrhi::d3d11::BindingSet*>(bindingSet.Get());
    if (d3d11Set) {
        // CRITICAL: Initialize ALL min/max ranges immediately after creation
        // NVRHI uses these values in VSSetShaderResources/etc, must not be garbage!
        d3d11Set->minSRVSlot = 0;
        d3d11Set->maxSRVSlot = 0;
        d3d11Set->minSamplerSlot = 0;
        d3d11Set->maxSamplerSlot = 0;
        d3d11Set->minConstantBufferSlot = 0;
        d3d11Set->maxConstantBufferSlot = 0;
        d3d11Set->minUAVSlot = 0;
        d3d11Set->maxUAVSlot = 0;

        if (pass) {
            // Get X-Ray's texture list from the pass
            STextureList* texList = pass->T._get();
            if (texList && texList->size() > 0) {
                Msg("  [MaterialCache] Manually setting SRVs from X-Ray textures...");

                // Reset min to UINT_MAX so we can find the actual minimum
                d3d11Set->minSRVSlot = UINT_MAX;
            }

            // Set SRVs for each texture slot
            // NOTE: texList is a vector of (stage, texture) pairs where stage is the slot number!
            for (u32 i = 0; i < texList->size(); i++) {
                const auto& texPair = (*texList)[i];
                u32 stage = texPair.first;  // Actual SRV slot (t0, t1, t2, etc.)
                CTexture* tex = texPair.second._get();

                // Validate slot range
                if (stage >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT) {
                    Msg("  ! Texture '%s' has invalid slot %u (max %u)",
                        tex ? tex->cName.c_str() : "NULL", stage, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
                    continue;
                }

                if (tex) {
                    ID3DShaderResourceView* xraySRV = tex->get_SRView();
                    if (xraySRV) {
                        // Don't AddRef - NVRHI and X-Ray manage lifetimes
                        d3d11Set->SRVs[stage] = xraySRV;  // Use STAGE, not i!
                        Msg("    Set SRV at slot %u: %p (from X-Ray texture '%s', AddRef'd)",
                            stage, xraySRV, tex->cName.c_str());

                        // Update min/max range
                        if (d3d11Set->minSRVSlot > stage)
                            d3d11Set->minSRVSlot = stage;
                        if (d3d11Set->maxSRVSlot < stage)
                            d3d11Set->maxSRVSlot = stage;
                    } else {
                        Msg("  ! Texture '%s' has NULL SRV at slot %u", tex->cName.c_str(), stage);
                    }
                }
            }

            Msg("  [MaterialCache] SRV range: [%u, %u]",
                d3d11Set->minSRVSlot, d3d11Set->maxSRVSlot);
        }

        // ═══════════════════════════════════════════════════════
        //  MANUALLY SET CONSTANT BUFFERS IN BINDING SET
        // ═══════════════════════════════════════════════════════
        // NVRHI's createBindingSet doesn't properly handle our new global CB buffers,
        // so we need to manually set ALL CBs (including slot 0) in the D3D11 binding set
        if (!matPSO->constantBuffers.empty()) {
            Msg("  [MaterialCache] Manually setting %u constant buffers in D3D11 binding set...",
                (u32)matPSO->constantBuffers.size());

            // Reset min to find actual minimum
            d3d11Set->minConstantBufferSlot = UINT_MAX;

            for (const auto& cbInfo : matPSO->constantBuffers) {
                nvrhi::IBuffer* nvrhiBufferToUse = nullptr;

                if (cbInfo.isPerObject) {
                    // Slot 0: Use the VCB passed in (same as in BindingSetDesc)
                    nvrhiBufferToUse = perObjectVCB;
                    Msg("    Setting slot %u to VCB (per-object)", cbInfo.slot);
                } else {
                    // Slots 1+: Use our custom global CB
                    nvrhiBufferToUse = cbInfo.nvrhiBuffer.Get();
                    Msg("    Setting slot %u to custom global CB", cbInfo.slot);
                }

                if (nvrhiBufferToUse) {
                    // Get D3D11 buffer from NVRHI handle
                    ID3D11Buffer* d3dBuffer = static_cast<ID3D11Buffer*>(
                        nvrhiBufferToUse->getNativeObject(nvrhi::ObjectTypes::D3D11_Buffer).pointer);

                    if (d3dBuffer) {
                        // Don't AddRef - NVRHI manages buffer lifetimes
                        // Set in the D3D11 binding set's constant buffer array
                        d3d11Set->constantBuffers[cbInfo.slot] = d3dBuffer;

                        Msg("    Set CB at slot %u: %p (%u bytes, %s)",
                            cbInfo.slot, d3dBuffer, cbInfo.size,
                            cbInfo.isPerObject ? "per-object" : "global");

                        // Update CB range
                        if (d3d11Set->minConstantBufferSlot > cbInfo.slot)
                            d3d11Set->minConstantBufferSlot = cbInfo.slot;
                        if (d3d11Set->maxConstantBufferSlot < cbInfo.slot)
                            d3d11Set->maxConstantBufferSlot = cbInfo.slot;
                    }
                }
            }

            Msg("  [MaterialCache] CB range: [%u, %u]",
                d3d11Set->minConstantBufferSlot, d3d11Set->maxConstantBufferSlot);
        }
    }
#endif

    // ═══════════════════════════════════════════════════════
    //  CACHE IN MaterialPSO for reuse
    // ═══════════════════════════════════════════════════════

    matPSO->bindingSet = bindingSet;
    Msg("  [MaterialCache] Successfully created and cached binding set");

    return bindingSet;
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

    // Hash X-Ray CTexture pointers (stable across frames)
    // We hash the actual CTexture pointer, NOT the wrapped NVRHI handle
    u32 hash = 0;
    for (const auto& texPair : *texList) {
        CTexture* tex = texPair.second._get();
        if (tex) {
            // Hash the CTexture pointer (identifies the texture uniquely)
            void* texPtr = tex;
            hash = crc32(&texPtr, sizeof(texPtr), hash);
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
            Msg("! [MaterialCache] Unknown DXGI format size: %d", format);
            return 4;  // Default fallback
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

        // Update stride to be at least this large
        bufferStrides[slot] = std::max(bufferStrides[slot], endOffset);
    }

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

        // CRITICAL: Set the element stride for this buffer slot!
        // This is what NVRHI uses to bind vertex buffers with correct stride
        attr.elementStride = bufferStrides[d3dElem.InputSlot];

        psoDesc.vertexAttributes.push_back(attr);
    }

    // Debug: Log computed strides
    for (const auto& [slot, stride] : bufferStrides) {
        Msg("  [MaterialCache] Vertex buffer slot %u: stride = %u bytes", slot, stride);
    }
}

// ══════════════════════════════════════════════════════════
//  STATE CONVERSION HELPERS
// ══════════════════════════════════════════════════════════

namespace {
    // Convert D3D11 cull mode to NVRHI
    ng::CullMode ConvertCullMode(D3D11_CULL_MODE d3dCull) {
        switch (d3dCull) {
            case D3D11_CULL_NONE: return ng::CullMode::None;
            case D3D11_CULL_FRONT: return ng::CullMode::Front;
            case D3D11_CULL_BACK: return ng::CullMode::Back;
            default: return ng::CullMode::Back;
        }
    }

    // Convert D3D11 fill mode to NVRHI
    ng::FillMode ConvertFillMode(D3D11_FILL_MODE d3dFill) {
        switch (d3dFill) {
            case D3D11_FILL_WIREFRAME: return ng::FillMode::Wireframe;
            case D3D11_FILL_SOLID: return ng::FillMode::Solid;
            default: return ng::FillMode::Solid;
        }
    }

    // Convert D3D11 comparison func to our abstraction
    ng::ComparisonFunc ConvertComparisonFunc(D3D11_COMPARISON_FUNC d3dFunc) {
        switch (d3dFunc) {
            case D3D11_COMPARISON_NEVER: return ng::ComparisonFunc::Never;
            case D3D11_COMPARISON_LESS: return ng::ComparisonFunc::Less;
            case D3D11_COMPARISON_EQUAL: return ng::ComparisonFunc::Equal;
            case D3D11_COMPARISON_LESS_EQUAL: return ng::ComparisonFunc::LessEqual;
            case D3D11_COMPARISON_GREATER: return ng::ComparisonFunc::Greater;
            case D3D11_COMPARISON_NOT_EQUAL: return ng::ComparisonFunc::NotEqual;
            case D3D11_COMPARISON_GREATER_EQUAL: return ng::ComparisonFunc::GreaterEqual;
            case D3D11_COMPARISON_ALWAYS: return ng::ComparisonFunc::Always;
            default: return ng::ComparisonFunc::Less;
        }
    }

    // Convert D3D11 blend factor to our abstraction
    ng::BlendFactor ConvertBlendFactor(D3D11_BLEND d3dBlend) {
        switch (d3dBlend) {
            case D3D11_BLEND_ZERO: return ng::BlendFactor::Zero;
            case D3D11_BLEND_ONE: return ng::BlendFactor::One;
            case D3D11_BLEND_SRC_COLOR: return ng::BlendFactor::SrcColor;
            case D3D11_BLEND_INV_SRC_COLOR: return ng::BlendFactor::InvSrcColor;
            case D3D11_BLEND_SRC_ALPHA: return ng::BlendFactor::SrcAlpha;
            case D3D11_BLEND_INV_SRC_ALPHA: return ng::BlendFactor::InvSrcAlpha;
            case D3D11_BLEND_DEST_ALPHA: return ng::BlendFactor::DstAlpha;
            case D3D11_BLEND_INV_DEST_ALPHA: return ng::BlendFactor::InvDstAlpha;
            case D3D11_BLEND_DEST_COLOR: return ng::BlendFactor::DstColor;
            case D3D11_BLEND_INV_DEST_COLOR: return ng::BlendFactor::InvDstColor;
            case D3D11_BLEND_SRC_ALPHA_SAT: return ng::BlendFactor::SrcAlphaSat;
            case D3D11_BLEND_BLEND_FACTOR: return ng::BlendFactor::BlendFactor;
            case D3D11_BLEND_INV_BLEND_FACTOR: return ng::BlendFactor::InvBlendFactor;
            default: return ng::BlendFactor::One;
        }
    }

    // Convert D3D11 blend op to our abstraction
    ng::BlendOp ConvertBlendOp(D3D11_BLEND_OP d3dOp) {
        switch (d3dOp) {
            case D3D11_BLEND_OP_ADD: return ng::BlendOp::Add;
            case D3D11_BLEND_OP_SUBTRACT: return ng::BlendOp::Subtract;
            case D3D11_BLEND_OP_REV_SUBTRACT: return ng::BlendOp::RevSubtract;
            case D3D11_BLEND_OP_MIN: return ng::BlendOp::Min;
            case D3D11_BLEND_OP_MAX: return ng::BlendOp::Max;
            default: return ng::BlendOp::Add;
        }
    }

    // Convert D3D11 color write mask to NVRHI
    ng::ColorWriteMask ConvertColorWriteMask(u8 d3dMask) {
        ng::ColorWriteMask mask = ng::ColorWriteMask::None;
        if (d3dMask & D3D11_COLOR_WRITE_ENABLE_RED)   mask = mask | ng::ColorWriteMask::Red;
        if (d3dMask & D3D11_COLOR_WRITE_ENABLE_GREEN) mask = mask | ng::ColorWriteMask::Green;
        if (d3dMask & D3D11_COLOR_WRITE_ENABLE_BLUE)  mask = mask | ng::ColorWriteMask::Blue;
        if (d3dMask & D3D11_COLOR_WRITE_ENABLE_ALPHA) mask = mask | ng::ColorWriteMask::Alpha;
        return mask;
    }
}

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

            // Note: Stencil ref is set separately in D3D11, not part of PSO
            // It's stored in dx11State but applied at draw time
            psoDesc.depthStencilState.frontFace.compareFunc = ConvertComparisonFunc(dsDesc.FrontFace.StencilFunc);
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
    }
#endif // USE_DX11
}

// ══════════════════════════════════════════════════════════
//  SETUP RENDER TARGETS
// ══════════════════════════════════════════════════════════

void MaterialCache::SetupRenderTargets(
    MaterialPSO* matPSO,
    const passes::GBufferOutputs& outputs,
    const xray::render::framegraph::FrameGraph& fg,
    ng::PipelineStateDesc& psoDesc)
{
    // Extract actual formats from FrameGraph resources
    nvrhi::ITexture* albedoTex = fg.GetPhysicalTexture(outputs.albedo);
    nvrhi::ITexture* normalTex = fg.GetPhysicalTexture(outputs.normal);
    nvrhi::ITexture* materialTex = fg.GetPhysicalTexture(outputs.material);
    nvrhi::ITexture* depthTex = fg.GetPhysicalTexture(outputs.depth);

    if (!albedoTex || !normalTex || !materialTex || !depthTex) {
        // Fallback to hardcoded formats (X-Ray convention: slot0=normal, slot1=albedo, slot2=material)
        psoDesc.renderTargetCount = 3;
        psoDesc.renderTargetFormats[0] = nvrhi::Format::RGBA8_SNORM;   // Normal
        psoDesc.renderTargetFormats[1] = nvrhi::Format::RGBA8_UNORM;   // Albedo
        psoDesc.renderTargetFormats[2] = nvrhi::Format::R16_UINT;      // Material
        psoDesc.depthStencilFormat = nvrhi::Format::D24S8;
        return;
    }

    // ═══════════════════════════════════════════════════════
    //  USE SHADER REFLECTION TO MAP RTS TO CORRECT SLOTS DYNAMICALLY
    // ═══════════════════════════════════════════════════════
    // Use semantic information from shader reflection to determine
    // which physical texture goes in which slot

    psoDesc.renderTargetCount = static_cast<u32>(matPSO->rtBindings.outputRTs.size());

    // Clear all slots first
    for (u32 i = 0; i < 8; i++) {
        psoDesc.renderTargetFormats[i] = nvrhi::Format::UNKNOWN;
    }

    // Map each RT semantic to its shader output slot dynamically
    using RTSemantic = framegraph::ShaderRTBindings::RTSemantic;

    for (const auto& outputRT : matPSO->rtBindings.outputRTs) {
        u32 slot = outputRT.slot;
        nvrhi::Format format = nvrhi::Format::UNKNOWN;
        const char* rtName = "Unknown";

        // Determine which physical GBuffer texture to use based on semantic
        switch (outputRT.semantic) {
            case RTSemantic::Normal:
                format = normalTex->getDesc().format;
                rtName = "Normal";
                break;

            case RTSemantic::Albedo:
                format = albedoTex->getDesc().format;
                rtName = "Albedo";
                break;

            case RTSemantic::Material:
                format = materialTex->getDesc().format;
                rtName = "Material";
                break;

            case RTSemantic::Position:
                // X-Ray doesn't use position RT in GBuffer (reconstructed from depth)
                Msg("! [MaterialCache] Warning: Shader writes Position RT (slot %u)", slot);
                continue;

            case RTSemantic::Emissive:
                Msg("! [MaterialCache] Warning: Shader writes Emissive RT (slot %u)", slot);
                continue;

            case RTSemantic::Accumulator:
                Msg("! [MaterialCache] Warning: Shader writes Accumulator RT (slot %u)", slot);
                continue;

            default:
                Msg("! [MaterialCache] Unknown RT semantic for slot %u", slot);
                continue;
        }

        psoDesc.renderTargetFormats[slot] = format;
        Msg("! [MaterialCache] RT slot %u → %s (format %u)", slot, rtName, (u32)format);
    }

    psoDesc.depthStencilFormat = depthTex->getDesc().format;
}

// ══════════════════════════════════════════════════════════
//  CLEAR CACHE
// ══════════════════════════════════════════════════════════

void MaterialCache::Clear()
{
    m_cache.clear();
    m_stats = Stats{};
}

} // namespace xray::render
