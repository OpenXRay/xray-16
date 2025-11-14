// xrRender/Geometry/MaterialCache.cpp
#include "stdafx.h"
#include "MaterialCache.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/FrameGraphPasses/GBufferPass.h"
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
    return nvrhi::Format::UNKNOWN;
}

// Deleted ConvertVertexFormat - use ConvertDxgiFormatToNvrhi instead

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
    const framegraph::FrameGraph& fg)
{
    if (!visual) {
        return nullptr;
    }

    // Check if visual has shader
    if (!visual->shader || !visual->shader._get()) {
        return nullptr;
    }

    Shader* shader = visual->shader._get();

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
        //     shaderName, m_stats.numCacheHits, m_stats.numCacheMisses);
        return it->second.get();
    }

    // ═══════════════════════════════════════════════════════
    //  CACHE MISS - CREATE NEW PSO
    // ═══════════════════════════════════════════════════════

    m_stats.numCacheMisses++;

    MaterialPSO* pso = CreatePSO(visual, elem, pass, outputs, fg);
    if (!pso) {
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
    const framegraph::FrameGraph& fg)
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
        // Get base texture (usually slot 0)
        CTexture* baseTex = (*pass->T)[0].second._get();
        if (baseTex && baseTex->cName.size()) {
            // Query our cache first
            pso->detail_scale = GetDetailScale(baseTex->cName);
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
    //  ANALYZE CONSTANT BUFFERS & REGISTER WITH VCB POOL
    // ═══════════════════════════════════════════════════════

    if (m_vcbPool) {
        // Analyze vertex shader CBs
        auto vsCBs = framegraph::ShaderReflector::AnalyzeConstantBuffers(
            pso->vertexShader->bytecode->GetBufferPointer(),
            pso->vertexShader->bytecode->GetBufferSize());
        for (const auto& cbInfo : vsCBs.buffers) {
            // Create CB layout and register with pool
            framegraph::VolatileConstantBufferPool::CBLayout layout(
                cbInfo.name.c_str(),
                cbInfo.slot,
                cbInfo.size
            );

            // Get or create VCB from pool
            ng::BufferHandle vcbHandle = m_vcbPool->GetOrCreateVCB(layout);

            // Store requirement in MaterialPSO
            MaterialPSO::VCBRequirement req;
            req.slot = cbInfo.slot;
            req.size = cbInfo.size;
            req.name = cbInfo.name;
            req.vcbHandle = vcbHandle;
            pso->vcbRequirements.push_back(req);
        }

        // Analyze pixel shader CBs
        auto psCBs = framegraph::ShaderReflector::AnalyzeConstantBuffers(
            pso->pixelShader->bytecode->GetBufferPointer(),
            pso->pixelShader->bytecode->GetBufferSize());
        for (const auto& cbInfo : psCBs.buffers) {
            // Create CB layout and register with pool
            framegraph::VolatileConstantBufferPool::CBLayout layout(
                cbInfo.name.c_str(),
                cbInfo.slot,
                cbInfo.size
            );

            // Get or create VCB from pool
            ng::BufferHandle vcbHandle = m_vcbPool->GetOrCreateVCB(layout);

            // Store requirement in MaterialPSO
            MaterialPSO::VCBRequirement req;
            req.slot = cbInfo.slot;
            req.size = cbInfo.size;
            req.name = cbInfo.name;
            req.vcbHandle = vcbHandle;
            pso->vcbRequirements.push_back(req);
        }
    }

    // ═══════════════════════════════════════════════════════
    //  BUILD PIPELINE STATE DESCRIPTOR
    // ═══════════════════════════════════════════════════════

    ng::PipelineStateDesc psoDesc;
    psoDesc.vertexShader = nvrhiVS.Get();  // Direct NVRHI shader pointer
    psoDesc.pixelShader = nvrhiPS.Get();   // No wrapper layer!

    // Extract vertex attributes from visual's geometry declaration
    // CRITICAL: Use shader's input signature to determine correct order!
    SetupVertexAttributes(visual, pso.get(), psoDesc);

    // Set up render states from X-Ray pass
    SetupRenderStates(pass, psoDesc);

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

    ng::PipelineState* nvrhiPSO = psoCache->GetOrCreate(psoDesc);
    if (!nvrhiPSO) {
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

    // Get TextureManager from FGResourceManager
    resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
    VERIFY(texManager);

    // ═══════════════════════════════════════════════════
    //  LOAD TEXTURES NATIVELY VIA TEXTUREMANAGER
    //  No more D3D11 wrapping! Direct native NVRHI textures!
    // ═══════════════════════════════════════════════════

    // STextureList is a vector of (stage, ref_texture) pairs
    for (size_t i = 0; i < texList->size(); i++) {
        const auto& texPair = (*texList)[i];
        u32 stage = texPair.first;
        const ref_texture& texRef = texPair.second;

        CTexture* tex = texRef._get();
        if (!tex) {
            continue;
        }

        // Get texture name (e.g., "act\\act_glow")
        const char* textureName = tex->cName.c_str();

        // ───────────────────────────────────────────────────
        //  CHECK CACHE FIRST
        // ───────────────────────────────────────────────────

        auto cacheIt = m_textureHandleCache.find(textureName);
        if (cacheIt != m_textureHandleCache.end()) {
            // Cache HIT - reuse existing handle
            MaterialPSO::TextureSlot texSlot;
            texSlot.slot = stage;
            texSlot.handle = cacheIt->second;
            matPSO->textures.push_back(texSlot);
            continue;
        }

        // ───────────────────────────────────────────────────
        //  CACHE MISS - LOAD VIA TEXTUREMANAGER
        // ───────────────────────────────────────────────────

        // Load through TextureManager (native NVRHI, with streaming & memory management!)
        resources::TextureHandle resourceHandle = texManager->LoadTexture(
            textureName,
            resources::TexturePriority::High  // UI/material textures are high priority
        );

        if (!resourceHandle.IsValid()) {
            Msg("! [MaterialCache] Failed to load texture: %s", textureName);
            continue;
        }

        // ───────────────────────────────────────────────────
        //  CACHE THE HANDLE FOR REUSE
        // ───────────────────────────────────────────────────

        m_textureHandleCache[textureName] = resourceHandle;

        // ───────────────────────────────────────────────────
        //  ADD TO MATERIAL PSO
        // ───────────────────────────────────────────────────

        MaterialPSO::TextureSlot texSlot;
        texSlot.slot = stage;
        texSlot.handle = resourceHandle;
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

    // Analyze VERTEX shader to extract input signature (CRITICAL FOR INPUT LAYOUT!)
    // X-Ray's SVS structure has: sh (ID3D11VertexShader*), bytecode (ID3DBlob*)
    if (vs->sh && vs->bytecode) {

        matPSO->vsInputSignature = framegraph::ShaderReflector::AnalyzeVertexShader(
            vs->sh,
            vs->bytecode->GetBufferPointer(),
            vs->bytecode->GetBufferSize()
        );
    }

    // Analyze PIXEL shader via D3D reflection
    // X-Ray's SPS structure has: sh (ID3D11PixelShader*), bytecode (ID3DBlob*)
    if (ps->sh && ps->bytecode) {

        // Analyze pixel shader using SPS's bytecode directly
        matPSO->rtBindings = framegraph::ShaderReflector::AnalyzePixelShader(
            ps->sh,
            ps->bytecode->GetBufferPointer(),
            ps->bytecode->GetBufferSize()
        );

        matPSO->rtBindings.shaderName = ps->cName;
    }

    // ═══════════════════════════════════════════════════════
    //  EXTRACT ALL CONSTANT BUFFERS from VS and PS
    // ═══════════════════════════════════════════════════════

    matPSO->constantBuffers.clear();

    // Helper lambda to extract CBs from a shader's constant table
    auto extractCBsFromShader = [&](R_constant_table& constTable, const char* shaderName, MaterialPSO::ShaderStage stage) {
        // X-Ray has multiple rendering contexts (R__NUM_CONTEXTS = 5)
        // Check all contexts to find constant buffers
        for (u32 contextIdx = 0; contextIdx < R__NUM_CONTEXTS; contextIdx++) {
            const auto& cbTable = constTable.m_CBTable[contextIdx];

            if (cbTable.empty())
                continue;


            for (const auto& cbRecord : cbTable) {
                u32 encodedSlot = cbRecord.first;
                dx11ConstantBuffer* cb = cbRecord.second._get();

                // Decode the slot to get shader type and binding slot
                u32 shaderType = dx11ConstantBuffer::DecodeShaderType(encodedSlot);
                u32 bindingSlot = dx11ConstantBuffer::DecodeBindingSlot(encodedSlot);
                const char* shaderTypeName = dx11ConstantBuffer::GetShaderTypeName(shaderType);


                if (cb) {
                    ID3DBuffer* d3dBuffer = cb->GetBuffer();
                    if (d3dBuffer) {
                        D3D11_BUFFER_DESC bufDesc;
                        d3dBuffer->GetDesc(&bufDesc);


                    // Check if we already have this (slot, stage) combination
                    bool found = false;
                    for (const auto& existing : matPSO->constantBuffers) {
                        if (existing.slot == bindingSlot && existing.stage == stage) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        const char* cbName = cb->GetBufferName();
                        bool isPerObjectCB = (cbName && xr_strcmp(cbName, "$Globals") == 0);

                        // DEBUG: Log what D3DReflect reports for this CB
                        Msg("  [MaterialCache] D3DReflect reports CB: name=%s, slot=%d (from shader)",
                            cbName ? cbName : "NULL", bindingSlot);

                        nvrhi::BufferHandle bufferHandle;

                        if (isPerObjectCB) {
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
                            nvrhi::BufferDesc nvrhiDesc;
                            nvrhiDesc.byteSize = bufDesc.ByteWidth;
                            nvrhiDesc.isConstantBuffer = true;
                            nvrhiDesc.debugName = "FrameGraph_Global_CB";
                            nvrhiDesc.keepInitialState = false;
                            nvrhiDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

                            bufferHandle = m_device->GetNativeDevice()->createBuffer(nvrhiDesc);
                            if (bufferHandle) {
                                Msg("  [MaterialCache] Created CB: name=%s, slot=%d, size=%d bytes, stage=%s",
                                    cbName ? cbName : "NULL",
                                    bindingSlot,
                                    bufDesc.ByteWidth,
                                    stage == MaterialPSO::ShaderStage::Vertex ? "VS" : "PS");
                            } else {
                                Msg("! [MaterialCache] FAILED to create CB: name=%s, slot=%d",
                                    cbName ? cbName : "NULL", bindingSlot);
                            }
                        }

                        if (bufferHandle) {
                            MaterialPSO::ConstantBufferInfo cbInfo;
                            cbInfo.slot = bindingSlot;  // Use DECODED slot for NVRHI binding!
                            cbInfo.stage = stage;  // Store which shader stage this CB belongs to
                            cbInfo.nvrhiBuffer = bufferHandle;
                            cbInfo.size = bufDesc.ByteWidth;
                            cbInfo.isPerObject = isPerObjectCB;
                            cbInfo.name = cbName;

                            // DISABLED: CB data extraction causes memory corruption
                            // TODO: Debug why this corrupts the heap
                            #if 0
                            if (bindingSlot > 0) {
                                void* xrayData = cb->GetBufferData();
                                u32 xraySize = cb->GetBufferSize();

                                if (xrayData && xraySize > 0 && xraySize == bufDesc.ByteWidth) {
                                    cbInfo.initialData.resize(xraySize);
                                    memcpy(cbInfo.initialData.data(), xrayData, xraySize);
                                }
                            }
                            #endif

                            matPSO->constantBuffers.push_back(cbInfo);

                        } else {
                        }
                    }
                }
            }
            }  // end for cbRecord
        }  // end for contextIdx
    };

    // Extract from vertex shader
    extractCBsFromShader(vs->constants, vs->cName.c_str(), MaterialPSO::ShaderStage::Vertex);

    // Extract from pixel shader
    extractCBsFromShader(ps->constants, ps->cName.c_str(), MaterialPSO::ShaderStage::Pixel);

    // Store per-object CB size for convenience
    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.isPerObject) {
            matPSO->perObjectCBSize = cbInfo.size;
            break;
        }
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
    // Get X-Ray's render state from the pass
    // IMPORTANT: pass->state returns SState*, NOT dx11State* directly!
    SState* xrState = pass->state._get();
    if (!xrState || !xrState->state) {
        return;
    }

    // The actual dx11State is inside xrState->state
    dx11State* d3dState = static_cast<dx11State*>(xrState->state);
    if (!d3dState) {
        return;
    }

    const auto& psSamplers = d3dState->GetPSSamplers();


    // Helper to create NVRHI sampler from D3D11 sampler
    auto createNVRHISampler = [&](ID3DSamplerState* d3dSampler) -> nvrhi::SamplerHandle {
        D3D11_SAMPLER_DESC samplerDesc;
        d3dSampler->GetDesc(&samplerDesc);

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

        return m_device->GetNativeDevice()->createSampler(nvrhiDesc);
    };

    // Helper to create default sampler
    auto createDefaultSampler = [&]() -> nvrhi::SamplerHandle {
        nvrhi::SamplerDesc nvrhiDesc;
        nvrhiDesc.setMinFilter(true);  // Linear
        nvrhiDesc.setMagFilter(true);  // Linear
        nvrhiDesc.setMipFilter(true);  // Linear
        // UI elements should use Clamp mode to prevent tiling/repeating
        nvrhiDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
        nvrhiDesc.setMaxAnisotropy(8.0f);

        return m_device->GetNativeDevice()->createSampler(nvrhiDesc);
    };

    // Extract samplers based on shader reflection data
    for (const auto& samplerDecl : matPSO->rtBindings.samplers) {
        u32 slot = samplerDecl.slot;
        const char* samplerName = samplerDecl.name.c_str();

        MaterialPSO::SamplerInfo samplerInfo;
        samplerInfo.slot = slot;
        samplerInfo.stage = MaterialPSO::ShaderStage::Pixel;  // Currently only PS samplers
        samplerInfo.name = samplerName;

        // Try to get sampler from X-Ray state
        if (slot < psSamplers.size()) {
            auto samplerHandle = psSamplers[slot];
            if (samplerHandle != dx11SamplerStateCache::hInvalidHandle) {
                ID3DSamplerState* d3dSampler = SSManager.GetSamplerState(samplerHandle);
                if (d3dSampler) {
                    samplerInfo.nvrhiSampler = createNVRHISampler(d3dSampler);
                }
            }
        }

        // If not found in X-Ray state, create default sampler
        if (!samplerInfo.nvrhiSampler) {
            samplerInfo.nvrhiSampler = createDefaultSampler();
        }

        if (samplerInfo.nvrhiSampler) {
            matPSO->samplers.push_back(samplerInfo);
        } else {
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

    // Add ONLY resources for THIS stage
    u32 cbCount = 0;
    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.stage == stage) {
            if (cbInfo.isPerObject) {
                // Slot 0: Per-object VOLATILE constant buffer (updated per-draw)
                layoutDesc.bindings.push_back(
                    nvrhi::BindingLayoutItem::VolatileConstantBuffer(cbInfo.slot));
            } else {
                // Other slots: Regular constant buffers (global state, updated per-frame)
                layoutDesc.bindings.push_back(
                    nvrhi::BindingLayoutItem::ConstantBuffer(cbInfo.slot));
            }
            cbCount++;
        }
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

nvrhi::BindingSetHandle MaterialCache::GetOrCreateBindingSet(
    MaterialPSO* matPSO,
    nvrhi::IBuffer* perObjectVCB,
    SPass* pass)
{
    VERIFY(matPSO);
    VERIFY(perObjectVCB);
    VERIFY(matPSO->vsBindingLayout);
    VERIFY(matPSO->psBindingLayout);

    // ═══════════════════════════════════════════════════════
    //  CHECK CACHE - Return existing binding sets if already created
    // ═══════════════════════════════════════════════════════

    if (matPSO->vsBindingSet && matPSO->psBindingSet) {
        // Already cached - reuse them!
        // NOTE: This should be hit thousands of times per frame - if not, we have a leak!
        return matPSO->vsBindingSet;
    }

    // ═══════════════════════════════════════════════════════
    //  BUILD VS BINDING SET DESCRIPTOR
    // ═══════════════════════════════════════════════════════

    nvrhi::BindingSetDesc vsBindingDesc;

    // Add ONLY VS constant buffers
    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.stage == MaterialPSO::ShaderStage::Vertex) {
            if (cbInfo.isPerObject) {
                // Slot 0: Use the per-object VCB passed in (updated per-draw via WriteBuffer)
                vsBindingDesc.bindings.push_back(
                    nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, perObjectVCB));
                Msg("  [GetOrCreateBindingSet] VS: Added per-object CB at slot %d", cbInfo.slot);
            } else {
                // Slots 1+: Add global CBs
                if (cbInfo.nvrhiBuffer) {
                    vsBindingDesc.bindings.push_back(
                        nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, cbInfo.nvrhiBuffer.Get()));
                    Msg("  [GetOrCreateBindingSet] VS: Added global CB '%s' at slot %d (size=%d)",
                        cbInfo.name.empty() ? "EMPTY" : cbInfo.name.c_str(), cbInfo.slot, cbInfo.size);
                } else {
                    Msg("! [GetOrCreateBindingSet] VS: CB '%s' at slot %d has NULL buffer!",
                        cbInfo.name.empty() ? "EMPTY" : cbInfo.name.c_str(), cbInfo.slot);
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════
    //  BUILD PS BINDING SET DESCRIPTOR
    // ═══════════════════════════════════════════════════════

    nvrhi::BindingSetDesc psBindingDesc;

    // Add ONLY PS constant buffers
    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.stage == MaterialPSO::ShaderStage::Pixel) {
            if (cbInfo.isPerObject) {
                // Slot 0: Use the per-object VCB passed in (shouldn't happen for PS, but handle it)
                psBindingDesc.bindings.push_back(
                    nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, perObjectVCB));
            } else {
                // Slots 0+: Add global CBs (PS doesn't have per-object CB)
                if (cbInfo.nvrhiBuffer) {
                    psBindingDesc.bindings.push_back(
                        nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, cbInfo.nvrhiBuffer.Get()));
                }
            }
        }
    }

    // Add textures to PS binding set (textures are only in pixel shader)
    // IMPORTANT: Must add binding items for ALL slots declared in layout, even if NULL!
    // Now using TextureManager to get native NVRHI textures (no more D3D11 wrapping!)
    resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
    for (const auto& texSlot : matPSO->textures) {
        nvrhi::ITexture* nativeTex = texManager->GetNVRHITexture(texSlot.handle);

        if (nativeTex) {
            // Get texture descriptor for logging
            const nvrhi::TextureDesc& texDesc = nativeTex->getDesc();
            // Use NVRHI helper to create properly initialized item
            // Format::UNKNOWN means use texture's native format
            psBindingDesc.bindings.push_back(
                nvrhi::BindingSetItem::Texture_SRV(texSlot.slot, nativeTex,
                    nvrhi::Format::UNKNOWN, nvrhi::AllSubresources, texDesc.dimension));
        } else {
            // NULL texture - add NULL binding to match layout
            psBindingDesc.bindings.push_back(
                nvrhi::BindingSetItem::Texture_SRV(texSlot.slot, nullptr));
        }
    }

    // Add samplers to PS binding set (samplers are only in pixel shader)
    // IMPORTANT: Must add binding items for ALL slots declared in layout, even if NULL!
    for (const auto& samplerInfo : matPSO->samplers) {
        if (samplerInfo.stage == MaterialPSO::ShaderStage::Pixel) {
            // ALWAYS add binding item, even if sampler is NULL
            if (samplerInfo.nvrhiSampler) {
                psBindingDesc.bindings.push_back(
                    nvrhi::BindingSetItem::Sampler(samplerInfo.slot, samplerInfo.nvrhiSampler));
            } else {
                // Add NULL sampler binding
                psBindingDesc.bindings.push_back(
                    nvrhi::BindingSetItem::Sampler(samplerInfo.slot, nullptr));
            }
        }
    }

    // ═══════════════════════════════════════════════════════
    //  CREATE VS BINDING SET
    // ═══════════════════════════════════════════════════════

    matPSO->vsBindingSet = m_device->CreateBindingSet(
        vsBindingDesc,
        matPSO->vsBindingLayout);

    if (!matPSO->vsBindingSet) {
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  CREATE PS BINDING SET
    // ═══════════════════════════════════════════════════════

    matPSO->psBindingSet = m_device->CreateBindingSet(
        psBindingDesc,
        matPSO->psBindingLayout);

    if (!matPSO->psBindingSet) {
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  DONE - Binding sets cached in MaterialPSO
    // ═══════════════════════════════════════════════════════


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
            return 4;  // Default fallback
    }
}

// ══════════════════════════════════════════════════════════
//  SETUP VERTEX ATTRIBUTES
// ══════════════════════════════════════════════════════════

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
                continue;

            case RTSemantic::Emissive:
                continue;

            case RTSemantic::Accumulator:
                continue;

            default:
                continue;
        }

        psoDesc.renderTargetFormats[slot] = format;
    }

    psoDesc.depthStencilFormat = depthTex->getDesc().format;
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
    key.isUIPSO = true;
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

    Msg("  [MaterialCache::CreateUIPSO] Extracted %zu textures, %zu samplers",
        pso->textures.size(), pso->samplers.size());

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
    Msg("  [MaterialCache::CreateUIPSO] Calculated vertex stride: %u bytes", pso->vertexStride);

    // Log final attributes with stride
    for (size_t i = 0; i < psoDesc.vertexAttributes.size(); ++i) {
        const auto& attr = psoDesc.vertexAttributes[i];
        Msg("  [MaterialCache::CreateUIPSO] Vertex attribute[%zu]: %s%d -> format=%d, offset=%u, stride=%u",
            i, attr.semanticName, attr.semanticIndex, (int)attr.format, attr.offset, attr.elementStride);
    }

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

    nvrhi::ShaderHandle vsHandle;

    // Use native NVRHI shader directly (no wrapper needed!)
    if (vs->nvrhiShader)
    {
        // Native NVRHI shader - just use it directly!
        vsHandle = vs->nvrhiShader;
    }
    else if (vs->bytecode)
    {
        // Legacy path: create NVRHI shader from bytecode
        nvrhi::ShaderDesc desc(nvrhi::ShaderType::Vertex);
        desc.debugName = vs->cName.c_str();
        desc.entryName = "main";

        vsHandle = m_device->GetNVRHIDevice()->createShader(
            desc,
            vs->bytecode->GetBufferPointer(),
            vs->bytecode->GetBufferSize());
    }
    else
    {
        Msg("! [MaterialCache] ERROR: VS '%s' has no nvrhiShader and no bytecode", vs->cName.c_str());
        return nullptr;
    }

    if (!vsHandle) {
        Msg("! [MaterialCache] ERROR: Failed to get/create VS '%s'", vs->cName.c_str());
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

    nvrhi::ShaderHandle psHandle;

    // Use native NVRHI shader directly (no wrapper needed!)
    if (ps->nvrhiShader)
    {
        // Native NVRHI shader - just use it directly!
        psHandle = ps->nvrhiShader;
    }
    else if (ps->bytecode)
    {
        // Legacy path: create NVRHI shader from bytecode
        nvrhi::ShaderDesc desc(nvrhi::ShaderType::Pixel);
        desc.debugName = ps->cName.c_str();
        desc.entryName = "main";

        psHandle = m_device->GetNVRHIDevice()->createShader(
            desc,
            ps->bytecode->GetBufferPointer(),
            ps->bytecode->GetBufferSize());
    }
    else
    {
        Msg("! [MaterialCache] ERROR: PS '%s' has no nvrhiShader and no bytecode", ps->cName.c_str());
        return nullptr;
    }

    if (!psHandle) {
        Msg("! [MaterialCache] ERROR: Failed to get/create PS '%s'", ps->cName.c_str());
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
